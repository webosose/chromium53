// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/gpu_memory_buffer_impl_webos_surface.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/init/gl_factory.h"

namespace gpu {
namespace {

void FreeGpuMemoryBufferImplWebOSSurfaceForTesting() {
  // Nothing to do here.
}

}  // namespace

static bool InitializeGLImplementation() {
  static gl::GLImplementation implementation = gl::kGLImplementationNone;
  if (implementation == gl::kGLImplementationNone) {
    // Not initialized yet.
    DCHECK_EQ(implementation, gl::GetGLImplementation())
        << "GL already initialized by someone else to: "
        << gl::GetGLImplementation();

    if (!gl::init::InitializeGLOneOff()) {
      return false;
    }
    implementation = gl::GetGLImplementation();
  }

  return (implementation == gl::kGLImplementationEGLGLES2);
}

GpuMemoryBufferImplWebOSSurface::GpuMemoryBufferImplWebOSSurface(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    const DestructionCallback& callback,
    surface_handle_t surface,
    gfx::WebOSSurfaceHandle surface_handle)
    : GpuMemoryBufferImpl(id, size, format, callback),
      surface_(surface),
      surface_handle_(surface_handle),
      data_(nullptr) {}

GpuMemoryBufferImplWebOSSurface::~GpuMemoryBufferImplWebOSSurface() {
  if (surface_ != SURFACE_INVALID_HANDLE)
    webos_surface_destroy(surface_);
}

// static
std::unique_ptr<GpuMemoryBufferImplWebOSSurface>
GpuMemoryBufferImplWebOSSurface::CreateFromHandle(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const DestructionCallback& callback) {
  if (!InitializeGLImplementation())
    return nullptr;

  size_t data_size = handle.webos_surface_handle.data.size();
  size_t fds_count = handle.webos_surface_handle.descriptors.size();

  char data[data_size];
  for (size_t i = 0; i < data_size; i++)
    data[i] = handle.webos_surface_handle.data[i];

  int fds[fds_count];
  for (size_t i = 0; i < fds_count; i++)
    fds[i] = handle.webos_surface_handle.descriptors[i].fd;

  surface_handle_t surface =
      webos_surface_import(data, data_size, fds, fds_count);
  if (surface == SURFACE_INVALID_HANDLE)
    return nullptr;

  return base::WrapUnique(new GpuMemoryBufferImplWebOSSurface(
      handle.id, size, format, callback, surface, handle.webos_surface_handle));
}

// static
bool GpuMemoryBufferImplWebOSSurface::IsConfigurationSupported(
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return gpu::IsNativeGpuMemoryBufferConfigurationSupported(format, usage);
}

// static
base::Closure GpuMemoryBufferImplWebOSSurface::AllocateForTesting(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gfx::GpuMemoryBufferHandle* handle) {
  DCHECK(IsConfigurationSupported(format, usage));
  gfx::GpuMemoryBufferHandle buffer_handle;

  unsigned surface_usage =
      WEBOS_SURFACE_USAGE_HW_TEXTURE | WEBOS_SURFACE_USAGE_HW_RENDER |
      WEBOS_SURFACE_USAGE_SW_READ_OFTEN | WEBOS_SURFACE_USAGE_SW_WRITE_OFTEN;

  unsigned surface_format = (format == gfx::BufferFormat::BGRA_8888)
                                ? WEBOS_SURFACE_FORMAT_BGRA_8888
                                : WEBOS_SURFACE_FORMAT_RGBA_8888;

  surface_handle_t surface = webos_surface_create(
      size.width(), size.height(), surface_usage, surface_format);
  if (surface == SURFACE_INVALID_HANDLE)
    buffer_handle = gfx::GpuMemoryBufferHandle();

  size_t data_size = webos_surface_exported_size(surface);
  size_t fds_count = webos_surface_fd_count(surface);
  char data[data_size];
  int fds[fds_count];

  if (webos_surface_export(surface, data, data_size, fds, fds_count)) {
    webos_surface_destroy(surface);
    buffer_handle = gfx::GpuMemoryBufferHandle();
  }

  for (size_t i = 0; i < data_size; i++)
    buffer_handle.webos_surface_handle.data.push_back(data[i]);

  for (size_t i = 0; i < fds_count; i++)
    buffer_handle.webos_surface_handle.descriptors.push_back(
        base::FileDescriptor(HANDLE_EINTR(dup(fds[i])), true));
  webos_surface_destroy(surface);

  gfx::GpuMemoryBufferId kBufferId(1);
  handle->type = gfx::WEBOS_SURFACE_BUFFER;
  handle->id = kBufferId;
  handle->webos_surface_handle = buffer_handle.webos_surface_handle;

  return base::Bind(&FreeGpuMemoryBufferImplWebOSSurfaceForTesting);
}

bool GpuMemoryBufferImplWebOSSurface::Map() {
  DCHECK(!mapped_);
  DCHECK(!data_);
  DCHECK_NE(SURFACE_INVALID_HANDLE, surface_);
  data_ = webos_surface_map(surface_, WEBOS_SURFACE_USAGE_SW_READ_OFTEN |
                                          WEBOS_SURFACE_USAGE_SW_WRITE_OFTEN);
  if (!data_)
    return false;
  mapped_ = true;
  return mapped_;
}

void* GpuMemoryBufferImplWebOSSurface::memory(size_t plane) {
  DCHECK(mapped_);
  DCHECK_LT(plane, gfx::NumberOfPlanesForBufferFormat(format_));
  return data_;
}

void GpuMemoryBufferImplWebOSSurface::Unmap() {
  DCHECK(mapped_);
  DCHECK(data_);
  webos_surface_unmap(surface_);
  mapped_ = false;
  data_ = nullptr;
}

int GpuMemoryBufferImplWebOSSurface::stride(size_t plane) const {
  DCHECK_LT(plane, gfx::NumberOfPlanesForBufferFormat(format_));
  int stride = webos_surface_stride(surface_);
  return stride;
}

gfx::GpuMemoryBufferHandle GpuMemoryBufferImplWebOSSurface::GetHandle() const {
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::WEBOS_SURFACE_BUFFER;
  handle.id = id_;
  handle.webos_surface_handle = surface_handle_;
  return handle;
}

}  // namespace gpu
