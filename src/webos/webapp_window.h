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

#ifndef WEBOS_WEBAPP_WINDOW_H_
#define WEBOS_WEBAPP_WINDOW_H_

#include <map>

#include "base/timer/timer.h"
#include "ui/events/event_handler.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/display/display_observer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "webos/common/webos_constants.h"
#include "webos/common/webos_event.h"
#include "webos/common/webos_native_event_delegate.h"

namespace content {
class WebContents;
}

namespace views {
class DesktopWindowTreeHost;
}

namespace ui {
class WindowGroupConfiguration;
}

namespace gfx {
class Rect;
}

class WebAppWindowDelegate;
class WebOSView;

namespace webos {

class WebOSBrowserContext;

class WebAppWindow : public WebOSNativeEventDelegate,
                     public display::DisplayObserver,
                     public ui::EventHandler {
 public:
  WebAppWindow(const gfx::Rect& rect, int surface_id);
  virtual ~WebAppWindow();

  void Show();
  void Hide();

  gfx::NativeWindow GetNativeWindow();
  int DisplayWidth();
  int DisplayHeight();
  void AttachWebContents(content::WebContents* web_contents);
  void DetachWebContents();
  void SetDelegate(WebAppWindowDelegate* webapp_window_delegate);
  bool Event(WebOSEvent* webos_event);
  void SetWebOSView(WebOSView* webos_view) { webos_view_ = webos_view; }

  void SetHost(views::DesktopWindowTreeHost* host);
  views::DesktopWindowTreeHost* host() { return host_; }

  NativeWindowState GetWindowHostState() const {
    return window_host_state_;
  }
  NativeWindowState GetWindowHostStateAboutToChange() const {
    return window_host_state_about_to_change_;
  }

  void SetWindowHostState(NativeWindowState state);

  void SetKeyMask(WebOSKeyMask key_mask, bool value);
  void SetWindowProperty(const std::string& name, const std::string& value);
  void SetWindowSurfaceId(int surface_id);
  void SetOpacity(float opacity);
  void Resize(int width, int height);
  void SetUseVirtualKeyboard(bool enable);
  void SetAllowWindowResize(bool allow) { allow_window_resize_ = allow; }

  void CreateGroup(const ui::WindowGroupConfiguration& config);
  void AttachToGroup(const std::string& group, const std::string& layer);
  void FocusGroupOwner();
  void FocusGroupLayer();
  void DetachGroup();

  // Overridden from WebOSNativeEventDelegate:
  void CompositorBuffersSwapped() override;
  void CursorVisibilityChange(bool visible) override;
  void InputPanelHidden() override;
  void InputPanelShown() override;
  void InputPanelRectChanged() override;
  bool IsEnableResize() override { return allow_window_resize_; }
  void KeyboardEnter() override;
  void KeyboardLeave() override;
  void WindowHostClose() override;
  void WindowHostExposed() override;
  void WindowHostStateChanged(ui::WidgetState new_state) override;
  void WindowHostStateAboutToChange(ui::WidgetState state) override;

  // Overridden from display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override;
  void OnDisplayRemoved(const display::Display& old_display) override;
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  // Overridden from ui::EventHandler:
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnKeyEvent(ui::KeyEvent* event) override;

  // For VKB viewport shift
  void UpdateViewportY();

  void CheckKeyFilter(ui::KeyEvent* event);
  void CheckKeyFilterTable(unsigned keycode, unsigned& new_keycode, ui::DomCode& code, unsigned& modifier);

 private:
  void Restore();
  void ComputeScaleFactor();

  // Handle mouse events.
  bool OnMousePressed(float x, float y, int flags);
  bool OnMouseReleased(float x, float y, int flags);
  bool OnMouseMoved(float x, float y);
  bool OnMouseWheel(float x, float y, int x_offset, int y_offset);
  bool OnMouseEntered(float x, float y);
  bool OnMouseExited(float x, float y);

  // Handle key events
  bool OnKeyPressed(unsigned keycode);
  bool OnKeyReleased(unsigned keycode);

  WebAppWindowDelegate* webapp_window_delegate_;
  WebOSView* webos_view_;

  NativeWindowState window_host_state_;
  NativeWindowState window_host_state_about_to_change_;
  views::DesktopWindowTreeHost* host_;

  // For Restore.
  WebOSBrowserContext* browser_context_;
  content::WebContents* web_contents_;
  std::map<std::string, std::string> window_property_list_;
  int window_surface_id_;
  std::map<WebOSKeyMask, bool> key_mask_list_;
  float opacity_;
  gfx::Rect rect_;
  bool keyboard_enter_;

  float scale_factor_;

  // For VKB viewport shift
  base::OneShotTimer viewport_timer_;
  int viewport_shift_height_;
  bool input_panel_visible_;
  bool cursor_visible_;
  bool allow_window_resize_;
  int current_rotation_;

  DISALLOW_COPY_AND_ASSIGN(WebAppWindow);
};

}  // namespace webos

#endif  // WEBOS_WEBAPP_WINDOW_H_
