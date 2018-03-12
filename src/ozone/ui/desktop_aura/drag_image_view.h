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

// This file is taken from "ash/drag_drop/drag_image_view.h"

#ifndef OZONE_UI_DESKTOP_AURA_DRAG_IMAGE_VIEW_H_
#define OZONE_UI_DESKTOP_AURA_DRAG_IMAGE_VIEW_H_

#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/views_export.h"

namespace views {

class Widget;

// This class allows to show a (native) view always on top of everything. It
// does this by creating a widget and setting the content as the given view. The
// caller can then use this object to freely move / drag it around on the
// desktop in screen coordinates.
class VIEWS_EXPORT DragImageView : public views::ImageView {
 public:
  // |context is the native view context used to create the widget holding the
  // drag image.
  // |source| is the event source that started this drag drop operation (touch
  // or mouse). It is used to determine attributes of the drag image such as
  // whether to show drag operation hint on top of the image.
  DragImageView(gfx::NativeView context,
                ui::DragDropTypes::DragEventSource source);
  virtual ~DragImageView();

  // Sets the bounds of the native widget in screen
  // coordinates.
  // TODO(oshima): Looks like this is root window's
  // coordinate. Change this to screen's coordinate.
  void SetBoundsInScreen(const gfx::Rect& bounds);

  // Sets the position of the native widget.
  void SetScreenPosition(const gfx::Point& position);

  // Gets the image position in screen coordinates.
  gfx::Rect GetBoundsInScreen() const;

  // Sets the visibility of the native widget.
  void SetWidgetVisible(bool visible);

  // For touch drag drop, we show a hint corresponding to the drag operation
  // (since no mouse cursor is visible). These functions set the hint
  // parameters.
  void SetTouchDragOperationHintOff();

  // |operation| is a bit field indicating allowable drag operations from
  // ui::DragDropTypes::DragOperation.
  void SetTouchDragOperation(int operation);
  void SetTouchDragOperationHintPosition(const gfx::Point& position);

  // Sets the |opacity| of the image view between 0.0 and 1.0.
  void SetOpacity(float opacity);

 private:
  // Overridden from views::ImageView.
  void OnPaint(gfx::Canvas* canvas) override;

  std::unique_ptr<views::Widget> widget_;
  gfx::Size widget_size_;

  ui::DragDropTypes::DragEventSource drag_event_source_;
  int touch_drag_operation_;
  gfx::Point touch_drag_operation_indicator_position_;

  DISALLOW_COPY_AND_ASSIGN(DragImageView);
};

}  // namespace views

#endif  // OZONE_UI_DESKTOP_AURA_DRAG_IMAGE_VIEW_H_
