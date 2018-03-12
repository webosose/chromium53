// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_DESKTOP_ROOT_WINDOW_HOST_OZONE_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_DESKTOP_ROOT_WINDOW_HOST_OZONE_H_

#include "ozone/ui/desktop_aura/desktop_window_tree_host_ozone.h"
#include "chrome/browser/ui/views/frame/browser_desktop_window_tree_host.h"

class BrowserFrame;
class BrowserView;

namespace views {
class DesktopNativeWidgetAura;
}

class BrowserDesktopWindowTreeHostOzone
    : public BrowserDesktopWindowTreeHost,
      public views::DesktopWindowTreeHostOzone {
 public:
  BrowserDesktopWindowTreeHostOzone(
      views::internal::NativeWidgetDelegate* native_widget_delegate,
      views::DesktopNativeWidgetAura* desktop_native_widget_aura,
      BrowserView* browser_view,
      BrowserFrame* browser_frame);
  ~BrowserDesktopWindowTreeHostOzone() override;

 private:
  // Overridden from BrowserDesktopWindowTreeHost:
  DesktopWindowTreeHost* AsDesktopWindowTreeHost() override;
  int GetMinimizeButtonOffset() const override;
  bool UsesNativeSystemMenu() const override;

  // Overridden from views::DesktopWindowTreeHostOzone:
  void Init(aura::Window* content_window,
            const views::Widget::InitParams& params) override;
  void CloseNow() override;

  DISALLOW_COPY_AND_ASSIGN(BrowserDesktopWindowTreeHostOzone);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_BROWSER_DESKTOP_ROOT_WINDOW_HOST_OZONE_H_
