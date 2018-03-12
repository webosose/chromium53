// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/codecache/codecache_interceptor.h"

#include "base/command_line.h"
#include "content/browser/codecache/codecache_request_handler.h"
#include "content/browser/codecache/codecache_url_request_job.h"
#include "content/public/common/content_switches.h"
#include "net/url_request/url_request.h"

static int kHandlerKey;  // Value is not used.

namespace content {

void CodeCacheInterceptor::SetExtraRequestInfo(net::URLRequest* request,
                                               ResourceType resource_type) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableLocalResourceCodeCache) &&
      request->url().SchemeIsFile() &&
      resource_type == RESOURCE_TYPE_SCRIPT) {
    // Create a handler for this request and associate it with the request.
    // request takes ownership
    request->SetUserData(&kHandlerKey, new CodeCacheRequestHandler());
  }
}

net::URLRequestJob* CodeCacheInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  CodeCacheRequestHandler* handler =
      static_cast<CodeCacheRequestHandler*>(request->GetUserData(&kHandlerKey));
  if (!handler)
    return nullptr;
  return handler->MaybeLoadResource(request, network_delegate);
}

}  // namespace content
