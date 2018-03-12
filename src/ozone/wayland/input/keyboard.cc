// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/input/keyboard.h"

#include "ozone/wayland/display.h"
#include "ozone/wayland/window.h"
#include "ozone/wayland/seat.h"

namespace ozonewayland {

namespace {

// Key code for wheel (OK) from Magic Motion RCU
const int32_t kMotionBtnOK = -8;
}

WaylandKeyboard::WaylandKeyboard()
    : input_keyboard_(NULL),
      dispatcher_(NULL),
      source_type_(ui::SOURCE_TYPE_UNKNOWN) {}

WaylandKeyboard::~WaylandKeyboard() {
  if (input_keyboard_)
    wl_keyboard_destroy(input_keyboard_);
}

void WaylandKeyboard::OnSeatCapabilities(wl_seat *seat, uint32_t caps) {
  static const struct wl_keyboard_listener kInputKeyboardListener = {
    WaylandKeyboard::OnKeyboardKeymap,
    WaylandKeyboard::OnKeyboardEnter,
    WaylandKeyboard::OnKeyboardLeave,
    WaylandKeyboard::OnKeyNotify,
    WaylandKeyboard::OnKeyModifiers,
  };

  dispatcher_ =
      WaylandDisplay::GetInstance();

  if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !input_keyboard_) {
    input_keyboard_ = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(input_keyboard_, &kInputKeyboardListener,
        this);
  } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && input_keyboard_) {
    wl_keyboard_destroy(input_keyboard_);
    input_keyboard_ = NULL;
  }
}

void WaylandKeyboard::OnKeyNotify(void* data,
                                  wl_keyboard* input_keyboard,
                                  uint32_t serial,
                                  uint32_t time,
                                  uint32_t key,
                                  uint32_t state) {
  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);
  ui::EventType type = ui::ET_KEY_PRESSED;
  WaylandDisplay::GetInstance()->SetSerial(serial);
  if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    type = ui::ET_KEY_RELEASED;

  if (key == kMotionBtnOK)
    key = 28;  // Value corresponding to KEY_ENTER;

  const uint32_t device_id = wl_proxy_get_id(
      reinterpret_cast<wl_proxy*>(input_keyboard));
  device->dispatcher_->KeyNotify(type, key,
#if defined(OS_WEBOS)
                                 device->source_type_,
#endif
                                 device_id);
}

void WaylandKeyboard::OnKeyboardKeymap(void *data,
                                       struct wl_keyboard *keyboard,
                                       uint32_t format,
                                       int fd,
                                       uint32_t size) {
  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);

  if (!data) {
    close(fd);
    return;
  }

  if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
    close(fd);
    return;
  }

  base::SharedMemoryHandle section =  base::FileDescriptor(fd, true);
  device->dispatcher_->InitializeXKB(section, size);
}

void WaylandKeyboard::OnKeyboardEnter(void* data,
                                      wl_keyboard* input_keyboard,
                                      uint32_t serial,
                                      wl_surface* surface,
                                      wl_array* keys) {
  WaylandDisplay::GetInstance()->SetSerial(serial);

#if defined(OS_WEBOS)
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();

  if (!surface) {
    seat->SetFocusWindowHandle(0);
    return ;
  }

  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);
  WaylandWindow* window =
    static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));
  seat->SetFocusWindowHandle(window->Handle());
  device->dispatcher_->KeyboardEnter(window->Handle());
#endif
}

void WaylandKeyboard::OnKeyboardLeave(void* data,
                                      wl_keyboard* input_keyboard,
                                      uint32_t serial,
                                      wl_surface* surface) {
  WaylandDisplay::GetInstance()->SetSerial(serial);

#if defined(OS_WEBOS)
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();

  if (!surface)
    return ;

  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);
  WaylandWindow* window =
    static_cast<WaylandWindow*>(wl_surface_get_user_data(surface));
  device->dispatcher_->KeyboardLeave(window->Handle());
  seat->SetFocusWindowHandle(0);
#endif
}

void WaylandKeyboard::OnKeyModifiers(void *data,
                                     wl_keyboard *keyboard,
                                     uint32_t serial,
                                     uint32_t mods_depressed,
                                     uint32_t mods_latched,
                                     uint32_t mods_locked,
                                     uint32_t group) {
#if defined(OS_WEBOS)
  WaylandKeyboard* device = static_cast<WaylandKeyboard*>(data);
  device->dispatcher_->KeyboardModifier(mods_depressed, mods_locked);
#endif
}

}  // namespace ozonewayland
