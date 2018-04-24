// Copyright 2016-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/webapp_window.h"

#include "content/browser/media/session/media_session.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "ozone/ui/desktop_aura/desktop_screen_wayland.h"
#include "ui/aura/window.h"
#include "ui/base/ime/input_method.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/platform_window/webos/window_group_configuration.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/wm/core/cursor_manager.h"
#include "webos/app/webos_content_main_delegate.h"
#include "webos/browser/webos_browser_context.h"
#include "webos/browser/webos_browser_context_adapter.h"
#include "webos/public/runtime.h"
#include "webos/ui/views/frame/webos_view.h"
#include "webos/ui/views/frame/webos_widget.h"
#include "webos/webapp_window_base.h"

namespace webos {

namespace {

static const int kKeyboardHeightMargin = 20;
static const int kKeyboardAnimationTime = 600;

NativeWindowState ToNativeWindowState(ui::WidgetState state) {
  switch (state) {
    case ui::WidgetState::MINIMIZED:
      return NativeWindowState::NATIVE_WINDOW_MINIMIZED;
    case ui::WidgetState::MAXIMIZED:
      return NativeWindowState::NATIVE_WINDOW_MAXIMIZED;
    case ui::WidgetState::FULLSCREEN:
      return NativeWindowState::NATIVE_WINDOW_FULLSCREEN;
    default:
      return NativeWindowState::NATIVE_WINDOW_DEFAULT;
  }
}

ui::WidgetState ToWidgetState(NativeWindowState state) {
  switch (state) {
    case NativeWindowState::NATIVE_WINDOW_MINIMIZED:
      return ui::WidgetState::MINIMIZED;
    case NativeWindowState::NATIVE_WINDOW_MAXIMIZED:
      return ui::WidgetState::MAXIMIZED;
    case NativeWindowState::NATIVE_WINDOW_FULLSCREEN:
      return ui::WidgetState::FULLSCREEN;
    default:
      return ui::WidgetState::UNINITIALIZED;
  }
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// WebAppWindow, public:

WebAppWindow::WebAppWindow(const gfx::Rect& rect)
    : opacity_(1.f),
      rect_(rect),
      web_contents_(0),
      webapp_window_delegate_(0),
      host_(0),
      webos_view_(0),
      window_host_state_(NativeWindowState::NATIVE_WINDOW_DEFAULT),
      window_host_state_about_to_change_(
          NativeWindowState::NATIVE_WINDOW_DEFAULT),
      keyboard_enter_(false),
      viewport_shift_height_(0),
      input_panel_visible_(false),
      cursor_visible_(false),
      allow_window_resize_(false),
      current_rotation_(-1) {
  WebOSBrowserContextAdapter* browser_context =
      GetWebOSContentBrowserClient()
          ->GetMainParts()
          ->GetDefaultBrowserContext();
  CHECK(browser_context);

  browser_context_ = browser_context->GetBrowserContext();

  webos_view_ = new WebOSView(this);
  webos_view_->Init(browser_context_);
  webos_view_->SetNativeEventDelegate(this);
  (new WebOSWidget(webos_view_))->InitWebOSWidget(rect_);

  if (display::Screen::GetScreen()->GetNumDisplays() > 0) {
    current_rotation_ =
        display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree();
    ComputeScaleFactor();
  }

  display::Screen::GetScreen()->AddObserver(this);
}

WebAppWindow::~WebAppWindow() {
  display::Screen::GetScreen()->RemoveObserver(this);
  if (webos_view_) {
    webos_view_->SetNativeEventDelegate(0);
    webos_view_->Close();
    // WebOSView is owned by Widget's |non_client_view_|.
    webos_view_ = NULL;
  }
}

void WebAppWindow::Show() {
  if (!webos_view_ && web_contents_)
    Restore();

  if (webos_view_) {
    webos_view_->Activate();
    webos_view_->Show();
  }
}

void WebAppWindow::Hide() {
#if defined(OS_WEBOS)
  if (host_)
    host_->WebAppWindowHidden();
#endif
}

gfx::NativeWindow WebAppWindow::GetNativeWindow() {
  return webos_view_ ? webos_view_->GetNativeWindow() : NULL;
}

int WebAppWindow::DisplayWidth() {
  if (display::Screen::GetScreen()->GetNumDisplays() > 0)
    return display::Screen::GetScreen()->GetPrimaryDisplay().bounds().width();
  return 0;
}

int WebAppWindow::DisplayHeight() {
  if (display::Screen::GetScreen()->GetNumDisplays() > 0)
    return display::Screen::GetScreen()->GetPrimaryDisplay().bounds().height();
  return 0;
}

void WebAppWindow::AttachWebContents(content::WebContents* web_contents) {
  web_contents_ = web_contents;
  if (webos_view_)
    webos_view_->AttachWebContents(web_contents);
}

void WebAppWindow::DetachWebContents() {
  web_contents_ = 0;
  opacity_ = 1.f;
  key_mask_list_.clear();
  window_property_list_.clear();

  if (webos_view_)
    webos_view_->AttachWebContents(NULL);
}

void WebAppWindow::SetDelegate(WebAppWindowDelegate* webapp_window_delegate) {
  webapp_window_delegate_ = webapp_window_delegate;
}

void WebAppWindow::SetHost(views::DesktopWindowTreeHost* host) {
  host_ = host;

#if defined(OS_WEBOS)
  if (host_)
    host_->AddPreTargetHandler(this);
#endif
}

bool WebAppWindow::Event(WebOSEvent* webos_event) {
  if (webapp_window_delegate_)
    return webapp_window_delegate_->event(webos_event);

  return false;
}

void WebAppWindow::SetWindowHostState(NativeWindowState state) {
#if defined(OS_WEBOS)
  if (host_)
    host_->SetWindowHostState(ToWidgetState(state));
#endif
}

void WebAppWindow::SetKeyMask(WebOSKeyMask key_mask, bool value) {
  key_mask_list_[key_mask] = value;

  if (!host_)
    return ;

#if defined(OS_WEBOS)
  host_->SetKeyMask(key_mask, value);
#endif
}

void WebAppWindow::SetWindowProperty(const std::string& name,
                                     const std::string& value) {
  window_property_list_[name] = value;

  if (!host_)
    return ;

#if defined(OS_WEBOS)
  host_->SetWindowProperty(name, value);
#endif
}

void WebAppWindow::SetOpacity(float opacity) {
  opacity_ = opacity;

  if (!host_)
    return ;

  host_->SetOpacity(opacity);
}

void WebAppWindow::CreateGroup(const ui::WindowGroupConfiguration& config) {
  if (!host_)
    return;

#if defined(OS_WEBOS)
  host_->CreateGroup(config);
#endif
}

void WebAppWindow::AttachToGroup(const std::string& group,
                                 const std::string& layer) {
  if (!host_)
    return;

#if defined(OS_WEBOS)
  host_->AttachToGroup(group, layer);
#endif
}

void WebAppWindow::FocusGroupOwner() {
  if (!host_)
    return;

#if defined(OS_WEBOS)
  host_->FocusGroupOwner();
#endif
}

void WebAppWindow::FocusGroupLayer() {
  if (!host_)
    return;

#if defined(OS_WEBOS)
  host_->FocusGroupLayer();
#endif
}

void WebAppWindow::DetachGroup() {
  if (!host_)
    return;

#if defined(OS_WEBOS)
  host_->DetachGroup();
#endif
}

void WebAppWindow::Resize(int width, int height) {
  if (rect_.width() != width || rect_.height() != height) {
    rect_.set_width(width);
    rect_.set_height(height);

    ComputeScaleFactor();

    if (webos_view_ && webos_view_->widget())
      webos_view_->widget()->SetSize(gfx::Size(width, height));
  }
}

void WebAppWindow::SetUseVirtualKeyboard(bool enable) {
  webos_view_->SetImeEnabled(enable);
}

///////////////////////////////////////////////////////////////////////////////
// WebAppWindow, WebOSNativeEventDelegate overrides:

void WebAppWindow::CompositorBuffersSwapped() {
  WebOSEvent webos_event(WebOSEvent::Type::Swap);
  Event(&webos_event);
}

void WebAppWindow::InputPanelHidden() {
  viewport_shift_height_ = 0;
  UpdateViewportY();

  if (input_panel_visible_) {
    input_panel_visible_ = false;
    WebOSVirtualKeyboardEvent webos_event(WebOSEvent::Type::InputPanelVisible,
                                          input_panel_visible_, 0);
    Event(&webos_event);
  }
}

void WebAppWindow::InputPanelShown() {
#if defined(OS_WEBOS)
  int input_panel_height = host_->GetInputMethod()->GetInputPanelRect().height();
  gfx::Rect caret_bounds = host_->GetInputMethod()->GetCaretBounds();
  int caret_bottom = caret_bounds.y() + caret_bounds.height();
  int viewport_height = webos_view_->height();

  bool is_overlapped =
      viewport_height - (input_panel_height + kKeyboardHeightMargin) <
      caret_bottom;

  if (is_overlapped) {
    viewport_shift_height_ = -input_panel_height / scale_factor_;
    if (viewport_timer_.IsRunning())
      viewport_timer_.Reset();
    else
      viewport_timer_.Start(
          FROM_HERE, base::TimeDelta::FromMilliseconds(kKeyboardAnimationTime),
          this, &WebAppWindow::UpdateViewportY);
  }

  if (!input_panel_visible_) {
    input_panel_visible_ = true;
    WebOSVirtualKeyboardEvent webos_event(WebOSEvent::Type::InputPanelVisible,
                                          input_panel_visible_,
                                          input_panel_height);
    Event(&webos_event);
  }
#endif
}

void WebAppWindow::InputPanelRectChanged() {
#if defined(OS_WEBOS)
  int input_panel_height = host_->GetInputMethod()->GetInputPanelRect().height();
  gfx::Rect caret_bounds = host_->GetInputMethod()->GetCaretBounds();
  int caret_bottom =
      caret_bounds.y() + caret_bounds.height() - viewport_shift_height_;
  int viewport_height = webos_view_->height();

  bool is_overlapped =
      viewport_height - (input_panel_height + kKeyboardHeightMargin) <
      caret_bottom;
  if (input_panel_visible_ && is_overlapped) {
    viewport_shift_height_ = -input_panel_height / scale_factor_;
    UpdateViewportY();
  }
#endif
}

void WebAppWindow::UpdateViewportY() {
  gfx::Rect bounds = web_contents_->GetContentNativeView()->bounds();
  web_contents_->GetContentNativeView()->SetBounds(gfx::Rect(
      bounds.x(), viewport_shift_height_, bounds.width(), bounds.height()));

  web_contents_->GetRenderViewHost()->ContentsPositionChanged(
      viewport_shift_height_);
}

void WebAppWindow::KeyboardEnter() {
  if (keyboard_enter_)
    return;

  keyboard_enter_ = true;

  WebOSEvent webos_event(WebOSEvent::Type::FocusIn);
  Event(&webos_event);
}

void WebAppWindow::KeyboardLeave() {
  if (!keyboard_enter_)
    return;

  keyboard_enter_ = false;

  WebOSEvent webos_event(WebOSEvent::Type::FocusOut);
  Event(&webos_event);
}

void WebAppWindow::WindowHostClose() {
  if (input_panel_visible_)
    InputPanelHidden();

  WebOSEvent webos_event(WebOSEvent::Close);
  Event(&webos_event);
}

void WebAppWindow::WindowHostExposed() {
  WebOSEvent webos_event(WebOSEvent::Type::Expose);
  Event(&webos_event);
}

void WebAppWindow::WindowHostStateChanged(ui::WidgetState new_state) {
  // Do not compare new_state with window_host_state_
  // Just invoke Event() even though the state is not changed
  // For scenario of splash WAM needs this change
  window_host_state_ = ToNativeWindowState(new_state);

  WebOSEvent webos_event(WebOSEvent::Type::WindowStateChange);
  Event(&webos_event);
}

void WebAppWindow::WindowHostStateAboutToChange(ui::WidgetState state) {
  window_host_state_about_to_change_ = ToNativeWindowState(state);

  WebOSEvent webos_event(WebOSEvent::Type::WindowStateAboutToChange);
  Event(&webos_event);
}

void WebAppWindow::CursorVisibilityChange(bool visible) {
  cursor_visible_ = visible;
  Runtime::GetInstance()->OnCursorVisibilityChanged(visible);
  aura::Window* window = static_cast<aura::Window*>(GetNativeWindow());
  if (window) {
    aura::Window* root = window->GetRootWindow();
    aura::client::CursorClient* cursor_client =
        aura::client::GetCursorClient(root);
    static_cast<::wm::CursorManager*>(cursor_client)->CommitVisibility(visible);
  }
}

////////////////////////////////////////////////////////////////////////////////
// WebAppWindow, display::DisplayObserver overrides:

void WebAppWindow::OnDisplayAdded(const display::Display& new_display) {
  current_rotation_ =
      display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree();
  ComputeScaleFactor();
}

void WebAppWindow::OnDisplayRemoved(const display::Display& old_display) {}

void WebAppWindow::OnDisplayMetricsChanged(const display::Display& display,
                                           uint32_t metrics) {
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_ROTATION) {
    // Get new rotation from primary display
    int screen_rotation =
        display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree();

    if (screen_rotation == current_rotation_)
      return;

    // FIXME: Wayland shell surface already reports resize request but for now
    // LSM fails to report that resize is due to rotation change. We need this
    // information in order do swap width and height because otherwise by
    // default resizing window is denied by allow_window_resize_ flag. If LSM
    // would report it correctly then this width and height swapping would not
    // need to be done here but on ozone wayland level.
    int new_width = rect_.width();
    int new_height = rect_.height();
    if (rect_.width() != 0 && rect_.height() != 0) {
      if (current_rotation_ == 0 || current_rotation_ == 180) {
        if (screen_rotation == 90 || screen_rotation == 270) {
          new_width = rect_.height();
          new_height = rect_.width();
        }
      } else if (current_rotation_ == 90 || current_rotation_ == 270) {
        if (screen_rotation == 0 || screen_rotation == 180) {
          new_width = rect_.height();
          new_height = rect_.width();
        }
      }
    }

    current_rotation_ = screen_rotation;

    if (new_width != rect_.width() || new_height != rect_.height())
      Resize(new_width, new_height);

    ComputeScaleFactor();

    if (web_contents_ && content::MediaSession::Get(web_contents_)) {
      content::ScreenOrientationType content_screen_orientation;

      if (current_rotation_ == 0)
        content_screen_orientation = content::CONTENT_SCREEN_ORIENTATION_0;
      else if (current_rotation_ == 90)
        content_screen_orientation = content::CONTENT_SCREEN_ORIENTATION_90;
      else if (current_rotation_ == 180)
        content_screen_orientation = content::CONTENT_SCREEN_ORIENTATION_180;
      else if (current_rotation_ == 270)
        content_screen_orientation = content::CONTENT_SCREEN_ORIENTATION_270;
      else
        content_screen_orientation =
            content::CONTENT_SCREEN_ORIENTATION_UNDEFINED;

      content::MediaSession::Get(web_contents_)
          ->ScreenOrientationChanged(content_screen_orientation);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// WebAppWindow, ui::EventHandler overrides:

void WebAppWindow::OnMouseEvent(ui::MouseEvent* event) {
  switch (event->type()) {
    case ui::EventType::ET_MOUSE_PRESSED:
      if (OnMousePressed(event->x(), event->y(), event->flags()))
        event->StopPropagation();
      break;
    case ui::EventType::ET_MOUSE_RELEASED:
      if (OnMouseReleased(event->x(), event->y(), event->flags()))
        event->StopPropagation();
      break;
    case ui::EventType::ET_MOUSE_MOVED:
      if (OnMouseMoved(event->x(), event->y()))
        event->StopPropagation();
      break;
    case ui::EventType::ET_MOUSE_ENTERED:
      OnMouseEntered(event->x(), event->y());
      break;
    case ui::EventType::ET_MOUSE_EXITED:
      if (cursor_visible_) {
        OnMouseExited(event->x(), event->y());
        event->set_location(gfx::Point(INT_MIN, INT_MIN));
      } else
        event->StopPropagation();
      break;
    case ui::EventType::ET_MOUSEWHEEL: {
      ui::MouseWheelEvent* wheel_event =
        static_cast<ui::MouseWheelEvent*>(event);
      if (OnMouseWheel(wheel_event->x(),
                       wheel_event->y(),
                       wheel_event->x_offset(),
                       wheel_event->y_offset()))
        event->StopPropagation();
      break;
    }
    default:
      break;
  }
}

void WebAppWindow::OnKeyEvent(ui::KeyEvent* event) {
  CheckKeyFilter(event);

  switch (event->type()) {
    case ui::EventType::ET_KEY_PRESSED:
      if (OnKeyPressed(event->key_code()))
        event->StopPropagation();
      break;
    case ui::EventType::ET_KEY_RELEASED:
      if (OnKeyReleased(event->key_code()))
        event->StopPropagation();
      break;
    default:
      break;
  }
}

void WebAppWindow::CheckKeyFilter(ui::KeyEvent* event) {
  unsigned new_keycode;
  ui::DomCode code;
  unsigned modifier = 0;
  CheckKeyFilterTable(event->key_code(), new_keycode, code, modifier);

  if (new_keycode) {
    event->set_key_code(static_cast<ui::KeyboardCode>(new_keycode));
    event->set_code(code);
  }
}

void WebAppWindow::CheckKeyFilterTable(unsigned keycode, unsigned& new_keycode, ui::DomCode& code, unsigned& modifier) {
  new_keycode = 0;
  code = ui::DomCode::NONE;
}

////////////////////////////////////////////////////////////////////////////////
// WebAppWindow, private:

void WebAppWindow::Restore() {
  webos_view_ = new WebOSView(this);
  webos_view_->Init(browser_context_);
  webos_view_->SetNativeEventDelegate(this);
  (new WebOSWidget(webos_view_))->InitWebOSWidget(rect_);

  if (!host_)
    return;

#if defined(OS_WEBOS)
  std::map<WebOSKeyMask, bool>::const_iterator key_mask_it;
  for (key_mask_it = key_mask_list_.begin();
       key_mask_it != key_mask_list_.end();
       ++key_mask_it) {
    host_->SetKeyMask(key_mask_it->first, key_mask_it->second);
  }

  std::map<std::string, std::string>::const_iterator window_property_it;
  for (window_property_it = window_property_list_.begin();
       window_property_it != window_property_list_.end();
       ++window_property_it) {
    host_->SetWindowProperty(window_property_it->first,
                             window_property_it->second);
  }
#endif

  webos_view_->AttachWebContents(web_contents_);

  host_->SetOpacity(opacity_);
}

void WebAppWindow::ComputeScaleFactor() {
  scale_factor_ = static_cast<float>(DisplayHeight()) / rect_.height();
}

bool WebAppWindow::OnMousePressed(float x, float y, int flags) {
  WebOSMouseEvent event(WebOSEvent::MouseButtonPress, x, y, flags);
  return Event(&event);
}

bool WebAppWindow::OnMouseReleased(float x, float y, int flags) {
  WebOSMouseEvent event(WebOSEvent::Type::MouseButtonRelease, x, y, flags);
  return Event(&event);
}

bool WebAppWindow::OnMouseMoved(float x, float y) {
  WebOSMouseEvent event(WebOSEvent::Type::MouseMove, x, y);
  return Event(&event);
}

bool WebAppWindow::OnMouseWheel(float x, float y, int x_offset, int y_offset) {
  WebOSMouseWheelEvent event(WebOSEvent::Type::Wheel, x, y, x_offset, y_offset);
  return Event(&event);
}

bool WebAppWindow::OnMouseEntered(float x, float y) {
  WebOSMouseEvent event(WebOSEvent::Type::Enter, x, y);
  return Event(&event);
}

bool WebAppWindow::OnMouseExited(float x, float y) {
  WebOSMouseEvent event(WebOSEvent::Type::Leave, x, y);
  return Event(&event);
}

bool WebAppWindow::OnKeyPressed(unsigned keycode) {
  WebOSKeyEvent event(WebOSEvent::Type::KeyPress, keycode);
  return Event(&event);
}

bool WebAppWindow::OnKeyReleased(unsigned keycode) {
  WebOSKeyEvent event(WebOSKeyEvent::Type::KeyRelease, keycode);
  return Event(&event);
}

}  // namespace webos
