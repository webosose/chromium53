// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.

#include "ui/surface/accelerated_surface_webos.h"

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/init/gl_factory.h"
#include "ui/gl/scoped_make_current.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/gl/GrGLInterface.h"

AcceleratedSurfaceWebOS::SharedObjects AcceleratedSurfaceWebOS::objects_;

AcceleratedSurfaceWebOS::SharedObjects::SharedObjects() : use_count_(0) {}

AcceleratedSurfaceWebOS::SharedObjects::~SharedObjects() {
  Destroy();
}

bool AcceleratedSurfaceWebOS::SharedObjects::Initialize(
    gl::GpuPreference gpu_preference) {
  if (!use_count_) {
    ++use_count_;
    gl_surface_ = gl::init::CreateOffscreenGLSurface(gfx::Size(1, 1));
    if (!gl_surface_.get()) {
      Destroy();
      return false;
    }
    gl_context_ = gl::init::CreateGLContext(NULL, gl_surface_.get(),
                                                  gpu_preference);
    if (!gl_context_.get()) {
      Destroy();
      return false;
    }
    if (!gl_context_->MakeCurrent(gl_surface_.get())) {
      Destroy();
      return false;
    }
    sk_sp<const GrGLInterface> interface(GrGLCreateNativeInterface());
    gr_context_ = sk_sp<class GrContext>(
        GrContext::Create(kOpenGL_GrBackend,
                          reinterpret_cast<GrBackendContext>(interface.get())));
    if (!gr_context_) {
      Destroy();
      return false;
    }
  } else {
    ++use_count_;
  }
  return true;
}

void AcceleratedSurfaceWebOS::SharedObjects::Destroy() {
  --use_count_;
  if (!use_count_) {
    if (gr_context_)
      gr_context_->releaseResourcesAndAbandonContext();

    gl_context_ = NULL;
    gl_surface_ = NULL;
  }
}

void AcceleratedSurfaceWebOS::SharedObjects::BeginPaint() {
  MakeCurrent();
  gr_context_->resetContext();
}

void AcceleratedSurfaceWebOS::SharedObjects::EndPaint() {
  MakeCurrent();
  gr_context_->flush();
}

bool AcceleratedSurfaceWebOS::SharedObjects::MakeCurrent() {
  if (!gl_context_)
    return false;
  return gl_context_->MakeCurrent(gl_surface_.get());
}

void AcceleratedSurfaceWebOS::IncreaseSharedObjectsCount() {
  objects_.IncreaseUseCount();
}

void AcceleratedSurfaceWebOS::DecreaseSharedObjectsCount(bool destroy) {
  if (destroy)
    objects_.Destroy();
  else
    objects_.DecreaseUseCount();
}

std::unique_ptr<AcceleratedSurfaceWebOS> AcceleratedSurfaceWebOS::Create(
    gl::GpuPreference gpu_preference) {
  std::unique_ptr<AcceleratedSurfaceWebOS> surface(
      new AcceleratedSurfaceWebOS());
  if (!surface.get())
    return std::unique_ptr<AcceleratedSurfaceWebOS>();
  if (!surface->Initialize(gpu_preference))
    return std::unique_ptr<AcceleratedSurfaceWebOS>();
  return surface;
}

AcceleratedSurfaceWebOS::AcceleratedSurfaceWebOS()
    : surface_(SURFACE_INVALID_HANDLE),
      egl_image_(EGL_NO_IMAGE_KHR),
      texture_(0u),
      fbo_(0u) {}

AcceleratedSurfaceWebOS::~AcceleratedSurfaceWebOS() {
  objects_.MakeCurrent();

  if (fbo_)
    glDeleteFramebuffersEXT(1, &fbo_);

  if (texture_)
    glDeleteTextures(1, &texture_);

  if (surface_ != SURFACE_INVALID_HANDLE)
    webos_surface_destroy(surface_);

  if (egl_image_ != EGL_NO_IMAGE_KHR)
    eglDestroyImageKHR(gl::GLSurfaceEGL::GetHardwareDisplay(), egl_image_);

  objects_.Destroy();
}

bool AcceleratedSurfaceWebOS::Initialize(gl::GpuPreference gpu_preference) {
  DCHECK_NE(gl::GetGLImplementation(), gl::kGLImplementationNone);
  if (gl::GetGLImplementation() != gl::kGLImplementationEGLGLES2)
    return false;
  return objects_.Initialize(gpu_preference);
}

SkCanvas* AcceleratedSurfaceWebOS::CreateCanvas(
    int width,
    int height,
    unsigned internalformat,
    const gfx::GpuMemoryBufferHandle& handle) {
  ui::ScopedMakeCurrent make_current(objects_.GetGLContext(),
                                     objects_.GetGLSurface());
  if (!make_current.Succeeded())
    return nullptr;

  size_t data_size = handle.webos_surface_handle.data.size();
  size_t fds_count = handle.webos_surface_handle.descriptors.size();

  char data[data_size];
  for (size_t i = 0; i < data_size; i++)
    data[i] = handle.webos_surface_handle.data[i];

  int fds[fds_count];
  for (size_t i = 0; i < fds_count; i++)
    fds[i] = HANDLE_EINTR(dup(handle.webos_surface_handle.descriptors[i].fd));

  surface_ = webos_surface_import(data, data_size, fds, fds_count);
  if (surface_ == SURFACE_INVALID_HANDLE)
    return nullptr;

  egl_image_ = webos_surface_create_image(surface_);
  if (egl_image_ == EGL_NO_IMAGE_KHR) {
    DLOG(ERROR) << "Failed to create eglImageKHR";
    return nullptr;
  }

  glGenTextures(1, &texture_);
  if (!texture_)
    return nullptr;

  glBindTexture(GL_TEXTURE_2D, texture_);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)egl_image_);

  glGenFramebuffersEXT(1, &fbo_);
  if (!fbo_)
    return nullptr;

  glBindFramebufferEXT(GL_FRAMEBUFFER, fbo_);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                            texture_, 0);
  GLenum fbo_status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
  if (fbo_status != GL_FRAMEBUFFER_COMPLETE)
    return nullptr;

  GrBackendRenderTargetDesc desc;
  desc.fWidth = width;
  desc.fHeight = height;
  desc.fConfig = kBGRA_8888_GrPixelConfig;  //(internalformat == GL_BGRA_EXT) ?
                                            //kBGRA_8888_GrPixelConfig :
                                            //kRGBA_8888_GrPixelConfig;
  desc.fOrigin = kTopLeft_GrSurfaceOrigin;
  desc.fSampleCnt = 0;
  desc.fStencilBits = 0;
  desc.fRenderTargetHandle = fbo_;

  sk_sp<GrRenderTarget> render_target = sk_ref_sp<GrRenderTarget>(
      objects_.GetGrContext()->textureProvider()->wrapBackendRenderTarget(
          desc));
  if (!render_target)
    return nullptr;

  sk_surface_ = SkSurface::MakeRenderTargetDirect(render_target.get());
  return sk_surface_ ? sk_surface_->getCanvas() : nullptr;
}

void AcceleratedSurfaceWebOS::BeginPaint() {
  objects_.BeginPaint();
}

void AcceleratedSurfaceWebOS::EndPaint() {
  objects_.EndPaint();
  // This finish is absolutely essential -- it guarantees that the
  // rendering results are seen by the other process.
  glFinish();
}
