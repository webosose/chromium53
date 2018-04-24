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

#ifndef WEBOS_UI_VIEWS_FRAME_WEBOS_VIEW_H_
#define WEBOS_UI_VIEWS_FRAME_WEBOS_VIEW_H_

#include "ui/base/base_window.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"
#include "ui/views/window/client_view.h"
#include "webos/ui/views/frame/webos_widget.h"

class ContentsLayoutManager;
class FullscreenExitBubbleViews;
class TabStrip;

namespace views {
class DesktopWindowTreeHost;
class ExternalFocusTracker;
class WebView;
}

namespace webos {
class WebAppWindow;
class WebAppWindowObserver;
class WebOSNativeEventDelegate;
}

class WebOSView : public ui::BaseWindow,
                  public views::ClientView,
                  public views::WidgetDelegate,
                  public views::WidgetObserver {
 public:
  static const char kViewClassName[];

  explicit WebOSView(webos::WebAppWindow* webapp_window);
  virtual ~WebOSView();

  void Init(content::BrowserContext* browser_context);

  content::BrowserContext* browser_context() { return browser_context_; }
  const content::BrowserContext*
      browser_context() const { return browser_context_; }

  void set_widget(WebOSWidget* widget) { widget_ = widget; }
  WebOSWidget* widget() const { return widget_; }

  void AttachWebContents(content::WebContents* web_contents);
  content::WebContents* GetActiveWebContents() const;
  void RestoreFocus();
  bool IsBrowserTypeNormal() const { return true; }
  bool IsGuestSession() const { return false; }
  bool IsOffTheRecord() const { return false; }
  bool IsRegularOrGuestSession() const { return false; }
  bool ShouldShowAvatar() const { return false; }
  bool GetAccelerator(int cmd_id, ui::Accelerator* accelerator) const {
    return false;
  }
  bool IsTabStripVisible() const { return false; }
  int GetTabStripHeight() const { return 0; }
  gfx::Rect GetToolbarBounds() const { return gfx::Rect(); }
  bool FullscreenChangeProgress() { return false; }
  void ResetOnLanguageChange(content::WebContents* web_contents) {
      NOTIMPLEMENTED();
  }
  FullscreenExitBubbleViews* fullscreen_exit_bubble() {
      NOTIMPLEMENTED();
      return NULL;
  }
  webos::WebAppWindow* GetWebAppWindow() const { return webapp_window_; }
  void SetNativeEventDelegate(webos::WebOSNativeEventDelegate* delegate) {
      native_event_delegate_ = delegate;
  }
#if defined(OS_WEBOS)
  webos::WebOSNativeEventDelegate* GetNativeEventDelegate() const {
      return native_event_delegate_;
  }
#endif

  void SetFullscreen(bool fullscreen);

  void OnDesktopWindowTreeHostCreated(views::DesktopWindowTreeHost* host);
  void OnDesktopWindowTreeHostDestroyed();
  void SetImeEnabled(bool enable);

  // Overridden from BaseWindow:
  bool IsActive() const override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  bool IsFullscreen() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::Rect GetRestoredBounds() const override;
  ui::WindowShowState GetRestoredState() const override;
  gfx::Rect GetBounds() const override;
  void Show() override;
  void Hide() override { NOTIMPLEMENTED(); }
  void ShowInactive() override { NOTIMPLEMENTED(); }
  void Close() override;
  void Activate() override;
  void Deactivate() override { NOTIMPLEMENTED(); }
  void Maximize() override;
  void Minimize() override;
  void Restore() override { NOTIMPLEMENTED(); }
  void SetBounds(const gfx::Rect& bounds) override;
  void FlashFrame(bool flash) override;
  bool IsAlwaysOnTop() const override { return false; }
  void SetAlwaysOnTop(bool always_on_top) override { NOTIMPLEMENTED(); }

  // Overridden from views::WidgetObserver:
  void OnWidgetActivationChanged(views::Widget* widget, bool active) override {
    NOTIMPLEMENTED(); // TODO: Do we need this?
  }
  void OnWidgetDestroyed(views::Widget* widget) override;

  // Overridden from views::WidgetDelegate:
  bool CanResize() const override { return true; }
  bool CanMaximize() const override { return true; }
  bool CanActivate() const override { return true; }
  bool ShouldShowWindowTitle() const override { return false; }
  bool GetSavedWindowPlacement(const views::Widget* widget,
                               gfx::Rect* bounds,
                               ui::WindowShowState* show_state) const override {
    return true;
  }
  views::View* GetContentsView() override { return contents_web_view_; }
  views::ClientView* CreateClientView(views::Widget* widget) override {
    return this;
  }
  void OnWindowBeginUserBoundsChange() override { NOTIMPLEMENTED(); }
  void OnWidgetMove() override { NOTIMPLEMENTED(); }
  views::Widget* GetWidget() override { return View::GetWidget(); }
  const views::Widget* GetWidget() const override { return View::GetWidget(); }
  void GetAccessiblePanes(std::vector<View*>* panes) override {
    NOTIMPLEMENTED();
  }

  // Overridden from views::View:
  const char* GetClassName() const override;
  void Layout() override;
  void PaintChildren(const ui::PaintContext& context) override;
  void ViewHierarchyChanged(
      const ViewHierarchyChangedDetails& details) override;
  void ChildPreferredSizeChanged(View* child) override;
  void GetAccessibleState(ui::AXViewState* state) override;

 private:
  void InitViews();
  void LoadAccelerators() { NOTIMPLEMENTED(); }

  content::BrowserContext* browser_context_;
  webos::WebAppWindow* webapp_window_;
  WebOSWidget* widget_;

  views::WebView* contents_web_view_;
  webos::WebOSNativeEventDelegate* native_event_delegate_;

  bool initialized_;

  DISALLOW_COPY_AND_ASSIGN(WebOSView);
};

#endif  // WEBOS_UI_VIEWS_FRAME_WEBOS_VIEW_H_
