// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/code_cache/dummy_code_cache.h"

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "url/gurl.h"

class IOBufffer;

namespace net {

DummyCodeCache::DummyCodeCache(
    const base::FilePath& path,
    int max_size,
    const scoped_refptr<base::SingleThreadTaskRunner>& thread):path_(path) {
}

DummyCodeCache::~DummyCodeCache() {
  LOG(INFO) << "~DummyCodeCache";
}

void DummyCodeCache::WriteMetadata(const GURL& url,
                                  scoped_refptr<IOBufferWithSize> buf) {
}

void DummyCodeCache::ReadMetadata(const GURL& url,
                                 const base::Time& last_modified,
                                 ReadCallback& read_callback) {
}


void DummyCodeCache::ClearData(const CompletionCallback& callback) {
  const base::FilePath cache_path = path_.Append("CodeCache");
  bool code_cache_dir_existed = base::PathExists(cache_path);

  if (code_cache_dir_existed) {
    DeleteFile(cache_path, true);
  }
}

} // namespace net
