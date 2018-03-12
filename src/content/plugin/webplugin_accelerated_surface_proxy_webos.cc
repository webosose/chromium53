// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/plugin/webplugin_accelerated_surface_proxy_webos.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "content/child/npapi/webplugin_delegate_impl.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"
#include "content/plugin/webplugin_proxy.h"
#include "content/public/common/content_switches.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "gpu/command_buffer/service/image_factory.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "skia/ext/platform_canvas.h"
#include "ui/gfx/buffer_types.h"

#if defined(USE_WEBOS_SURFACE)
#include "ui/surface/accelerated_surface_webos.h"
#endif

static const size_t kDefaultSharedBuffersCount = 3;

namespace content {

void Noop(const gpu::SyncToken& sync) {}

// static
scoped_refptr<WebPluginAcceleratedSurfaceProxy::SharedBuffer>
WebPluginAcceleratedSurfaceProxy::SharedBuffer::Create(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    unsigned internalformat,
    gfx::BufferUsage usage,
    gfx::BufferFormat format,
    gl::GpuPreference gpu_preference) {
  scoped_refptr<SharedBuffer> buffer =
      new SharedBuffer(handle, size, internalformat, usage, format);
  if (!buffer->Initialize(gpu_preference))
    return scoped_refptr<SharedBuffer>();
  return buffer;
}

WebPluginAcceleratedSurfaceProxy::SharedBuffer::SharedBuffer(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    unsigned internalformat,
    gfx::BufferUsage usage,
    gfx::BufferFormat format)
    : handle_(handle),
      size_(size),
      internalformat_(internalformat),
      usage_(usage),
      format_(format),
      canvas_(nullptr),
      did_first_paint_(false),
      locked_for_write_(false) {}

WebPluginAcceleratedSurfaceProxy::SharedBuffer::~SharedBuffer() {
  canvas_owner_.reset();
#if defined(USE_WEBOS_SURFACE)
  surface_.reset();
#endif
}

bool WebPluginAcceleratedSurfaceProxy::SharedBuffer::Initialize(
    gl::GpuPreference gpu_preference) {
  buffer_ = gpu::GpuMemoryBufferImpl::CreateFromHandle(
      handle_, size_, format_, usage_, base::Bind(&Noop));

  if (!buffer_.get())
    return false;
  switch (handle_.type) {
    case gfx::SHARED_MEMORY_BUFFER:
      if (!buffer_->Map())
        return false;
      canvas_owner_ = std::unique_ptr<SkCanvas>(skia::CreatePlatformCanvas(
          size_.width(), size_.height(), true,
          reinterpret_cast<uint8_t*>(buffer_->memory(0)),
          skia::RETURN_NULL_ON_FAILURE));
      if (!canvas_owner_)
        return false;
      canvas_ = canvas_owner_.get();
      return true;
#if defined(USE_WEBOS_SURFACE)
    case gfx::WEBOS_SURFACE_BUFFER:
      surface_ = AcceleratedSurfaceWebOS::Create(gpu_preference);
      if (!surface_.get())
        return false;
      canvas_ = surface_->CreateCanvas(size_.width(), size_.height(),
                                       internalformat_, buffer_->GetHandle());
      if (!canvas_)
        return false;
      return true;
#endif
    default:
      NOTREACHED();
      return false;
  }
}

bool WebPluginAcceleratedSurfaceProxy::SharedBuffer::BeginPaint() {
  switch (handle_.type) {
    case gfx::SHARED_MEMORY_BUFFER:
      return true;
#if defined(USE_WEBOS_SURFACE)
    case gfx::WEBOS_SURFACE_BUFFER:
      if (surface_.get())
        surface_->BeginPaint();
      return true;
#endif
    default:
      NOTREACHED();
      return false;
  }
}

bool WebPluginAcceleratedSurfaceProxy::SharedBuffer::EndPaint() {
  switch (handle_.type) {
    case gfx::SHARED_MEMORY_BUFFER:
      return true;
#if defined(USE_WEBOS_SURFACE)
    case gfx::WEBOS_SURFACE_BUFFER:
      if (surface_.get())
        surface_->EndPaint();
      return true;
#endif
    default:
      NOTREACHED();
      return false;
  }
}

WebPluginAcceleratedSurfaceProxy* WebPluginAcceleratedSurfaceProxy::Create(
    WebPluginProxy* plugin_proxy,
    gl::GpuPreference gpu_preference) {
  // This code path shouldn't be taken if accelerated plugin
  // rendering is disabled.
  DCHECK(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableAcceleratedPluginRendering));

  return new WebPluginAcceleratedSurfaceProxy(plugin_proxy, gpu_preference);
}

WebPluginAcceleratedSurfaceProxy::WebPluginAcceleratedSurfaceProxy(
    WebPluginProxy* plugin_proxy,
    gl::GpuPreference gpu_preference)
    : plugin_proxy_(plugin_proxy),
      gpu_preference_(gpu_preference),
      best_internalformat_(GL_BGRA_EXT),
      best_usage_(gfx::BufferUsage::GPU_READ),
      buffer_format_(gfx::BufferFormat::BGRA_8888),
      shared_buffers_limit_(kDefaultSharedBuffersCount),
      windowless_buffer_id_(0),
      waiting_for_buffer_(false),
      waiting_for_swap_(false),
      suspended_(false),
#if defined(USE_WEBOS_SURFACE)
      has_ref_on_shared_objects_(false),
#endif
      weak_factory_(this) {
}

WebPluginAcceleratedSurfaceProxy::~WebPluginAcceleratedSurfaceProxy() {
#if defined(USE_WEBOS_SURFACE)
  if (has_ref_on_shared_objects_)
    AcceleratedSurfaceWebOS::DecreaseSharedObjectsCount(true);
#endif
}

WebPluginDelegateImpl* WebPluginAcceleratedSurfaceProxy::delegate() const {
  return plugin_proxy_->delegate();
}

void WebPluginAcceleratedSurfaceProxy::SetSize(const gfx::Size& size) {
  if (size_ == size)
    return;

  size_ = size;

  // Need to release all shared buffers, because surface size was changed
  // and we couldn't reuse old buffers.
  ReleaseBuffers();
  Invalidate();
}

void WebPluginAcceleratedSurfaceProxy::Suspend() {
  suspended_ = true;
  waiting_for_swap_ = false;
  waiting_for_buffer_ = false;

#if defined(USE_WEBOS_SURFACE)
  if (!has_ref_on_shared_objects_) {
    has_ref_on_shared_objects_ = true;
    AcceleratedSurfaceWebOS::IncreaseSharedObjectsCount();
  }
#endif

  ReleaseBuffers();
}

void WebPluginAcceleratedSurfaceProxy::Resume() {
  suspended_ = false;
  Invalidate();
}

void WebPluginAcceleratedSurfaceProxy::DidSwapBuffer(int32_t id) {
  waiting_for_swap_ = false;

  if (!damaged_rect_.IsEmpty())
    InvalidateRect(damaged_rect_);
}

void WebPluginAcceleratedSurfaceProxy::DidSwapBufferComplete(int32_t id) {
  waiting_for_buffer_ = false;

  SharedBufferMap::iterator it = shared_buffers_.find(id);
  if (it != shared_buffers_.end())
    it->second->UnlockForWrite();

  if (!damaged_rect_.IsEmpty())
    InvalidateRect(damaged_rect_);
}

void WebPluginAcceleratedSurfaceProxy::Invalidate() {
  gfx::Rect rect(0, 0, delegate()->GetRect().width(),
                 delegate()->GetRect().height());
  InvalidateRect(rect);
}

void WebPluginAcceleratedSurfaceProxy::InvalidateRect(const gfx::Rect& rect) {
  damaged_rect_.Union(rect);

  if (damaged_rect_.IsEmpty() ||
      !delegate()->GetClipRect().Intersects(damaged_rect_))
    return;

  if (!waiting_for_swap_ && !waiting_for_buffer_ && !suspended_) {
    waiting_for_swap_ = true;
    // Invalidates caused by calls to NPN_InvalidateRect/NPN_InvalidateRgn
    // need to be painted asynchronously as per the NPAPI spec.
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&WebPluginAcceleratedSurfaceProxy::OnPaint,
                              weak_factory_.GetWeakPtr(), damaged_rect_));
    damaged_rect_ = gfx::Rect();
  }
}

void WebPluginAcceleratedSurfaceProxy::Paint(const gfx::Rect& rect) {
  if (!windowless_canvas() || !windowless_canvas()->getDevice())
    return;

  // Clear the damaged area so that if the plugin doesn't paint there we won't
  // end up with the old values.
  gfx::Rect offset_rect = rect;
  offset_rect.Offset(delegate()->GetRect().OffsetFromOrigin());

  // See above comment about windowless_context_ changing.
  // http::/crbug.com/139462
  SkCanvas* saved_canvas = windowless_canvas();

  saved_canvas->save();

  // The given clip rect is relative to the plugin coordinate system.
  SkRect sk_rect = {SkIntToScalar(rect.x()), SkIntToScalar(rect.y()),
                    SkIntToScalar(rect.right()), SkIntToScalar(rect.bottom())};
  saved_canvas->clipRect(sk_rect);

  // Fill a transparent value so that if the plugin supports transparency that
  // will work.
  saved_canvas->drawColor(SkColorSetARGB(0, 0, 0, 0), SkXfermode::kSrc_Mode);

  // Bring the windowless canvas into the window coordinate system, which is
  // how the plugin expects to draw (since the windowless API was originally
  // designed just for scribbling over the web page).
  saved_canvas->translate(SkIntToScalar(-delegate()->GetRect().x()),
                          SkIntToScalar(-delegate()->GetRect().y()));

  // Before we send the invalidate, paint so that renderer uses the updated
  // bitmap.
  delegate()->Paint(saved_canvas, offset_rect);

  saved_canvas->restore();
}

bool WebPluginAcceleratedSurfaceProxy::BeginPaint() {
  return shared_buffers_[windowless_buffer_id_]->BeginPaint();
}

bool WebPluginAcceleratedSurfaceProxy::EndPaint() {
  return shared_buffers_[windowless_buffer_id_]->EndPaint();
}

void WebPluginAcceleratedSurfaceProxy::OnPaint(const gfx::Rect& damaged_rect) {
  if (suspended_) {
    damaged_rect_.Union(damaged_rect);
    return;
  }

  windowless_buffer_id_ = LookupBuffer();
  if (!windowless_buffer_id_) {
    waiting_for_buffer_ = true;
    waiting_for_swap_ = false;
    damaged_rect_.Union(damaged_rect);
    return;
  }

  gfx::Rect rect = ComputeDamagedRegion(damaged_rect);

  BeginPaint();
  Paint(rect);
  EndPaint();

  // Swap buffer for further compositing.
  plugin_proxy_->AcceleratedPluginSwapBuffer(windowless_buffer_id_);
}

gfx::Rect WebPluginAcceleratedSurfaceProxy::ComputeDamagedRegion(
    const gfx::Rect& damaged_rect) {
  gfx::Rect computed_damaged_region = damaged_rect;

  scoped_refptr<SharedBuffer> shared_buffer =
      shared_buffers_.at(windowless_buffer_id_);
  computed_damaged_region.Union(shared_buffer->GetDamagedRegion());
  shared_buffer->SetDamagedRegion(gfx::Rect());

  for (SharedBufferMap::iterator it = shared_buffers_.begin();
       it != shared_buffers_.end(); ++it) {
    if (it->first != windowless_buffer_id_)
      it->second->AddDamagedRegion(damaged_rect);
  }

  if (!shared_buffer->DidFirstPaint()) {
    shared_buffer->SetFirstPaint();
    computed_damaged_region = gfx::Rect(0, 0, size_.width(), size_.height());
  }
  return computed_damaged_region;
}

int32_t WebPluginAcceleratedSurfaceProxy::LookupBuffer() {
  for (SharedBufferMap::iterator it = shared_buffers_.begin();
       it != shared_buffers_.end(); ++it) {
    if (it->second->CanLockForWrite()) {
      it->second->LockForWrite();
      return it->first;
    }
  }

  // Limit shared buffers count
  if (shared_buffers_.size() >= shared_buffers_limit_)
    return 0;

  return AllocateBuffer();
}

int32_t WebPluginAcceleratedSurfaceProxy::AllocateBuffer() {
  int32_t id = 0;
  gfx::GpuMemoryBufferHandle handle;

  plugin_proxy_->AcceleratedPluginAllocateBuffer(
      size_.width(), size_.height(), best_internalformat_,
      (uint32_t)best_usage_, (uint32_t)buffer_format_, &id, &handle);

  if (handle.is_null())
    return 0;

  scoped_refptr<SharedBuffer> buffer =
      SharedBuffer::Create(handle, size_, best_internalformat_, best_usage_,
                           buffer_format_, gpu_preference_);
  if (!buffer.get()) {
    // Bad things happened, need to release dangling buffer.
    plugin_proxy_->AcceleratedPluginReleaseBuffer(id);
    return 0;
  }

#if defined(USE_WEBOS_SURFACE)
  if (has_ref_on_shared_objects_) {
    has_ref_on_shared_objects_ = false;
    AcceleratedSurfaceWebOS::DecreaseSharedObjectsCount();
  }
#endif

  buffer->LockForWrite();

  DCHECK(shared_buffers_.find(id) == shared_buffers_.end());
  std::pair<SharedBufferMap::iterator, bool> result =
      shared_buffers_.insert(std::make_pair(id, buffer));
  DCHECK(result.second);

  return id;
}

void WebPluginAcceleratedSurfaceProxy::ReleaseBuffers() {
  for (SharedBufferMap::iterator it = shared_buffers_.begin();
       it != shared_buffers_.end(); ++it) {
    plugin_proxy_->AcceleratedPluginReleaseBuffer(it->first);
  }
  shared_buffers_.clear();
  waiting_for_swap_ = false;
}

}  // namespace content
