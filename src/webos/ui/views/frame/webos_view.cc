// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/ui/views/frame/webos_view.h"

#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/web_contents.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/background.h"
#include "ui/views/focus/external_focus_tracker.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/layout/layout_manager.h"
#include "webos/webapp_window.h"

using content::NativeWebKeyboardEvent;
using content::WebContents;

namespace {
const char* const kWebOSViewKey = "__WebOS_VIEW__";
}  // namespace

// static
const char WebOSView::kViewClassName[] = "WebOSView";

class WebOSViewLayout : public views::LayoutManager {
 public:
  WebOSViewLayout(views::View* contents_container);
  virtual ~WebOSViewLayout();

  // views::LayoutManager overrides:
  void Layout(views::View* host) override;
  gfx::Size GetPreferredSize(const views::View* host) const override;
  void Installed(views::View* host) override;
  void Uninstalled(views::View* host) override;

 private:
  views::View* contents_container_;

  views::View* host_;

  DISALLOW_COPY_AND_ASSIGN(WebOSViewLayout);
};

WebOSViewLayout::WebOSViewLayout(views::View* contents_container)
    : contents_container_(contents_container),
      host_(NULL) {
}

WebOSViewLayout::~WebOSViewLayout() {
}

void WebOSViewLayout::Layout(views::View* webos_view) {
  DCHECK(host_ == webos_view);

  contents_container_->SetBoundsRect(webos_view->bounds());
  contents_container_->SchedulePaint();
}

gfx::Size WebOSViewLayout::GetPreferredSize(
    const views::View* host) const {
  return gfx::Size();
}

void WebOSViewLayout::Installed(views::View* host) {
  DCHECK(!host_);
  host_ = host;
}

void WebOSViewLayout::Uninstalled(views::View* host) {
  DCHECK(host_ == host);
  host_ = NULL;
}


///////////////////////////////////////////////////////////////////////////////
// WebOSView, public:

WebOSView::WebOSView(webos::WebAppWindow* webapp_window)
    : views::ClientView(NULL, NULL),
      webapp_window_(webapp_window),
      widget_(NULL),
      browser_context_(NULL),
      contents_web_view_(NULL),
      initialized_(false) {
}

WebOSView::~WebOSView() {
}

void WebOSView::Init(content::BrowserContext* browser_context) {
  browser_context_ = browser_context;
}

WebContents* WebOSView::GetActiveWebContents() const {
  return contents_web_view_ ? contents_web_view_->GetWebContents() : NULL;
}

void WebOSView::RestoreFocus() {
  WebContents* selected_web_contents = GetActiveWebContents();
  if (selected_web_contents)
    selected_web_contents->RestoreFocus();
}

void WebOSView::SetFullscreen(bool fullscreen) {
  if (widget_)
    widget_->SetFullscreen(fullscreen);
}

void WebOSView::OnDesktopWindowTreeHostCreated(views::DesktopWindowTreeHost* host) {
  if (webapp_window_)
    webapp_window_->SetHost(host);
}

void WebOSView::OnDesktopWindowTreeHostDestroyed() {
  if (webapp_window_)
    webapp_window_->SetHost(NULL);
}

void WebOSView::SetImeEnabled(bool enable) {
  ui::InputMethod* input_method = GetWidget()->GetInputMethod();

  if (!input_method)
    return;

  input_method->SetImeEnabled(enable);
}

///////////////////////////////////////////////////////////////////////////////
// WebOSView, BrowserWindow implementation:

bool WebOSView::IsActive() const {
  return widget_?widget_->IsActive():false;
}

bool WebOSView::IsMaximized() const {
  if (widget_)
    return widget_->IsMaximized();

  return false;
}

bool WebOSView::IsMinimized() const {
  if (widget_)
    return widget_->IsMinimized();

  return false;
}

bool WebOSView::IsFullscreen() const {
  if (widget_)
    return widget_->IsFullscreen();

  return false;
}

gfx::NativeWindow WebOSView::GetNativeWindow() const {
  if (!GetWidget())
    return NULL;
  return GetWidget()->GetTopLevelWidget()->GetNativeWindow();
}

gfx::Rect WebOSView::GetRestoredBounds() const {
  gfx::Rect bounds;
  if (widget_)
    bounds = widget_->GetRestoredBounds();
  else if (GetWidget())
    bounds = GetWidget()->GetRestoredBounds();

  return bounds;
}

ui::WindowShowState WebOSView::GetRestoredState() const {
  return ui::SHOW_STATE_DEFAULT;
}

gfx::Rect WebOSView::GetBounds() const {
  return widget_?widget_->GetWindowBoundsInScreen():gfx::Rect();
}

void WebOSView::Show() {
  RestoreFocus();
  if (widget_)
    widget_->Show();
}

void WebOSView::Close() {
  // This function is called by the destructor of WebAppWindow. So, WebOSView
  // should not access |webapp_window_|.
  webapp_window_ = NULL;

  if (widget_)
    widget_->Close();
}

void WebOSView::Activate() {
  if (widget_)
    widget_->Activate();
}

void WebOSView::Maximize() {
  if (widget_)
    widget_->Maximize();
}

void WebOSView::Minimize() {
  if (widget_)
    widget_->Minimize();
}

void WebOSView::SetBounds(const gfx::Rect& bounds) {
  GetWidget()->SetBounds(bounds);
}

void WebOSView::FlashFrame(bool flash) {
  if (widget_)
    widget_->FlashFrame(flash);
}

// FIXME(jb) no header...
#if 0
void WebOSView::WebContentsFocused(WebContents* contents) {
  if (contents_web_view_->GetWebContents() == contents)
    contents_web_view_->OnWebContentsFocused(contents);
  else
    devtools_web_view_->OnWebContentsFocused(contents);
}
#endif

void WebOSView::AttachWebContents(content::WebContents* web_contents) {
  // If |contents_container_| already has the correct WebContents, we can save
  // some work.  This also prevents extra events from being reported by the
  // Visibility API under Windows, as ChangeWebContents will briefly hide
  // the WebContents window.
  bool change_tab_contents = contents_web_view_->web_contents() != web_contents;

  // When we toggle the NTP floating bookmarks bar and/or the info bar,
  // we don't want any WebContents to be attached, so that we
  // avoid an unnecessary resize and re-layout of a WebContents.
  if (change_tab_contents) {
    contents_web_view_->SetWebContents(NULL);
  }

  if (change_tab_contents) {
    contents_web_view_->SetWebContents(web_contents);
  }

  if (GetWidget()->IsActive() && GetWidget()->IsVisible()) {
    // We only restore focus if our window is visible, to avoid invoking blur
    // handlers when we are eventually shown.
    web_contents->RestoreFocus();
  }
}

///////////////////////////////////////////////////////////////////////////////
// WebOSView, views::WidgetObserver implementation:

void WebOSView::OnWidgetDestroyed(views::Widget* widget) {
  if (webapp_window_) {
    // WebOSView is destroyed before WebAppWindow is destroyed.
    webapp_window_->SetWebOSView(NULL);
    webapp_window_ = NULL;
  }
}

///////////////////////////////////////////////////////////////////////////////
// WebOSView, views::View implementation:

const char* WebOSView::GetClassName() const {
  return kViewClassName;
}

void WebOSView::Layout() {
  if (!initialized_)
    return;

  views::View::Layout();
}

void WebOSView::PaintChildren(const ui::PaintContext& context) {
  // Paint the last so that it may paint its overlapping tabs.
  for (int i = 0; i < child_count(); ++i) {
    View* child = child_at(i);
    if (!child->layer())
      child->Paint(context);
  }
}

void WebOSView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (!initialized_ && details.child == this && GetWidget()) {
    InitViews();
    initialized_ = true;
  }
}

void WebOSView::ChildPreferredSizeChanged(View* child) {
  Layout();
}

void WebOSView::GetAccessibleState(ui::AXViewState* state) {
  state->role = ui::AX_ROLE_CLIENT;
}

///////////////////////////////////////////////////////////////////////////////
// WebOSView, private:
void WebOSView::InitViews() {
  GetWidget()->AddObserver(this);

  // Stow a pointer to this object onto the window handle so that we can get at
  // it later when all we have is a native view.
  GetWidget()->SetNativeWindowProperty(kWebOSViewKey, this);

  LoadAccelerators();

  contents_web_view_ = new views::WebView(browser_context_);
  contents_web_view_->SetEmbedFullscreenWidgetMode(true);

  WebOSViewLayout* webos_view_layout = new WebOSViewLayout(contents_web_view_);
  SetLayoutManager(webos_view_layout);

  set_background(
      views::Background::CreateSolidBackground(SK_ColorTRANSPARENT));
  AddChildView(contents_web_view_);
  set_contents_view(contents_web_view_);
}
