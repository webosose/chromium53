// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/webview_profile.h"

#include "base/time/time.h"
#include "webos/browser/browsing_data/webos_browsing_data_remover.h"
#include "webos/browser/webos_browser_context_adapter.h"

namespace webos {

WebViewProfile::WebViewProfile(const std::string& storage_name)
    : browser_context_adapter_(new WebOSBrowserContextAdapter(storage_name)) {}

WebViewProfile* WebViewProfile::GetDefaultProfile() {
  static WebViewProfile* profile =
      new WebViewProfile(WebOSBrowserContextAdapter::GetDefaultContext());
  return profile;
}

WebViewProfile::WebViewProfile(WebOSBrowserContextAdapter* adapter)
    : browser_context_adapter_(adapter) {}

WebOSBrowserContextAdapter* WebViewProfile::GetBrowserContextAdapter() const {
  return browser_context_adapter_;
}

void WebViewProfile::AppendExtraWebSocketHeader(const std::string& key,
                                                const std::string& value) {
  browser_context_adapter_->AppendExtraWebSocketHeader(key, value);
}

void WebViewProfile::SetProxyRules(const std::string& proxy_rules) {
  browser_context_adapter_->SetProxyRules(proxy_rules);
}

void WebViewProfile::FlushCookieStore() {
  browser_context_adapter_->FlushCookieStore();
}

void WebViewProfile::RemoveBrowsingData(int remove_browsing_data_mask) {
  WebOSBrowsingDataRemover* remover =
      WebOSBrowsingDataRemover::GetForBrowserContext(
          browser_context_adapter_->GetBrowserContext());
  // WebOSBrowsingDataRemover instance gets deleted internally when removal is
  // complete
  remover->Remove(WebOSBrowsingDataRemover::Unbounded(),
                  remove_browsing_data_mask);
}

}  // namespace webos
