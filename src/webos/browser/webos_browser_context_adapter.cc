// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/webos_browser_context_adapter.h"

#include "webos/app/webos_content_main_delegate.h"
#include "webos/browser/webos_browser_context.h"

namespace webos {

WebOSBrowserContextAdapter::WebOSBrowserContextAdapter(
    const std::string& storage_name)
    : storage_name_(storage_name),
      browser_context_(new WebOSBrowserContext(this)) {}

WebOSBrowserContextAdapter::~WebOSBrowserContextAdapter() {
  delete browser_context_;
}

WebOSBrowserContextAdapter* WebOSBrowserContextAdapter::GetDefaultContext() {
  return GetWebOSContentBrowserClient()
      ->GetMainParts()
      ->GetDefaultBrowserContext();
}

WebOSBrowserContext* WebOSBrowserContextAdapter::GetBrowserContext() const {
  return browser_context_;
}

std::string WebOSBrowserContextAdapter::GetStorageName() const {
  return storage_name_;
}

void WebOSBrowserContextAdapter::AppendExtraWebSocketHeader(
    const std::string& key,
    const std::string& value) {
  browser_context_->AppendExtraWebSocketHeader(key, value);
}

void WebOSBrowserContextAdapter::SetProxyRules(const std::string& proxy_rules) {
  browser_context_->SetProxyRules(proxy_rules);
}

void WebOSBrowserContextAdapter::FlushCookieStore() {
  browser_context_->FlushCookieStore();
}

}  // namespace webos
