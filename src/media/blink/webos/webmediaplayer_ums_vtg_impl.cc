// Copyright (c) 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/webmediaplayer_ums_vtg_impl.h"

#include <algorithm>
#include <limits>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "cc/layers/video_layer.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_params.h"
#include "media/blink/webmediaplayer_util.h"
#include "content/common/gpu/client/context_provider_command_buffer.h"
#include "content/public/renderer/render_frame.h"
#include "content/renderer/media/render_media_log.h"
#include "content/renderer/render_thread_impl.h"
#include "content/renderer/media/webos/stream_texture_factory.h"
#include "cc/blink/web_layer_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerSource.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSettings.h"
#include "third_party/WebKit/public/web/WebView.h"

#define INFO_LOG(format, ...)                                           \
  RAW_PMLOG_INFO("WebMediaPlayerUMSVTGImpl", ":%04d " format, __LINE__, \
                 ##__VA_ARGS__)
#define DEBUG_LOG(format, ...)                                       \
  RAW_PMLOG_DEBUG("WebMediaPlayerUMSVTGImpl:%04d " format, __LINE__, \
                  ##__VA_ARGS__)

using blink::WebCanvas;
using blink::WebMediaPlayer;
using blink::WebRect;
using blink::WebSize;
using blink::WebString;
using blink::WebMediaPlayerClient;
using media::PipelineStatus;
using media::WebMediaPlayerParams;

namespace {

class SyncTokenClientImpl : public media::VideoFrame::SyncTokenClient {
 public:
  explicit SyncTokenClientImpl(gpu::gles2::GLES2Interface* gl) : gl_(gl) {}
  ~SyncTokenClientImpl() override {}
  void GenerateSyncToken(gpu::SyncToken* sync_token) override {
    const GLuint64 fence_sync = gl_->InsertFenceSyncCHROMIUM();
    gl_->ShallowFlushCHROMIUM();
    gl_->GenSyncTokenCHROMIUM(fence_sync, sync_token->GetData());
  }
  void WaitSyncToken(const gpu::SyncToken& sync_token) override {
    gl_->WaitSyncTokenCHROMIUM(sync_token.GetConstData());
  }

 private:
  gpu::gles2::GLES2Interface* gl_;
};

}  // namespace

namespace media {

#define BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(function)      \
  (DCHECK(main_loop_->task_runner()->BelongsToCurrentThread()), \
   BindToCurrentLoop(base::Bind(                                \
       function, base::AsWeakPtr(this->video_frame_provider_vtg_))))

#define BIND_TO_RENDER_LOOP(function)                           \
  (DCHECK(main_loop_->task_runner()->BelongsToCurrentThread()), \
   BindToCurrentLoop(base::Bind(function, base::AsWeakPtr(this))))

WebMediaPlayerUMSVTGImpl::WebMediaPlayerUMSVTGImpl(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    base::WeakPtr<WebMediaPlayerDelegate> delegate,
    const WebMediaPlayerParams& params,
    blink::WebFloatPoint additional_contents_scale,
    const blink::WebString& app_id)
    : WebMediaPlayerUMS(frame,
                        client,
                        delegate,
                        params,
                        additional_contents_scale,
                        app_id),
      // Threaded compositing isn't enabled universally yet.
      compositor_task_runner_(params.compositor_task_runner()
                                  ? params.compositor_task_runner()
                                  : base::ThreadTaskRunnerHandle::Get()) {
  video_frame_provider_vtg_ =
      new VideoFrameProviderVTGImpl(compositor_task_runner_);
  video_frame_provider_vtg_->setWebLocalFrame(frame);
  video_frame_provider_vtg_->setWebMediaPlayerClient(client);

  if (GetClient()->isVideo()) {
#if defined(VIDEO_HOLE)
    video_frame_provider_vtg_->createVideoFrameHole();
#endif
  }
}

WebMediaPlayerUMSVTGImpl::~WebMediaPlayerUMSVTGImpl() {
  delete video_frame_provider_vtg_;
  video_frame_provider_vtg_ = nullptr;
}

void WebMediaPlayerUMSVTGImpl::LoadMedia() {
  umedia_client_->SetActiveRegionCB(BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(
      &VideoFrameProviderVTGImpl::activeRegionChanged));

  umedia_client_->load(
      GetClient()->isVideo(), is_local_source_, app_id_, url_,
      std::string(GetClient()->contentMIMEType().utf8().data()),
      std::string(GetClient()->referrer().utf8().data()),
      std::string(GetClient()->userAgent().utf8().data()),
      std::string(GetClient()->cookies().utf8().data()),
      std::string(GetClient()->contentMediaOption().utf8().data()),
      BIND_TO_RENDER_LOOP(
          &WebMediaPlayerUMSVTGImpl::OnPipelinePlaybackStateChanged),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::OnPipelineEnded),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::OnPipelineSeek),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::OnPipelineError),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::OnPipelineBufferingState),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::OnDurationChange),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::OnVideoSizeChange),
      BIND_TO_RENDER_LOOP(
          &WebMediaPlayerUMSVTGImpl::OnVideoDisplayWindowChange),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::updateUMSInfo),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMSVTGImpl::focusChanged));
}

bool WebMediaPlayerUMSVTGImpl::copyVideoTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* web_graphics_context,
    unsigned int texture,
    unsigned int internal_format,
    unsigned int type,
    bool premultiply_alpha,
    bool flip_y) {
  scoped_refptr<media::VideoFrame> video_frame;
  {
    base::AutoLock auto_lock(video_frame_provider_vtg_->getLock());
    video_frame = video_frame_provider_vtg_->getCurrentVideoFrame();
  }

  if (!video_frame.get() || !video_frame->HasTextures()) {
    return false;
  }

  const gpu::MailboxHolder& mailbox_holder = video_frame->mailbox_holder(0);
  if (mailbox_holder.texture_target != GL_TEXTURE_2D) {
    return false;
  }

// Since this method changes which texture is bound to the TEXTURE_2D target,
// ideally it would restore the currently-bound texture before returning.
// The cost of getIntegerv is sufficiently high, however, that we want to
// avoid it in user builds. As a result assume (below) that |texture| is
// bound when this method is called, and only verify this fact when
// DCHECK_IS_ON.
#if DCHECK_IS_ON()
  GLint bound_texture = 0;
  web_graphics_context->getIntegerv(GL_TEXTURE_BINDING_2D, &bound_texture);
  DCHECK_EQ(static_cast<GLuint>(bound_texture), texture);
#endif

  web_graphics_context->WaitSyncTokenCHROMIUM(
      mailbox_holder.sync_token.GetConstData());

  uint32_t src_texture = web_graphics_context->CreateAndConsumeTextureCHROMIUM(
      mailbox_holder.texture_target, mailbox_holder.mailbox.name);

  // Application itself needs to take care of setting the right flip_y
  // value down to get the expected result.
  // flip_y==true means to reverse the video orientation while
  // flip_y==false means to keep the intrinsic orientation.
  web_graphics_context->CopyTextureCHROMIUM(src_texture, texture,
                                            internal_format, type, flip_y,
                                            premultiply_alpha, false);

  web_graphics_context->DeleteTextures(1, &src_texture);

  // The flush() operation is not necessary here. It is kept since the
  // performance will be better when it is added than not.
  web_graphics_context->Flush();

  SyncTokenClientImpl client(web_graphics_context);
  video_frame->UpdateReleaseSyncToken(&client);
  return true;
}

void WebMediaPlayerUMSVTGImpl::updateVideo(const blink::WebRect& rect,
                                           bool fullscreen) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  video_frame_provider_vtg_->setNaturalVideoSize(
      umedia_client_->naturalVideoSize());

  if (!is_suspended_)
    video_frame_provider_vtg_->UpdateVideoFrame();

  if (pending_size_change_ || previous_video_rect_ != rect) {
    bool prev_rect_exists = true;

    if (previous_video_rect_.width <= 0 && previous_video_rect_.height <= 0)
      prev_rect_exists = false;

    bool forced = pending_size_change_;
    pending_size_change_ = false;
    previous_video_rect_ = rect;

    blink::WebRect inRect;
    display_window_in_rect_ = ScaleWebRect(inRect, additional_contents_scale_);
    display_window_out_rect_ = ScaleWebRect(rect, additional_contents_scale_);

    if (prev_rect_exists && video_frame_provider_vtg_->useVideoTexture())
      return;

    bool checked_fullscreen = fullscreen;
    bool clipping_rect = false;
    blink::WebRect display_window_out_rect = display_window_out_rect_;

    blink::WebRect display_rect(0, 0, display_resolution_.width(),
                                display_resolution_.height());


    blink::WebRect clip_rect =
        gfx::IntersectRects(display_window_out_rect, display_rect);
    if (clip_rect != display_window_out_rect)
      clipping_rect = true;

    if (fullscreen_ != checked_fullscreen)
      GetClient()->sizeChanged();

    fullscreen_ = checked_fullscreen;

    if (clipping_rect && !checked_fullscreen) {
      int moved_x = 0;
      int moved_y = 0;

      if (display_window_out_rect.x + display_window_out_rect.width <
          display_window_out_rect.width)
        moved_x = display_window_out_rect.x;
      else if (display_window_out_rect.x + display_window_out_rect.width >
               display_rect.width)
        moved_x = display_window_out_rect.x + display_window_out_rect.width -
                  display_rect.width;

      if (display_window_out_rect.y + display_window_out_rect.height <
          display_window_out_rect.height)
        moved_y = display_window_out_rect.y;
      else if (display_window_out_rect.y + display_window_out_rect.height >
               display_rect.height)
        moved_y = display_window_out_rect.y + display_window_out_rect.height -
                  display_rect.height;

      int clipped_x = (moved_x < 0) ? abs(moved_x) : 0;
      int clipped_y = (moved_y < 0) ? abs(moved_y) : 0;
      int clipped_width = (moved_x < display_window_out_rect.width)
                              ? display_window_out_rect.width - abs(moved_x)
                              : display_window_out_rect.width;
      int clipped_height = (moved_y < display_window_out_rect.height)
                               ? display_window_out_rect.height - abs(moved_y)
                               : display_window_out_rect.height;

      if (clipped_width > display_rect.width)
        clipped_width = display_rect.width;
      if (clipped_height > display_rect.height)
        clipped_height = display_rect.height;

      gfx::Size natural_size = umedia_client_->naturalVideoSize();
      if (natural_size == gfx::Size()) {
        previous_video_rect_ = blink::WebRect();
        return;
      }

      if (display_window_out_rect.width > 0 &&
          display_window_out_rect.height > 0) {
        display_window_in_rect_ = WebRect(
            clipped_x * natural_size.width() / display_window_out_rect.width,
            clipped_y * natural_size.height() / display_window_out_rect.height,
            clipped_width * natural_size.width() /
                display_window_out_rect.width,
            clipped_height * natural_size.height() /
                display_window_out_rect.height);
      }

      if (display_window_out_rect.x < 0)
        display_window_out_rect.x = 0;
      if (display_window_out_rect.y < 0)
        display_window_out_rect.y = 0;

      if (display_window_out_rect.x > display_rect.width)
        display_window_out_rect.x = display_rect.width;
      if (display_window_out_rect.y > display_rect.height)
        display_window_out_rect.y = display_rect.height;

      display_window_out_rect.width = clipped_width;
      display_window_out_rect.height = clipped_height;

      // Modified inner rect only
      if (clipped_width == display_rect.width ||
          clipped_height == display_rect.height) {
        forced = true;
        checked_fullscreen = true;
      }

      display_window_out_rect_ = display_window_out_rect;
    }

    // check offscreen video visibility
    if (clipping_rect && !checked_fullscreen) {
      if (((blink::WebRect)gfx::IntersectRects(display_window_out_rect,
                                               display_rect))
              .isEmpty() ||
          display_window_out_rect.isEmpty()) {
        is_video_offscreen_ = true;
        umedia_client_->setVisibility(false);

        return;
      }
      if (is_video_offscreen_) {
        is_video_offscreen_ = false;
        umedia_client_->setVisibility(true);
      }
    }
#if defined(PLATFORM_APOLLO)
    display_window_out_rect.y +=
        delegate_ ? delegate_->GetRenderViewBounds().y() : 0;
#endif

    if (video_frame_provider_vtg_->useVideoTexture()) {
      if (video_frame_provider_vtg_->getActiveVideoRegion().isEmpty())
        umedia_client_->switchToAutoLayout();
    } else {
      umedia_client_->setDisplayWindow(display_window_out_rect,
                                       display_window_in_rect_,
                                       checked_fullscreen, forced);
    }
  }
}

void WebMediaPlayerUMSVTGImpl::OnPipelineBufferingState(
    UMediaClientImpl::BufferingState buffering_state) {
  DVLOG(1) << __FUNCTION__ << "(" << buffering_state << ")";

  switch (buffering_state) {
    case UMediaClientImpl::kHaveMetadata:
      video_frame_provider_vtg_->setCurrentVideoFrame(
          media::VideoFrame::CreateHoleFrame(gfx::Size(1, 1)));
      video_frame_provider_vtg_->setNaturalVideoSize(
          umedia_client_->naturalVideoSize());
      video_frame_provider_vtg_->UpdateVideoFrame();

      if (ready_state_ >= WebMediaPlayer::ReadyStateHaveMetadata)
        break;

      SetReadyState(WebMediaPlayer::ReadyStateHaveMetadata);

      if (hasVideo()) {
        if (!video_weblayer_) {
          video_weblayer_.reset(
              new cc_blink::WebLayerImpl(cc::VideoLayer::Create(
                  video_frame_provider_vtg_, media::VIDEO_ROTATION_0)));
          GetClient()->setWebLayer(video_weblayer_.get());
        }
      }
      break;
    case UMediaClientImpl::kPreloadCompleted:
      SetReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
      break;
    case UMediaClientImpl::kLoadCompleted:
      video_frame_provider_vtg_->setCurrentVideoFrame(
          media::VideoFrame::CreateHoleFrame(gfx::Size(1, 1)));
      // Only transition to ReadyStateHaveEnoughData if we don't have
      // any pending seeks because the transition can cause Blink to
      // report that the most recent seek has completed.
      if (!pending_seek_)
        SetReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
      break;
    case UMediaClientImpl::kWebOSBufferingStart:
      SetReadyState(WebMediaPlayer::ReadyStateHaveCurrentData);
      break;
    case UMediaClientImpl::kWebOSBufferingEnd:
      SetReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
      break;
  }

  // Repaint to trigger UI update.
  Repaint();
}

void WebMediaPlayerUMSVTGImpl::OnVideoDisplayWindowChange() {
  if (video_frame_provider_vtg_->useVideoTexture())
    return;

  bool forced = true;

  blink::WebRect display_window_out_rect = display_window_out_rect_;

  if (video_frame_provider_vtg_->useVideoTexture()) {
    if (video_frame_provider_vtg_->getActiveVideoRegion().isEmpty())
      umedia_client_->switchToAutoLayout();
  } else {
    umedia_client_->setDisplayWindow(
        display_window_out_rect, display_window_in_rect_, fullscreen_, forced);
  }
}

void WebMediaPlayerUMSVTGImpl::suspend() {
  if (is_suspended_)
    return;

  is_suspended_ = true;
  status_on_suspended_ = paused() ? PausedStatus : PlayingStatus;
  if (status_on_suspended_ == PlayingStatus) {
    pause();
    GetClient()->playbackStateChanged();
  }
  if (hasVideo()) {
    video_frame_provider_vtg_->setNaturalVideoSize(
        umedia_client_->naturalVideoSize());
    video_frame_provider_vtg_->UpdateVideoFrame();
    Repaint();
  }
  umedia_client_->suspend();
}

void WebMediaPlayerUMSVTGImpl::resume() {
  if (!is_suspended_)
    return;

  is_suspended_ = false;
  umedia_client_->resume();

  if (hasVideo()) {
    video_frame_provider_vtg_->setNaturalVideoSize(
        umedia_client_->naturalVideoSize());
    video_frame_provider_vtg_->UpdateVideoFrame();
    Repaint();
  }

  if (status_on_suspended_ == PlayingStatus) {
    previous_video_rect_ = blink::WebRect(-1, -1, -1, -1);
    play();
    GetClient()->playbackStateChanged();
    status_on_suspended_ = UnknownStatus;
  }
}

void WebMediaPlayerUMSVTGImpl::OnScreenOrientationUpdate(
    ScreenOrientationType screen_orientation) {
  video_frame_provider_vtg_->setScreenOrientation(screen_orientation);
  if (video_frame_provider_vtg_->useVideoTexture()) {
    umedia_client_->switchToAutoLayout();
  } else {
    video_frame_provider_vtg_->setActiveVideoRegionChanged(false);
    video_frame_provider_vtg_->setActiveVideoRegion(blink::WebRect());
    umedia_client_->setDisplayWindow(
        display_window_out_rect_, display_window_in_rect_, fullscreen_, true);
  }
}

}  // namespace media
