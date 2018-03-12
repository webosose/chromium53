// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/npapi/webplugin_delegate_impl.h"

#include "content/child/npapi/plugin_instance.h"
#include "content/common/cursors/webcursor.h"

using blink::WebInputEvent;

namespace content {

#if defined(WEBOS_PLUGIN_SUPPORT)
WebPluginDelegateImpl::WebPluginDelegateImpl(WebPlugin* plugin,
                                             PluginInstance* instance) :
    plugin_(plugin),
    instance_(instance),
    quirks_(0),
    handle_event_depth_(0),
    first_set_window_call_(true),
    creation_succeeded_(false) {
  memset(&window_, 0, sizeof(window_));
}
#else
WebPluginDelegateImpl::WebPluginDelegateImpl(WebPlugin* plugin,
                                             PluginInstance* instance) {
}
#endif

WebPluginDelegateImpl::~WebPluginDelegateImpl() {
#if defined(WEBOS_PLUGIN_SUPPORT)
  DestroyInstance();
#endif
}

bool WebPluginDelegateImpl::PlatformInitialize() {
  return true;
}

void WebPluginDelegateImpl::PlatformDestroyInstance() {
  // Nothing to do here.
}

void WebPluginDelegateImpl::Paint(SkCanvas* canvas, const gfx::Rect& rect) {
}

void WebPluginDelegateImpl::WindowlessUpdateGeometry(
    const gfx::Rect& window_rect,
    const gfx::Rect& clip_rect) {
#if defined(WEBOS_PLUGIN_SUPPORT)
  clip_rect_ = clip_rect;
  window_rect_ = window_rect;

  if (!instance())
    return;

  if (window_rect_.IsEmpty())  // wait for geometry to be set.
    return;

  // Mozilla docs say that this window param is not used for windowless
  // plugins; rather, the window is passed during the GraphicsExpose event.
  DCHECK_EQ(window_.window, static_cast<void*>(NULL));

  window_.clipRect.top = clip_rect_.y() + window_rect_.y();
  window_.clipRect.left = clip_rect_.x() + window_rect_.x();
  window_.clipRect.bottom =
      clip_rect_.y() + clip_rect_.height() + window_rect_.y();
  window_.clipRect.right =
      clip_rect_.x() + clip_rect_.width() + window_rect_.x();
  window_.height = window_rect_.height();
  window_.width = window_rect_.width();
  window_.x = window_rect_.x();
  window_.y = window_rect_.y();
  window_.type = NPWindowTypeDrawable;

  NPError err = instance()->NPP_SetWindow(&window_);
  DCHECK(err == NPERR_NO_ERROR);
#endif
}

void WebPluginDelegateImpl::WindowlessPaint(gfx::NativeDrawingContext context,
                                            const gfx::Rect& damage_rect) {
}

bool WebPluginDelegateImpl::PlatformSetPluginHasFocus(bool focused) {
  return true;
}

bool WebPluginDelegateImpl::PlatformHandleInputEvent(
    const WebInputEvent& event, WebCursor::CursorInfo* cursor_info) {
  return false;
}

}  // namespace content
