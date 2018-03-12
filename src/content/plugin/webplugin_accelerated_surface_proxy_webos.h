// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PLUGIN_WEBPLUGIN_ACCELERATED_SURFACE_PROXY_H_
#define CONTENT_PLUGIN_WEBPLUGIN_ACCELERATED_SURFACE_PROXY_H_

#include <map>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/child/npapi/webplugin_accelerated_surface_webos.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gl/gpu_preference.h"

#if defined(USE_WEBOS_SURFACE)
class AcceleratedSurfaceWebOS;
#endif

namespace content {
class WebPluginProxy;
class WebPluginDelegateImpl;

// Out-of-process implementation of WebPluginAcceleratedSurface that proxies
// calls through a WebPluginProxy.
class WebPluginAcceleratedSurfaceProxy : public WebPluginAcceleratedSurface {
 public:
  // Creates a new WebPluginAcceleratedSurfaceProxy that uses plugin_proxy
  // to proxy calls. plugin_proxy must outlive this object. Returns NULL if
  // initialization fails.
  static WebPluginAcceleratedSurfaceProxy* Create(
      WebPluginProxy* plugin_proxy,
      gl::GpuPreference gpu_preference);

  virtual ~WebPluginAcceleratedSurfaceProxy();

  // WebPluginAcceleratedSurface implementation.
  virtual void SetSize(const gfx::Size& size) override;
  virtual void Suspend() override;
  virtual void Resume() override;

  void Invalidate();
  void InvalidateRect(const gfx::Rect& rect);

  void DidSwapBuffer(int32_t id);
  void DidSwapBufferComplete(int32_t id);

 private:
  WebPluginAcceleratedSurfaceProxy(WebPluginProxy* plugin_proxy,
                                   gl::GpuPreference gpu_preference);

  WebPluginDelegateImpl* delegate() const;

  // Lookup free buffer for paint
  int32_t LookupBuffer();

  // Allocate new shared buffer
  int32_t AllocateBuffer();

  // Release all shared buffers
  void ReleaseBuffers();

  // For windowless plugins, paints the given rectangle into the buffer.
  void Paint(const gfx::Rect& rect);

  // Make extra work before/after paint.
  bool BeginPaint();
  bool EndPaint();

  // Handler for sending over the paint event to the plugin.
  void OnPaint(const gfx::Rect& damaged_rect);

  // Computing rect for re-paint only damaged rect
  gfx::Rect ComputeDamagedRegion(const gfx::Rect& damaged_rect);

  SkCanvas* windowless_canvas() const {
    return shared_buffers_.at(windowless_buffer_id_)->canvas();
  }

  class SharedBuffer : public base::RefCounted<SharedBuffer> {
   public:
    static scoped_refptr<SharedBuffer> Create(
        const gfx::GpuMemoryBufferHandle& handle,
        const gfx::Size& size,
        unsigned internalformat,
        gfx::BufferUsage usage,
        gfx::BufferFormat format,
        gl::GpuPreference gpu_preference);
    bool Initialize(gl::GpuPreference gpu_preference);

    gfx::GpuMemoryBuffer* buffer() const { return buffer_.get(); }
    SkCanvas* canvas() const { return canvas_; }

    bool BeginPaint();
    bool EndPaint();

    void LockForWrite() { locked_for_write_ = true; }
    void UnlockForWrite() { locked_for_write_ = false; }
    bool CanLockForWrite() const { return !locked_for_write_; }
    void SetFirstPaint() { did_first_paint_ = true; }
    bool DidFirstPaint() const { return did_first_paint_; }
    void AddDamagedRegion(gfx::Rect damaged_region) {
      damaged_region_.Union(damaged_region);
    }
    void SetDamagedRegion(gfx::Rect damaged_region) {
      damaged_region_ = damaged_region;
    }
    gfx::Rect GetDamagedRegion() const { return damaged_region_; }

   private:
    friend class base::RefCounted<SharedBuffer>;

    SharedBuffer(const gfx::GpuMemoryBufferHandle& handle,
                 const gfx::Size& size,
                 unsigned internalformat,
                 gfx::BufferUsage usage,
                 gfx::BufferFormat format);
    ~SharedBuffer();

    gfx::GpuMemoryBufferHandle handle_;
    gfx::Size size_;
    unsigned internalformat_;
    gfx::BufferUsage usage_;
    gfx::BufferFormat format_;

    std::unique_ptr<gfx::GpuMemoryBuffer> buffer_;
    std::unique_ptr<SkCanvas> canvas_owner_;
    SkCanvas* canvas_;
#if defined(USE_WEBOS_SURFACE)
    std::unique_ptr<AcceleratedSurfaceWebOS> surface_;
#endif

    gfx::Rect damaged_region_;
    bool did_first_paint_;
    bool locked_for_write_;

    DISALLOW_COPY_AND_ASSIGN(SharedBuffer);
  };

  WebPluginProxy* plugin_proxy_;  // Weak ref.

  gl::GpuPreference gpu_preference_;

  gfx::Size size_;
  gfx::Rect damaged_rect_;

  typedef std::map<int32_t, scoped_refptr<SharedBuffer>> SharedBufferMap;
  SharedBufferMap shared_buffers_;

  unsigned best_internalformat_;
  gfx::BufferUsage best_usage_;
  gfx::BufferFormat buffer_format_;

  size_t shared_buffers_limit_;
  int windowless_buffer_id_;
  bool waiting_for_buffer_;
  bool waiting_for_swap_;
  bool suspended_;
#if defined(USE_WEBOS_SURFACE)
  bool has_ref_on_shared_objects_;
#endif

  base::WeakPtrFactory<WebPluginAcceleratedSurfaceProxy> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebPluginAcceleratedSurfaceProxy);
};

}  // namespace content

#endif  // CONTENT_PLUGIN_WEBPLUGIN_ACCELERATED_SURFACE_PROXY_H_
