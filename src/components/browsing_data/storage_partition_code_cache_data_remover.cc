// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browsing_data/storage_partition_code_cache_data_remover.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/sdch_manager.h"
#include "net/disk_cache/blockfile/backend_impl.h"
#include "net/disk_cache/disk_cache.h"
#include "net/disk_cache/simple/simple_backend_impl.h"
#include "net/code_cache/code_cache.h"
#include "net/code_cache/dummy_code_cache.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

using content::BrowserThread;

namespace browsing_data {

StoragePartitionCodeCacheDataRemover::StoragePartitionCodeCacheDataRemover(
    base::Callback<bool(const GURL&)> url_predicate,
    base::Time delete_begin,
    base::Time delete_end,
    net::URLRequestContextGetter* main_context_getter)
    : url_predicate_(url_predicate),
      delete_begin_(delete_begin),
      delete_end_(delete_end),
      main_context_getter_(main_context_getter) {
}

StoragePartitionCodeCacheDataRemover::~StoragePartitionCodeCacheDataRemover() {
}

// static.
StoragePartitionCodeCacheDataRemover*
StoragePartitionCodeCacheDataRemover::CreateForRange(
    content::StoragePartition* storage_partition,
    base::Time delete_begin,
    base::Time delete_end) {
  return new StoragePartitionCodeCacheDataRemover(
      base::Callback<bool(const GURL&)>(),  // Null callback.
      delete_begin, delete_end,
      storage_partition->GetURLRequestContext());
}

void StoragePartitionCodeCacheDataRemover::Remove(
    const base::Closure& done_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!done_callback.is_null());
  done_callback_ = done_callback;

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(
          &StoragePartitionCodeCacheDataRemover::ClearCodeCacheOnIOThread,
          base::Unretained(this)));
}

void StoragePartitionCodeCacheDataRemover::ClearCodeCacheOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(main_context_getter_.get());

  DoClearCache();
}

void StoragePartitionCodeCacheDataRemover::ClearedCodeCache(int rv) {
  done_callback_.Run();
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void StoragePartitionCodeCacheDataRemover::DoClearCache() {
  // Get a pointer to the cache.
  net::URLRequestContextGetter* getter = main_context_getter_.get();
  net::CodeCache* code_cache = getter->GetURLRequestContext()
                               ->code_cache();

  if (code_cache) {
    code_cache->ClearData(base::Bind(
          &StoragePartitionCodeCacheDataRemover::ClearedCodeCache,
          base::Unretained(this)));
  }
}

}  // namespace browsing_data
