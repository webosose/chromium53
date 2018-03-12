// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/common/webos_event.h"

#include "ui/events/event.h"

////////////////////////////////////////////////////////////////////////////////
// WebOSMouseEvent, public:

int WebOSMouseEvent::GetButton() {
  int buttons = ButtonNone;

  if (GetFlags() & ui::EF_LEFT_MOUSE_BUTTON)
    buttons |= ButtonLeft;
  if (GetFlags() & ui::EF_MIDDLE_MOUSE_BUTTON)
    buttons |= ButtonMiddle;
  if (GetFlags() & ui::EF_RIGHT_MOUSE_BUTTON)
    buttons |= ButtonRight;

  return buttons;
}
