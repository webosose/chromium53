// Copyright 2014 Intel Corporation. All rights reserved.
// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/platform/ozone_wayland_window.h"

#include "base/bind.h"
#include "ozone/platform/messages.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"
#include "ozone/platform/window_manager_wayland.h"
#include "ui/base/cursor/ozone/bitmap_cursor_factory_ozone.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/ozone/events_ozone.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/platform_window/platform_window_delegate.h"

#if defined(OS_WEBOS)
#include "base/files/file.h"
#include "base/memory/ref_counted_memory.h"
#include "base/threading/thread_restrictions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/ozone/public/cursor_factory_ozone.h"

namespace webos {

scoped_refptr<base::RefCountedBytes> ReadFileData(const base::FilePath& path) {
  if (path.empty())
    return 0;

  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  if (!file.IsValid())
    return 0;

  int64_t length = file.GetLength();
  if (length > 0 && length < INT_MAX) {
    int size = static_cast<int>(length);
    std::vector<unsigned char> raw_data;
    raw_data.resize(size);
    char* data = reinterpret_cast<char*>(&(raw_data.front()));
    if (file.ReadAtCurrentPos(data, size) == length)
      return base::RefCountedBytes::TakeVector(&raw_data);
  }
  return 0;
}

void CreateBitmapFromPng(
    webos::CustomCursorType type,
    const std::string& path,
    int hotspot_x,
    int hotspot_y,
    const base::Callback<void(webos::CustomCursorType /*type*/,
                              SkBitmap* /*cursor_image*/,
                              int /*hotspot_x*/,
                              int /*hotspot_y*/)> callback) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  scoped_refptr<base::RefCountedBytes> memory(
      ReadFileData(base::FilePath(path)));
  if (!memory.get()) {
    LOG(INFO) << "Unable to read file path = " << path;
    return;
  }

  SkBitmap* bitmap = new SkBitmap();
  if (!gfx::PNGCodec::Decode(memory->front(), memory->size(), bitmap)) {
    LOG(INFO) << "Unable to decode image path = " << path;
    delete bitmap;
    return;
  }

  base::MessageLoopForUI::current()->PostTask(
      FROM_HERE, base::Bind(callback, type, bitmap, hotspot_x, hotspot_y));
}

}  // namespace webos
#endif

namespace ui {

OzoneWaylandWindow::OzoneWaylandWindow(PlatformWindowDelegate* delegate,
                                       OzoneGpuPlatformSupportHost* sender,
                                       WindowManagerWayland* window_manager,
                                       const gfx::Rect& bounds)
    :
#if defined(OS_WEBOS)
      custom_cursor_type_(webos::CustomCursorType::CUSTOM_CURSOR_NOT_USE),
      weak_factory_(this),
#endif
      delegate_(delegate),
      sender_(sender),
      window_manager_(window_manager),
      transparent_(false),
      bounds_(bounds),
      parent_(0),
      type_(ui::WINDOWFRAMELESS),
      state_(UNINITIALIZED),
      region_(NULL),
      init_window_(false) {
  static int opaque_handle = 0;
  opaque_handle++;
  handle_ = opaque_handle;
  delegate_->OnAcceleratedWidgetAvailable(opaque_handle, 1.0);

  char* env;
  if ((env = getenv("OZONE_WAYLAND_IVI_SURFACE_ID")))
    surface_id_ = atoi(env);
  else
    surface_id_ = getpid();
  PlatformEventSource::GetInstance()->AddPlatformEventDispatcher(this);
  sender_->AddChannelObserver(this);
  window_manager_->OnRootWindowCreated(this);
}

OzoneWaylandWindow::~OzoneWaylandWindow() {
  sender_->RemoveChannelObserver(this);
  PlatformEventSource::GetInstance()->RemovePlatformEventDispatcher(this);
  if (region_)
    delete region_;
}

void OzoneWaylandWindow::InitNativeWidget() {
  if (init_window_)
    sender_->Send(new WaylandDisplay_InitWindow(handle_, parent_, bounds_.x(),
                                                bounds_.y(), type_, surface_id_));

  if (state_)
    sender_->Send(new WaylandDisplay_State(handle_, state_));

  if (title_.length())
    sender_->Send(new WaylandDisplay_Title(handle_, title_));

#if defined(OS_WEBOS)
  if (!pending_window_properties_.empty()) {
    for (auto property : pending_window_properties_) {
      std::string name = property.first;
      std::string value = property.second;
      sender_->Send(new WaylandDisplay_Property(handle_, name, value));
    }
  }
#endif

  AddRegion();
}

void OzoneWaylandWindow::InitPlatformWindow(
    PlatformWindowType type, gfx::AcceleratedWidget parent_window) {
  switch (type) {
    case PLATFORM_WINDOW_TYPE_POPUP:
    case PLATFORM_WINDOW_TYPE_MENU: {
      parent_ = parent_window;
      if (!parent_ && window_manager_->GetActiveWindow())
        parent_ = window_manager_->GetActiveWindow()->GetHandle();
      type_ = ui::POPUP;
      ValidateBounds();
      break;
    }
    case PLATFORM_WINDOW_TYPE_TOOLTIP: {
      parent_ = parent_window;
      if (!parent_ && window_manager_->GetActiveWindow())
        parent_ = window_manager_->GetActiveWindow()->GetHandle();
      type_ = ui::TOOLTIP;
      bounds_.set_origin(gfx::Point(0, 0));
      break;
    }
    case PLATFORM_WINDOW_TYPE_BUBBLE:
    case PLATFORM_WINDOW_TYPE_WINDOW:
      parent_ = 0;
      type_ = ui::WINDOW;
      break;
    case PLATFORM_WINDOW_TYPE_WINDOW_FRAMELESS:
      NOTIMPLEMENTED();
      break;
    default:
      break;
  }

  init_window_ = true;

  if (!sender_->IsConnected())
    return;

  sender_->Send(new WaylandDisplay_InitWindow(handle_, parent_, bounds_.x(),
                                              bounds_.y(), type_, surface_id_));
}

void OzoneWaylandWindow::SetTitle(const base::string16& title) {
  title_ = title;
  if (!sender_->IsConnected())
    return;

  sender_->Send(new WaylandDisplay_Title(handle_, title_));
}

void OzoneWaylandWindow::SetSurfaceId(int surface_id) {
  surface_id_ = surface_id;
}

void OzoneWaylandWindow::SetWindowShape(const SkPath& path) {
  ResetRegion();
  if (transparent_)
    return;

  region_ = new SkRegion();
  SkRegion clip_region;
  clip_region.setRect(0, 0, bounds_.width(), bounds_.height());
  region_->setPath(path, clip_region);
  AddRegion();
}

void OzoneWaylandWindow::SetOpacity(unsigned char opacity) {
  if (opacity == 255) {
    if (transparent_) {
      AddRegion();
      transparent_ = false;
    }
  } else if (!transparent_) {
    ResetRegion();
    transparent_ = true;
  }
}

void OzoneWaylandWindow::RequestDragData(const std::string& mime_type) {
  sender_->Send(new WaylandDisplay_RequestDragData(mime_type));
}

void OzoneWaylandWindow::RequestSelectionData(const std::string& mime_type) {
  sender_->Send(new WaylandDisplay_RequestSelectionData(mime_type));
}

void OzoneWaylandWindow::DragWillBeAccepted(uint32_t serial,
                                            const std::string& mime_type) {
  sender_->Send(new WaylandDisplay_DragWillBeAccepted(serial, mime_type));
}

void OzoneWaylandWindow::DragWillBeRejected(uint32_t serial) {
  sender_->Send(new WaylandDisplay_DragWillBeRejected(serial));
}

gfx::Rect OzoneWaylandWindow::GetBounds() {
  return bounds_;
}

void OzoneWaylandWindow::SetBounds(const gfx::Rect& bounds) {
  if (bounds_ == bounds) {
    return;
  }

  int original_x = bounds_.x();
  int original_y = bounds_.y();
  bounds_ = bounds;
  if (type_ == ui::TOOLTIP)
    ValidateBounds();

  if ((original_x != bounds_.x()) || (original_y  != bounds_.y())) {
    sender_->Send(new WaylandDisplay_MoveWindow(handle_, parent_,
                                                type_, bounds_));
  }

  delegate_->OnBoundsChanged(bounds_);
}

void OzoneWaylandWindow::Show() {
  state_ = ui::SHOW;
  SendWidgetState();
}

void OzoneWaylandWindow::Hide() {
  state_ = ui::HIDE;

  if (type_ == ui::TOOLTIP)
    delegate_->OnCloseRequest();
  else
    SendWidgetState();
}

void OzoneWaylandWindow::Close() {
  if (type_ != ui::TOOLTIP)
    window_manager_->OnRootWindowClosed(this);
}

void OzoneWaylandWindow::SetCapture() {
  window_manager_->GrabEvents(handle_);
}

void OzoneWaylandWindow::ReleaseCapture() {
  window_manager_->UngrabEvents(handle_);
}

void OzoneWaylandWindow::ToggleFullscreen() {
  display::Screen *screen = display::Screen::GetScreen();
  if (!screen)
    NOTREACHED() << "Unable to retrieve valid display::Screen";

  SetBounds(screen->GetPrimaryDisplay().bounds());
  state_ = ui::FULLSCREEN;
  SendWidgetState();
}

void OzoneWaylandWindow::Maximize() {
  state_ = ui::MAXIMIZED;
  SendWidgetState();
}

void OzoneWaylandWindow::Minimize() {
  SetBounds(gfx::Rect());
  state_ = ui::MINIMIZED;
  SendWidgetState();
}

void OzoneWaylandWindow::Restore() {
  window_manager_->Restore(this);
  state_ = ui::RESTORE;
  SendWidgetState();
}

void OzoneWaylandWindow::SetCursor(PlatformCursor cursor) {
  if (window_manager_->GetPlatformCursor() == cursor)
    return;

  scoped_refptr<BitmapCursorOzone> bitmap =
      BitmapCursorFactoryOzone::GetBitmapCursor(cursor);
  bitmap_ = bitmap;
  window_manager_->SetPlatformCursor(cursor);
  if (!sender_->IsConnected())
    return;

  SetCursor();
}

void OzoneWaylandWindow::MoveCursorTo(const gfx::Point& location) {
  sender_->Send(new WaylandDisplay_MoveCursor(location));
}

void OzoneWaylandWindow::ConfineCursorToBounds(const gfx::Rect& bounds) {
}

#if defined(OS_WEBOS)
void OzoneWaylandWindow::WebOSXInputActivate(const std::string& type) {
  sender_->Send(new WaylandDisplay_WebOSXInputActivate(type));
}

void OzoneWaylandWindow::WebOSXInputDeactivate() {
  sender_->Send(new WaylandDisplay_WebOSXInputDeactivate());
}

void OzoneWaylandWindow::WebOSXInputInvokeAction(
    uint32_t keysym,
    ui::WebOSXInputSpecialKeySymbolType symbol_type,
    ui::WebOSXInputEventType event_type) {
  sender_->Send(new WaylandDisplay_WebOSXInputInvokeAction(keysym, symbol_type,
                                                           event_type));
}

void OzoneWaylandWindow::SetCustomCursor(webos::CustomCursorType type,
                                         const std::string& path,
                                         int hotspot_x,
                                         int hotspot_y) {
  custom_cursor_type_ = type;

  if (type == webos::CustomCursorType::CUSTOM_CURSOR_PATH) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&webos::CreateBitmapFromPng, type, path, hotspot_x,
                   hotspot_y,
                   base::Bind(&OzoneWaylandWindow::SetCustomCursorFromBitmap,
                              weak_factory_.GetWeakPtr())));
  } else if (type == webos::CustomCursorType::CUSTOM_CURSOR_BLANK) {
    // BLANK : Disable cursor(hiding cursor)
    sender_->Send(new WaylandDisplay_CursorSet(std::vector<SkBitmap>(),
                                               gfx::Point(254, 254)));
    bitmap_ = NULL;
  } else {
    // NOT_USE : Restore cursor(wayland cursor or IM's cursor)
    sender_->Send(new WaylandDisplay_CursorSet(std::vector<SkBitmap>(),
                                               gfx::Point(255, 255)));
    bitmap_ = NULL;
  }
}

void OzoneWaylandWindow::ResetCustomCursor() {
  if (bitmap_) {
    SetCursor();
    return;
  }

  SetCustomCursor(custom_cursor_type_, "", 0, 0);
}

void OzoneWaylandWindow::SetCustomCursorFromBitmap(webos::CustomCursorType type,
                                                   SkBitmap* cursor_image,
                                                   int hotspot_x,
                                                   int hotspot_y) {
  if (!cursor_image) {
    SetCustomCursor(webos::CustomCursorType::CUSTOM_CURSOR_NOT_USE, "", 0, 0);
    return;
  }

  SetCursor(CursorFactoryOzone::GetInstance()->CreateImageCursor(
    *cursor_image, gfx::Point(hotspot_x, hotspot_y)));
  delete cursor_image;
}

void OzoneWaylandWindow::SetKeyMask(webos::WebOSKeyMask key_mask, bool value) {
  sender_->Send(new WaylandDisplay_KeyMask(handle_, static_cast<uint32_t>(key_mask), value));
}

void OzoneWaylandWindow::SetWindowProperty (const std::string& name,
                                            const std::string& value) {
  if (!sender_->IsConnected()) {
    pending_window_properties_.push_back(WindowProperty(name, value));
    return;
  }

  sender_->Send(new WaylandDisplay_Property(handle_, name, value));
}

void OzoneWaylandWindow::CreateGroup(
    const ui::WindowGroupConfiguration& config) {
  sender_->Send(new WaylandDisplay_CreateWindowGroup(handle_, config));
}

void OzoneWaylandWindow::AttachToGroup(const std::string& group,
                                       const std::string& layer) {
  sender_->Send(new WaylandDisplay_AttachToWindowGroup(handle_, group, layer));
}

void OzoneWaylandWindow::FocusGroupOwner() {
  sender_->Send(new WaylandDisplay_FocusWindowGroupOwner(handle_));
}

void OzoneWaylandWindow::FocusGroupLayer() {
  sender_->Send(new WaylandDisplay_FocusWindowGroupLayer(handle_));
}

void OzoneWaylandWindow::DetachGroup() {
  sender_->Send(new WaylandDisplay_DetachWindowGroup(handle_));
}

void OzoneWaylandWindow::ToggleFullscreenWithSize(const gfx::Size& size) {
  if (size.width() == 0 || size.height() == 0) {
    ToggleFullscreen();
    return;
  } else {
    SetBounds(gfx::Rect(size.width(), size.height()));
  }
  state_ = ui::FULLSCREEN;
  SendWidgetState();
}

#endif

////////////////////////////////////////////////////////////////////////////////
// WindowTreeHostDelegateWayland, ui::PlatformEventDispatcher implementation:
bool OzoneWaylandWindow::CanDispatchEvent(
    const ui::PlatformEvent& ne) {
  gfx::AcceleratedWidget grabber = window_manager_->event_grabber();
  if (grabber != gfx::kNullAcceleratedWidget)
    return grabber == handle_;

  // TODO(jani) this is just quick and dirty hack to support touch event
  // dispatching
  if (static_cast<Event*>(ne)->IsLocatedEvent()) {
    const LocatedEvent* event =
        static_cast<const LocatedEvent*>(ne);
    return GetBounds().Contains(event->root_location());
  }
}

uint32_t OzoneWaylandWindow::DispatchEvent(
    const ui::PlatformEvent& ne) {
  DispatchEventFromNativeUiEvent(
      ne, base::Bind(&PlatformWindowDelegate::DispatchEvent,
                     base::Unretained(delegate_)));
  return POST_DISPATCH_STOP_PROPAGATION;
}

void OzoneWaylandWindow::OnChannelEstablished() {
  sender_->Send(new WaylandDisplay_Create(handle_));
  InitNativeWidget();
}

void OzoneWaylandWindow::OnChannelDestroyed() {
}

void OzoneWaylandWindow::SendWidgetState() {
  if (!sender_->IsConnected())
    return;

  sender_->Send(new WaylandDisplay_State(handle_, state_));
}

void OzoneWaylandWindow::AddRegion() {
  if (sender_->IsConnected() && region_ && !region_->isEmpty()) {
     const SkIRect& rect = region_->getBounds();
     sender_->Send(new WaylandDisplay_AddRegion(handle_,
                                                rect.left(),
                                                rect.top(),
                                                rect.right(),
                                                rect.bottom()));
  }
}

void OzoneWaylandWindow::ResetRegion() {
  if (region_) {
    if (sender_->IsConnected() && !region_->isEmpty()) {
      const SkIRect& rect = region_->getBounds();
      sender_->Send(new WaylandDisplay_SubRegion(handle_,
                                                 rect.left(),
                                                 rect.top(),
                                                 rect.right(),
                                                 rect.bottom()));
    }

    delete region_;
    region_ = NULL;
  }
}

void OzoneWaylandWindow::SetCursor() {
  if (bitmap_) {
    sender_->Send(new WaylandDisplay_CursorSet(bitmap_->bitmaps(),
                                               bitmap_->hotspot()));
  } else {
    sender_->Send(new WaylandDisplay_CursorSet(std::vector<SkBitmap>(),
                                               gfx::Point()));
  }
}

void OzoneWaylandWindow::ValidateBounds() {
  DCHECK(parent_);
  gfx::Rect parent_bounds = window_manager_->GetWindow(parent_)->GetBounds();
  int x = bounds_.x() - parent_bounds.x();
  int y = bounds_.y() - parent_bounds.y();

  if (x < parent_bounds.x()) {
    x = parent_bounds.x();
  } else {
    int width = x + bounds_.width();
    if (width > parent_bounds.width())
      x -= width - parent_bounds.width();
  }

  if (y < parent_bounds.y()) {
    y = parent_bounds.y();
  } else {
    int height = y + bounds_.height();
    if (height > parent_bounds.height())
      y -= height - parent_bounds.height();
  }

  bounds_.set_origin(gfx::Point(x, y));
}

PlatformImeController* OzoneWaylandWindow::GetPlatformImeController() {
  return nullptr;
}

}  // namespace ui
