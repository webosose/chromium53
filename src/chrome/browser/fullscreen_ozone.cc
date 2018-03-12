// Copyright 2016 The LG Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/fullscreen.h"

#include "ash/root_window_controller.h"

bool IsFullScreenMode() {
#if defined(USE_ASH)
  ash::RootWindowController* controller =
      ash::RootWindowController::ForTargetRootWindow();
  return controller && controller->GetWindowForFullscreenMode();
#else
  // FIXME: Need to implement
  return false;
#endif  // USE_ASH
}