// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/egl/surface_ozone_wayland.h"

#include "ozone/wayland/display.h"
#include "ozone/wayland/window.h"
#include "third_party/khronos/EGL/egl.h"
#include "ui/ozone/common/egl_util.h"
#include "ui/gfx/vsync_provider.h"

namespace ozonewayland {

SurfaceOzoneWayland::SurfaceOzoneWayland(unsigned handle)
    : handle_(handle) {
}

SurfaceOzoneWayland::~SurfaceOzoneWayland() {
  WaylandDisplay::GetInstance()->DestroyWindow(handle_);
  WaylandDisplay::GetInstance()->FlushDisplay();
}

intptr_t SurfaceOzoneWayland::GetNativeWindow() {
  return WaylandDisplay::GetInstance()->GetNativeWindow(handle_);
}

bool SurfaceOzoneWayland::ResizeNativeWindow(
    const gfx::Size& viewport_size) {
  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(handle_);
  DCHECK(window);
  window->Resize(viewport_size.width(), viewport_size.height());
  return true;
}

bool SurfaceOzoneWayland::OnSwapBuffers() {
#if defined(OS_WEBOS)
  WaylandDisplay::GetInstance()->CompositorBuffersSwapped(handle_);
  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(handle_);
  WaylandDisplay::GetInstance()->FlushDisplay();
#endif
  return true;
}

void SurfaceOzoneWayland::OnSwapBuffersAsync(
    const ui::SwapCompletionCallback& callback) {
#if defined(OS_WEBOS)
  OnSwapBuffers();
#endif
}

std::unique_ptr<gfx::VSyncProvider> SurfaceOzoneWayland::CreateVSyncProvider() {
  return std::unique_ptr<gfx::VSyncProvider>();
}

void* /* EGLConfig */ SurfaceOzoneWayland::GetEGLSurfaceConfig(
    const ui::EglConfigCallbacks& egl) {
  EGLint config_attribs[] = {EGL_BUFFER_SIZE,
                             32,
                             EGL_ALPHA_SIZE,
                             8,
                             EGL_BLUE_SIZE,
                             8,
                             EGL_GREEN_SIZE,
                             8,
                             EGL_RED_SIZE,
                             8,
                             EGL_RENDERABLE_TYPE,
                             EGL_OPENGL_ES2_BIT,
                             EGL_SURFACE_TYPE,
                             EGL_WINDOW_BIT,
                             EGL_NONE};
  return ChooseEGLConfig(egl, config_attribs);
}

}  // namespace ozonewayland
