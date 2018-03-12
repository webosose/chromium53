// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_WAYLAND_SEAT_H_
#define OZONE_WAYLAND_SEAT_H_

#include <wayland-client.h>
#include <vector>

#include "third_party/skia/include/core/SkBitmap.h"
#include "ozone/platform/input_content_type.h"
#include "ui/gfx/geometry/rect.h"

#if defined(OS_WEBOS)
#include "wayland-webos-input-manager-client-protocol.h"
#endif

namespace ozonewayland {

class WaylandDataDevice;
class WaylandKeyboard;
class WaylandPointer;
class WaylandDisplay;
class WaylandTouchscreen;
class WaylandTextInput;

class WaylandSeat {
 public:
  WaylandSeat(WaylandDisplay* display, uint32_t id);
  ~WaylandSeat();

  wl_seat* GetWLSeat() const { return seat_; }
#if defined(USE_DATA_DEVICE_MANAGER)
  WaylandDataDevice* GetDataDevice() const { return data_device_; }
#endif
  WaylandKeyboard* GetKeyBoard() const { return input_keyboard_; }
  WaylandPointer* GetPointer() const { return input_pointer_; }
  unsigned GetFocusWindowHandle() const { return focused_window_handle_; }
  unsigned GetGrabWindowHandle() const { return grab_window_handle_; }
  uint32_t GetGrabButton() const { return grab_button_; }
  void SetFocusWindowHandle(unsigned windowhandle);
  void SetGrabWindowHandle(unsigned windowhandle, uint32_t button);
  void SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                       const gfx::Point& location);
  void MoveCursor(const gfx::Point& location);

  void ResetIme();
  void ImeCaretBoundsChanged(gfx::Rect rect);
  void ShowInputPanel(unsigned windowhandle);
  void HideInputPanel();
  void SetInputContentType(ui::InputContentType content_type,
                           int text_input_flags);
#if defined(OS_WEBOS)
  void SetSurroundingText(const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position);
  WaylandTextInput* GetTextInput() const { return text_input_; }
#endif
 private:
  static void OnSeatCapabilities(void *data,
                                 wl_seat *seat,
                                 uint32_t caps);
#if defined(OS_WEBOS)
  static void OnWebosSeatCapabilities(void* data,
                                      wl_webos_seat* wl_webos_seat,
                                      uint32_t id,
                                      const char* name,
                                      uint32_t designator,
                                      uint32_t capabilities);
#endif

  // Keeps track of current focused window.
  unsigned focused_window_handle_;
  unsigned grab_window_handle_;
  uint32_t grab_button_;
  struct wl_seat* seat_;
#if defined(USE_DATA_DEVICE_MANAGER)
  WaylandDataDevice* data_device_;
#endif
  WaylandKeyboard* input_keyboard_;
  WaylandPointer* input_pointer_;
  WaylandTouchscreen* input_touch_;
  WaylandTextInput* text_input_;

  DISALLOW_COPY_AND_ASSIGN(WaylandSeat);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_SEAT_H_
