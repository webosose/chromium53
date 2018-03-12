// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/desktop_browser_frame_aura.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/web_applications/web_app.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_observer.h"
#include "ui/base/hit_test.h"
#include "ui/base/models/simple_menu_model.h"
#include "ui/gfx/font.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/visibility_controller.h"

#if defined(OS_WEBOS)
#include "base/base_switches.h"
#include "base/command_line.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#endif

using aura::Window;

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, public:

DesktopBrowserFrameAura::DesktopBrowserFrameAura(
    BrowserFrame* browser_frame,
    BrowserView* browser_view)
    : views::DesktopNativeWidgetAura(browser_frame),
      browser_view_(browser_view),
      browser_frame_(browser_frame),
      browser_desktop_window_tree_host_(nullptr) {
  GetNativeWindow()->SetName("BrowserFrameAura");
}

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, protected:

DesktopBrowserFrameAura::~DesktopBrowserFrameAura() {
}

///////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, views::DesktopNativeWidgetAura overrides:

void DesktopBrowserFrameAura::OnHostClosed() {
  aura::client::SetVisibilityClient(GetNativeView()->GetRootWindow(), nullptr);
  DesktopNativeWidgetAura::OnHostClosed();
}

void DesktopBrowserFrameAura::InitNativeWidget(
    const views::Widget::InitParams& params) {
  browser_desktop_window_tree_host_ =
      BrowserDesktopWindowTreeHost::CreateBrowserDesktopWindowTreeHost(
          browser_frame_,
          this,
          browser_view_,
          browser_frame_);
  views::Widget::InitParams modified_params = params;
  modified_params.desktop_window_tree_host =
      browser_desktop_window_tree_host_->AsDesktopWindowTreeHost();
  DesktopNativeWidgetAura::InitNativeWidget(modified_params);

  visibility_controller_.reset(new wm::VisibilityController);
  aura::client::SetVisibilityClient(GetNativeView()->GetRootWindow(),
                                    visibility_controller_.get());
  wm::SetChildWindowVisibilityChangesAnimated(
      GetNativeView()->GetRootWindow());

#if defined(OS_WEBOS)
  std::string appId("com.webos.app.browser");
  base::CommandLine* com_line = base::CommandLine::ForCurrentProcess();
  if (com_line->HasSwitch(switches::kWebOSAppId))
    appId.assign(com_line->GetSwitchValueASCII(switches::kWebOSAppId));
  modified_params.desktop_window_tree_host->SetWindowProperty(
      "appId", appId.c_str());
  modified_params.desktop_window_tree_host->SetWindowProperty(
      "_WEBOS_ACCESS_POLICY_KEYS_BACK", "true");
  modified_params.desktop_window_tree_host->SetWindowProperty(
      "_WEBOS_LAUNCH_INFO_RECENT", "true");
  modified_params.desktop_window_tree_host->SetWindowProperty(
      "_WEBOS_LAUNCH_INFO_REASON", "true");
#endif
}

#if defined(USE_OZONE) && defined(OS_WEBOS)
webos::WebOSNativeEventDelegate*
DesktopBrowserFrameAura::GetNativeEventDelegate() const {
  return ChromeBrowserWebOSNativeEventDelegate::Get();
}
#endif

////////////////////////////////////////////////////////////////////////////////
// DesktopBrowserFrameAura, NativeBrowserFrame implementation:

views::Widget::InitParams DesktopBrowserFrameAura::GetWidgetParams() {
  views::Widget::InitParams params;
  params.native_widget = this;
  return params;
}

bool DesktopBrowserFrameAura::UseCustomFrame() const {
  return true;
}

bool DesktopBrowserFrameAura::UsesNativeSystemMenu() const {
  return browser_desktop_window_tree_host_->UsesNativeSystemMenu();
}

int DesktopBrowserFrameAura::GetMinimizeButtonOffset() const {
  return browser_desktop_window_tree_host_->GetMinimizeButtonOffset();
}

bool DesktopBrowserFrameAura::ShouldSaveWindowPlacement() const {
  // The placement can always be stored.
  return true;
}

void DesktopBrowserFrameAura::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  *bounds = GetWidget()->GetRestoredBounds();
  if (IsMaximized())
    *show_state = ui::SHOW_STATE_MAXIMIZED;
  else if (IsMinimized())
    *show_state = ui::SHOW_STATE_MINIMIZED;
  else
    *show_state = ui::SHOW_STATE_NORMAL;
}
