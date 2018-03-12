// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/webview_base.h"

#include "base/memory/memory_pressure_listener.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/WebKit/public/platform/WebPageVisibilityState.h"
#include "webos/common/webos_event.h"
#include "webos/webview.h"

namespace webos {

void WebViewBase::SetFileAccessBlocked(bool blocked) {
  WebView::SetFileAccessBlocked(blocked);
}

WebViewBase::WebViewBase(int width, int height)
    : m_webview(new WebView(width, height)) {}

WebViewBase::~WebViewBase() {
  delete m_webview;
}

WebViewProfile* WebViewBase::GetProfile() const {
  return m_webview->GetProfile();
}

void WebViewBase::SetProfile(WebViewProfile* profile) {
  m_webview->SetProfile(profile);
}

void WebViewBase::Initialize(const std::string& app_id,
                             const std::string& app_path,
                             const std::string& trust_level,
                             const std::string& v8_snapshot_path,
                             const std::string& v8_extra_flags,
                             bool use_native_scroll) {
  SetAppPath(app_path);
  SetTrustLevel(trust_level);

  m_webview->Initialize(this, v8_snapshot_path, v8_extra_flags,
                        use_native_scroll);

  if (GetWebContents()) {
    //The arcitecture of using WebContentsImpl is not good way and is not what we want to do
    //Need to change it
    content::RenderViewHost *rvh = GetWebContents()->GetRenderViewHost();
    if (!rvh->IsRenderViewLive()) {
      content::FrameReplicationState state; // FIXME(jb) how can I get this?
      static_cast<content::WebContentsImpl*>(
          GetWebContents())->
              CreateRenderViewForRenderManager(rvh,
                                               MSG_ROUTING_NONE,
                                               MSG_ROUTING_NONE,
                                               state);
    }
  }

  SetAppId(app_id);
}

content::WebContents* WebViewBase::GetWebContents() {
  return m_webview->GetWebContents();
}

void WebViewBase::AddUserStyleSheet(const std::string& sheet)
{
  m_webview->AddUserStyleSheet(sheet);
}

std::string WebViewBase::DefaultUserAgent() const {
  return m_webview->DefaultUserAgent();
}

std::string WebViewBase::UserAgent() const {
  return m_webview->UserAgent();
}

void WebViewBase::LoadUrl(const std::string& url) {
  m_webview->LoadUrl(GURL(url));
}

void WebViewBase::StopLoading() {
  m_webview->StopLoading();
}

void WebViewBase::LoadExtension(const std::string& name) {
  m_webview->LoadExtension(name);
}

void WebViewBase::ClearExtensions() {
  m_webview->ClearExtensions();
}

void WebViewBase::EnableInspectablePage() {
  GetWebContents()->EnableInspectable();
}

void WebViewBase::SetInspectable(bool enable) {
  m_webview->SetInspectable(enable);
}

void WebViewBase::AddAvailablePluginDir(const std::string& directory) {
  GetWebContents()->AddAvailablePluginDir(directory.c_str());
}

void WebViewBase::AddCustomPluginDir(const std::string& directory) {
  m_webview->AddCustomPluginDir(directory);
}

void WebViewBase::SetUserAgent(const std::string& useragent) {
  GetWebContents()->SetUserAgentOverride(useragent);
}

void WebViewBase::SetBackgroundColor(int r, int g, int b, int alpha) {
  m_webview->SetBackgroundColor(r, g, b, alpha);
}

void WebViewBase::SetAllowFakeBoldText(bool allowFakeBoldText) {
  m_webview->SetAllowFakeBoldText(allowFakeBoldText);
}

void WebViewBase::SetShouldSuppressDialogs(bool suppress) {
  m_webview->SetShouldSuppressDialogs(suppress);
}

void WebViewBase::SetAppId(const std::string& appId) {
  m_webview->SetAppId(appId);
}

void WebViewBase::SetAcceptLanguages(const std::string& languages) {
  m_webview->SetAcceptLanguages(languages);
}

void WebViewBase::SetUseLaunchOptimization(bool enabled) {
  m_webview->SetUseLaunchOptimization(enabled);
}

void WebViewBase::SetUseEnyoOptimization(bool enabled) {
  m_webview->SetUseEnyoOptimization(enabled);
}

void WebViewBase::SetUseAccessibility(bool enabled) {
  if (enabled)
    GetWebContents()->EnableCompleteAccessibilityMode();
}

void WebViewBase::SetBlockWriteDiskcache(bool blocked) {
  m_webview->SetBlockWriteDiskcache(blocked);
}

void WebViewBase::SetTransparentBackground(bool enable) {
  m_webview->SetTransparentBackground(enable);
}

void WebViewBase::SetBoardType(const std::string& boardType) {
  m_webview->SetBoardType(boardType);
}

void WebViewBase::SetMediaCodecCapability(const std::string& capability) {
  m_webview->SetMediaCodecCapability(capability);
}

void WebViewBase::SetSupportDolbyHDRContents(bool support) {
  m_webview->SetSupportDolbyHDRContents(support);
}

void WebViewBase::SetViewportSize(int width, int height) {
  m_webview->SetViewportSize(width, height);
}

void WebViewBase::NotifyMemoryPressure(MemoryPressureLevel level) {
  base::MemoryPressureListener::MemoryPressureLevel pressure_level =
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
  if (level == MemoryPressureLevel::MEMORY_PRESSURE_LOW) {
    pressure_level =
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;
  } else if (level == MemoryPressureLevel::MEMORY_PRESSURE_CRITICAL) {
    pressure_level =
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
  }

  m_webview->NotifyMemoryPressure(pressure_level);
}

void WebViewBase::SetVisible(bool visible) {
  m_webview->SetVisible(visible);
}

void WebViewBase::SetVisibilityState(WebPageVisibilityState visibilityState) {
  m_webview->SetVisibilityState(
      static_cast<blink::WebPageVisibilityState>(visibilityState));
}

void WebViewBase::DeleteWebStorages(const std::string& identifier) {
  m_webview->DeleteWebStorages(identifier);
}

std::string WebViewBase::DocumentTitle() const {
  return m_webview->DocumentTitle();
}

void WebViewBase::SuspendWebPageDOM() {
  m_webview->SuspendDOM();
}

void WebViewBase::ReplaceBaseURL(const std::string& newUrl,
                                 const std::string& oldUrl) {
  if (newUrl == oldUrl)
    return;

  m_webview->ReplaceBaseURL(GURL(newUrl));
}

void WebViewBase::ResumeWebPageDOM() {
  m_webview->ResumeDOM();
}

void WebViewBase::SuspendWebPageMedia() {
  m_webview->SuspendMedia();
}

void WebViewBase::ResumeWebPageMedia() {
  m_webview->ResumeMedia();
}

void WebViewBase::SuspendPaintingAndSetVisibilityHidden() {
  m_webview->SuspendPaintingAndSetVisibilityHidden();
}

void WebViewBase::ResumePaintingAndSetVisibilityVisible() {
  m_webview->ResumePaintingAndSetVisibilityVisible();
}

const std::string& WebViewBase::GetUrl() {
  return m_webview->GetUrl();
}

void WebViewBase::RunJavaScript(const std::string& jsCode) {
  m_webview->RunJavaScript(jsCode);
}

void WebViewBase::RunJavaScriptInAllFrames(const std::string& jsCode) {
  m_webview->RunJavaScriptInAllFrames(jsCode);
}

void WebViewBase::Reload() {
  m_webview->Reload();
}

int WebViewBase::RenderProcessPid() const {
  return m_webview->RenderProcessPid();
}

void WebViewBase::SetFocus(bool focus) {
  m_webview->SetFocus(focus);
}

void WebViewBase::SetZoomFactor(double factor) {
  m_webview->SetZoomFactor(factor);
}

void WebViewBase::SetDoNotTrack(bool dnt) {
  m_webview->SetDoNotTrack(dnt);
}

void WebViewBase::ForwardWebOSEvent(WebOSEvent* event) {
  m_webview->ForwardWebOSEvent(event);
}

bool WebViewBase::CanGoBack() const {
  return m_webview->CanGoBack();
}

void WebViewBase::RequestGetCookies(const std::string& url) {
  m_webview->RequestGetCookies(url);
}

bool WebViewBase::IsKeyboardVisible() {
  return m_webview->IsKeyboardVisible();
}

bool WebViewBase::IsInputMethodActive() {
  return m_webview->IsInputMethodActive();
}

void WebViewBase::SetHardwareResolution(int width, int height) {
  m_webview->SetHardwareResolution(width, height);
}

void WebViewBase::SetEnableHtmlSystemKeyboardAttr(bool enable) {
  m_webview->SetEnableHtmlSystemKeyboardAttr(enable);
}

void WebViewBase::DropAllPeerConnections(DropPeerConnectionReason reason) {
  m_webview->DropAllPeerConnections(reason);
}

//WebPreferences
void WebViewBase::SetAllowRunningInsecureContent(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::AllowRunningInsecureContent, on);
}

void WebViewBase::SetAllowScriptsToCloseWindows(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::AllowScriptsToCloseWindows, on);
}

void WebViewBase::SetAllowUniversalAccessFromFileUrls(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::AllowUniversalAccessFromFileUrls, on);
}

void WebViewBase::SetSuppressesIncrementalRendering(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::SuppressesIncrementalRendering, on);
}

void WebViewBase::SetDisallowScrollbarsInMainFrame(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::DisallowScrollbarsInMainFrame, on);
}

void WebViewBase::SetDisallowScrollingInMainFrame(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::DisallowScrollingInMainFrame, on);
}

void WebViewBase::SetJavascriptCanOpenWindows(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::JavascriptCanOpenWindows, on);
}

void WebViewBase::SetSpatialNavigationEnabled(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::SpatialNavigationEnabled, on);
}

void WebViewBase::SetSupportsMultipleWindows(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::SupportsMultipleWindows, on);
}

void WebViewBase::SetCSSNavigationEnabled(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::CSSNavigationEnabled, on);
}

void WebViewBase::SetV8DateUseSystemLocaloffset(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::V8DateUseSystemLocaloffset, on);
}

void WebViewBase::SetAllowLocalResourceLoad(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::AllowLocalResourceLoad, on);
}

void WebViewBase::SetLocalStorageEnabled(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::LocalStorageEnabled, on);
}

void WebViewBase::SetWebSecurityEnabled(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::WebSecurityEnabled, on);
}

void WebViewBase::SetFixedPositionCreatesStackingContext(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::FixedPositionCreatesStackingContext, on);
}

void WebViewBase::SetKeepAliveWebApp(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::KeepAliveWebApp, on);
}

void WebViewBase::SetAdditionalFontFamilyEnabled(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::AdditionalFontFamilyEnabled, on);
}

void WebViewBase::SetDatabaseIdentifier(const std::string& identifier) {
  m_webview->SetDatabaseIdentifier(identifier);
}

void WebViewBase::SetBackHistoryAPIDisabled(const bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::BackHistoryAPIDisabled, on);
}

void WebViewBase::SetForceVideoTexture(bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::ForceVideoTexture, on);
}

void WebViewBase::SetNotifyFMPDirectly(const bool on) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::NotifyFMPDirectly, on);
}

void WebViewBase::SetNetworkStableTimeout(const double timeout) {
  m_webview->UpdatePreferencesAttribute(
      WebView::Attribute::NetworkStableTimeout, timeout);
}

//FontFamily
void WebViewBase::SetStandardFontFamily(const std::string& font) {
  m_webview->SetFontFamily(WebView::FontFamily::StandardFont, font);
}

void WebViewBase::SetFixedFontFamily(const std::string& font) {
  m_webview->SetFontFamily(WebView::FontFamily::FixedFont, font);
}

void WebViewBase::SetSerifFontFamily(const std::string& font) {
  m_webview->SetFontFamily(WebView::FontFamily::SerifFont, font);
}

void WebViewBase::SetSansSerifFontFamily(const std::string& font) {
  m_webview->SetFontFamily(WebView::FontFamily::SansSerifFont, font);
}

void WebViewBase::SetCursiveFontFamily(const std::string& font) {
  m_webview->SetFontFamily(WebView::FontFamily::CursiveFont, font);
}

void WebViewBase::SetFantasyFontFamily(const std::string& font) {
  m_webview->SetFontFamily(WebView::FontFamily::FantasyFont, font);
}

void WebViewBase::SetFontHinting(WebViewBase::FontRenderParams hinting) {
  switch (hinting) {
    case WebViewBase::FontRenderParams::HINTING_NONE :
      m_webview->SetFontHintingNone();
      return;
    case WebViewBase::FontRenderParams::HINTING_SLIGHT :
      m_webview->SetFontHintingSlight();
      return;
    case WebViewBase::FontRenderParams::HINTING_MEDIUM :
      m_webview->SetFontHintingMedium();
      return;
    case WebViewBase::FontRenderParams::HINTING_FULL :
      m_webview->SetFontHintingFull();
      return;
    default :
      m_webview->SetFontHintingNone();
      return;
  }
}

void WebViewBase::UpdatePreferences() {
  m_webview->UpdatePreferences();
}

void WebViewBase::SetAudioGuidanceOn(bool on) {
  m_webview->SetAudioGuidanceOn(on);
}

void WebViewBase::ResetStateToMarkNextPaintForContainer() {
#if defined(USE_SPLASH_SCREEN)
  m_webview->ResetStateToMarkNextPaintForContainer();
#endif
}

void WebViewBase::SetTrustLevel(const std::string& trust_level) {
  m_webview->SetTrustLevel(trust_level);
}

void WebViewBase::SetAppPath(const std::string& app_path) {
  m_webview->SetAppPath(app_path);
}

void WebViewBase::SetSSLCertErrorPolicy(SSLCertErrorPolicy policy) {
  m_webview->SetSSLCertErrorPolicy(policy);
}

} //namespace webos
