// Copyright 2016-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/webview.h"

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "components/media_capture_util/devices_dispatcher.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/browser/renderer_host/web_input_event_aura.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/favicon_url.h"
#include "content/public/common/page_zoom.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/web_preferences.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/net_errors.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/ime/input_method.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/font_render_params.h"
#include "webos/webview_delegate.h"
#include "webos/webview_profile.h"
#include "webos/app/webos_content_main_delegate.h"
#include "webos/common/webos_event.h"
#include "webos/common/webos_view_messages.h"
#include "webos/browser/webos_browser_context.h"
#include "webos/browser/webos_browser_context_adapter.h"
#include "webos/browser/webos_webview_renderer_state.h"

#if defined(USE_INJECTIONS)
#include "content/common/browser_control_messages.h"
#include "content/common/injection_messages.h"
#endif

#if defined(ENABLE_PLUGINS)
void GetPluginsCallback(const std::vector<content::WebPluginInfo>& plugins) {
}
#endif

namespace webos {

void WebView::SetFileAccessBlocked(bool blocked) {
  content::BrowserContext::SetFileAccessBlocked(blocked);
}

WebView::WebView(int width, int height)
    : profile_(WebViewProfile::GetDefaultProfile()),
      m_delegate(NULL),
      m_shouldSuppressDialogs(false),
      m_width(width),
      m_height(height),
      fullscreen_(false),
#if defined(USE_SPLASH_SCREEN)
      first_meaningful_paint_detected_(0.),
      arriving_meaningful_paint_(0.),
      load_visually_committed_(false),
#endif
      ssl_cert_error_policy_(SSL_CERT_ERROR_POLICY_DEFAULT) {}

WebView::~WebView() {
  profile_->FlushCookieStore();

  if (m_webContents) {
    RemoveStateFromIOThread(m_webContents.get());
    m_webContents->SetDelegate(NULL);
  }
}

WebViewProfile* WebView::GetProfile() const {
  return profile_;
}

void WebView::SetProfile(WebViewProfile* profile) {
  profile_ = profile;
}

void WebView::Initialize(WebViewDelegate* delegate,
                         const std::string& v8_snapshot_path,
                         const std::string& v8_extra_flags,
                         bool use_native_scroll) {
  m_delegate = delegate;
  CreateWebContents(delegate);

  m_webContents->SetDelegate(this);
  Observe(m_webContents.get());
  content::RenderViewHost* rvh =  m_webContents->GetRenderViewHost();
  rvh->SyncRendererPrefs();
  m_webPreferences.reset(
      new content::WebPreferences(rvh->GetWebkitPreferences()));

  // set v8 snapshot_blob path & extra flags for new renderer
  int id = rvh->GetProcess()->GetID();
  if (!v8_snapshot_path.empty())
    GetWebOSContentBrowserClient()->SetV8SnapshotPath(id, v8_snapshot_path);
  if (!v8_extra_flags.empty())
    GetWebOSContentBrowserClient()->SetV8ExtraFlags(id, v8_extra_flags);
  if (use_native_scroll)
    GetWebOSContentBrowserClient()->SetUseNativeScroll(id, use_native_scroll);

  if (m_delegate) {
    m_webPreferences->allow_mouse_on_off_event =
        m_delegate->AllowMouseOnOffEvent();
    rvh->UpdateWebkitPreferences(*m_webPreferences.get());
  }
}

void WebView::CreateWebContents(WebViewDelegate* delegate) {
  WebOSBrowserContext* browser_context =
      profile_->GetBrowserContextAdapter()->GetBrowserContext();

  content::WebContents::CreateParams params(browser_context, NULL);
  params.routing_id = MSG_ROUTING_NONE;
  params.initial_size = gfx::Size(m_width, m_height);

  m_webContents.reset(content::WebContents::Create(params));

  PushStateToIOThread();
}

content::WebContents* WebView::GetWebContents() {
  return m_webContents.get();
}

void WebView::AddUserStyleSheet(const std::string& sheet) {
  m_webContents->InjectCSS(sheet);
}

std::string WebView::DefaultUserAgent() const {
  return content::BuildUserAgentFromProduct(
             "Chrome/" CHROMIUM_VERSION);
}

std::string WebView::UserAgent() const {
  return m_webContents->GetUserAgentOverride();
}

void WebView::LoadUrl(const GURL& url) {
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_TYPED |
      ui::PAGE_TRANSITION_FROM_API);
  params.frame_name = std::string("");
  params.override_user_agent =
      content::NavigationController::UA_OVERRIDE_TRUE;
  m_webContents->GetController().LoadURLWithParams(params);
}

void WebView::StopLoading() {
  int index = m_webContents->GetController().GetPendingEntryIndex();
  if (index != -1)
    m_webContents->GetController().RemoveEntryAtIndex(index);

  m_webContents->Stop();
  m_webContents->Focus();
}

void WebView::LoadExtension(const std::string& name) {
#if defined(USE_INJECTIONS)
  Send(new InjectionMsg_LoadExtension(routing_id(), name));
#endif
}

void WebView::ClearExtensions() {
#if defined(USE_INJECTIONS)
  Send(new InjectionMsg_ClearExtensions(routing_id()));
#endif
}

void WebView::ReplaceBaseURL (const GURL& newUrl) {
  m_webContents->GetController().ReplaceBaseURL(newUrl);
}

const std::string& WebView::GetUrl() {
  return m_webContents->GetVisibleURL().spec();
}

void WebView::SuspendDOM() {
  content::RenderViewHost* rvh =  m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->SuspendDOM();
}

void WebView::ResumeDOM() {
  content::RenderViewHost* rvh =  m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->ResumeDOM();
}

void WebView::SuspendMedia() {
  content::RenderViewHost* rvh =  m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->SuspendMedia();
}

void WebView::ResumeMedia() {
  content::RenderViewHost* rvh =  m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->ResumeMedia();
}

void WebView::SuspendPaintingAndSetVisibilityHidden() {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          m_webContents->GetRenderViewHost()->GetWidget()->GetView());
  if (host_view) {
    host_view->Hide();
  }
}

void WebView::ResumePaintingAndSetVisibilityVisible() {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          m_webContents->GetRenderViewHost()->GetWidget()->GetView());
  if (host_view) {
    host_view->Show();
  }
}

std::string WebView::DocumentTitle() const {
  return base::UTF16ToUTF8(m_webContents->GetDocumentTitle());
}

void WebView::RunJavaScript(const std::string& jsCode) {
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->GetMainFrame()->ExecuteJavaScript(base::UTF8ToUTF16(jsCode));
}

void WebView::RunJavaScriptInAllFrames(const std::string& jsCode) {
  m_webContents->ExecuteJavaScriptInAllFrames(base::UTF8ToUTF16(jsCode));
}

void WebView::Reload() {
  m_webContents->GetController().Reload(false);
  m_webContents->Focus();
}

int WebView::RenderProcessPid() const {
  return m_webContents->GetRenderProcessHost()->GetHandle();
}

void WebView::SetInspectable(bool enable) {
  if (enable) {
    static_cast<WebOSBrowserMainParts*>(
        GetWebOSContentBrowserClient()->GetMainParts())->EnableDevTools();
  }
}

void WebView::AddCustomPluginDir(const std::string& directory) {
  content::PluginService::GetInstance()->AddExtraPluginDir(
      base::FilePath(directory));
  content::PluginService::GetInstance()->RefreshPlugins();
}

void WebView::SetBackgroundColor(int r, int g, int b, int alpha) {
  Send(new WebOSViewMsg_SetBackgroundColor(routing_id(),
                                           SkColorSetARGB(alpha, r, g, b)));
}

void WebView::SetAllowFakeBoldText(bool allowFakeBoldText) {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  if (rendererPrefs->allow_fake_bold_text == allowFakeBoldText)
    return;

  rendererPrefs->allow_fake_bold_text = allowFakeBoldText;
}

void WebView::LoadProgressChanged(content::WebContents* source,
                                  double progress) {
  if (m_delegate)
    m_delegate->OnLoadProgressChanged(progress);
}

void WebView::CloseContents(content::WebContents* source) {
  if (m_delegate)
    m_delegate->Close();
}

#if 0 // need to cherry-pick upper codes
void WebView::DidHistoryBackOnTopPage() {
  if (m_delegate)
    m_delegate->DidHistoryBackOnTopPage();
}
#endif

void WebView::NavigationStateChanged(content::WebContents* source,
                                     content::InvalidateTypes changed_flags) {
  if (m_delegate && (content::INVALIDATE_TYPE_TITLE & changed_flags))
    m_delegate->TitleChanged(base::UTF16ToUTF8(source->GetTitle()));
}

bool WebView::DecidePolicyForResponse(bool isMainFrame, int statusCode,
                                      const GURL& url,
                                      const base::string16& statusText) {
  if (m_delegate)
    return m_delegate->DecidePolicyForResponse(
        isMainFrame,
        statusCode,
        url.spec(),
        base::UTF16ToUTF8(statusText));

  return false;
}

void WebView::DidFirstFrameFocused() {
  if (m_delegate)
    m_delegate->DidFirstFrameFocused();
}

gfx::Size WebView::GetSizeForNewRenderView(content::WebContents* web_contents) const {
  return gfx::Size(m_width, m_height);
}

bool WebView::ShouldSuppressDialogs(content::WebContents* source) {
  return m_shouldSuppressDialogs;
}

void WebView::EnterFullscreenModeForTab(content::WebContents* web_contents,
                                        const GURL& origin) {
  fullscreen_ = true;

  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (!rvh)
    return;
  content::RenderWidgetHost* rwh = rvh->GetWidget();
  if (rwh)
    rwh->WasResized();
}

void WebView::ExitFullscreenModeForTab(content::WebContents* web_contents) {
  fullscreen_ = false;

  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (!rvh)
    return;
  content::RenderWidgetHost* rwh = rvh->GetWidget();
  if (rwh)
    rwh->WasResized();
}

bool WebView::IsFullscreenForTabOrPending(
    const content::WebContents* web_contents) const {
  return fullscreen_;
}

bool WebView::CheckMediaAccessPermission(content::WebContents* web_contents,
                                         const GURL& security_origin,
                                         content::MediaStreamType type) {
  switch (type) {
    case content::MEDIA_DEVICE_AUDIO_CAPTURE:
      return m_delegate->AcceptsAudioCapture();
    case content::MEDIA_DEVICE_VIDEO_CAPTURE:
      return m_delegate->AcceptsVideoCapture();
  }
  return false;
}

void WebView::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  media_capture_util::DevicesDispatcher::GetInstance()
      ->ProcessMediaAccessRequest(web_contents, request,
                                  m_delegate->AcceptsVideoCapture(),
                                  m_delegate->AcceptsAudioCapture(), callback);
}

void WebView::DropAllPeerConnections(DropPeerConnectionReason reason) {
  content::DropPeerConnectionReason content_reason;
  switch (reason) {
    case DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN:
      content_reason = content::DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN;
      break;
    default:
      content_reason = content::DROP_PEER_CONNECTION_REASON_UNKNOWN;
  }
  m_webContents->DropAllPeerConnections(content_reason);
}

void WebView::SetShouldSuppressDialogs(bool suppress) {
  m_shouldSuppressDialogs = suppress;
}

void WebView::SetAppId(const std::string& appId) {
  // TODO(jose.dapena): app id

  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  if (!rendererPrefs->application_id.compare(appId))
    return;

  rendererPrefs->application_id = appId;
}

void WebView::SetAcceptLanguages(const std::string& languages) {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  if (!rendererPrefs->accept_languages.compare(languages))
    return;

  rendererPrefs->accept_languages = languages;
  accept_language_ = net::HttpUtil::GenerateAcceptLanguageHeader(languages);

  PushStateToIOThread();
}

void WebView::SetUseLaunchOptimization(bool enabled, int delayMs) {
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->SetUseLaunchOptimization(enabled, delayMs);
}

void WebView::SetUseEnyoOptimization(bool enabled) {
  // TODO(jose.dapena): enyo optimization
#if 0
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->SetUseEnyoOptimization(enabled);
#endif
}

void WebView::SetBlockWriteDiskcache(bool blocked) {
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->SetBlockWriteDiskcache(blocked);
}

void WebView::SetTransparentBackground(bool enable) {
  content::RenderWidgetHostView * const host_view =
      m_webContents->GetRenderWidgetHostView();
  if (host_view)
    host_view->SetBackgroundColor(enable ? SK_ColorTRANSPARENT : SK_ColorBLACK);
}

void WebView::SetBoardType(const std::string& boardType) {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  if (!rendererPrefs->board_type.compare(boardType))
    return;

  rendererPrefs->board_type = boardType;
}

void WebView::SetMediaCodecCapability(const std::string& capability) {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  if (!rendererPrefs->media_codec_capability.compare(capability))
    return;

  rendererPrefs->media_codec_capability = capability;
}

void WebView::SetSupportDolbyHDRContents(bool support) {
  // TODO(jose.dapena): ums integration
#if 0
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  if (rendererPrefs->support_dolby_hdr_contents == support)
    return;

  rendererPrefs->support_dolby_hdr_contents = support;
#endif
}

void WebView::UpdatePreferencesAttribute(
    WebView::Attribute attribute,
    bool on) {
  switch (attribute) {
    case Attribute::AllowRunningInsecureContent :
      m_webPreferences->allow_running_insecure_content = on;
      break;
    case Attribute::AllowScriptsToCloseWindows :
      m_webPreferences->allow_scripts_to_close_windows = on;
      break;
    case Attribute::AllowUniversalAccessFromFileUrls :
      m_webPreferences->allow_universal_access_from_file_urls = on;
      break;
      // TODO: remove incremental rendering suppression API
#if 0
    case Attribute::SuppressesIncrementalRendering :
      m_webPreferences->suppresses_incremental_rendering = on;
      break;
#endif
    case Attribute::DisallowScrollbarsInMainFrame :
      m_webPreferences->disallow_scrollbars_in_main_frame = on;
      break;
#if 0 // TODO(jose.dapena): custom scroll policy
    case Attribute::DisallowScrollingInMainFrame :
      m_webPreferences->disallow_scrolling_in_main_frame = on;
      break;
#endif
    case Attribute::JavascriptCanOpenWindows :
      m_webPreferences->javascript_can_open_windows_automatically = on;
      break;
    case Attribute::SpatialNavigationEnabled :
      m_webPreferences->spatial_navigation_enabled = on;
      break;
    case Attribute::SupportsMultipleWindows :
      m_webPreferences->supports_multiple_windows = on;
      break;
    case Attribute::CSSNavigationEnabled :
      m_webPreferences->css_navigation_enabled = on;
      break;
      // TODO(jose.dapena): v8 local offset fix
#if 0
    case Attribute::V8DateUseSystemLocaloffset :
      m_webPreferences->v8_use_system_local_offset = on;
      break;
#endif
    case Attribute::AllowLocalResourceLoad :
      m_webPreferences->allow_local_resource_load = on;
      break;
    case Attribute::LocalStorageEnabled :
      m_webPreferences->local_storage_enabled = on;
      break;
    case Attribute::WebSecurityEnabled :
      m_webPreferences->web_security_enabled = on;
      break;
      // TODO(jose.dapena): revert stacking behavior for compatibility
#if 0
    case Attribute::FixedPositionCreatesStackingContext :
      m_webPreferences->fixed_position_creates_stacking_context = on;
      break;
#endif
    case Attribute::KeepAliveWebApp :
      m_webPreferences->keep_alive_webapp = on;
      break;
    case Attribute::NotifyFMPDirectly :
      m_webPreferences->notify_fmp_directly = on;
      break;
    case Attribute::BackHistoryAPIDisabled :
      m_webPreferences->back_history_api_disabled = on;
      break;
    case Attribute::ForceVideoTexture:
      m_webPreferences->force_video_texture = on;
      break;
    default:
      return;
  }
}

void WebView::UpdatePreferencesAttribute(WebView::Attribute attribute,
                                         double value) {
  switch (attribute) {
    case Attribute::NetworkStableTimeout:
      m_webPreferences->network_stable_timeout = value;
      break;
    default:
      return;
  }
}

void WebView::SetFontFamily(
    WebView::FontFamily fontFamily,
    const std::string& font) {
  switch (fontFamily) {
    case FontFamily::StandardFont :
      m_webPreferences->standard_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::FixedFont :
      m_webPreferences->fixed_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::SerifFont :
      m_webPreferences->serif_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::SansSerifFont :
      m_webPreferences->sans_serif_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::CursiveFont :
      m_webPreferences->cursive_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::FantasyFont :
      m_webPreferences->fantasy_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    default:
      return;
  }
}

void WebView::SetFontHintingNone() {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  rendererPrefs->hinting = gfx::FontRenderParams::HINTING_NONE;
}

void WebView::SetFontHintingSlight() {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  rendererPrefs->hinting = gfx::FontRenderParams::HINTING_SLIGHT;
}

void WebView::SetFontHintingMedium() {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  rendererPrefs->hinting = gfx::FontRenderParams::HINTING_MEDIUM;
}

void WebView::SetFontHintingFull() {
  content::RendererPreferences* rendererPrefs =
      m_webContents->GetMutableRendererPrefs();
  rendererPrefs->hinting = gfx::FontRenderParams::HINTING_FULL;
}

void WebView::SetViewportSize(int width, int height) {
  Send(new WebOSViewMsg_SetViewportSize(routing_id(), width, height));
}

void WebView::NotifyMemoryPressure(base::MemoryPressureListener::MemoryPressureLevel level) {
  if (m_webContents->GetRenderViewHost())
    m_webContents->GetRenderViewHost()->NotifyMemoryPressure(level);
}

void WebView::SetVisible(bool visible) {
  if (!m_webContents)
    return;

  if (visible)
    m_webContents->WasShown();
  else
    m_webContents->WasHidden();
}

void WebView::SetDatabaseIdentifier(const std::string& identifier) {
  // TODO(jose.dapena): local storage
#if 0
  m_webPreferences->database_identifier = identifier;
#endif
}

void WebView::SetVisibilityState(
    blink::WebPageVisibilityState visibilityState) {
  Send(new WebOSViewMsg_SetVisibilityState(routing_id(), visibilityState));
}

void WebView::DeleteWebStorages(const std::string& identifier) {
  WebOSBrowserContext* browser_context =
      profile_->GetBrowserContextAdapter()->GetBrowserContext();
  content::StoragePartition* storage_partition =
      content::BrowserContext::GetStoragePartition(browser_context, NULL);
  std::string origin = std::string("file://").append(identifier);
  storage_partition->GetDOMStorageContext()->DeleteLocalStorage(GURL(origin));
}

void WebView::SetFocus(bool focus) {
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (!rvh)
    return;
  content::RenderWidgetHost* rwh = rvh->GetWidget();

  if (focus) {
    m_webContents->Focus();
    if (rwh)
      rwh->Focus();
  } else if (rwh) {
    rwh->Blur();
  }
}

void WebView::SetZoomFactor(double factor) {
  content::HostZoomMap::SetZoomLevel(m_webContents.get(),
                                     content::ZoomFactorToZoomLevel(factor));
}

void WebView::SetDoNotTrack(bool dnt) {
  GetWebOSContentBrowserClient()->SetDoNotTrack(dnt);
}

void WebView::ForwardWebOSEvent(WebOSEvent* event) {
  content::RenderWidgetHostView* rwhv =
    m_webContents->GetRenderWidgetHostView();
  if (!rwhv)
    return;
  content::RenderWidgetHost* rwh = rwhv->GetRenderWidgetHost();
  if (!rwh)
    return;

  switch (event->GetType()) {
    case WebOSEvent::MouseButtonRelease: {
        WebOSMouseEvent* webos_event = static_cast<WebOSMouseEvent*>(event);
        ui::MouseEvent mouse_event = ui::MouseEvent(
            ui::ET_MOUSE_RELEASED,
            gfx::Point(webos_event->GetX(), webos_event->GetY()),
            gfx::Point(webos_event->GetX(), webos_event->GetY()),
            ui::EventTimeForNow(), webos_event->GetFlags(),
            0);

        blink::WebMouseEvent released_event =
            content::MakeWebMouseEvent(mouse_event);

        rwh->ForwardMouseEvent(released_event);
        break;
      }
    case WebOSEvent::MouseMove: {
        WebOSMouseEvent* webos_event = static_cast<WebOSMouseEvent*>(event);
        ui::MouseEvent mouse_event = ui::MouseEvent(
            ui::ET_MOUSE_MOVED,
            gfx::Point(webos_event->GetX(), webos_event->GetY()),
            gfx::Point(webos_event->GetX(), webos_event->GetY()),
            ui::EventTimeForNow(), webos_event->GetFlags(), 0);

         blink::WebMouseEvent moved_event =
             content::MakeWebMouseEvent(mouse_event);
         rwh->ForwardMouseEvent(moved_event);
         break;
    }
    case WebOSEvent::KeyPress:
    case WebOSEvent::KeyRelease: {
      WebOSKeyEvent* key_event = static_cast<WebOSKeyEvent*>(event);
      uint32_t keycode = key_event->GetCode();
      ui::KeyEvent ui_key_event = ui::KeyEvent(
          event->GetType() == WebOSEvent::KeyPress ? ui::ET_KEY_PRESSED
                                                   : ui::ET_KEY_RELEASED,
          static_cast<ui::KeyboardCode>(keycode), key_event->GetFlags());
      content::NativeWebKeyboardEvent native_event =
          content::NativeWebKeyboardEvent(ui_key_event);
      native_event.windowsKeyCode = keycode;
      native_event.nativeKeyCode = keycode;
      native_event.text[0] = '\0';
      native_event.unmodifiedText[0] = '\0';
      native_event.setKeyIdentifierFromWindowsKeyCode();
      native_event.type = event->GetType() == WebOSEvent::KeyPress
                              ? blink::WebInputEvent::Type::KeyDown
                              : blink::WebInputEvent::Type::KeyUp;
      rwh->ForwardKeyboardEvent(native_event);
      break;
    }
    default:
      break;
  }
}

bool WebView::CanGoBack() const {
  return m_webContents->GetController().CanGoBack();
}

void WebView::SendGetCookiesResponse(WebView* webview,
                                     const std::string& cookies) {
  if (webview->m_delegate)
    webview->m_delegate->SendCookiesForHostname(cookies);
}

void WebView::RequestGetCookies(const std::string& url) {
  WebOSBrowserContext* browser_context =
      profile_->GetBrowserContextAdapter()->GetBrowserContext();
  net::CookieStore* cookieStore = browser_context->GetResourceContext()
                                      ->GetRequestContext()
                                      ->cookie_store();

  if (!cookieStore)
    return;

  net::CookieOptions opt;
  opt.set_include_httponly();

  cookieStore->GetCookiesWithOptionsAsync(
      GURL(url), opt,
      base::Bind(&SendGetCookiesResponse, this));
}

void WebView::SetHardwareResolution(int width, int height) {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          m_webContents->GetRenderWidgetHostView());

  if (!host_view)
    return;

  host_view->SetHardwareResolution(width, height);
}

void WebView::SetEnableHtmlSystemKeyboardAttr(bool enable) {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          m_webContents->GetRenderViewHost()->GetWidget()->GetView());

  if (!host_view)
    return;

  host_view->SetEnableHtmlSystemKeyboardAttr(enable);
}

bool WebView::IsKeyboardVisible() const {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          m_webContents->GetRenderWidgetHostView());

  ui::InputMethod* input_method = host_view->GetInputMethod();
  if (input_method)
    return input_method->IsVisible();
  else
    return false;
}

bool WebView::IsInputMethodActive() const {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          m_webContents->GetRenderWidgetHostView());

  ui::InputMethod* input_method = host_view->GetInputMethod();
  if (input_method)
    return input_method->IsInputMethodActive();
  return false;
}

//////////////////////////////////////////////////////////////////////////////
// WebView, content::WebContentsObserver implementation:

void WebView::DidStartLoading() {
  if (m_delegate)
    m_delegate->LoadStarted();
}

void WebView::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                            const GURL& validated_url) {
#if defined(ENABLE_PLUGINS)
  if (!GetWebOSContentBrowserClient()->PluginLoaded()) {
    GetWebOSContentBrowserClient()->SetPluginLoaded(true);
    content::PluginService::GetInstance()->GetPlugins(
        base::Bind(&GetPluginsCallback));
  }
#endif
  std::string url = validated_url.GetContent();
  if (m_delegate)
    m_delegate->LoadFinished(url);
}

void WebView::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& candidates) {
  for (auto& candidate : candidates) {
    if (candidate.icon_type == content::FaviconURL::FAVICON &&
        !candidate.icon_url.is_empty()) {
      content::NavigationEntry *entry =
          web_contents()->GetController().GetActiveEntry();
      if (!entry)
        continue;
      content::FaviconStatus &favicon = entry->GetFavicon();
      favicon.url = candidate.icon_url;
      favicon.valid = favicon.url.is_valid();
      break;
    }
  }
}

void WebView::DidCommitProvisionalLoadForFrame(
    content::RenderFrameHost* render_frame_host,
    const GURL& url,
    ui::PageTransition transition_type) {
  if (!static_cast<content::RenderFrameHostImpl*>(
      render_frame_host)->frame_tree_node()->IsMainFrame())
    return;

  if (m_delegate)
    m_delegate->NavigationHistoryChanged();
}

void WebView::DidFailProvisionalLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url,
    int error_code,
    const base::string16& error_description,
    bool was_ignored_by_handler) {
  DidFailLoad(render_frame_host, validated_url, error_code, error_description, was_ignored_by_handler);
}

void WebView::DidFailLoad(content::RenderFrameHost* render_frame_host,
                          const GURL& validated_url,
                          int error_code,
                          const base::string16& error_description,
                          bool was_ignored_by_handler) {
  std::string url = validated_url.is_valid() ? validated_url.spec() : std::string();
  if (m_delegate) {
    if (error_code == net::ERR_ABORTED)
      m_delegate->LoadStopped(url);
    else
      m_delegate->LoadFailed(
          url,
          error_code,
          base::UTF16ToUTF8(error_description));
  }
}

void WebView::RenderProcessCreated(base::ProcessHandle handle) {
  // Helps in initializing resources for the renderer process
  std::string locale = GetWebOSContentBrowserClient()->GetApplicationLocale();
  for (content::RenderProcessHost::iterator it(
           content::RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    if (it.GetCurrentValue()->GetHandle() == handle) {
      it.GetCurrentValue()->NotifyLocaleChanged(locale);
      break;
    }
  }

  if (m_delegate)
    m_delegate->RenderProcessCreated(handle);
}

void WebView::RenderProcessGone(base::TerminationStatus status) {
  if (m_delegate)
    m_delegate->RenderProcessGone();
}

void WebView::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  if (m_delegate &&
      static_cast<content::RenderFrameHostImpl*>(render_frame_host)->
          frame_tree_node()->IsMainFrame())
    m_delegate->DocumentLoadFinished();
}

void WebView::DidDropAllPeerConnections(
    content::DropPeerConnectionReason reason) {
  if (m_delegate) {
    DropPeerConnectionReason webos_reason;
    switch (reason) {
      case content::DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN:
        webos_reason = DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN;
        break;
      case content::DROP_PEER_CONNECTION_REASON_UNKNOWN:
      default:
        webos_reason = DROP_PEER_CONNECTION_REASON_UNKNOWN;
    }
    m_delegate->DidDropAllPeerConnections(webos_reason);
  }
}

bool WebView::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebView, message)
#if defined(USE_INJECTIONS)
    IPC_MESSAGE_HANDLER(BrowserControlMsg_Command, OnBrowserControlCommand)
    IPC_MESSAGE_HANDLER(BrowserControlMsg_Function,
                        OnBrowserControlFunction)
#endif
    IPC_MESSAGE_HANDLER(WebOSViewMsgHost_DidClearWindowObject,
                        OnDidClearWindowObject)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

bool WebView::OnMessageReceived(const IPC::Message& message,
                               content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(WebView, message, render_frame_host)
#if defined(USE_SPLASH_SCREEN)
    IPC_MESSAGE_HANDLER(WebOSViewMsgHost_DidFirstMeaningfulPaint,
                        OnDidFirstMeaningfulPaint)
    IPC_MESSAGE_HANDLER(WebOSViewMsgHost_DidNonFirstMeaningfulPaint,
                        OnDidNonFirstMeaningfulPaint)
#endif
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebView::OnBrowserControlCommand(
    const std::string& command,
    const std::vector<std::string>& arguments) {
  if (m_delegate)
    m_delegate->HandleBrowserControlCommand(command, arguments);
}

void WebView::OnBrowserControlFunction(
    const std::string& command,
    const std::vector<std::string>& arguments,
    std::string* result) {
  if (m_delegate)
    m_delegate->HandleBrowserControlFunction(command, arguments, result);
}

void WebView::OnDidClearWindowObject() {
  if (m_delegate)
    m_delegate->DidClearWindowObject();
}

void WebView::UpdatePreferences() {
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (rvh) {
    rvh->SyncRendererPrefs();
    rvh->UpdateWebkitPreferences(*m_webPreferences.get());
  }
}

void WebView::SetAudioGuidanceOn(bool on) {
  m_webPreferences->audio_guidance_on = on;
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (rvh)
    rvh->UpdateWebkitPreferences(*m_webPreferences.get());
}

#if defined(USE_SPLASH_SCREEN)
void WebView::OnDidFirstMeaningfulPaint(
    content::RenderFrameHost* render_frame_host,
    double fmp_detected) {
  if (render_frame_host != web_contents()->GetMainFrame())
    return;

  RAW_PMLOG_INFO("WebView",
                 "OnDidFirstMeaningfulPaint. FMP detected at %f",
                 fmp_detected);
  first_meaningful_paint_detected_ = fmp_detected;
  LoadVisuallyCommittedIfNeed();
}

void WebView::OnDidNonFirstMeaningfulPaint(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host != web_contents()->GetMainFrame())
    return;
  if (load_visually_committed_)
    return;

  RAW_PMLOG_INFO("WebView", "OnDidNonFirstMeaningfulPaint.");
  LoadVisuallyCommitted();
}

void WebView::WillSwapMeaningfulPaint(double detected_time) {
  RAW_PMLOG_INFO("WebView",
                 "WillSwapMeaningfulPaint. FMP detected at %f",
                 detected_time);
  arriving_meaningful_paint_ = detected_time;
  LoadVisuallyCommittedIfNeed();
}

void WebView::LoadVisuallyCommittedIfNeed() {
  if (load_visually_committed_ || first_meaningful_paint_detected_ == .0)
    return;
  if (first_meaningful_paint_detected_ <= arriving_meaningful_paint_) {
    RAW_PMLOG_INFO("WebView", "LoadVisuallyCommittedIfNeed");
    LoadVisuallyCommitted();
  }
}

void WebView::LoadVisuallyCommitted() {
  m_delegate->LoadVisuallyCommitted();
  load_visually_committed_ = true;
}
#endif

void WebView::ResetStateToMarkNextPaintForContainer() {
  content::RenderViewHost* rvh = m_webContents->GetRenderViewHost();
  if (rvh) {
    rvh->ResetStateToMarkNextPaintForContainer();
    first_meaningful_paint_detected_ = 0;
    arriving_meaningful_paint_ = 0;
    load_visually_committed_ = false;
  }
}

void WebView::SetTrustLevel(const std::string& trust_level) {
  if (trust_level_ == trust_level)
    return;

  trust_level_ = trust_level;

  PushStateToIOThread();
}

void WebView::SetAppPath(const std::string& app_path) {
  if (app_path_ == app_path)
    return;

  app_path_ = app_path;

  PushStateToIOThread();
}

void WebView::PushStateToIOThread() {
  if (!m_webContents)
    return;

  WebOSWebViewRendererState::WebViewInfo web_view_info;
  web_view_info.app_path = app_path_;
  web_view_info.trust_level = trust_level_;
  web_view_info.accept_language = accept_language_;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&WebOSWebViewRendererState::RegisterWebViewInfo,
                 base::Unretained(WebOSWebViewRendererState::GetInstance()),
                 m_webContents->GetRenderProcessHost()->GetID(),
                 m_webContents->GetRoutingID(), web_view_info));
}

void WebView::RemoveStateFromIOThread(content::WebContents* web_contents) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&WebOSWebViewRendererState::UnRegisterWebViewInfo,
                 base::Unretained(WebOSWebViewRendererState::GetInstance()),
                 web_contents->GetRenderProcessHost()->GetID(),
                 web_contents->GetRoutingID()));
}

} // namespace webos
