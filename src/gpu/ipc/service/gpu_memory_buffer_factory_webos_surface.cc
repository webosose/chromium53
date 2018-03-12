// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory_webos_surface.h"

#include <vector>

#include "base/logging.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_webos_surface.h"
#include "ui/gl/gl_image.h"
#include "ui/gl/gl_image_webos_surface.h"

namespace gpu {

GpuMemoryBufferFactoryWebOSSurface::GpuMemoryBufferFactoryWebOSSurface() {}

GpuMemoryBufferFactoryWebOSSurface::~GpuMemoryBufferFactoryWebOSSurface() {}

gfx::GpuMemoryBufferHandle
GpuMemoryBufferFactoryWebOSSurface::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int client_id,
    SurfaceHandle surface_handle) {
  gfx::GpuMemoryBufferHandle handle;
  unsigned surface_usage =
      WEBOS_SURFACE_USAGE_HW_TEXTURE | WEBOS_SURFACE_USAGE_HW_RENDER |
      WEBOS_SURFACE_USAGE_SW_READ_OFTEN | WEBOS_SURFACE_USAGE_SW_WRITE_OFTEN;

  unsigned surface_format = (format == gfx::BufferFormat::BGRA_8888)
                                ? WEBOS_SURFACE_FORMAT_BGRA_8888
                                : WEBOS_SURFACE_FORMAT_RGBA_8888;

  surface_handle_t surface = webos_surface_create(
      size.width(), size.height(), surface_usage, surface_format);
  if (surface == SURFACE_INVALID_HANDLE) {
    handle.type = gfx::EMPTY_BUFFER;
    return gfx::GpuMemoryBufferHandle();
  }

  size_t data_size = webos_surface_exported_size(surface);
  size_t fds_count = webos_surface_fd_count(surface);
  char data[data_size];
  int fds[fds_count];

  if (webos_surface_export(surface, data, data_size, fds, fds_count)) {
    handle.type = gfx::EMPTY_BUFFER;
    webos_surface_destroy(surface);
    return gfx::GpuMemoryBufferHandle();
  }

  {
    base::AutoLock lock(webos_surfaces_lock_);

    WebOSSurfaceMapKey key(id, client_id);
    DCHECK(webos_surfaces_.find(key) == webos_surfaces_.end());
    webos_surfaces_[key] = surface;
  }

  for (size_t i = 0; i < data_size; i++)
    handle.webos_surface_handle.data.push_back(data[i]);

  for (size_t i = 0; i < fds_count; i++)
    handle.webos_surface_handle.descriptors.push_back(
        base::FileDescriptor(HANDLE_EINTR(dup(fds[i])), true));

  handle.type = gfx::WEBOS_SURFACE_BUFFER;
  handle.id = id;

  return handle;
}

void GpuMemoryBufferFactoryWebOSSurface::DestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id) {
  base::AutoLock lock(webos_surfaces_lock_);

  WebOSSurfaceMapKey key(id, client_id);
  DCHECK(webos_surfaces_.find(key) != webos_surfaces_.end());
  surface_handle_t surface = webos_surfaces_[key];
  webos_surfaces_.erase(key);
  webos_surface_destroy(surface);
}

gpu::ImageFactory* GpuMemoryBufferFactoryWebOSSurface::AsImageFactory() {
  return this;
}

scoped_refptr<gl::GLImage>
GpuMemoryBufferFactoryWebOSSurface::CreateImageForGpuMemoryBuffer(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    unsigned internalformat,
    int client_id,
    SurfaceHandle surface_handle) {
  base::AutoLock lock(webos_surfaces_lock_);

  DCHECK_EQ(handle.type, gfx::WEBOS_SURFACE_BUFFER);
  WebOSSurfaceMapKey key(handle.id, client_id);
  WebOSSurfaceMap::iterator it = webos_surfaces_.find(key);
  if (it == webos_surfaces_.end())
    return scoped_refptr<gl::GLImage>();

  scoped_refptr<gl::GLImageWebOSSurface> image(
      new gl::GLImageWebOSSurface(size, internalformat));
  if (!image->Initialize(handle))
    return scoped_refptr<gl::GLImage>();

  return image;
}

}  // namespace content
