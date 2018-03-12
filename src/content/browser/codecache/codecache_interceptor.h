// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CODECACHE_CODECACHE_INTERCEPTOR_H_
#define CONTENT_BROWSER_CODECACHE_CODECACHE_INTERCEPTOR_H_

#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request_interceptor.h"

class GURL;

namespace net {
class URLRequest;
}

namespace content {
class CodeCacheRequestHandler;

// An interceptor to hijack requests and potentially service them out of
// the codecache.
class CONTENT_EXPORT CodeCacheInterceptor : public net::URLRequestInterceptor {
 public:
  // Must be called to make a request eligible for retrieval from an codecache.
  static void SetExtraRequestInfo(net::URLRequest* request,
                                  ResourceType resource_type);

  CodeCacheInterceptor() {}

 protected:
  // Override from net::URLRequestInterceptor:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CodeCacheInterceptor);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CODECACHE_CODECACHE_INTERCEPTOR_H_
