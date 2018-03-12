// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
#include "ui/gl/gl_image_webos_surface.h"

#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_surface_egl.h"

namespace gl {

GLImageWebOSSurface::GLImageWebOSSurface(const gfx::Size& size,
                                         unsigned internalformat)
    : surface_(SURFACE_INVALID_HANDLE),
      egl_image_(EGL_NO_IMAGE_KHR),
      size_(size),
      internalformat_(internalformat),
      texture_2d_(0),
      texture_external_oes_(0) {}

GLImageWebOSSurface::~GLImageWebOSSurface() {
  DCHECK_EQ(SURFACE_INVALID_HANDLE, surface_);
  DCHECK_EQ(EGL_NO_IMAGE_KHR, egl_image_);
}

bool GLImageWebOSSurface::Initialize(const gfx::GpuMemoryBufferHandle& handle) {
  DCHECK_EQ(SURFACE_INVALID_HANDLE, surface_);
  DCHECK_EQ(EGL_NO_IMAGE_KHR, egl_image_);

  size_t data_size = handle.webos_surface_handle.data.size();
  size_t fds_count = handle.webos_surface_handle.descriptors.size();

  char data[data_size];
  for (size_t i = 0; i < data_size; i++)
    data[i] = handle.webos_surface_handle.data[i];

  int fds[fds_count];
  for (size_t i = 0; i < fds_count; i++)
    fds[i] = handle.webos_surface_handle.descriptors[i].fd;

  surface_ = webos_surface_import(data, data_size, fds, fds_count);
  if (surface_ == SURFACE_INVALID_HANDLE) {
    LOG(ERROR) << "Error importing webOS surface";
    return false;
  }

  egl_image_ = webos_surface_create_image(surface_);
  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    LOG(ERROR) << "Error creating EGLImage: " << eglGetError();
    webos_surface_destroy(surface_);
    return false;
  }

  return true;
}

void GLImageWebOSSurface::Destroy(bool have_context) {
  if (surface_ != SURFACE_INVALID_HANDLE) {
    webos_surface_destroy(surface_);
    surface_ = SURFACE_INVALID_HANDLE;
  }

  if (egl_image_ != EGL_NO_IMAGE_KHR) {
    eglDestroyImageKHR(GLSurfaceEGL::GetHardwareDisplay(), egl_image_);
    egl_image_ = EGL_NO_IMAGE_KHR;
  }
}

gfx::Size GLImageWebOSSurface::GetSize() {
  return size_;
}

unsigned GLImageWebOSSurface::GetInternalFormat() {
  return internalformat_;
}

bool GLImageWebOSSurface::BindTexImage(unsigned target) {
  DCHECK_NE(EGL_NO_IMAGE_KHR, egl_image_);
  GLint texture;

  switch (target) {
    case GL_TEXTURE_2D:
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
      if (texture_2d_ == texture)
        return true;
      texture_2d_ = texture;
      break;
    case GL_TEXTURE_EXTERNAL_OES:
      glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &texture);
      if (texture_external_oes_ == texture)
        return true;
      texture_external_oes_ = texture;
      break;
    default:
      LOG(ERROR) << "EGLImage can only be bound to \
          TEXTURE_2D or TEXTURE_EXTERNAL_OES targets";
      return false;
  }

  glEGLImageTargetTexture2DOES(target, (GLeglImageOES)egl_image_);
  DCHECK_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
  return true;
}

bool GLImageWebOSSurface::ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                                               int z_order,
                                               gfx::OverlayTransform transform,
                                               const gfx::Rect& bounds_rect,
                                               const gfx::RectF& crop_rect) {
  return false;
}

}  // namespace gfx
