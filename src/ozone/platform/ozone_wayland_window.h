// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef OZONE_PLATFORM_OZONE_WAYLAND_WINDOW_H_
#define OZONE_PLATFORM_OZONE_WAYLAND_WINDOW_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "ozone/platform/channel_observer.h"
#include "ozone/platform/window_constants.h"
#include "third_party/skia/include/core/SkRegion.h"
#include "ui/events/platform/platform_event_dispatcher.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/platform_window.h"

#if defined(OS_WEBOS)
#include "base/memory/weak_ptr.h"
#include "webos/common/webos_constants.h"

class SkBitmap;
#endif

namespace ui {

class BitmapCursorOzone;
class OzoneGpuPlatformSupportHost;
class PlatformWindowDelegate;
class WindowManagerWayland;

class OzoneWaylandWindow : public PlatformWindow,
                           public PlatformEventDispatcher,
                           public ChannelObserver {
 public:
  OzoneWaylandWindow(PlatformWindowDelegate* delegate,
                     OzoneGpuPlatformSupportHost* sender,
                     WindowManagerWayland* window_manager,
                     const gfx::Rect& bounds);
  ~OzoneWaylandWindow() override;

  unsigned GetHandle() const { return handle_; }
  PlatformWindowDelegate* GetDelegate() const { return delegate_; }
  void InitNativeWidget();

  // PlatformWindow:
  void InitPlatformWindow(PlatformWindowType type,
                          gfx::AcceleratedWidget parent_window) override;
  void SetTitle(const base::string16& title) override;
  void SetWindowShape(const SkPath& path) override;
  void SetOpacity(unsigned char opacity) override;
  void RequestDragData(const std::string& mime_type) override;
  void RequestSelectionData(const std::string& mime_type) override;
  void DragWillBeAccepted(uint32_t serial,
                          const std::string& mime_type) override;
  void DragWillBeRejected(uint32_t serial) override;
  gfx::Rect GetBounds() override;
  void SetBounds(const gfx::Rect& bounds) override;
  void Show() override;
  void Hide() override;
  void Close() override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void ToggleFullscreen() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  void SetCursor(PlatformCursor cursor) override;
  void MoveCursorTo(const gfx::Point& location) override;
  void ConfineCursorToBounds(const gfx::Rect& bounds) override;
  PlatformImeController* GetPlatformImeController() override;
#if defined(OS_WEBOS)
  void SetCustomCursor(webos::CustomCursorType type,
                       const std::string& path,
                       int hotspot_x,
                       int hotspot_y) override;
  void ResetCustomCursor() override;
  void SetKeyMask(webos::WebOSKeyMask key_mask, bool value) override;
  void SetWindowProperty (const std::string& name, const std::string& value) override;

  void CreateGroup(const WindowGroupConfiguration&) override;
  void AttachToGroup(const std::string& group,
                     const std::string& layer) override;
  void FocusGroupOwner() override;
  void FocusGroupLayer() override;
  void DetachGroup() override;
  void ToggleFullscreenWithSize(const gfx::Size& size) override;
  void WebOSXInputActivate(const std::string& type) override;
  void WebOSXInputDeactivate() override;
  void WebOSXInputInvokeAction(uint32_t keysym,
                               ui::WebOSXInputSpecialKeySymbolType symbol_type,
                               ui::WebOSXInputEventType event_type) override;
#endif

  // PlatformEventDispatcher:
  bool CanDispatchEvent(const PlatformEvent& event) override;
  uint32_t DispatchEvent(const PlatformEvent& event) override;

  // ChannelObserver:
  void OnChannelEstablished() override;
  void OnChannelDestroyed() override;

 private:
  void SendWidgetState();
  void AddRegion();
  void ResetRegion();
  void SetCursor();
  void ValidateBounds();
#if defined(OS_WEBOS)
  void SetCustomCursorFromBitmap(webos::CustomCursorType type,
                                 SkBitmap* cursor_image,
                                 int hotspot_x,
                                 int hotspot_y);
  webos::CustomCursorType custom_cursor_type_;
  base::WeakPtrFactory<OzoneWaylandWindow> weak_factory_;
  typedef std::pair<std::string, std::string> WindowProperty;
  std::vector<WindowProperty> pending_window_properties_;
#endif
  PlatformWindowDelegate* delegate_;   // Not owned.
  OzoneGpuPlatformSupportHost* sender_;  // Not owned.
  WindowManagerWayland* window_manager_;  // Not owned.
  bool transparent_;
  gfx::Rect bounds_;
  unsigned handle_;
  unsigned parent_;
  ui::WidgetType type_;
  ui::WidgetState state_;
  SkRegion* region_;
  base::string16 title_;
  // The current cursor bitmap (immutable).
  scoped_refptr<BitmapCursorOzone> bitmap_;
  bool init_window_;

  DISALLOW_COPY_AND_ASSIGN(OzoneWaylandWindow);
};

}  // namespace ui

#endif  // OZONE_PLATFORM_OZONE_WAYLAND_WINDOW_H_
