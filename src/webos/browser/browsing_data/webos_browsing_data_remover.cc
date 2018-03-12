// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/browsing_data/webos_browsing_data_remover.h"

#include "base/message_loop/message_loop.h"
#include "components/browsing_data/storage_partition_code_cache_data_remover.h"
#include "components/browsing_data/storage_partition_http_cache_data_remover.h"
#include "components/web_cache/browser/web_cache_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/ssl/channel_id_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "webos/browser/webos_browser_context.h"
#include "url/gurl.h"

namespace webos {

// static
WebOSBrowsingDataRemover::TimeRange WebOSBrowsingDataRemover::Unbounded() {
  return TimeRange(base::Time(), base::Time::Max());
}

WebOSBrowsingDataRemover* WebOSBrowsingDataRemover::GetForBrowserContext(
    WebOSBrowserContext* browser_context) {
  return new WebOSBrowsingDataRemover(browser_context);
}

WebOSBrowsingDataRemover::WebOSBrowsingDataRemover(
    WebOSBrowserContext* browser_context)
    : browser_context_(browser_context),
      waiting_for_clear_channel_ids_(false),
      waiting_for_clear_cache_(false),
      waiting_for_clear_code_cache_(false),
      waiting_for_clear_storage_partition_data_(false),
      weak_ptr_factory_(this) {}

WebOSBrowsingDataRemover::~WebOSBrowsingDataRemover() {}

void OnClearedChannelIDsOnIOThread(net::URLRequestContextGetter* rq_context,
                                   const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  rq_context->GetURLRequestContext()
      ->ssl_config_service()
      ->NotifySSLConfigChange();
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   callback);
}

void ClearChannelIDsOnIOThread(
    scoped_refptr<net::URLRequestContextGetter> rq_context,
    const base::Closure& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  net::ChannelIDService* channel_id_service =
      rq_context->GetURLRequestContext()->channel_id_service();
  channel_id_service->GetChannelIDStore()->DeleteAll(
      base::Bind(&OnClearedChannelIDsOnIOThread,
                 base::RetainedRef(std::move(rq_context)), callback));
}

void WebOSBrowsingDataRemover::Remove(const TimeRange& time_range,
                                      int remove_mask) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK_NE(base::Time(), time_range.end);

  // Start time to delete from.
  base::Time delete_begin = time_range.begin;

  // End time to delete to.
  base::Time delete_end = time_range.end;

  // TODO (jani) REMOVE_HISTORY support this?
  // Currently we don't track this in WebView

  // TODO (jani) REMOVE_DOWNLOADS currently we don't provide DownloadManager
  // for webview yet

  // Channel IDs are not separated for protected and unprotected web
  // origins. We check the origin_type_mask_ to prevent unintended deletion.
  if (remove_mask & REMOVE_CHANNEL_IDS) {
    // Since we are running on the UI thread don't call GetURLRequestContext().
    scoped_refptr<net::URLRequestContextGetter> rq_context =
        content::BrowserContext::GetDefaultStoragePartition(browser_context_)
            ->GetURLRequestContext();
    waiting_for_clear_channel_ids_ = true;
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&ClearChannelIDsOnIOThread, std::move(rq_context),
                   base::Bind(&WebOSBrowsingDataRemover::OnClearedChannelIDs,
                              weak_ptr_factory_.GetWeakPtr())));
  }

  uint32_t storage_partition_remove_mask = 0;
  if (remove_mask & REMOVE_COOKIES) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_COOKIES;
  }
  if (remove_mask & REMOVE_LOCAL_STORAGE) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE;
  }
  if (remove_mask & REMOVE_INDEXEDDB) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_INDEXEDDB;
  }
  if (remove_mask & REMOVE_WEBSQL) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_WEBSQL;
  }
  if (remove_mask & REMOVE_APPCACHE) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_APPCACHE;
  }
  if (remove_mask & REMOVE_SERVICE_WORKERS) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS;
  }
  if (remove_mask & REMOVE_CACHE_STORAGE) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_CACHE_STORAGE;
  }
  if (remove_mask & REMOVE_FILE_SYSTEMS) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS;
  }

  if (remove_mask & REMOVE_PLUGIN_DATA) {
    // TODO (jani) Do we need this?
  }

  if (remove_mask & REMOVE_CACHE) {
    // Tell the renderers to clear their cache.
    web_cache::WebCacheManager::GetInstance()->ClearCache();

    waiting_for_clear_cache_ = true;
    // StoragePartitionHttpCacheDataRemover deletes itself when it is done.
    browsing_data::StoragePartitionHttpCacheDataRemover::CreateForRange(
        content::BrowserContext::GetDefaultStoragePartition(browser_context_),
        delete_begin, delete_end)
        ->Remove(base::Bind(&WebOSBrowsingDataRemover::OnClearedCache,
                            weak_ptr_factory_.GetWeakPtr()));

    // Tell the shader disk cache to clear.
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE;

    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_WEBRTC_IDENTITY;
  }

  if (remove_mask & REMOVE_WEBRTC_IDENTITY) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_WEBRTC_IDENTITY;
  }

  // Content Decryption Modules used by Encrypted Media store licenses in a
  // private filesystem. These are different than content licenses used by
  // Flash (which are deleted father down in this method).
  if (remove_mask & REMOVE_MEDIA_LICENSES) {
    storage_partition_remove_mask |=
        content::StoragePartition::REMOVE_DATA_MASK_PLUGIN_PRIVATE_DATA;
  }

  if (remove_mask & REMOVE_CODE_CACHE) {
    // StoragePartitionCodeCacheDataRemover deletes itself when it is done.
    browsing_data::StoragePartitionCodeCacheDataRemover::CreateForRange(
        content::BrowserContext::GetDefaultStoragePartition(browser_context_),
        delete_begin, delete_end)
        ->Remove(base::Bind(&WebOSBrowsingDataRemover::OnClearedCodeCache,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  if (storage_partition_remove_mask) {
    waiting_for_clear_storage_partition_data_ = true;

    content::StoragePartition* storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(browser_context_);

    const uint32_t quota_storage_remove_mask = 0xFFFFFFFF;
    storage_partition->ClearData(
        storage_partition_remove_mask, quota_storage_remove_mask, GURL(),
        content::StoragePartition::OriginMatcherFunction(), delete_begin,
        delete_end,
        base::Bind(&WebOSBrowsingDataRemover::OnClearedStoragePartitionData,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void WebOSBrowsingDataRemover::NotifyAndDelete() {
  base::MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

void WebOSBrowsingDataRemover::NotifyAndDeleteIfDone() {
  if (!AllDone())
    return;

  NotifyAndDelete();
}

void WebOSBrowsingDataRemover::OnClearedChannelIDs() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_channel_ids_ = false;
  NotifyAndDeleteIfDone();
}

void WebOSBrowsingDataRemover::OnClearedCache() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_cache_ = false;
  NotifyAndDeleteIfDone();
}

void WebOSBrowsingDataRemover::OnClearedCodeCache() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_code_cache_ = false;
  NotifyAndDeleteIfDone();
}

void WebOSBrowsingDataRemover::OnClearedStoragePartitionData() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  waiting_for_clear_storage_partition_data_ = false;
  NotifyAndDeleteIfDone();
}

bool WebOSBrowsingDataRemover::AllDone() {
  return !waiting_for_clear_channel_ids_ && !waiting_for_clear_cache_ &&
         !waiting_for_clear_code_cache_ && !waiting_for_clear_storage_partition_data_;
}

}  // namespace webos
