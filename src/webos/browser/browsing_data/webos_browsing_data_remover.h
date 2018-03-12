// Copyright (c) 2017-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef WEBOS_BROWSER_BROWSING_DATA_WEBOS_BROWSING_DATA_REMOVER_H
#define WEBOS_BROWSER_BROWSING_DATA_WEBOS_BROWSING_DATA_REMOVER_H

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

namespace webos {

class WebOSBrowserContext;

// WebOSBrowsingDataRemover is responsible for removing data related to
// browsing:
// visits in url database, downloads, cookies ...
class WebOSBrowsingDataRemover {
 public:
  enum RemoveBrowsingDataMask {
    REMOVE_APPCACHE = 1 << 0,
    REMOVE_CACHE = 1 << 1,
    REMOVE_COOKIES = 1 << 2,
    REMOVE_DOWNLOADS = 1 << 3,
    REMOVE_FILE_SYSTEMS = 1 << 4,
    REMOVE_FORM_DATA = 1 << 5,
    // In addition to visits, REMOVE_HISTORY removes keywords, last session and
    // passwords UI statistics.
    REMOVE_HISTORY = 1 << 6,
    REMOVE_INDEXEDDB = 1 << 7,
    REMOVE_LOCAL_STORAGE = 1 << 8,
    REMOVE_PLUGIN_DATA = 1 << 9,
    REMOVE_PASSWORDS = 1 << 10,
    REMOVE_WEBSQL = 1 << 11,
    REMOVE_CHANNEL_IDS = 1 << 12,
    REMOVE_MEDIA_LICENSES = 1 << 13,
    REMOVE_SERVICE_WORKERS = 1 << 14,
    REMOVE_SITE_USAGE_DATA = 1 << 15,
    // REMOVE_NOCHECKS intentionally does not check if the Profile's prohibited
    // from deleting history or downloads.
    REMOVE_NOCHECKS = 1 << 16,
    REMOVE_WEBRTC_IDENTITY = 1 << 17,
    REMOVE_CACHE_STORAGE = 1 << 18,
    REMOVE_CODE_CACHE = 1 << 19,

    // "Site data" includes cookies, appcache, file systems, indexedDBs, local
    // storage, webSQL, service workers, cache storage, plugin data,
    // and statistics about passwords.
    REMOVE_SITE_DATA = REMOVE_APPCACHE | REMOVE_COOKIES | REMOVE_FILE_SYSTEMS |
                       REMOVE_INDEXEDDB |
                       REMOVE_LOCAL_STORAGE |
                       REMOVE_PLUGIN_DATA |
                       REMOVE_SERVICE_WORKERS |
                       REMOVE_CACHE_STORAGE |
                       REMOVE_WEBSQL |
                       REMOVE_CHANNEL_IDS |
                       REMOVE_SITE_USAGE_DATA |
                       REMOVE_WEBRTC_IDENTITY,

    // Includes all the available remove options. Meant to be used by clients
    // that wish to wipe as much data as possible from a Profile, to make it
    // look like a new Profile.
    REMOVE_ALL = REMOVE_SITE_DATA | REMOVE_CACHE | REMOVE_DOWNLOADS |
                 REMOVE_FORM_DATA |
                 REMOVE_HISTORY |
                 REMOVE_PASSWORDS |
                 REMOVE_MEDIA_LICENSES |
                 REMOVE_CODE_CACHE,

    // Includes all available remove options. Meant to be used when the Profile
    // is scheduled to be deleted, and all possible data should be wiped from
    // disk as soon as possible.
    REMOVE_WIPE_PROFILE = REMOVE_ALL | REMOVE_NOCHECKS,
  };

  struct TimeRange {
    TimeRange(base::Time begin, base::Time end) : begin(begin), end(end) {}

    base::Time begin;
    base::Time end;
  };

  static TimeRange Unbounded();

  static WebOSBrowsingDataRemover* GetForBrowserContext(
      WebOSBrowserContext* browser_context);

  // Should be private but because of deleted by posting to message loop this
  // needs to be public
  ~WebOSBrowsingDataRemover();

  // Deletes itself when removing is complete
  void Remove(const TimeRange& time_range, int remove_mask);

 private:
  WebOSBrowsingDataRemover(WebOSBrowserContext* browser_context);

  // Notifies observers and transitions to the idle state.
  void NotifyAndDelete();

  // Checks if we are all done, and if so, calls NotifyAndDelete().
  void NotifyAndDeleteIfDone();

  // Callback for when channel IDs have been deleted.
  // Invokes NotifyAndDeleteIfDone.
  void OnClearedChannelIDs();

  // Callback for when the cache has been deleted. Invokes
  // NotifyAndDeleteIfDone.
  void OnClearedCache();

  // Callback on UI thread when the storage partition related data are cleared.
  // Invokes NotifyAndDeleteIfDone.
  void OnClearedStoragePartitionData();

  // Callback for when the code cache has been deleted. Invokes
  // NotifyAndDeleteIfDone.
  void OnClearedCodeCache();

  // Returns true if we're all done.
  bool AllDone();

  WebOSBrowserContext* browser_context_;
  bool waiting_for_clear_channel_ids_;
  bool waiting_for_clear_cache_;
  bool waiting_for_clear_code_cache_;
  bool waiting_for_clear_storage_partition_data_;

  base::WeakPtrFactory<WebOSBrowsingDataRemover> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebOSBrowsingDataRemover);
};

}  // namespace webos

#endif  // WEBOS_BROWSER_BROWSING_DATA_WEBOS_BROWSING_DATA_REMOVER_H
