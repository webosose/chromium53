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

#ifndef WEBOS_UI_VIEWS_FRAME_WEBOS_WIDGET_H_
#define WEBOS_UI_VIEWS_FRAME_WEBOS_WIDGET_H_

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "ui/views/widget/widget.h"

#include <memory>

class NativeBrowserFrame;
class NonClientFrameView;
class WebOSRootView;
class WebOSView;
class WebOSWidgetView;

namespace gfx {
class FontList;
class Rect;
}

namespace ui {
class EventHandler;
class ThemeProvider;
}

namespace views {
class View;
}

// This is a virtual interface that allows system specific browser frames.
class WebOSWidget : public views::Widget {
 public:
  explicit WebOSWidget(WebOSView* view);
  virtual ~WebOSWidget();

  static const gfx::FontList& GetTitleFontList();

  // Initialize the frame (creates the underlying native window).
  void InitWebOSWidget(const gfx::Rect& rect);

  // Determine the distance of the left edge of the minimize button from the
  // left edge of the window. Used in our Non-Client View's Layout.
  int GetMinimizeButtonOffset() const;

  // Returns the NonClientFrameView of this frame.
  views::View* GetFrameView() const;

  // Overridden from views::Widget:
  views::internal::RootView* CreateRootView() override;
  views::NonClientFrameView* CreateNonClientFrameView() override;
  bool GetAccelerator(int command_id,
                      ui::Accelerator* accelerator) const override;
  ui::ThemeProvider* GetThemeProvider() const override;
  void SchedulePaintInRect(const gfx::Rect& rect) override;
  void OnNativeWidgetActivationChanged(bool active) override;

  // Returns true if we should leave any offset at the frame caption. Typically
  // when the frame is maximized/full screen we want to leave no offset at the
  // top.
  bool ShouldLeaveOffsetNearTopBorder();

 private:

  // A weak reference to the root view associated with the window. We save a
  // copy as a WebOSRootView to avoid evil casting later, when we need to call
  // functions that only exist on WebOSRootView (versus RootView).
  WebOSRootView* root_view_;

  // A pointer to our NonClientFrameView as a WebOSNonClientFrameView.
  WebOSWidgetView* widget_view_;

  // The WebOSView is our ClientView. This is a pointer to it.
  WebOSView* view_;

  ui::ThemeProvider* theme_provider_;


  std::unique_ptr<ui::EventHandler> browser_command_handler_;

  DISALLOW_COPY_AND_ASSIGN(WebOSWidget);
};

#endif  // WEBOS_UI_VIEWS_FRAME_WEBOS_WIDGET_H_
