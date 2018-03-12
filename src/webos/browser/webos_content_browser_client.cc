// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/webos_content_browser_client.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/accessibility/webos/accessibility_localization_webos.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/web_preferences.h"
#include "ui/base/resource/resource_bundle.h"
#include "webos/browser/devtools/webos_devtools_delegate.h"
#include "webos/browser/webos_browser_main_parts.h"
#include "webos/common/webos_constants.h"
#include "webos/webview.h"

#if defined(USE_AURA)
#include "webos/browser/ui/aura/webos_browser_main_extra_parts_aura.h"
#endif

#if defined(ENABLE_PLUGINS)
#include "content/public/browser/browser_ppapi_host.h"
#include "ppapi/host/ppapi_host.h"
#include "webos/browser/renderer_host/pepper/webos_pepper_host_factory.h"
#endif

namespace webos {

class WebOSQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  explicit WebOSQuotaPermissionContext();

  // The callback will be dispatched on the IO thread.
  void RequestQuotaPermission(const content::StorageQuotaParams& params,
                              int render_process_id,
                              const PermissionCallback& callback) override;

 private:
  virtual ~WebOSQuotaPermissionContext();

  DISALLOW_COPY_AND_ASSIGN(WebOSQuotaPermissionContext);
};

WebOSQuotaPermissionContext::WebOSQuotaPermissionContext() {}

void WebOSQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    const PermissionCallback& callback) {
  if ((params.storage_type != storage::kStorageTypePersistent) ||
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableFileAPIDirectoriesAndSystem)) {
    // For now we only support requesting quota with this interface
    // for Persistent storage type.
    callback.Run(QUOTA_PERMISSION_RESPONSE_DISALLOW);
    return;
  }
  callback.Run(QUOTA_PERMISSION_RESPONSE_ALLOW);
}

WebOSQuotaPermissionContext::~WebOSQuotaPermissionContext() {}

content::QuotaPermissionContext*
WebOSContentBrowserClient::CreateQuotaPermissionContext() {
  return new WebOSQuotaPermissionContext;
}

WebOSContentBrowserClient::WebOSContentBrowserClient() {
#if defined(ENABLE_PLUGINS)
  plugin_loaded_ = false;
#endif
}

WebOSContentBrowserClient::~WebOSContentBrowserClient() {
}

content::BrowserMainParts* WebOSContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  main_parts_ = new WebOSBrowserMainParts();

#if defined(USE_AURA)
  main_parts_->AddParts(new WebOSBrowserMainExtraPartsAura());
#endif

  return main_parts_;
}

void WebOSContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    content::ResourceType resource_type,
    bool overridable,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(bool)>& callback,
    content::CertificateRequestResultType* result) {
  // HCAP requirements: For SSL Certificate error, follows the policy settings
  if (web_contents && web_contents->GetDelegate()) {
    WebView* webView = static_cast<WebView*>(web_contents->GetDelegate());
    switch (webView->GetSSLCertErrorPolicy()) {
      case SSL_CERT_ERROR_POLICY_IGNORE:
        callback.Run(true);
        *result = content::CERTIFICATE_REQUEST_RESULT_TYPE_CONTINUE;
        return;
      case SSL_CERT_ERROR_POLICY_DENY:
        *result = content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY;
        return;
      default:
        break;
    }
  }

  if (resource_type != content::RESOURCE_TYPE_MAIN_FRAME) {
    // A sub-resource has a certificate error. The user doesn't really
    // have a context for making the right decision, so block the
    // request hard, without and info bar to allow showing the insecure
    // content.
    *result = content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY;
    return;
  }

  if (!web_contents) {
    NOTREACHED();
    return;
  }

  content::WebContentsImpl* web_contents_impl =
      static_cast<content::WebContentsImpl*>(web_contents);

  *result = content::CERTIFICATE_REQUEST_RESULT_TYPE_CANCEL;
  web_contents_impl->DidFailLoadWithError(
      web_contents_impl->GetMainFrame(),
      request_url, cert_error, base::ASCIIToUTF16("SSL_ERROR"), false);
}

content::DevToolsManagerDelegate*
WebOSContentBrowserClient::GetDevToolsManagerDelegate() {
  return new WebOSDevToolsManagerDelegate;
}

void WebOSContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  command_line->AppendSwitch(switches::kNoSandbox);
  command_line->AppendSwitch(switches::kWebOSWAM);

  // Append v8 snapshot path if exists
  auto iter = v8_snapshot_pathes_.find(child_process_id);
  if (iter != v8_snapshot_pathes_.end()) {
    command_line->AppendSwitchPath(switches::kV8SnapshotBlobPath,
                                   base::FilePath(iter->second));
    v8_snapshot_pathes_.erase(iter);
  }

  // Append v8 extra flags if exists
  iter = v8_extra_flags_.find(child_process_id);
  if (iter != v8_extra_flags_.end()) {
    std::string js_flags = iter->second;
    // If already has, append it also
    if (command_line->HasSwitch(switches::kJavaScriptFlags)) {
      js_flags.append(" ");
      js_flags.append(
          command_line->GetSwitchValueASCII(switches::kJavaScriptFlags));
    }
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, js_flags);
    v8_extra_flags_.erase(iter);
  }

  // Append native scroll related flags if native scroll is on by appinfo.json
  auto iter_ns = use_native_scroll_map_.find(child_process_id);
  if (iter_ns != use_native_scroll_map_.end()) {
    bool use_native_scroll = iter_ns->second;
    if (use_native_scroll) {
      // Enables EnableNativeScroll, which is only enabled when there is
      // 'useNativeScroll': true in appinfo.json. If this flag is enabled,
      // Duration of the scroll offset animation is modified.
      if (!command_line->HasSwitch(switches::kEnableWebOSNativeScroll))
        command_line->AppendSwitch(switches::kEnableWebOSNativeScroll);

      // Enables SmoothScrolling, which is mandatory to enable
      // CSSOMSmoothScroll.
      if (!command_line->HasSwitch(switches::kEnableSmoothScrolling))
        command_line->AppendSwitch(switches::kEnableSmoothScrolling);

      // Adds CSSOMSmoothScroll to EnableBlinkFeatures.
      std::string enable_blink_features_flags = "CSSOMSmoothScroll";
      if (command_line->HasSwitch(switches::kEnableBlinkFeatures)) {
        enable_blink_features_flags.append(",");
        enable_blink_features_flags.append(
            command_line->GetSwitchValueASCII(switches::kEnableBlinkFeatures));
      }
      command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                      enable_blink_features_flags);

      // Enables PreferCompositingToLCDText. If this flag is enabled, Compositor
      // thread handles scrolling and disable LCD-text(AntiAliasing) in the
      // scroll area.
      // See PaintLayerScrollableArea.cpp::layerNeedsCompositingScrolling()
      if (!command_line->HasSwitch(switches::kEnablePreferCompositingToLCDText))
        command_line->AppendSwitch(switches::kEnablePreferCompositingToLCDText);

      // Sets kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll.
      // If this value is provided from command line argument, then propagate
      // the value to render process. If not, initialize this flag as default
      // value.
      static const int kDefaultGestureScrollDistanceOnNativeScroll = 180;
      // We should find in browser's switch value.
      if (base::CommandLine::ForCurrentProcess()->HasSwitch(
              switches::
                  kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll)) {
        std::string propagated_value(
            base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
                switches::
                    kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll));
        command_line->AppendSwitchASCII(
            switches::kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll,
            propagated_value);
      } else
        command_line->AppendSwitchASCII(
            switches::kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll,
            std::to_string(kDefaultGestureScrollDistanceOnNativeScroll));

      RAW_PMLOG_INFO(
          "[UseNativeScroll]",
          "kEnableNativeScroll: %s, kEnableBlinkFeatures: %s, "
          "kEnablePreferCompositingToLCDText: %s, kEnableSmoothScrolling: %s, "
          "kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll: %s",
          command_line->HasSwitch(switches::kEnableWebOSNativeScroll) ? "true"
                                                                      : "false",
          (command_line->GetSwitchValueASCII(switches::kEnableBlinkFeatures)
               .find("CSSOMSmoothScroll") != std::string::npos)
              ? "true"
              : "false",
          command_line->HasSwitch(switches::kEnablePreferCompositingToLCDText)
              ? "true"
              : "false",
          command_line->HasSwitch(switches::kEnableSmoothScrolling) ? "true"
                                                                    : "false",
          command_line
              ->GetSwitchValueASCII(
                  switches::
                      kCustomMouseWheelGestureScrollDeltaOnWebOSNativeScroll)
              .c_str());
    }

    use_native_scroll_map_.erase(iter_ns);
  }
}

void WebOSContentBrowserClient::SetV8SnapshotPath(int child_process_id,
                                                  const std::string& path) {
  v8_snapshot_pathes_.insert(
      std::pair<int, std::string>(child_process_id, path));
}

void WebOSContentBrowserClient::SetV8ExtraFlags(int child_process_id,
                                                const std::string& flags) {
  v8_extra_flags_.insert(std::pair<int, std::string>(child_process_id, flags));
}

void WebOSContentBrowserClient::SetUseNativeScroll(int child_process_id,
                                                   bool use_native_scroll) {
  use_native_scroll_map_.insert(
      std::pair<int, bool>(child_process_id, use_native_scroll));
}

void WebOSContentBrowserClient::ResourceDispatcherHostCreated() {
  resource_dispatcher_host_delegate_.reset(
      new WebOSResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_host_delegate_.get());
}

#if defined(ENABLE_PLUGINS)
void WebOSContentBrowserClient::DidCreatePpapiPlugin(
    content::BrowserPpapiHost* browser_host) {
  browser_host->GetPpapiHost()->AddHostFactoryFilter(
      std::unique_ptr<ppapi::host::HostFactory>(
          new WebOSPepperHostFactory(browser_host)));
}
#endif

void WebOSContentBrowserClient::SetApplicationLocale(
    const std::string& locale) {
  if (current_locale_ == locale)
    return;

  current_locale_ = locale;
  base::i18n::SetICUDefaultLocale(locale);

  if (ResourceBundle::HasSharedInstance()) {
    ResourceBundle::GetSharedInstance().ReloadLocaleResources(locale);
    ResourceBundle::GetSharedInstance().ReloadFonts();
  }

  // notify locale change for accessibility
  content::AccessibilityLocalizationWebos::GetInstance()->SetLocale(locale);

  for (content::RenderProcessHost::iterator it(
           content::RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    it.GetCurrentValue()->NotifyLocaleChanged(locale);
  }
}

void WebOSContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    content::WebPreferences* prefs) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
}

}  // namespace webos
