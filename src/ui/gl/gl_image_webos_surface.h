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

#ifndef UI_GL_GL_IMAGE_WEBOS_SURFACE_H_
#define UI_GL_GL_IMAGE_WEBOS_SURFACE_H_

#include <webossurface/webossurface.h>

#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_image.h"

namespace gl {

class GL_EXPORT GLImageWebOSSurface : public GLImage {
 public:
  GLImageWebOSSurface(const gfx::Size& size, unsigned internalformat);

  bool Initialize(const gfx::GpuMemoryBufferHandle& handle);

  // Overridden from GLImage:
  void Destroy(bool have_context) override;
  gfx::Size GetSize() override;
  unsigned GetInternalFormat() override;
  bool BindTexImage(unsigned target) override;
  void ReleaseTexImage(unsigned target) override {}
  bool CopyTexImage(unsigned target) override { return false; }
  bool CopyTexSubImage(unsigned target,
                       const gfx::Point& offset,
                       const gfx::Rect& rect) override {
    return false;
  }
  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int z_order,
                            gfx::OverlayTransform transform,
                            const gfx::Rect& bounds_rect,
                            const gfx::RectF& crop_rect) override;
  void OnMemoryDump(base::trace_event::ProcessMemoryDump* pmd,
                    uint64_t process_tracing_id,
                    const std::string& dump_name) override{};

 protected:
  virtual ~GLImageWebOSSurface();

 private:
  surface_handle_t surface_;
  EGLImageKHR egl_image_;
  const gfx::Size size_;
  const unsigned internalformat_;

  GLint texture_2d_;
  GLint texture_external_oes_;

  DISALLOW_COPY_AND_ASSIGN(GLImageWebOSSurface);
};

}  // namespace gfx

#endif  // UI_GL_GL_IMAGE_WEBOS_SURFACE_H_
