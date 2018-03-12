// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/seat.h"

#include "base/logging.h"
#if defined(USE_DATA_DEVICE_MANAGER)
#include "ozone/wayland/data_device.h"
#endif
#include "ozone/wayland/display.h"
#include "ozone/wayland/input/cursor.h"
#include "ozone/wayland/input/keyboard.h"
#include "ozone/wayland/input/pointer.h"
#if defined(OS_WEBOS)
#include "base/strings/string_util.h"
#include "ozone/wayland/input/webos_text_input.h"
#else
#include "ozone/wayland/input/text_input.h"
#endif
#include "ozone/wayland/input/touchscreen.h"

namespace ozonewayland {

WaylandSeat::WaylandSeat(WaylandDisplay* display,
                         uint32_t id)
    : focused_window_handle_(0),
      grab_window_handle_(0),
      grab_button_(0),
      seat_(NULL),
      input_keyboard_(NULL),
      input_pointer_(NULL),
      input_touch_(NULL),
      text_input_(NULL) {
  static const struct wl_seat_listener kInputSeatListener = {
    WaylandSeat::OnSeatCapabilities,
  };

  seat_ = static_cast<wl_seat*>(
      wl_registry_bind(display->registry(), id, &wl_seat_interface, 1));
  DCHECK(seat_);
  wl_seat_add_listener(seat_, &kInputSeatListener, this);
  wl_seat_set_user_data(seat_, this);

#if defined(USE_DATA_DEVICE_MANAGER)
  data_device_ = new WaylandDataDevice(display, seat_);
#endif
#if !defined(OS_WEBOS)
  text_input_ = new WaylandTextInput(this);
#else
  text_input_ = WaylandTextInput::GetInstance();

  if (!display->GetWebosInputManager())
    return;

  static const struct wl_webos_seat_listener kWebosSeatListener = {
      WaylandSeat::OnWebosSeatCapabilities,
  };

  wl_webos_seat* webos_seat = wl_webos_input_manager_get_webos_seat(
      display->GetWebosInputManager(), seat_);
  wl_webos_seat_add_listener(webos_seat, &kWebosSeatListener, this);
#endif
}

WaylandSeat::~WaylandSeat() {
#if defined(USE_DATA_DEVICE_MANAGER)
  delete data_device_;
#endif
  delete input_keyboard_;
  delete input_pointer_;
#if !defined(OS_WEBOS)
  delete text_input_;
#endif
  if (input_touch_ != NULL) {
    delete input_touch_;
  }
  wl_seat_destroy(seat_);
}

void WaylandSeat::OnSeatCapabilities(void *data, wl_seat *seat, uint32_t caps) {
  WaylandSeat* device = static_cast<WaylandSeat*>(data);
  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !device->input_keyboard_) {
    device->input_keyboard_ = new WaylandKeyboard();
    device->input_keyboard_->OnSeatCapabilities(seat, caps);
  } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && device->input_keyboard_) {
    device->input_keyboard_->OnSeatCapabilities(seat, caps);
    delete device->input_keyboard_;
    device->input_keyboard_ = NULL;
  }

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !device->input_pointer_) {
    device->input_pointer_ = new WaylandPointer();
    device->input_pointer_->OnSeatCapabilities(seat, caps);
  } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && device->input_pointer_) {
    device->input_pointer_->OnSeatCapabilities(seat, caps);
    delete device->input_pointer_;
    device->input_pointer_ = NULL;
  }

  if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !device->input_touch_) {
    device->input_touch_ = new WaylandTouchscreen();
    device->input_touch_->OnSeatCapabilities(seat, caps);
  } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && device->input_touch_) {
    device->input_touch_->OnSeatCapabilities(seat, caps);
    delete device->input_touch_;
    device->input_touch_ = NULL;
  }
}

void WaylandSeat::SetFocusWindowHandle(unsigned windowhandle) {
  focused_window_handle_ = windowhandle;
  WaylandWindow* window = NULL;
  if (windowhandle)
    window = WaylandDisplay::GetInstance()->GetWindow(windowhandle);
  text_input_->SetActiveWindow(window);

#if defined(OS_WEBOS)
  if (!window) {
    input_pointer_->Cursor()->UpdateBitmap(
        std::vector<SkBitmap>(), gfx::Point(255, 255),
        WaylandDisplay::GetInstance()->GetSerial());
  }
#endif
}

void WaylandSeat::SetGrabWindowHandle(unsigned windowhandle, uint32_t button) {
  grab_window_handle_ = windowhandle;
  grab_button_ = button;
}

void WaylandSeat::SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                                  const gfx::Point& location) {
  if (!input_pointer_) {
    LOG(WARNING) << "Tried to change cursor without input configured";
    return;
  }
  input_pointer_->Cursor()->UpdateBitmap(
      bitmaps, location, WaylandDisplay::GetInstance()->GetSerial());
}

void WaylandSeat::MoveCursor(const gfx::Point& location) {
  if (!input_pointer_) {
    LOG(WARNING) << "Tried to move cursor without input configured";
    return;
  }

  input_pointer_->Cursor()->MoveCursor(
      location, WaylandDisplay::GetInstance()->GetSerial());
}

void WaylandSeat::ResetIme() {
  text_input_->ResetIme();
}

void WaylandSeat::ImeCaretBoundsChanged(gfx::Rect rect) {
  NOTIMPLEMENTED();
}

void WaylandSeat::ShowInputPanel(unsigned windowhandle) {
  text_input_->ShowInputPanel(seat_, windowhandle);
}

void WaylandSeat::HideInputPanel() {
  text_input_->HideInputPanel(seat_);
}

#if defined(OS_WEBOS)
void WaylandSeat::OnWebosSeatCapabilities(void* data,
                                          wl_webos_seat* wl_webos_seat1,
                                          uint32_t id,
                                          const char* name,
                                          uint32_t designator,
                                          uint32_t capabilities) {
  WaylandSeat* seat = static_cast<WaylandSeat*>(data);
  ui::EventSourceType source_type = ui::SOURCE_TYPE_KEYBOARD;
  std::string device_name = base::ToLowerASCII(std::string(name));
  if (device_name.find("usb") != std::string::npos ||
      device_name.find("keyboard") != std::string::npos ||
      device_name.find("hid") != std::string::npos) {
    source_type = ui::SOURCE_TYPE_KEYBOARD;
  } else if (device_name.find("mouse") != std::string::npos) {
    source_type = ui::SOURCE_TYPE_MOUSE;
  }

  if (seat->GetKeyBoard())
    seat->GetKeyBoard()->SetSourceType(source_type);

  if (seat->GetPointer())
    seat->GetPointer()->SetSourceType(source_type);
}

void WaylandSeat::SetSurroundingText(const std::string& text,
                                     size_t cursor_position,
                                     size_t anchor_position) {
  text_input_->SetSurroundingText(text, cursor_position, anchor_position);
}
#endif

void WaylandSeat::SetInputContentType(ui::InputContentType content_type,
                                      int text_input_flags) {
#if defined(OS_WEBOS)
  text_input_->SetInputContentType(content_type, text_input_flags);
#endif
}

}  // namespace ozonewayland
