// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/ui/views/frame/webos_native_widget_aura.h"

#include "chrome/app/chrome_command_ids.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_observer.h"
#include "ui/aura/window_tree_host.h"
#include "ui/views/view.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/wm/core/visibility_controller.h"
#include "ui/wm/public/drag_drop_client.h"
#include "ui/wm/public/scoped_tooltip_disabler.h"
#include "webos/ui/views/frame/webos_view.h"
#include "webos/common/webos_native_event_delegate.h"

using aura::Window;

///////////////////////////////////////////////////////////////////////////////
// WebOSNativeWidgetAura, public:

WebOSNativeWidgetAura::WebOSNativeWidgetAura(
    WebOSWidget* widget,
    WebOSView* view)
    : views::DesktopNativeWidgetAura(widget),
      view_(view),
      widget_(widget),
      root_window_tree_host_(NULL) {
  GetNativeWindow()->SetName("WebOSNativeWidgetAura");
}

///////////////////////////////////////////////////////////////////////////////
// WebOSNativeWidgetAura, protected:

WebOSNativeWidgetAura::~WebOSNativeWidgetAura() {
}

///////////////////////////////////////////////////////////////////////////////
// WebOSNativeWidgetAura, views::DesktopNativeWidgetAura overrides:

void WebOSNativeWidgetAura::OnHostClosed() {
  aura::client::SetVisibilityClient(GetNativeView()->GetRootWindow(), NULL);
  DesktopNativeWidgetAura::OnHostClosed();
}

void WebOSNativeWidgetAura::OnDesktopWindowTreeHostDestroyed(
    aura::WindowTreeHost* host) {
  view_->OnDesktopWindowTreeHostDestroyed();
  DesktopNativeWidgetAura::OnDesktopWindowTreeHostDestroyed(host);
}

void WebOSNativeWidgetAura::InitNativeWidget(
    const views::Widget::InitParams& params) {

  root_window_tree_host_ = views::DesktopWindowTreeHost::Create(widget_, this);
  view_->OnDesktopWindowTreeHostCreated(root_window_tree_host_);

  views::Widget::InitParams modified_params = params;
  modified_params.desktop_window_tree_host = root_window_tree_host_;
  DesktopNativeWidgetAura::InitNativeWidget(modified_params);

  visibility_controller_.reset(new wm::VisibilityController);
  aura::Window* root_window = GetNativeWindow()->GetRootWindow();
  aura::client::SetVisibilityClient(root_window, visibility_controller_.get());
  wm::SetChildWindowVisibilityChangesAnimated(root_window);
  aura::client::SetDragDropClient(host()->window(), NULL);
  tooltip_disabler_.reset(new aura::client::ScopedTooltipDisabler(root_window));
}

#if defined(OS_WEBOS)
webos::WebOSNativeEventDelegate* WebOSNativeWidgetAura::GetNativeEventDelegate() const {
  return view_->GetNativeEventDelegate();
}
#endif
