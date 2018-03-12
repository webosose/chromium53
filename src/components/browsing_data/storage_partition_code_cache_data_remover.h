// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSING_DATA_STORAGE_PARTITION_CODE_CACHE_DATA_REMOVER_H_
#define COMPONENTS_BROWSING_DATA_STORAGE_PARTITION_CODE_CACHE_DATA_REMOVER_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/time/time.h"
#include "content/public/browser/browser_context.h"
#include "net/base/completion_callback.h"
#include "url/gurl.h"

namespace content {
class StoragePartition;
class BrowserContext;
}

namespace disk_cache {
class Backend;
}

namespace net {
class URLRequestContextGetter;
class CodeCache;
}

namespace browsing_data {

// Helper to remove code cache data from a StoragePartition.
class StoragePartitionCodeCacheDataRemover {
 public:
  // Creates a StoragePartitionCodeCacheDataRemover that deletes cache entries
  static StoragePartitionCodeCacheDataRemover* CreateForRange(
      content::StoragePartition* storage_partition,
      base::Time delete_begin,
      base::Time delete_end);

  // Calls |done_callback| upon completion and also destroys itself.
  void Remove(const base::Closure& done_callback);

 private:

  StoragePartitionCodeCacheDataRemover(
      base::Callback<bool(const GURL&)> url_predicate,
      base::Time delete_begin,
      base::Time delete_end,
      net::URLRequestContextGetter* main_context_getter);

  // StoragePartitionCodeCacheDataRemover deletes itself (using DeleteHelper)
  // and is not supposed to be deleted by other objects so make destructor
  // private and DeleteHelper a friend.
  friend class base::DeleteHelper<StoragePartitionCodeCacheDataRemover>;

  ~StoragePartitionCodeCacheDataRemover();

  void ClearCodeCacheOnIOThread();

  void ClearedCodeCache(int rv);

  // Performs the actual work to delete or count the cache.
  void DoClearCache();

  base::Callback<bool(const GURL&)> url_predicate_;
  const base::Time delete_begin_;
  const base::Time delete_end_;

  const scoped_refptr<net::URLRequestContextGetter> main_context_getter_;

  base::Closure done_callback_;

  DISALLOW_COPY_AND_ASSIGN(StoragePartitionCodeCacheDataRemover);
};

}  // namespace browsing_data

#endif  // COMPONENTS_BROWSING_DATA_STORAGE_PARTITION_CODE_CACHE_DATA_REMOVER_H_
