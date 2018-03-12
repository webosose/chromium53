// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#ifndef OZONE_WAYLAND_SHELL_WEBOS_SURFACE_H_
#define OZONE_WAYLAND_SHELL_WEBOS_SURFACE_H_

#include "ozone/wayland/shell/wl_shell_surface.h"
#include "wayland-webos-shell-client-protocol.h"
#include "webos/common/webos_constants.h"

namespace ozonewayland {

class WaylandSurface;
class WaylandWindow;

class WebosShellSurface : public WLShellSurface {
 public:
  WebosShellSurface();
  ~WebosShellSurface() override;

  void InitializeShellSurface(WaylandWindow* window,
                              WaylandWindow::ShellType type) override;
  void UpdateShellSurface(WaylandWindow::ShellType type,
                          WaylandShellSurface* shell_parent,
                          int x,
                          int y) override;
  void Maximize() override;
  void Minimize() override;
  bool IsMinimized() const override;
  void SetKeyMask(webos::WebOSKeyMask key_mask) override;
  void SetKeyMask(webos::WebOSKeyMask key_mask, bool value) override;
  void SetInputRegion(std::vector<gfx::Rect> region) override;
  void SetWindowProperty(const std::string& name,
                         const std::string& value) override;
  static void HandleStateChanged(
      void* data,
      struct wl_webos_shell_surface* webos_shell_surface,
      uint32_t state);

  static void HandlePositionChanged(
      void* data,
      struct wl_webos_shell_surface* webos_shell_surface,
      int32_t x,
      int32_t y);

  static void HandleClose(void* data,
                          struct wl_webos_shell_surface* webos_shell_surface);

  static void HandleExposed(void* data,
                            struct wl_webos_shell_surface* webos_shell_surface,
                            struct wl_array* rectangles);

  static void HandleStateAboutToChange(
      void* data,
      struct wl_webos_shell_surface* webos_shell_surface,
      uint32_t state);

 private:
  typedef uint32_t WebOSKeyMasks;
  WebOSKeyMasks webos_key_masks_;
  WebOSKeyMasks webos_group_key_masks_;
  bool minimized_;
  wl_webos_shell_surface* webos_shell_surface_;
  DISALLOW_COPY_AND_ASSIGN(WebosShellSurface);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_SHELL_WEBOS_SURFACE_H_
