// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/renderer/webos_render_view_observer.h"

#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebFrameWidget.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "webos/common/webos_view_messages.h"

namespace webos {

WebOSRenderViewObserver::WebOSRenderViewObserver(
    content::RenderView* render_view)
    : content::RenderViewObserver(render_view) {
}

bool WebOSRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebOSRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(WebOSViewMsg_SetVisibilityState, OnSetVisibilityState)
    IPC_MESSAGE_HANDLER(WebOSViewMsg_SetBackgroundColor, OnSetBackgroundColor)
    IPC_MESSAGE_HANDLER(WebOSViewMsg_SetViewportSize, OnSetViewportSize)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebOSRenderViewObserver::OnSetVisibilityState(
    blink::WebPageVisibilityState visibilityState) {
  render_view()->GetWebView()->setVisibilityState(
      visibilityState,
      visibilityState ==
          blink::WebPageVisibilityState::WebPageVisibilityStateLaunching);
}

void WebOSRenderViewObserver::OnSetBackgroundColor(SkColor color) {
  render_view()->GetWebFrameWidget()->
      setBaseBackgroundColor(static_cast<blink::WebColor>(color));
}

void WebOSRenderViewObserver::OnSetViewportSize(int width, int height) {
  render_view()->GetWebView()->mainFrame()->setViewportSize(
      blink::WebSize(width, height));
}

void WebOSRenderViewObserver::OnDestruct() {
  delete this;
}

void WebOSRenderViewObserver::DidClearWindowObject(blink::WebLocalFrame* frame) {
  Send(new WebOSViewMsgHost_DidClearWindowObject(routing_id()));
}

} // namespace webos
