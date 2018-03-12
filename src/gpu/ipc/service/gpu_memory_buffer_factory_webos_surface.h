// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_WEBOS_SURFACE_H_
#define GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_WEBOS_SURFACE_H_

#include <map>

#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "gpu/command_buffer/service/image_factory.h"
#include "gpu/ipc/service/gpu_memory_buffer_factory.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gfx/native_widget_types.h"

#include <webossurface/webossurface.h>

namespace gl {
class GLImage;
}

namespace gpu {

class GPU_EXPORT GpuMemoryBufferFactoryWebOSSurface
    : public GpuMemoryBufferFactory,
      public gpu::ImageFactory {
 public:
  GpuMemoryBufferFactoryWebOSSurface();
  ~GpuMemoryBufferFactoryWebOSSurface() override;

  gfx::GpuMemoryBufferHandle CreateGpuMemoryBuffer(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      int client_id,
      SurfaceHandle surface_handle) override;
  void DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                              int client_id) override;
  gpu::ImageFactory* AsImageFactory() override;

  // Overridden from gpu::ImageFactory:
  scoped_refptr<gl::GLImage> CreateImageForGpuMemoryBuffer(
      const gfx::GpuMemoryBufferHandle& handle,
      const gfx::Size& size,
      gfx::BufferFormat format,
      unsigned internalformat,
      int client_id,
      SurfaceHandle surface_handle) override;

 private:
  typedef std::pair<gfx::GpuMemoryBufferId, int> WebOSSurfaceMapKey;
  typedef base::hash_map<WebOSSurfaceMapKey, surface_handle_t> WebOSSurfaceMap;
  WebOSSurfaceMap webos_surfaces_;
  base::Lock webos_surfaces_lock_;

  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferFactoryWebOSSurface);
};

}  // namespace content

#endif  // GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_WEBOS_SURFACE_H_
