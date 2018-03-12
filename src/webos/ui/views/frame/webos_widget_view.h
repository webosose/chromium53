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

#ifndef WEBOS_UI_VIEWS_FRAME_WEBOS_WIDGET_VIEW_H_
#define WEBOS_UI_VIEWS_FRAME_WEBOS_WIDGET_VIEW_H_

#include "chrome/browser/ui/view_ids.h"
#include "ui/views/window/non_client_view.h"

class WebOSView;
class WebOSWidget;

class WebOSWidgetView : public views::NonClientFrameView {
 public:
  // Constructs a non-client view for an BrowserFrame.
  WebOSWidgetView(WebOSWidget* widget, WebOSView* view);
  virtual ~WebOSWidgetView();

  gfx::Size GetMinimumSize() const override;

  // views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size, gfx::Path* window_mask) override;
  void ResetWindowControls() override;
  void SizeConstraintsChanged() override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;

  // views::View:
  void GetAccessibleState(ui::AXViewState* state) override;

 protected:
  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

  WebOSView* view() const { return view_; }
  WebOSWidget* widget() const { return widget_; }

 private:
  // views::NonClientFrameView:
  bool DoesIntersectRect(const views::View* target,
                         const gfx::Rect& rect) const override;

  // The WebOSWidget that hosts this view.
  WebOSWidget* widget_;

  // The WebOSView hosted within this View.
  WebOSView* view_;

  DISALLOW_COPY_AND_ASSIGN(WebOSWidgetView);
};

#endif  // WEBOS_UI_VIEWS_FRAME_WEBOS_WIDGET_VIEW_H_
