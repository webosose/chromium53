// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_PLATFORM_WINDOW_H_
#define UI_PLATFORM_WINDOW_PLATFORM_WINDOW_H_

#include <memory>

#include "base/strings/string16.h"
#include "ui/base/cursor/cursor.h"
#include "ui/gfx/native_widget_types.h"

#if defined(OS_WEBOS)
#include "ui/platform_window/webos/webos_xinput_types.h"
#include "webos/common/webos_constants.h"
#endif

namespace gfx {
class Rect;
}

class SkPath;

namespace ui {

class PlatformImeController;
class PlatformWindowDelegate;

#if defined(OS_WEBOS)
class WindowGroupConfiguration;
#endif

// Platform window.
//
// Each instance of PlatformWindow represents a single window in the
// underlying platform windowing system (i.e. X11/Win/OSX).
class PlatformWindow {
 public:
  enum PlatformWindowType {
    PLATFORM_WINDOW_UNKNOWN,
    PLATFORM_WINDOW_TYPE_TOOLTIP,
    PLATFORM_WINDOW_TYPE_POPUP,
    PLATFORM_WINDOW_TYPE_MENU,
    PLATFORM_WINDOW_TYPE_BUBBLE,
    PLATFORM_WINDOW_TYPE_WINDOW,
    PLATFORM_WINDOW_TYPE_WINDOW_FRAMELESS
  };

  virtual ~PlatformWindow() {}

  virtual void InitPlatformWindow(PlatformWindowType type,
                                  gfx::AcceleratedWidget parent_window) { }
  virtual void SetWindowShape(const SkPath& path) { }
  virtual void SetOpacity(unsigned char opacity) { }

  // Asks the GPU process to send data of type |mime_type|.
  virtual void RequestDragData(const std::string& mime_type) { }
  virtual void RequestSelectionData(const std::string& mime_type) { }

  // Indicates to the drag source that the data will or will not be accepted
  // at the current (x, y) coordinates. Note that there is a harmless race here.
  // The browser process could decide to accept or reject the data based on
  // old (x, y) coordinates that have since been updated by a new DragMotion
  // event in the GPU process. This doesn't matter because the browser process
  // will promptly correct the matter by calling one of these functions again
  // when it receives the DragMotion event, and these functions are only used to
  // provide user feedback: they don't affect correctness.
  virtual void DragWillBeAccepted(uint32_t serial,
                                  const std::string& mime_type) { }
  virtual void DragWillBeRejected(uint32_t serial) { }

  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void Close() = 0;

  // Sets and gets the bounds of the platform-window. Note that the bounds is in
  // physical pixel coordinates.
  virtual void SetBounds(const gfx::Rect& bounds) = 0;
  virtual gfx::Rect GetBounds() = 0;

  virtual void SetTitle(const base::string16& title) = 0;
  virtual void SetSurfaceId(int surface_id) = 0;

#if defined(OS_WEBOS)
  virtual void SetCustomCursor(webos::CustomCursorType type,
                       const std::string& path,
                       int hotspot_x,
                       int hotspot_y) {}
  virtual void ResetCustomCursor() {}
  virtual void SetKeyMask(webos::WebOSKeyMask key_mask, bool value) {}
  virtual void SetWindowProperty (const std::string& name, const std::string& value) {}

  virtual void CreateGroup(const WindowGroupConfiguration&) {}
  virtual void AttachToGroup(const std::string& name,
                             const std::string& layer) {}
  virtual void FocusGroupOwner() {}
  virtual void FocusGroupLayer() {}
  virtual void DetachGroup() {}
  virtual void ToggleFullscreenWithSize(const gfx::Size& size) = 0;
  virtual void WebOSXInputActivate(const std::string& type) {}
  virtual void WebOSXInputDeactivate() {}
  virtual void WebOSXInputInvokeAction(
      uint32_t keysym,
      WebOSXInputSpecialKeySymbolType symbol_type,
      WebOSXInputEventType event_type) {}
#endif

  virtual void SetCapture() = 0;
  virtual void ReleaseCapture() = 0;

  virtual void ToggleFullscreen() = 0;
  virtual void Maximize() = 0;
  virtual void Minimize() = 0;
  virtual void Restore() = 0;

  virtual void SetCursor(PlatformCursor cursor) = 0;

  // Moves the cursor to |location|. Location is in platform window coordinates.
  virtual void MoveCursorTo(const gfx::Point& location) = 0;

  // Confines the cursor to |bounds| when it is in the platform window. |bounds|
  // is in platform window coordinates.
  virtual void ConfineCursorToBounds(const gfx::Rect& bounds) = 0;

  // The PlatformImeController is owned by the PlatformWindow, the ownership is
  // not transferred.
  virtual PlatformImeController* GetPlatformImeController() = 0;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_PLATFORM_WINDOW_H_
