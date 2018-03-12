// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CODECACHE_CODECACHE_REQUEST_HANDLER_H_
#define CONTENT_BROWSER_CODECACHE_CODECACHE_REQUEST_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/supports_user_data.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

namespace net {
class NetworkDelegate;
class URLRequest;
class URLRequestJob;
}  // namespace net

namespace content {
class CodeCacheURLRequestJob;

// An instance is created for supporting v8 code cache for file:// scheme
class CONTENT_EXPORT CodeCacheRequestHandler
    : public base::SupportsUserData::Data {
 public:
  CodeCacheRequestHandler();

  CodeCacheURLRequestJob* MaybeLoadResource(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate);

 private:
  bool request_job_delivered_;

  DISALLOW_COPY_AND_ASSIGN(CodeCacheRequestHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CODECACHE_CODECACHE_REQUEST_HANDLER_H_
