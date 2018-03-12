// Copyright (c) 2014 LG Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/npapi/webplugin_delegate_impl.h"

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/child/npapi/plugin_instance.h"
#include "content/child/npapi/plugin_lib.h"
#include "content/child/npapi/webplugin.h"
#include "content/child/npapi/webplugin_accelerated_surface_webos.h"
#include "content/common/cursors/webcursor.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/content_switches.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

using blink::WebInputEvent;
using blink::WebKeyboardEvent;
using blink::WebMouseEvent;
using blink::WebMouseWheelEvent;

namespace content {

WebPluginDelegateImpl::WebPluginDelegateImpl(WebPlugin* plugin,
                                             PluginInstance* instance)
    : plugin_(plugin),
      instance_(instance),
      quirks_(0),
      handle_event_depth_(0),
      first_set_window_call_(true),
      plugin_has_focus_(false),
      has_webkit_focus_(false),
      containing_view_has_focus_(true),
      creation_succeeded_(false),
      surface_(NULL) {
  memset(&window_, 0, sizeof(window_));
}

WebPluginDelegateImpl::~WebPluginDelegateImpl() {
  DestroyInstance();
}

bool WebPluginDelegateImpl::PlatformInitialize() {
  // WebOS supports only windowless NPAPI plugins, so the
  // PLUGIN_QUIRK_DONT_SET_NULL_WINDOW_HANDLE_ON_DESTROY needs
  // to be set for the NPAPI plugins and the window doesn't get
  // NULL window handle(NPWindow.window) when destroying the NPAPI plugin.
  quirks_ |= PLUGIN_QUIRK_DONT_SET_NULL_WINDOW_HANDLE_ON_DESTROY;

  bool in_accelerated_plugin_rendering_blacklist = false;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAcceleratedPluginRenderingBlacklist)) {
    const std::string blacklist =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kAcceleratedPluginRenderingBlacklist);
    base::string16 name = instance_->plugin_lib()->plugin_info().name;
    for (const std::string& name_from_list : base::SplitString(
             blacklist, ",;", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL)) {
      if (name.find(base::ASCIIToUTF16(name_from_list)) != std::wstring::npos) {
        in_accelerated_plugin_rendering_blacklist = true;
        break;
      }
    }
  }

  bool is_flash =
      instance_->mime_type() == kFlashPluginSwfMimeType ? true : false;
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableNpapiVisualPlugins) &&
      is_flash) {
    return false;
  }

  if (!in_accelerated_plugin_rendering_blacklist &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableAcceleratedPluginRendering)) {
    if (is_flash) {
      surface_ = plugin_->GetAcceleratedSurface(gl::PreferDiscreteGpu);
      if (surface_) {
        plugin_->AcceleratedPluginEnabledRendering(false);
        return true;
      }
    } else {
      plugin_->AcceleratedPluginEnabledRendering(true);
      return true;
    }
  }

  if (is_flash)
    plugin_->PluginEnabledRendering();

  return true;
}

void WebPluginDelegateImpl::PlatformDestroyInstance() {
  // Nothing to do here.
}

void WebPluginDelegateImpl::Paint(SkCanvas* canvas, const gfx::Rect& rect) {
  NPEvent np_event;
  memset(&np_event, 0, sizeof(np_event));
  np_event.eventType = NPWebosPaint;
  np_event.data.paint.graphicsContext = static_cast<void*>(canvas);
  np_event.data.paint.x = rect.x();
  np_event.data.paint.y = rect.y();
  np_event.data.paint.width = rect.width();
  np_event.data.paint.height = rect.height();

  instance()->NPP_HandleEvent(&np_event);
}

void WebPluginDelegateImpl::WindowlessUpdateGeometry(
    const gfx::Rect& window_rect,
    const gfx::Rect& clip_rect) {
  clip_rect_ = clip_rect;
  window_rect_ = window_rect;

  if (!instance())
    return;

  if (window_rect_.IsEmpty())  // wait for geometry to be set.
    return;

  if (surface_)
    surface_->SetSize(window_rect_.size());

  window_.clipRect.top = clip_rect_.y();
  window_.clipRect.left = clip_rect_.x();
  window_.clipRect.bottom = clip_rect_.y() + clip_rect_.height();
  window_.clipRect.right = clip_rect_.x() + clip_rect_.width();
  window_.height = window_rect_.height();
  window_.width = window_rect_.width();
  window_.x = window_rect_.x();
  window_.y = window_rect_.y();
  window_.type = NPWindowTypeDrawable;

  NPError err = instance()->NPP_SetWindow(&window_);
  DCHECK(err == NPERR_NO_ERROR);
}

void WebPluginDelegateImpl::WindowlessPaint(gfx::NativeDrawingContext context,
                                            const gfx::Rect& damage_rect) {}

bool WebPluginDelegateImpl::PlatformSetPluginHasFocus(bool focused) {
  NPEvent focus_event;
  memset(&focus_event, 0, sizeof(focus_event));
  focus_event.eventType = NPWebosFocusChanged;
  focus_event.data.focusChanged.in = focused;

  instance()->NPP_HandleEvent(&focus_event);
  return true;
}

static bool NPEventFromWebMouseEvent(const WebMouseEvent& event,
                                     NPEvent* np_event) {
  switch (event.type) {
    case WebInputEvent::MouseDown:
      np_event->eventType = NPWebosMouseDown;
      break;
    case WebInputEvent::MouseUp:
      np_event->eventType = NPWebosMouseUp;
      break;
    case WebInputEvent::MouseMove:
      np_event->eventType = NPWebosMouseMove;
      break;
    case WebInputEvent::MouseEnter:
      np_event->eventType = NPWebosMouseEnter;
      return true;
    case WebInputEvent::MouseLeave:
      np_event->eventType = NPWebosMouseLeave;
      return true;
    default:
      NOTREACHED();
      return false;
  }

  np_event->data.mouse.x = event.x;
  np_event->data.mouse.y = event.y;
  np_event->data.mouse.button = static_cast<NPWebosMouseButton>(event.button);

  return true;
}

static bool NPEventFromWebMouseWheelEvent(const WebMouseWheelEvent& event,
                                          NPEvent* np_event) {
  np_event->eventType =
      (event.deltaY < 0.0f) ? NPWebosWheelDown : NPWebosWheelUp;

  np_event->data.wheel.x = event.x;
  np_event->data.wheel.y = event.y;
  np_event->data.wheel.delta = event.deltaY;
  np_event->data.wheel.ticks = event.wheelTicksY;

  return true;
}

enum NpEventModifiers {
  ShiftNpEventModifier = 0x02000000,
  ControlNpEventModifier = 0x04000000,
  AltNpEventModifier = 0x08000000,
  MetaNpEventModifier = 0x10000000
};

static unsigned modifiersForEvent(const WebInputEvent& event) {
  unsigned result = 0;

  if (event.modifiers & WebInputEvent::ShiftKey)
    result |= ShiftNpEventModifier;
  if (event.modifiers & WebInputEvent::ControlKey)
    result |= ControlNpEventModifier;
  if (event.modifiers & WebInputEvent::AltKey)
    result |= AltNpEventModifier;
  if (event.modifiers & WebInputEvent::MetaKey)
    result |= MetaNpEventModifier;

  return result;
}

static bool NPEventFromWebKeyboardEvent(const WebKeyboardEvent& event,
                                        NPEvent* np_event) {
  switch (event.type) {
    case WebInputEvent::Char:
      np_event->eventType = NPWebosChar;
      break;
    case WebInputEvent::KeyUp:
      np_event->eventType = NPWebosKeyUp;
      break;
    default:
      NOTREACHED();
      return false;
  }

  np_event->data.key.chr = event.unmodifiedText[0];
  np_event->data.key.code = event.nativeKeyCode;
  np_event->data.key.modifiers = modifiersForEvent(event);

  return true;
}

static bool NPEventFromWebInputEvent(const WebInputEvent& event,
                                     NPEvent* np_event) {
  switch (event.type) {
    case WebInputEvent::MouseDown:
    case WebInputEvent::MouseUp:
    case WebInputEvent::MouseMove:
    case WebInputEvent::MouseEnter:
    case WebInputEvent::MouseLeave:
      if (event.size < sizeof(WebMouseEvent)) {
        NOTREACHED();
        return false;
      }
      return NPEventFromWebMouseEvent(
          *static_cast<const WebMouseEvent*>(&event), np_event);

    case WebInputEvent::MouseWheel:
      if (event.size < sizeof(WebMouseWheelEvent)) {
        NOTREACHED();
        return false;
      }
      return NPEventFromWebMouseWheelEvent(
          *static_cast<const WebMouseWheelEvent*>(&event), np_event);

    case WebInputEvent::Char:
    case WebInputEvent::KeyUp:
      if (event.size < sizeof(WebKeyboardEvent)) {
        NOTREACHED();
        return false;
      }
      return NPEventFromWebKeyboardEvent(
          *static_cast<const WebKeyboardEvent*>(&event), np_event);

    default:
      return false;
  }
}

bool WebPluginDelegateImpl::PlatformHandleInputEvent(
    const WebInputEvent& event,
    WebCursor::CursorInfo* cursor_info) {
  DCHECK(cursor_info != NULL);

  NPEvent np_event;
  memset(&np_event, 0, sizeof(np_event));

  if (!NPEventFromWebInputEvent(event, &np_event))
    return false;

  instance()->NPP_HandleEvent(&np_event);
  return true;
}

void WebPluginDelegateImpl::Suspend() {
  if (surface_)
    surface_->Suspend();

  NPEvent np_event;
  memset(&np_event, 0, sizeof(np_event));
  np_event.eventType = NPWebosPause;

  instance()->NPP_HandleEvent(&np_event);
}

void WebPluginDelegateImpl::Resume() {
  NPEvent np_event;
  memset(&np_event, 0, sizeof(np_event));
  np_event.eventType = NPWebosResume;

  instance()->NPP_HandleEvent(&np_event);

  if (surface_)
    surface_->Resume();
}

void WebPluginDelegateImpl::SetDeviceInfo(const gfx::Size& device_viewport_size,
                                          const gfx::Size& view_size) {
  NPEvent np_event;
  memset(&np_event, 0, sizeof(np_event));
  np_event.eventType = NPWebosDeviceInfo;
  np_event.data.device.deviceWidth = device_viewport_size.width();
  np_event.data.device.deviceHeight = device_viewport_size.height();
  np_event.data.device.displayWidth = view_size.width();
  np_event.data.device.displayHeight = view_size.height();

  instance()->NPP_HandleEvent(&np_event);
}

}  // namespace content
