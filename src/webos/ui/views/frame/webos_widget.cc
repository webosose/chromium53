// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/ui/views/frame/webos_widget.h"

#include "base/command_line.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/hit_test.h"
#include "ui/base/theme_provider.h"
#include "ui/events/event_handler.h"
#include "ui/gfx/font_list.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/native_widget.h"
#include "webos/ui/views/frame/webos_native_widget_aura.h"
#include "webos/ui/views/frame/webos_root_view.h"
#include "webos/ui/views/frame/webos_view.h"
#include "webos/ui/views/frame/webos_widget_view.h"

////////////////////////////////////////////////////////////////////////////////
// WebOSWidget, public:

WebOSWidget::WebOSWidget(WebOSView* view)
    : root_view_(NULL),
      widget_view_(NULL),
      view_(view),
      theme_provider_(NULL) {
  view_->set_widget(this);
  set_is_secondary_widget(false);
  // Don't focus anything on creation, selecting a tab will set the focus.
  set_focus_on_creation(false);
}

WebOSWidget::~WebOSWidget() {
  if (browser_command_handler_ && GetNativeView())
    GetNativeView()->RemovePreTargetHandler(browser_command_handler_.get());
}

// static
const gfx::FontList& WebOSWidget::GetTitleFontList() {
  static const gfx::FontList* title_font_list = new gfx::FontList();
  ANNOTATE_LEAKING_OBJECT_PTR(title_font_list);
  return *title_font_list;
}

void WebOSWidget::InitWebOSWidget(const gfx::Rect& rect) {
  views::Widget::InitParams params;
  params.delegate = view_;
  params.native_widget = new WebOSNativeWidgetAura(this, view_);
  params.bounds = rect;
  params.show_state = ui::WindowShowState::SHOW_STATE_DEFAULT;
  params.opacity = views::Widget::InitParams::TRANSLUCENT_WINDOW;
  params.wm_role_name = "WebApp";
  params.remove_standard_frame = false;
  set_frame_type(FRAME_TYPE_FORCE_NATIVE);

  Init(params);

  if (browser_command_handler_)
    GetNativeWindow()->AddPreTargetHandler(browser_command_handler_.get());
}

int WebOSWidget::GetMinimizeButtonOffset() const {
  NOTIMPLEMENTED();
  return 0;
}

views::View* WebOSWidget::GetFrameView() const {
  return widget_view_;
}

///////////////////////////////////////////////////////////////////////////////
// WebOSWidget, views::Widget overrides:

views::internal::RootView* WebOSWidget::CreateRootView() {
  delete root_view_;
  root_view_ = new WebOSRootView(view_, this);
  return root_view_;
}

views::NonClientFrameView* WebOSWidget::CreateNonClientFrameView() {
  delete widget_view_;
  widget_view_ = new WebOSWidgetView(this, view_);
  return widget_view_;
}

bool WebOSWidget::GetAccelerator(int command_id,
                                 ui::Accelerator* accelerator) const {
  return view_->GetAccelerator(command_id, accelerator);
}

ui::ThemeProvider* WebOSWidget::GetThemeProvider() const {
  return theme_provider_;
}

void WebOSWidget::SchedulePaintInRect(const gfx::Rect& rect) {
  views::Widget::SchedulePaintInRect(rect);
}

void WebOSWidget::OnNativeWidgetActivationChanged(bool active) {
  if (view_)
    view_->OnWidgetActivationChanged(this, active);
  Widget::OnNativeWidgetActivationChanged(active);
}

bool WebOSWidget::ShouldLeaveOffsetNearTopBorder() {
  return !IsMaximized();
}
