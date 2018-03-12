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

#ifndef WEBOS_WEBVIEW_BASE_H_
#define WEBOS_WEBVIEW_BASE_H_

#include <string>
#include <vector>

#include "third_party/WebKit/public/platform/WebColor.h"
#include "webos/common/webos_constants.h"
#include "webos/common/webos_export.h"
#include "webos/webview_delegate.h"

namespace content {
class WebContents;
} // namespace content

class WebOSEvent;

namespace webos {

class WebView;
class WebViewProfile;

class WEBOS_EXPORT WebViewBase : public WebViewDelegate {
 public:
  enum FontRenderParams {
    HINTING_NONE,
    HINTING_SLIGHT,
    HINTING_MEDIUM,
    HINTING_FULL
  };

  // Originally, WebPageVisibilityState.h, PageVisibilityState.h
  enum WebPageVisibilityState {
    WebPageVisibilityStateVisible,
    WebPageVisibilityStateHidden,
    WebPageVisibilityStateLaunching,
    WebPageVisibilityStatePrerender,
    WebPageVisibilityStateLast = WebPageVisibilityStatePrerender
  };

  enum MemoryPressureLevel {
    MEMORY_PRESSURE_NONE = 0,
    MEMORY_PRESSURE_LOW = 1,
    MEMORY_PRESSURE_CRITICAL = 2
  };

  static void SetFileAccessBlocked(bool blocked);

  WebViewBase(int width = 1920, int height = 1080);
  virtual ~WebViewBase();

  WebViewProfile* GetProfile() const;
  void SetProfile(WebViewProfile* profile);

  void Initialize(const std::string& app_id,
                  const std::string& app_path,
                  const std::string& trust_level,
                  const std::string& v8_snapshot_path,
                  const std::string& v8_extra_flags,
                  bool use_native_scroll = false);

  void AddUserStyleSheet(const std::string& sheet);
  std::string DefaultUserAgent() const;
  std::string UserAgent() const;
  void LoadUrl(const std::string& url);
  void StopLoading();
  void LoadExtension(const std::string& name);
  void ClearExtensions();
  void ReplaceBaseURL(const std::string& newUrl, const std::string& oldUrl);
  void EnableInspectablePage();
  void SetInspectable(bool enable);
  void AddAvailablePluginDir(const std::string& directory);
  void AddCustomPluginDir(const std::string& directory);
  void SetUserAgent(const std::string& useragent);
  void SetBackgroundColor(int r, int g, int b, int alpha);
  void SetShouldSuppressDialogs(bool suppress);
  void SetUseAccessibility(bool enabled);
  void SetViewportSize(int width, int height);
  void NotifyMemoryPressure(MemoryPressureLevel level);
  void SetVisible(bool visible);
  void SetVisibilityState(WebPageVisibilityState visibilityState);
  void DeleteWebStorages(const std::string& identifier);
  std::string DocumentTitle() const;
  void SuspendWebPageDOM();
  void ResumeWebPageDOM();
  void SuspendWebPageMedia();
  void ResumeWebPageMedia();
  void SuspendPaintingAndSetVisibilityHidden();
  void ResumePaintingAndSetVisibilityVisible();
  void RunJavaScript(const std::string& jsCode);
  void RunJavaScriptInAllFrames(const std::string& jsCode);
  void Reload();
  int RenderProcessPid() const;
  void SetFocus(bool focus);
  void SetZoomFactor(double factor);
  void SetDoNotTrack(bool dnt);
  void ForwardWebOSEvent(WebOSEvent* event);
  bool CanGoBack() const;
  void RequestGetCookies(const std::string& url);
  bool IsKeyboardVisible();
  bool IsInputMethodActive();
  void SetHardwareResolution(int width, int height);
  void SetEnableHtmlSystemKeyboardAttr(bool enable);
  void DropAllPeerConnections(DropPeerConnectionReason reason);

  const std::string& GetUrl();
  content::WebContents* GetWebContents();

  //RenderViewHost
  void SetUseLaunchOptimization(bool enabled);
  void SetUseEnyoOptimization(bool enabled);
  void SetBlockWriteDiskcache(bool blocked);
  void SetTransparentBackground(bool enabled);
  void ResetStateToMarkNextPaintForContainer();

  //RenderPreference
  void SetAllowFakeBoldText(bool allowFakeBoldText);
  void SetAppId(const std::string& appId);
  void SetAcceptLanguages(const std::string& lauguages);
  void SetBoardType(const std::string& boardType);
  void SetMediaCodecCapability(const std::string& capability);
  void SetSupportDolbyHDRContents(bool support);

  //WebPreferences
  void SetAllowRunningInsecureContent(bool on);
  void SetAllowScriptsToCloseWindows(bool on);
  void SetAllowUniversalAccessFromFileUrls(bool on);
  void SetSuppressesIncrementalRendering(bool on);
  void SetDisallowScrollbarsInMainFrame(bool on);
  void SetDisallowScrollingInMainFrame(bool on);
  void SetJavascriptCanOpenWindows(bool on);
  void SetSpatialNavigationEnabled(bool on);
  void SetSupportsMultipleWindows(bool on);
  void SetCSSNavigationEnabled(bool on);
  void SetV8DateUseSystemLocaloffset(bool use);
  void SetAllowLocalResourceLoad(bool on);
  void SetLocalStorageEnabled(bool on);
  void SetDatabaseIdentifier(const std::string& identifier);
  void SetWebSecurityEnabled(bool on);
  void SetFixedPositionCreatesStackingContext(bool on);
  void SetKeepAliveWebApp(bool on);
  void SetAdditionalFontFamilyEnabled(bool on);
  void SetForceVideoTexture(bool on);
  void SetNotifyFMPDirectly(bool on);
  void SetNetworkStableTimeout(double timeout);

  //FontFamily
  void SetStandardFontFamily(const std::string& font);
  void SetFixedFontFamily(const std::string& font);
  void SetSerifFontFamily(const std::string& font);
  void SetSansSerifFontFamily(const std::string& font);
  void SetCursiveFontFamily(const std::string& font);
  void SetFantasyFontFamily(const std::string& font);
  void SetFontHinting(WebViewBase::FontRenderParams hinting);

  void UpdatePreferences();

  void SetAudioGuidanceOn(bool on);
  void SetTrustLevel(const std::string& trust_level);
  void SetAppPath(const std::string& app_path);
  void SetBackHistoryAPIDisabled(const bool on);

  void SetSSLCertErrorPolicy(SSLCertErrorPolicy policy);

 private:
  WebView* m_webview;
};

} // namespace webos

#endif // WEBOS_WEBVIEW_BASE_H_
