// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CODE_CACHE_DUMMY_CODE_CACHE_H_
#define NET_CODE_CACHE_iDUMMY_CODE_CACHE_H_

#include "net/code_cache/code_cache.h"

#include "base/files/file_path.h"
#include "base/threading/non_thread_safe.h"
#include "net/base/io_buffer.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"
#include "url/gurl.h"

#include <memory>
#include <vector>

class GURL;

namespace base {
class SingleThreadTaskRunner;
}

namespace net {

class NET_EXPORT DummyCodeCache : public CodeCache,
                                 NON_EXPORTED_BASE(public base::NonThreadSafe) {

 public:
  DummyCodeCache(const base::FilePath& path,
                int max_size,
                const scoped_refptr<base::SingleThreadTaskRunner>& thread);

  ~DummyCodeCache() override;

   void WriteMetadata(const GURL& url,
                             scoped_refptr<IOBufferWithSize> buf) override;

   void ReadMetadata(const GURL& url,
                            const base::Time& last_modified,
                            ReadCallback& callback) override;

   void ClearData(const CompletionCallback& callback = CompletionCallback()) override;

 private:
  base::FilePath path_;
  DISALLOW_COPY_AND_ASSIGN(DummyCodeCache);
};

}  // namespace net

#endif  // NET_CODE_CACHE_DUMMY_CODE_CACHE_H_
