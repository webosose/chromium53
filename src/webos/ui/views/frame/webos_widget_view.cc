// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/ui/views/frame/webos_widget_view.h"

#include <algorithm>
#include <string>

#include "base/compiler_specific.h"
#include "chrome/browser/themes/theme_properties.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/hit_test.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/canvas.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/window/window_shape.h"
#include "webos/ui/views/frame/webos_view.h"
#include "webos/ui/views/frame/webos_widget.h"

namespace {

// In the window corners, the resize areas don't actually expand bigger, but the
// 16 px at the end of each edge triggers diagonal resizing.
const int kResizeAreaCornerSize = 16;

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// WebOSWidgetView, public:

WebOSWidgetView::WebOSWidgetView(WebOSWidget* widget,
                                 WebOSView* view)
    : widget_(widget),
      view_(view) {
}

WebOSWidgetView::~WebOSWidgetView() {
}

gfx::Size WebOSWidgetView::GetMinimumSize() const {
  NOTIMPLEMENTED();
  return gfx::Size();
}

///////////////////////////////////////////////////////////////////////////////
// WebOSWidgetView, views::NonClientFrameView implementation:

gfx::Rect WebOSWidgetView::GetBoundsForClientView() const {
  return bounds();
}

gfx::Rect WebOSWidgetView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  return bounds();
}

int WebOSWidgetView::NonClientHitTest(const gfx::Point& point) {
  if (!bounds().Contains(point))
    return HTNOWHERE;

  int frame_component = widget()->client_view()->NonClientHitTest(point);
  if (frame_component != HTNOWHERE)
    return frame_component;

  views::WidgetDelegate* delegate = widget()->widget_delegate();
  if (!delegate) {
    LOG(WARNING) << "delegate is NULL, returning safe default.";
    return HTCAPTION;
  }
  int window_component = GetHTComponentForFrame(point, 0,
      0, kResizeAreaCornerSize, kResizeAreaCornerSize,
      delegate->CanResize());
  // Fall back to the caption if no other component matches.
  return (window_component == HTNOWHERE) ? HTCAPTION : window_component;
}

void WebOSWidgetView::GetWindowMask(const gfx::Size& size,
                                    gfx::Path* window_mask) {
  DCHECK(window_mask);

  if (widget()->IsFullscreen())
    return;

  views::GetDefaultWindowMask(
      size, widget()->GetCompositor()->device_scale_factor(), window_mask);
}

void WebOSWidgetView::ResetWindowControls() {
  NOTIMPLEMENTED();
}

void WebOSWidgetView::SizeConstraintsChanged() {
  NOTIMPLEMENTED();
}

void WebOSWidgetView::UpdateWindowIcon() {
  NOTIMPLEMENTED();
}

void WebOSWidgetView::UpdateWindowTitle() {
  NOTIMPLEMENTED();
}

///////////////////////////////////////////////////////////////////////////////
// WebOSWidgetView, views::View overrides:

void WebOSWidgetView::GetAccessibleState(
    ui::AXViewState* state) {
  state->role = ui::AX_ROLE_TITLE_BAR;
}

///////////////////////////////////////////////////////////////////////////////
// WebOSWidgetView, views::View overrides:

void WebOSWidgetView::OnPaint(gfx::Canvas* canvas) {
}

///////////////////////////////////////////////////////////////////////////////
// WebOSWidgetView, private:

// views::NonClientFrameView:
bool WebOSWidgetView::DoesIntersectRect(const views::View* target,
                                        const gfx::Rect& rect) const {
  CHECK_EQ(target, this);
  return false;
}
