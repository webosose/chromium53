// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host_ozone_wayland_external.h"

#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/views/frame/browser_frame.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHostOzone, public:

BrowserDesktopWindowTreeHostOzone::BrowserDesktopWindowTreeHostOzone(
    views::internal::NativeWidgetDelegate* native_widget_delegate,
    views::DesktopNativeWidgetAura* desktop_native_widget_aura,
    BrowserView* browser_view,
    BrowserFrame* browser_frame)
    : DesktopWindowTreeHostOzone(native_widget_delegate,
                                 desktop_native_widget_aura) {
}


BrowserDesktopWindowTreeHostOzone::~BrowserDesktopWindowTreeHostOzone() {
}

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHostOzone,
//     BrowserDesktopWindowTreeHost implementation:

views::DesktopWindowTreeHost*
    BrowserDesktopWindowTreeHostOzone::AsDesktopWindowTreeHost() {
  return this;
}

int BrowserDesktopWindowTreeHostOzone::GetMinimizeButtonOffset() const {
  return 0;
}

bool BrowserDesktopWindowTreeHostOzone::UsesNativeSystemMenu() const {
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHostOzone,
//     views::DesktopWindowTreeHostOzone implementation:

void BrowserDesktopWindowTreeHostOzone::Init(
    aura::Window* content_window,
    const views::Widget::InitParams& params) {
  views::DesktopWindowTreeHostOzone::Init(content_window, params);

  // TODO(kalyan): Support for Global Menu.
}

void BrowserDesktopWindowTreeHostOzone::CloseNow() {
  views::DesktopWindowTreeHostOzone::CloseNow();
}

////////////////////////////////////////////////////////////////////////////////
// BrowserDesktopWindowTreeHost, public:

// static
BrowserDesktopWindowTreeHost*
    BrowserDesktopWindowTreeHost::CreateBrowserDesktopWindowTreeHost(
        views::internal::NativeWidgetDelegate* native_widget_delegate,
        views::DesktopNativeWidgetAura* desktop_native_widget_aura,
        BrowserView* browser_view,
        BrowserFrame* browser_frame) {
  return new BrowserDesktopWindowTreeHostOzone(native_widget_delegate,
                                               desktop_native_widget_aura,
                                               browser_view,
                                               browser_frame);
}
