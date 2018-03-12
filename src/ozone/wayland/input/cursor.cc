// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/input/cursor.h"

#include <sys/mman.h>
#include <vector>

#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "ozone/wayland/display.h"

namespace ozonewayland {

#if defined(OS_WEBOS)
// LSM defines hotspot for hide cursor (blank cursor)
const gfx::Point g_webos_hide_location = gfx::Point(254, 254);
// LSM defines hotspot for restoring to default webOS cursor
const gfx::Point g_webos_restore_location = gfx::Point(255, 255);
#endif

WaylandCursor::WaylandCursor()
    :
      input_pointer_(NULL),
      pointer_surface_(NULL),
      buffer_(NULL),
      sh_memory_(new base::SharedMemory()),
      width_(0),
      height_(0) {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  shm_ = display->GetShm();
  pointer_surface_ = wl_compositor_create_surface(display->GetCompositor());
}

WaylandCursor::~WaylandCursor() {
  wl_surface_destroy(pointer_surface_);
  if (buffer_)
    wl_buffer_destroy(buffer_);
  if (sh_memory_->handle().fd) {
    sh_memory_->Unmap();
    sh_memory_->Close();
  }
  delete sh_memory_;
}

void WaylandCursor::UpdateBitmap(const std::vector<SkBitmap>& cursor_image,
                                 const gfx::Point& location,
                                 uint32_t serial) {
  if (!input_pointer_)
    return;

  if (!cursor_image.size()) {
#if defined(OS_WEBOS)
    if (g_webos_hide_location == location)
      HideWebOSCursor(serial);
    else if (g_webos_restore_location == location)
      RestoreWebOSCursor(serial);
    else
#endif
      HideCursor(serial);
    return;
  }

  const SkBitmap& image = cursor_image[0];
  int width = image.width();
  int height = image.height();
  if (!width || !height) {
    HideCursor(serial);
    return;
  }

#if defined(OS_WEBOS)
  // If we already have wl_buffer and it has not been freed yet in above
  // conditions, then it will leak memory, since CreateSHMBuffer will create
  // new buffer for new width, height values. So destroy it here.
  if (buffer_ && (width_ != width || height_ != height))
    DestroyAndUnmapBuffer();
#endif

  if (!CreateSHMBuffer(width, height)) {
    LOG(INFO) << "Failed to create SHM buffer for Cursor Bitmap.";
    wl_pointer_set_cursor(input_pointer_, serial, NULL, 0, 0);
    return;
  }

  // The |bitmap| contains ARGB image, so just copy it.
  memcpy(sh_memory_->memory(), image.getPixels(), width_ * height_ * 4);

  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                        location.x(), location.y());
  wl_surface_attach(pointer_surface_, buffer_, 0, 0);
  wl_surface_damage(pointer_surface_, 0, 0, width_, height_);
  wl_surface_commit(pointer_surface_);
}

void WaylandCursor::MoveCursor(const gfx::Point& location, uint32_t serial) {
  if (!input_pointer_)
    return;

  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                         location.x(), location.y());
}

void WaylandCursor::SetInputPointer(wl_pointer* pointer) {
  if (input_pointer_ == pointer)
    return;

  if (input_pointer_)
    wl_pointer_destroy(input_pointer_);

  input_pointer_ = pointer;
}

bool WaylandCursor::CreateSHMBuffer(int width, int height) {
  if (width == width_ && height == height_)
    return true;

  struct wl_shm_pool *pool;
  int size, stride;

  width_ = width;
  height_ = height;
  stride = width_ * 4;
  SkImageInfo info = SkImageInfo::MakeN32Premul(width_, height_);
  size = info.getSafeSize(stride);

  if (sh_memory_->handle().fd) {
    sh_memory_->Unmap();
    sh_memory_->Close();
  }

  if (!sh_memory_->CreateAndMapAnonymous(size)) {
    LOG(INFO) << "Create and mmap failed.";
    return false;
  }

  pool = wl_shm_create_pool(shm_,
                            sh_memory_->handle().fd,
                            size);
  buffer_ = wl_shm_pool_create_buffer(pool, 0,
                                      width_, height_,
                                      stride, WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  return true;
}

void WaylandCursor::HideCursor(uint32_t serial) {
  width_ = 0;
  height_ = 0;
  wl_pointer_set_cursor(input_pointer_, serial, NULL, 0, 0);

  if (buffer_) {
    wl_buffer_destroy(buffer_);
    buffer_ = NULL;
  }

  if (sh_memory_->handle().fd) {
    sh_memory_->Unmap();
    sh_memory_->Close();
  }
}

#if defined(OS_WEBOS)
void WaylandCursor::DestroyAndUnmapBuffer() {
  if (buffer_) {
    wl_buffer_destroy(buffer_);
    buffer_ = NULL;
  }

  if (sh_memory_->handle().fd) {
    sh_memory_->Unmap();
    sh_memory_->Close();
  }
}

// WaylandCursor::HideCursor(uint32_t) hides cursor only temporarily and it's
// shown again when pointer is activated or button is pressed.
//
// WebOS implements permanent hiding of cursor. Cursor stays hidden and
// inactive until set visible again via SetWebOSCursorVisibility or if cursor
// hotspot is changed to something else than gfx::Point(254,254).

void WaylandCursor::HideWebOSCursor(uint32_t serial) {
  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                        g_webos_hide_location.x(), g_webos_hide_location.y());
  wl_surface_commit(pointer_surface_);
}

void WaylandCursor::RestoreWebOSCursor(uint32_t serial) {
  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_,
                        g_webos_restore_location.x(),
                        g_webos_restore_location.y());
  wl_surface_commit(pointer_surface_);
}
#endif

}  // namespace ozonewayland
