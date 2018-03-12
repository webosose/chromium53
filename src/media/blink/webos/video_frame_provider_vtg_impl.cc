// Copyright (c) 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/video_frame_provider_vtg_impl.h"

#include <GLES2/gl2ext.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "content/renderer/render_thread_impl.h"
#include "content/common/gpu/client/context_provider_command_buffer.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "content/renderer/media/webos/stream_texture_factory.h"
#include "content/public/renderer/render_frame.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSettings.h"
#include "third_party/WebKit/public/web/WebView.h"

#define INFO_LOG(format, ...)                                            \
  RAW_PMLOG_INFO("VideoFrameProviderVTGImpl", ":%04d " format, __LINE__, \
                 ##__VA_ARGS__)
#define DEBUG_LOG(format, ...)                                        \
  RAW_PMLOG_DEBUG("VideoFrameProviderVTGImpl:%04d " format, __LINE__, \
                  ##__VA_ARGS__)

namespace media {

bool VideoFrameProviderVTGImpl::stream_texture_busy_ = false;

VideoFrameProviderVTGImpl::VideoFrameProviderVTGImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& compositor_task_runner)
    : frame_(nullptr),
      client_(nullptr),
      is_suspended_(false),
      natural_video_size_(gfx::Size(1, 1)),
      video_frame_provider_client_(nullptr),
      main_loop_(base::MessageLoop::current()),
      compositor_task_runner_(compositor_task_runner),
      active_video_region_changed_(false),
      texture_id_(0),
      stream_id_(0),
      screen_orientation_(MEDIA_SCREEN_ORIENTATION_0) {}

VideoFrameProviderVTGImpl::~VideoFrameProviderVTGImpl() {
  SetVideoFrameProviderClient(NULL);
  DeleteStreamTexture();
}

void VideoFrameProviderVTGImpl::CreateStreamTextureProxyIfNeeded() {
  // Return if the proxy is already created, create it otherwise
  if (stream_texture_proxy_)
    return;

  // No factory to create proxy.
  if (!stream_texture_factory_)
    return;

  stream_texture_proxy_.reset(stream_texture_factory_->CreateProxy());

  if (stream_texture_proxy_ && video_frame_provider_client_) {
    stream_texture_proxy_->BindToLoop(stream_id_, video_frame_provider_client_,
                                      compositor_task_runner_);
  }
}

void VideoFrameProviderVTGImpl::CreateStreamTexture() {
  DCHECK(!stream_id_);
  DCHECK(!texture_id_);
  scoped_refptr<gpu::GpuChannelHost> gpu_channel_host =
      content::RenderThreadImpl::current()->EstablishGpuChannelSync(
          content::CAUSE_FOR_GPU_LAUNCH_VIDEODECODEACCELERATOR_INITIALIZE);

  if (!gpu_channel_host.get()) {
    INFO_LOG("Failed to establish GPU channel for media player");
    return;
  }

  scoped_refptr<content::ContextProviderCommandBuffer> context_provider =
      content::RenderThreadImpl::current()->SharedMainThreadContextProvider();

  if (!context_provider.get()) {
    INFO_LOG("Failed to get context3d for media player");
    return;
  }

  stream_texture_factory_ =
      content::RenderThreadImpl::current()->GetStreamTexureFactory();

  stream_id_ = stream_texture_factory_->CreateStreamTexture(
      GL_TEXTURE_EXTERNAL_OES, &texture_id_, &texture_mailbox_);

  if (stream_id_)
    CreateStreamTextureProxyIfNeeded();
}

void VideoFrameProviderVTGImpl::DeleteStreamTexture() {
  if (stream_id_) {
    stream_texture_busy_ = true;

    GLES2Interface* gl = stream_texture_factory_->ContextGL();
    gl->DeleteTextures(1, &texture_id_);
    texture_id_ = 0;
    texture_mailbox_ = gpu::Mailbox();
    stream_id_ = 0;
    gl->Flush();

    if (stream_texture_proxy_)
      stream_texture_proxy_.reset();

    stream_texture_factory_ = nullptr;
    active_video_region_ = blink::WebRect();
  }
}

void VideoFrameProviderVTGImpl::OnReleaseTexture(
    const scoped_refptr<content::StreamTextureFactory>& factories,
    const gpu::SyncToken& release_sync_token) {
  if (release_sync_token.HasData()) {
    GLES2Interface* gl = factories->ContextGL();
    gl->WaitSyncTokenCHROMIUM(release_sync_token.GetConstData());
    stream_texture_busy_ = false;
  }
}

void VideoFrameProviderVTGImpl::activeRegionChanged(
    const blink::WebRect& active_region) {
  if (active_video_region_ != active_region) {
    active_video_region_ = active_region;
    active_video_region_changed_ = true;
  }
}

void VideoFrameProviderVTGImpl::UpdateVideoFrame() {
  if (!current_frame_)
    return;

  if (is_suspended_ &&
      current_frame_->storage_type() != media::VideoFrame::STORAGE_BLACK) {
    current_frame_ = CreateVideoFrame(media::VideoFrame::STORAGE_BLACK);
    DeleteStreamTexture();
    return;
  }

  if (video_frame_provider_client_ == NULL)
    return;

  if (stream_texture_busy_)
    return;

  if (useVideoTexture() && active_video_region_changed_) {
    active_video_region_changed_ = false;
    if (current_frame_->storage_type() == media::VideoFrame::STORAGE_OPAQUE) {
      current_frame_ = CreateVideoFrame(media::VideoFrame::STORAGE_OPAQUE);
    }
  }

  if (useVideoTexture() && !active_video_region_.isEmpty() &&
      current_frame_->storage_type() != media::VideoFrame::STORAGE_OPAQUE) {
    CreateStreamTexture();
    current_frame_ = CreateVideoFrame(media::VideoFrame::STORAGE_OPAQUE);
  } else if (!useVideoTexture() &&
             current_frame_->storage_type() !=
                 media::VideoFrame::STORAGE_HOLE) {
    current_frame_ = CreateVideoFrame(media::VideoFrame::STORAGE_HOLE);
    DeleteStreamTexture();
  }
}

scoped_refptr<VideoFrame> VideoFrameProviderVTGImpl::CreateVideoFrame(
    media::VideoFrame::StorageType frame_storage_type) {
  DEBUG_LOG("%s:frame_storage_type=%d", __PRETTY_FUNCTION__,
            frame_storage_type);
  DVLOG(1) << __FUNCTION__ << "(" << frame_storage_type << ")";

  switch (frame_storage_type) {
    case media::VideoFrame::STORAGE_OPAQUE: {
      if (stream_texture_factory_)
        stream_texture_factory_->SetStreamTextureActiveRegion(
            stream_id_, active_video_region_);

      GLES2Interface* gl = stream_texture_factory_->ContextGL();
      GLuint texture_target = GL_TEXTURE_EXTERNAL_OES;
      const GLuint64 fence_sync = gl->InsertFenceSyncCHROMIUM();
      gl->Flush();

      gpu::SyncToken texture_mailbox_sync_token;
      gl->GenUnverifiedSyncTokenCHROMIUM(fence_sync,
                                         texture_mailbox_sync_token.GetData());
      if (texture_mailbox_sync_token.namespace_id() ==
          gpu::CommandBufferNamespace::IN_PROCESS) {
        // TODO(boliu): Remove this once Android WebView switches to IPC-based
        // command buffer for video.
        GLbyte* sync_tokens[] = {texture_mailbox_sync_token.GetData()};
        gl->VerifySyncTokensCHROMIUM(sync_tokens, arraysize(sync_tokens));
      }

      gpu::MailboxHolder holders[media::VideoFrame::kMaxPlanes] = {
          gpu::MailboxHolder(texture_mailbox_, texture_mailbox_sync_token,
                             texture_target)};

      return media::VideoFrame::WrapNativeTextures(
          media::PIXEL_FORMAT_ARGB, holders,
          media::BindToCurrentLoop(
              base::Bind(&VideoFrameProviderVTGImpl::OnReleaseTexture,
                         stream_texture_factory_)),
          gfx::Size(active_video_region_.width, active_video_region_.height),
          active_video_region_, natural_video_size_, base::TimeDelta());
    }
    case media::VideoFrame::STORAGE_HOLE:
      return media::VideoFrame::CreateHoleFrame(natural_video_size_);
    case media::VideoFrame::STORAGE_BLACK:
      return media::VideoFrame::CreateBlackFrame(natural_video_size_);
    default:
      return NULL;
  }
  return NULL;
}

bool VideoFrameProviderVTGImpl::useVideoTexture() const {
  return client_->useTexture() ||
         frame_->view()->settings()->forceVideoTexture() ||
         screen_orientation_ != MEDIA_SCREEN_ORIENTATION_0;
}

blink::WebMediaPlayerClient* VideoFrameProviderVTGImpl::GetClient() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DCHECK(client_);
  return client_;
}

void VideoFrameProviderVTGImpl::SetVideoFrameProviderClient(
    cc::VideoFrameProvider::Client* client) {
  // This is called from both the main renderer thread and the compositor
  // thread (when the main thread is blocked).
  if (video_frame_provider_client_)
    video_frame_provider_client_->StopUsingProvider();
  video_frame_provider_client_ = client;
}

void VideoFrameProviderVTGImpl::PutCurrentFrame() {}

scoped_refptr<media::VideoFrame> VideoFrameProviderVTGImpl::GetCurrentFrame() {
  return current_frame_;
}

bool VideoFrameProviderVTGImpl::UpdateCurrentFrame(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max) {
  NOTIMPLEMENTED();
  return false;
}

bool VideoFrameProviderVTGImpl::HasCurrentFrame() {
  return current_frame_ != nullptr;
}

void VideoFrameProviderVTGImpl::Repaint() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  GetClient()->repaint();
}

void VideoFrameProviderVTGImpl::setNaturalVideoSize(
    gfx::Size natural_video_size) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  natural_video_size_ =
      natural_video_size.IsEmpty() ? gfx::Size(1, 1) : natural_video_size;
}

void VideoFrameProviderVTGImpl::createVideoFrameHole() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

#if defined(VIDEO_HOLE)
  if (GetClient()->isVideo()) {
    current_frame_ = CreateVideoFrame(media::VideoFrame::STORAGE_HOLE);
  }
#endif
}

}  // namespace media
