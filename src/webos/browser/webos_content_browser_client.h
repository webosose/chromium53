// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef WEBOS_WEBOS_CONTENT_BROWSER_CLIENT_H_
#define WEBOS_WEBOS_CONTENT_BROWSER_CLIENT_H_

#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/quota_permission_context.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

#include "webos/browser/webos_browser_main_parts.h"
#include <memory>

namespace content {
class RenderViewHost;
class WebContents;
class WebPreferences;
}

namespace webos {

class URLRequestContextFactory;
class WebOSResourceDispatcherHostDelegate;

class WebOSContentBrowserClient : public content::ContentBrowserClient {
 public:
  WebOSContentBrowserClient();
  virtual ~WebOSContentBrowserClient();

  // content::ContentBrowserClient implementations.
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool overridable,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(bool)>& callback,
      content::CertificateRequestResultType* result) override;
  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;

  std::string GetApplicationLocale() override { return current_locale_; }

  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  virtual void ResourceDispatcherHostCreated() override;
  WebOSBrowserMainParts* GetMainParts() { return main_parts_; }
  void SetDoNotTrack(bool dnt) { do_not_track_ = dnt; }
  bool DoNotTrack() { return do_not_track_; }

#if defined(ENABLE_PLUGINS)
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  bool PluginLoaded() const { return plugin_loaded_; }
  void SetPluginLoaded(bool loaded) { plugin_loaded_ = loaded; }
#endif

  void SetV8SnapshotPath(int child_process_id, const std::string& path);
  void SetV8ExtraFlags(int child_process_id, const std::string& flags);
  void SetUseNativeScroll(int child_process_id, bool use_native_scroll);
  void SetApplicationLocale(const std::string& locale);

  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;

 private:
  class MainURLRequestContextGetter;

  WebOSBrowserMainParts* main_parts_;
  bool do_not_track_;
  std::unique_ptr<WebOSResourceDispatcherHostDelegate>
      resource_dispatcher_host_delegate_;

#if defined(ENABLE_PLUGINS)
  bool plugin_loaded_;
#endif

  std::map<int, std::string> v8_snapshot_pathes_;
  std::map<int, std::string> v8_extra_flags_;

  // Stores (int child_process_id, bool use_native_scroll) and apply the flags
  // related to native scroll when use_native_scroll flag for the render process
  // is true.
  std::map<int, bool> use_native_scroll_map_;

  std::string current_locale_;
};

class WebOSResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  virtual ~WebOSResourceDispatcherHostDelegate() {}
};

} // namespace webos

#endif /* WPL_WEBOS_CONTENT_BROWSER_CLIENT_H */
