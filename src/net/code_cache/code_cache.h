// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CODE_CACHE_CODE_CACHE_H_
#define NET_CODE_CACHE_CODE_CACHE_H_

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "net/base/completion_callback.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"

class GURL;

namespace base {
class time;
}
namespace net {

class IOBufferWithSize;

class NET_EXPORT CodeCache {
 public:
  typedef base::Callback<void(int, scoped_refptr<IOBufferWithSize>)>
      ReadCallback;
  virtual ~CodeCache() {}

  virtual void WriteMetadata(const GURL& url,
                             scoped_refptr<IOBufferWithSize> buf) = 0;

  virtual void ReadMetadata(const GURL& url,
                            const base::Time& last_modified,
                            ReadCallback& callback) = 0;
  virtual void ClearData(const CompletionCallback& callback = CompletionCallback()) = 0;
};

}  // namespace net

#endif  // NET_CODE_CACHE_CODE_CACHE_H_
