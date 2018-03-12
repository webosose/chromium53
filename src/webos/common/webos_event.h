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

#ifndef WEBOS_COMMON_WEBOS_EVENT_H_
#define WEBOS_COMMON_WEBOS_EVENT_H_

#include "webos/common/webos_export.h"

class WEBOS_EXPORT WebOSEvent {
 public:
  enum Type {
    None,
    Close,
    Expose,
    WindowStateChange,
    WindowStateAboutToChange,
    KeyPress,
    KeyRelease,
    MouseMove,
    MouseButtonPress,
    MouseButtonRelease,
    Wheel,
    Enter,
    Leave,
    Swap,
    FocusIn,
    FocusOut,
    InputPanelVisible,
  };

  explicit WebOSEvent(Type type, int flags = 0) : type_(type), flags_(flags) {}
  virtual ~WebOSEvent() {}

  inline Type GetType() { return type_; }
  inline void SetType(Type type) { type_ = type; }
  inline int GetFlags() { return flags_; }
  inline void SetFlags(int flags) { flags_ = flags; }

 protected:
  Type type_;
  int flags_;
};

class WEBOS_EXPORT WebOSKeyEvent : public WebOSEvent {
 public:
  explicit WebOSKeyEvent(WebOSEvent::Type type,
      unsigned code)
      : WebOSEvent(type),
        code_(code) {}
  ~WebOSKeyEvent() {}

  unsigned GetCode() const { return code_; }

 protected:
  unsigned code_;
};

class WEBOS_EXPORT WebOSMouseEvent : public WebOSEvent {
 public:
  enum Button {
    ButtonNone    = 0,
    ButtonLeft    = 1 << 0,
    ButtonMiddle  = 1 << 1,
    ButtonRight   = 1 << 2,
  };

  explicit WebOSMouseEvent(Type type, float x, float y, int flags = 0)
      : WebOSEvent(type, flags),
        x_(x),
        y_(y) {}
  ~WebOSMouseEvent() {}

  float GetX() { return x_; }
  float GetY() { return y_; }
  int GetButton();

 protected:
  float x_;
  float y_;
};

class WEBOS_EXPORT WebOSMouseWheelEvent : public WebOSMouseEvent {
 public:
  explicit WebOSMouseWheelEvent(
      Type type, float x, float y, float x_offset, float y_offset)
      : WebOSMouseEvent(type, x, y),
        x_offset_(x_offset),
        y_offset_(y_offset) {}
  ~WebOSMouseWheelEvent() {}

  float GetXOffset() { return x_offset_; }
  float GetYOffset() { return y_offset_; }

 protected:
  float x_offset_;
  float y_offset_;
};

class WEBOS_EXPORT WebOSVirtualKeyboardEvent : public WebOSEvent {
 public:
  explicit WebOSVirtualKeyboardEvent(Type type,
                                     bool visible,
                                     int height,
                                     int flags = 0)
      : WebOSEvent(type, flags), visible_(visible), height_(height) {}
  ~WebOSVirtualKeyboardEvent() {}

  bool GetVisible() { return visible_; }
  int GetHeight() { return height_; }

 protected:
  bool visible_;
  float height_;
};

#endif  // WEBOS_COMMON_WEBOS_EVENT_H_
