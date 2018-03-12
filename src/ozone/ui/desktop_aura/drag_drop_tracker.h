// Copyright (c) 2015-2018 LG Electronics, Inc.
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

// This file is taken from "ash/drag_drop/drag_drop_tracker.h"

#ifndef OZONE_UI_DESKTOP_AURA_DRAG_DROP_TRACKER_H_
#define OZONE_UI_DESKTOP_AURA_DRAG_DROP_TRACKER_H_

#include "ui/events/event.h"
#include "ui/views/views_export.h"

namespace aura {
class Window;
class WindowDelegate;
}

namespace views {

// Provides functions for handling drag events inside and outside the root
// window where drag is started. This internally sets up a capture window for
// tracking drag events outside the root window where drag is initiated.
// ash/wm/coordinate_conversion.h is used internally and only X11 environment
// is supported for now.
class VIEWS_EXPORT DragDropTracker {
 public:
  DragDropTracker(aura::Window* context_root,
                  aura::WindowDelegate* delegate);
  ~DragDropTracker();

  aura::Window* capture_window() { return capture_window_.get(); }

  // Tells our |capture_window_| to take capture. This is not done right at
  // creation to give the caller a chance to perform any operations needed
  // before the capture is transfered.
  void TakeCapture();

  // Gets the target located at |event| in the coordinates of the active root
  // window.
  aura::Window* GetTarget(const ui::LocatedEvent& event);

  // Converts the locations of |event| in the coordinates of the active root
  // window to the ones in |target|'s coordinates.
  // Caller takes ownership of the returned object.
  ui::LocatedEvent* ConvertEvent(aura::Window* target,
                                 const ui::LocatedEvent& event);

 private:
  // A window for capturing drag events while dragging.
  std::unique_ptr<aura::Window> capture_window_;

  DISALLOW_COPY_AND_ASSIGN(DragDropTracker);
};

}  // namespace views

#endif  // OZONE_UI_DESKTOP_AURA_DRAG_DROP_TRACKER_H_
