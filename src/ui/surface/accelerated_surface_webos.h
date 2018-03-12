// Copyright (c) 2015-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef UI_SURFACE_ACCELERATED_SURFACE_WEBOS_H_
#define UI_SURFACE_ACCELERATED_SURFACE_WEBOS_H_

#include <webossurface/webossurface.h>

#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gpu_preference.h"
#include "ui/surface/surface_export.h"

class SkCanvas;
class SkSurface;

class SURFACE_EXPORT AcceleratedSurfaceWebOS {
 public:
  virtual ~AcceleratedSurfaceWebOS();

  static std::unique_ptr<AcceleratedSurfaceWebOS> Create(
      gl::GpuPreference gpu_preference);

  bool Initialize(gl::GpuPreference gpu_preference);

  SkCanvas* CreateCanvas(int width,
                         int height,
                         unsigned internalformat,
                         const gfx::GpuMemoryBufferHandle& handle);

  void BeginPaint();
  void EndPaint();

  static void IncreaseSharedObjectsCount();
  static void DecreaseSharedObjectsCount(bool destroy = false);

 private:
  AcceleratedSurfaceWebOS();

  class SharedObjects {
   public:
    SharedObjects();
    ~SharedObjects();

    bool Initialize(gl::GpuPreference gpu_preference);
    void Destroy();

    // Sets the GL context to be the current one for painting.
    // Returns true if it succeeded.
    bool MakeCurrent();

    void BeginPaint();
    void EndPaint();

    void IncreaseUseCount() { use_count_++; }
    void DecreaseUseCount() { use_count_--; }

    gl::GLSurface* GetGLSurface() const { return gl_surface_.get(); }
    gl::GLContext* GetGLContext() const { return gl_context_.get(); }
    class GrContext* GetGrContext() const {
      return gr_context_.get();
    }

   private:
    // The shared OpenGL context and pbuffer drawable.
    scoped_refptr<gl::GLSurface> gl_surface_;
    scoped_refptr<gl::GLContext> gl_context_;
    sk_sp<class GrContext> gr_context_;
    int use_count_;
  };

  static AcceleratedSurfaceWebOS::SharedObjects objects_;
  sk_sp<SkSurface> sk_surface_;

  surface_handle_t surface_;
  EGLImageKHR egl_image_;
  unsigned texture_;
  unsigned fbo_;
};

#endif  // UI_SURFACE_ACCELERATED_SURFACE_WEBOS_H_
