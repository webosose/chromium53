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

#ifndef WEBOS_WEBVIEW_H_
#define WEBOS_WEBVIEW_H_

#include "base/memory/memory_pressure_listener.h"
#include "base/timer/timer.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/WebKit/public/platform/WebPageVisibilityState.h"
#include "webos/common/webos_constants.h"

#include <memory>

namespace content {
class RenderWidgetHost;
class WebContents;
class WebPreferences;
} // namespace content

class WebOSEvent;

namespace webos {

class WebOSBrowserContext;
class WebViewDelegate;
class WebViewProfile;

class WebView : public content::WebContentsDelegate,
                public content::WebContentsObserver {
 public:
  enum Attribute {
    AllowRunningInsecureContent,
    AllowScriptsToCloseWindows,
    AllowUniversalAccessFromFileUrls,
    SuppressesIncrementalRendering,
    DisallowScrollbarsInMainFrame,
    DisallowScrollingInMainFrame,
    JavascriptCanOpenWindows,
    SpatialNavigationEnabled,
    SupportsMultipleWindows,
    CSSNavigationEnabled,
    V8DateUseSystemLocaloffset,
    AllowLocalResourceLoad,
    LocalStorageEnabled,
    WebSecurityEnabled,
    FixedPositionCreatesStackingContext,
    KeepAliveWebApp,
    AdditionalFontFamilyEnabled,
    BackHistoryAPIDisabled,
    ForceVideoTexture,
    NotifyFMPDirectly,
    NetworkStableTimeout
  };

  enum FontFamily {
    StandardFont,
    FixedFont,
    SerifFont,
    SansSerifFont,
    CursiveFont,
    FantasyFont
  };

  static void SetFileAccessBlocked(bool blocked);

  WebView(int width, int height);
  virtual ~WebView();

  WebViewProfile* GetProfile() const;
  void SetProfile(WebViewProfile* profile);

  void Initialize(WebViewDelegate* delegate,
                  const std::string& v8_snapshot_path,
                  const std::string& v8_extra_flags,
                  bool use_native_scroll = false);

  void CreateWebContents(WebViewDelegate* delegate);
  content::WebContents* GetWebContents();
  void AddUserStyleSheet(const std::string& sheet);
  std::string DefaultUserAgent() const;
  std::string UserAgent() const;
  void LoadUrl(const GURL& url);
  void StopLoading();
  void LoadExtension(const std::string& name);
  void ClearExtensions();
  void ReplaceBaseURL(const GURL& newUrl);
  const std::string& GetUrl();

  void SuspendDOM();
  void ResumeDOM();
  void SuspendMedia();
  void ResumeMedia();
  void SuspendPaintingAndSetVisibilityHidden();
  void ResumePaintingAndSetVisibilityVisible();
  void DropAllPeerConnections(DropPeerConnectionReason reason);

  std::string DocumentTitle() const;
  void RunJavaScript(const std::string& jsCode);
  void RunJavaScriptInAllFrames(const std::string& jsCode);
  void Reload();
  int RenderProcessPid() const;

  void SetInspectable(bool enable);
  void AddCustomPluginDir(const std::string& directory);
  void SetBackgroundColor(int r, int g, int b, int alpha);
  void SetAllowFakeBoldText(bool allowFakeBoldText);
  void SetShouldSuppressDialogs(bool suppress);
  void SetAppId(const std::string& appId);
  void SetAcceptLanguages(const std::string& languages);
  void SetUseLaunchOptimization(bool enabled, int delayMs);
  void SetUseEnyoOptimization(bool enabled);
  void SetBlockWriteDiskcache(bool blocked);
  void SetTransparentBackground(bool enable);
  void SetBoardType(const std::string& boardType);
  void SetMediaCodecCapability(const std::string& capability);
  void SetSupportDolbyHDRContents(bool support);
  void UpdatePreferencesAttribute(WebView::Attribute attribute,
                                  bool on);
  void UpdatePreferencesAttribute(WebView::Attribute attribute, double value);
  void SetFontFamily(WebView::FontFamily fontFamily,
                     const std::string& font);
  void SetFontHintingNone();
  void SetFontHintingSlight();
  void SetFontHintingMedium();
  void SetFontHintingFull();
  void SetActiveOnNonBlankPaint(bool active);
  void SetViewportSize(int width, int height);
  void NotifyMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel level);
  void SetVisible(bool visible);
  void SetDatabaseIdentifier(const std::string& identifier);
  void SetVisibilityState(blink::WebPageVisibilityState visibilityState);
  void DeleteWebStorages(const std::string& identifier);
  void SetFocus(bool focus);
  void SetZoomFactor(double factor);
  void SetDoNotTrack(bool dnt);
  void ForwardWebOSEvent(WebOSEvent* event);
  bool CanGoBack() const;
  void RequestGetCookies(const std::string& url);
  void SetHardwareResolution(int width, int height);
  void SetEnableHtmlSystemKeyboardAttr(bool enable);

  bool IsKeyboardVisible() const;
  bool IsInputMethodActive() const;

  // WebContentsDelegate
  void LoadProgressChanged(content::WebContents* source,
                           double progress) override;

  void CloseContents(content::WebContents* source) override;
#if 0 // need to cherry-pick upper codes
  void DidHistoryBackOnTopPage() override;
#endif
  void NavigationStateChanged(content::WebContents* source,
                              content::InvalidateTypes changed_flags) override;
  bool DecidePolicyForResponse(bool isMainFrame,
                               int statusCode,
                               const GURL& url,
                               const base::string16& statusText) override;
  void DidFirstFrameFocused() override;
  bool ShouldSuppressDialogs(content::WebContents* source) override;
  gfx::Size GetSizeForNewRenderView(
      content::WebContents* web_contents) const override;

  // WebContentsObserver
  void DidStartLoading() override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL> &candidates) override;
  void DidCommitProvisionalLoadForFrame(
      content::RenderFrameHost* render_frame_host,
      const GURL& url,
      ui::PageTransition transition_type) override;
  void DidFailProvisionalLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& validated_url,
                              int error_code,
                              const base::string16& error_description,
                              bool was_ignored_by_handler) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description,
                   bool was_ignored_by_handler) override;
  bool OnMessageReceived(const IPC::Message& message) override;
#if defined(USE_SPLASH_SCREEN)
  void WillSwapMeaningfulPaint(double detected_time) override;
#endif

  // content::WebContentsObserver implementation:
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;
  void RenderProcessCreated(base::ProcessHandle handle) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DocumentLoadedInFrame(content::RenderFrameHost* frame_host) override;
  void DidDropAllPeerConnections(
      content::DropPeerConnectionReason reason) override;
  void DidSwapCompositorFrame(content::RenderWidgetHost* render_widget_host) override;

  void UpdatePreferences();

  void SetAudioGuidanceOn(bool on);
  void ResetStateToMarkNextPaintForContainer();
  void SetTrustLevel(const std::string& trust_level);
  void SetAppPath(const std::string& app_path);

  void EnterFullscreenModeForTab(content::WebContents* web_contents,
                                 const GURL& origin) override;
  void ExitFullscreenModeForTab(content::WebContents* web_contents) override;
  bool IsFullscreenForTabOrPending(
      const content::WebContents* web_contents) const override;
  bool CheckMediaAccessPermission(content::WebContents* web_contents,
                                  const GURL& security_origin,
                                  content::MediaStreamType type) override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) override;

  void SetSSLCertErrorPolicy(SSLCertErrorPolicy policy) {
    ssl_cert_error_policy_ = policy;
  }
  SSLCertErrorPolicy GetSSLCertErrorPolicy() const {
    return ssl_cert_error_policy_;
  }

 private:
  static void SendGetCookiesResponse(WebView* webview,
                                     const std::string& cookies);
  void OnBrowserControlCommand(const std::string& command,
                               const std::vector<std::string>& arguments);
  void OnBrowserControlFunction(const std::string& command,
                                const std::vector<std::string>& arguments,
                                std::string* result);
  void OnDidClearWindowObject();
#if defined(USE_SPLASH_SCREEN)
  void OnDidFirstMeaningfulPaint(content::RenderFrameHost* render_frame_host,
                                 double fmp_detected);
  void OnDidNonFirstMeaningfulPaint(
      content::RenderFrameHost* render_frame_host);
  void LoadVisuallyCommittedIfNeed();
  void LoadVisuallyCommitted();
#endif

  void PushStateToIOThread();
  void RemoveStateFromIOThread(content::WebContents* web_contents);

  WebViewProfile* profile_;
  WebViewDelegate* m_delegate;
  std::unique_ptr<content::WebContents> m_webContents;
  std::unique_ptr<content::WebPreferences> m_webPreferences;
  bool m_shouldSuppressDialogs;

  int m_width;
  int m_height;

  std::string app_path_;
  std::string trust_level_;
  bool fullscreen_;

#if defined(USE_SPLASH_SCREEN)
  double first_meaningful_paint_detected_;
  double arriving_meaningful_paint_;
  bool load_visually_committed_;
#endif
  std::string accept_language_;

  SSLCertErrorPolicy ssl_cert_error_policy_;
};

} // namespace webos

#endif // WEBOS_WEBOS_WEB_CONTENTS_DELEGATE_H_
