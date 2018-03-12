// Copyright 2014 LG Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/shell/webos_shell_surface.h"

#include "base/strings/utf_string_conversions.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/shell/shell.h"

namespace ozonewayland {

namespace {

ui::WidgetState ToWidgetState(uint32_t state) {
  switch (state) {
    case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED:
      return ui::WidgetState::MINIMIZED;
    case WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED:
      return ui::WidgetState::MAXIMIZED;
    case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN:
      return ui::WidgetState::FULLSCREEN;
    default:
      return ui::WidgetState::UNINITIALIZED;
  }
}

}  // namespace

WebosShellSurface::WebosShellSurface()
    : WLShellSurface(),
      webos_key_masks_(webos::WebOSKeyMask::KEY_MASK_DEFAULT),
      webos_group_key_masks_(webos::WebOSKeyMask::KEY_MASK_DEFAULT),
      minimized_(false),
      webos_shell_surface_(NULL) {
}

WebosShellSurface::~WebosShellSurface() {
  if (webos_shell_surface_)
    wl_webos_shell_surface_destroy(webos_shell_surface_);
}

void WebosShellSurface::InitializeShellSurface(WaylandWindow* window,
                                               WaylandWindow::ShellType type) {
  WLShellSurface::InitializeShellSurface(window, type);
  DCHECK(!webos_shell_surface_);
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  DCHECK(display);
  WaylandShell* shell = WaylandDisplay::GetInstance()->GetShell();
  DCHECK(shell && shell->GetWebosWLShell());
  webos_shell_surface_ = wl_webos_shell_get_shell_surface(
      shell->GetWebosWLShell(), GetWLSurface());

  static const wl_webos_shell_surface_listener webos_shell_surface_listener = {
      WebosShellSurface::HandleStateChanged,
      WebosShellSurface::HandlePositionChanged,
      WebosShellSurface::HandleClose,
      WebosShellSurface::HandleExposed,
      WebosShellSurface::HandleStateAboutToChange};

  wl_webos_shell_surface_add_listener(webos_shell_surface_,
                                      &webos_shell_surface_listener, window);
  DCHECK(webos_shell_surface_);
}

void WebosShellSurface::UpdateShellSurface(WaylandWindow::ShellType type,
                                           WaylandShellSurface* shell_parent,
                                           int x,
                                           int y) {
  switch (type) {
    case WaylandWindow::FULLSCREEN:
      wl_webos_shell_surface_set_state(webos_shell_surface_,
                                       WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN);
      minimized_ = false;
      break;
    default:
      break;
  }

}

void WebosShellSurface::Maximize() {
  wl_webos_shell_surface_set_state(webos_shell_surface_,
                                   WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED);
  minimized_ = false;
}

void WebosShellSurface::Minimize() {
  wl_webos_shell_surface_set_state(webos_shell_surface_,
                                   WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED);
  minimized_ = true;
}

bool WebosShellSurface::IsMinimized() const {
  return minimized_;
}

void WebosShellSurface::SetKeyMask(webos::WebOSKeyMask key_mask) {
  if (key_mask == webos_group_key_masks_)
    return ;

  webos_group_key_masks_ = key_mask;
  wl_webos_shell_surface_set_key_mask(webos_shell_surface_,
                                      webos_group_key_masks_);
}

void WebosShellSurface::SetKeyMask(webos::WebOSKeyMask key_mask, bool value) {
  WebOSKeyMasks webos_key_masks =
    value ? webos_key_masks_ | key_mask : webos_key_masks_ & ~key_mask;
  if (webos_key_masks == webos_key_masks_)
    return ;

  webos_key_masks_ = webos_key_masks;
  wl_webos_shell_surface_set_key_mask(webos_shell_surface_, webos_key_masks);
}

void WebosShellSurface::SetInputRegion(std::vector<gfx::Rect>region) {
  wl_compositor *wlcompositor = WaylandDisplay::GetInstance()->GetCompositor();
  wl_region *wlregion = wl_compositor_create_region(wlcompositor);

  std::vector<gfx::Rect>::iterator iter;
  for (iter = region.begin(); iter != region.end(); ++iter)
    wl_region_add(wlregion, (*iter).x(),
                  (*iter).y(), (*iter).width(), (*iter).height());

  wl_surface *wlsurface = GetWLSurface();
  wl_surface_set_input_region(wlsurface, wlregion);
  wl_surface_commit(wlsurface);
  wl_region_destroy(wlregion);
}

void WebosShellSurface::SetWindowProperty(const std::string& name,
                                          const std::string& value) {
  wl_webos_shell_surface_set_property(
      webos_shell_surface_, name.c_str(), value.c_str());
}

void WebosShellSurface::HandleStateChanged(
    void* data,
    struct wl_webos_shell_surface* webos_shell_surface,
    uint32_t state) {

  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandWindow *window = static_cast<WaylandWindow*>(data);
  if (window) {
    ui::WidgetState widget_state = ToWidgetState(state);
    dispatcher->NativeWindowStateChanged(window->Handle(), widget_state);

    switch (state) {
      case WL_WEBOS_SHELL_SURFACE_STATE_MINIMIZED: return window->Minimize();
      case WL_WEBOS_SHELL_SURFACE_STATE_MAXIMIZED: return window->Maximize();
      case WL_WEBOS_SHELL_SURFACE_STATE_FULLSCREEN: return window->SetFullscreen();
      default: return;
    }
  }
}

void WebosShellSurface::HandlePositionChanged(
    void* data,
    struct wl_webos_shell_surface* webos_shell_surface,
    int32_t x,
    int32_t y) {}

void WebosShellSurface::HandleClose(
    void* data,
    struct wl_webos_shell_surface* webos_shell_surface) {
  WaylandWindow* window = static_cast<WaylandWindow*>(data);
  if (window)
    WaylandDisplay::GetInstance()->WindowClose(window->Handle());
}

void WebosShellSurface::HandleExposed(
    void* data,
    struct wl_webos_shell_surface* webos_shell_surface,
    struct wl_array* rectangles) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandWindow *window = static_cast<WaylandWindow*>(data);
  if (window)
    dispatcher->NativeWindowExposed(window->Handle());
}

void WebosShellSurface::HandleStateAboutToChange(
    void* data,
    struct wl_webos_shell_surface* webos_shell_surface,
    uint32_t state) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandWindow *window = static_cast<WaylandWindow*>(data);
  if (window) {
    ui::WidgetState widget_state = ToWidgetState(state);
    dispatcher->NativeWindowStateAboutToChange(window->Handle(), widget_state);
  }
}

}  // namespace ozonewayland
