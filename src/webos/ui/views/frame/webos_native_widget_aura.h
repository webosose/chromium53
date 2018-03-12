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

#ifndef WEBOS_UI_VIEWS_FRAME_WEBOS_NATIVE_WIDGET_AURA_H_
#define WEBOS_UI_VIEWS_FRAME_WEBOS_NATIVE_WIDGET_AURA_H_

#include "chrome/browser/ui/views/frame/native_browser_frame.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

class WebOSView;
class WebOSWidget;

namespace webos {
class WebOSNativeEventDelegate;
}

namespace wm {
class VisibilityController;
}

namespace aura {
namespace client {
class ScopedTooltipDisabler;
}
}

////////////////////////////////////////////////////////////////////////////////
// WebOSNativeWidgetAura
//
//  WebOSNativeWidgetAura is a DesktopNativeWidgetAura subclass that provides
//  the window for the WebApplicationManager window.
//
class WebOSNativeWidgetAura : public views::DesktopNativeWidgetAura {
 public:
  WebOSNativeWidgetAura(WebOSWidget* widget,
                        WebOSView* view);

  WebOSView* view() const { return view_; }

  // Overridden from views::DesktopNativeWidgetAura:
  webos::WebOSNativeEventDelegate* GetNativeEventDelegate() const override;

 protected:
  virtual ~WebOSNativeWidgetAura();

  // Overridden from views::DesktopNativeWidgetAura:
  void OnHostClosed() override;
  void OnDesktopWindowTreeHostDestroyed(aura::WindowTreeHost* host) override;
  void InitNativeWidget(const views::Widget::InitParams& params) override;

 private:
  // The WebOSView is our ClientView. This is a pointer to it.
  WebOSView* view_;
  WebOSWidget* widget_;

  // Owned by the RootWindow.
  views::DesktopWindowTreeHost* root_window_tree_host_;

  std::unique_ptr<wm::VisibilityController> visibility_controller_;
  std::unique_ptr<aura::client::ScopedTooltipDisabler> tooltip_disabler_;

  DISALLOW_COPY_AND_ASSIGN(WebOSNativeWidgetAura);
};

#endif  // WEBOS_UI_VIEWS_FRAME_WEBOS_NATIVE_WIDGET_AURA_H_
