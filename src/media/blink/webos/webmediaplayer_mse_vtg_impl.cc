// Copyright (c) 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/webmediaplayer_mse_vtg_impl.h"

#include "base/metrics/histogram.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/video_layer.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"

#define INFO_LOG(format, ...)                                           \
  RAW_PMLOG_INFO("WebMediaPlayerMSEVTGImpl", ":%04d " format, __LINE__, \
                 ##__VA_ARGS__)
#define DEBUG_LOG(format, ...)                                       \
  RAW_PMLOG_DEBUG("WebMediaPlayerMSEVTGImpl:%04d " format, __LINE__, \
                  ##__VA_ARGS__)

namespace media {

#define BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(function) \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()),    \
   BindToCurrentLoop(base::Bind(                           \
       function, base::AsWeakPtr(this->video_frame_provider_vtg_))))

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   BindToCurrentLoop(base::Bind(function, base::AsWeakPtr(this))))

WebMediaPlayerMSEVTGImpl::WebMediaPlayerMSEVTGImpl(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
    std::unique_ptr<media::RendererFactory> renderer_factory,
    linked_ptr<media::UrlIndex> url_index,
    const media::WebMediaPlayerParams& params,
    blink::WebFloatPoint additional_contents_scale,
    const blink::WebString& app_id)
    : media::WebMediaPlayerMSE(frame,
                               client,
                               encrypted_client,
                               delegate,
                               std::move(renderer_factory),
                               url_index,
                               params,
                               additional_contents_scale,
                               app_id),
      checked_fullscreen_(false) {
  video_frame_provider_vtg_ =
      new VideoFrameProviderVTGImpl(compositor_task_runner_);
  video_frame_provider_vtg_->setWebLocalFrame(frame);
  video_frame_provider_vtg_->setWebMediaPlayerClient(client);
  video_frame_provider_vtg_->createVideoFrameHole();

  // Create MediaApis Wrapper
  media_apis_wrapper_ = new media::MediaAPIsWrapper(
      media_task_runner_, client_->isVideo(), app_id_,
      BIND_TO_RENDER_LOOP_VIDEO_FRAME_PROVIDER(
          &VideoFrameProviderVTGImpl::activeRegionChanged),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerMSEVTGImpl::OnError));

  renderer_factory_->SetMediaAPIsWrapper(media_apis_wrapper_);
  pipeline_.SetMediaAPIsWrapper(media_apis_wrapper_);

#if defined(ENABLE_LG_SVP)
  if (params.initial_cdm()) {
    const std::string ks =
        media::ToWebContentDecryptionModuleImpl(params.initial_cdm())
            ->GetKeySystem();
    DEBUG_LOG("Setting key_system to media APIs = '%s'", ks.c_str());
    media_apis_wrapper_->setKeySystem(ks);
  }
#endif
}

WebMediaPlayerMSEVTGImpl::~WebMediaPlayerMSEVTGImpl() {
}

void WebMediaPlayerMSEVTGImpl::updateVideo(const blink::WebRect& rect,
                                           bool fullScreen) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  video_frame_provider_vtg_->setNaturalVideoSize(
      pipeline_metadata_.natural_size);

  if (!is_suspended_)
    video_frame_provider_vtg_->UpdateVideoFrame();

  scaled_rect_ = scaleWebRect(rect, additional_contents_scale_);
  if (previous_video_rect_ != scaled_rect_) {
    if (pipeline_metadata_.natural_size == gfx::Size())
      return;

    bool prev_rect_exists = true;
    if (previous_video_rect_.width <= 0 && previous_video_rect_.height <= 0)
      prev_rect_exists = false;

    previous_video_rect_ = scaled_rect_;

    checked_fullscreen_ = fullScreen;
    bool clipping_rect = false;

    scaled_in_rect_ = blink::WebRect();
    blink::WebRect display_rect(0, 0, display_resolution_.width(),
                                display_resolution_.height());
    blink::WebRect clip_rect = gfx::IntersectRects(scaled_rect, display_rect);

    if (clip_rect != scaled_rect_)
      clipping_rect = true;

    if (clipping_rect && !checked_fullscreen_) {
      int moved_x = 0;
      int moved_y = 0;

      if (scaled_rect_.x + scaled_rect_.width < scaled_rect_.width)
        moved_x = scaled_rect_.x;
      else if (scaled_rect_.x + scaled_rect_.width > display_rect.width)
        moved_x = scaled_rect_.x + scaled_rect_.width - display_rect.width;

      if (scaled_rect_.y + scaled_rect_.height < scaled_rect_.height)
        moved_y = scaled_rect_.y;
      else if (scaled_rect_.y + scaled_rect_.height > display_rect.height)
        moved_y = scaled_rect_.y + scaled_rect_.height - display_rect.height;

      int clipped_x = (moved_x < 0) ? abs(moved_x) : 0;
      int clipped_y = (moved_y < 0) ? abs(moved_y) : 0;
      int clipped_width = (moved_x < scaled_rect_.width)
                              ? scaled_rect_.width - abs(moved_x)
                              : scaled_rect_.width;
      int clipped_height = (moved_y < scaled_rect_.height)
                               ? scaled_rect_.height - abs(moved_y)
                               : scaled_rect_.height;

      if (clipped_width > display_rect.width)
        clipped_width = display_rect.width;
      if (clipped_height > display_rect.height)
        clipped_height = display_rect.height;

      if (scaled_rect_.width > 0 && scaled_rect_.height > 0) {
        scaled_in_rect_ = gfx::Rect(
            clipped_x * pipeline_metadata_.natural_size.width() /
                scaled_rect_.width,
            clipped_y * pipeline_metadata_.natural_size.height() /
                scaled_rect_.height,
            clipped_width * pipeline_metadata_.natural_size.width() /
                scaled_rect_.width,
            clipped_height * pipeline_metadata_.natural_size.height() /
                scaled_rect_.height);
      }

      if (scaled_rect_.x < 0)
        scaled_rect_.x = 0;
      if (scaled_rect_.y < 0)
        scaled_rect_.y = 0;

      if (scaled_rect_.x > display_rect.width)
        scaled_rect_.x = display_rect.width;
      if (scaled_rect_.y > display_rect.height)
        scaled_rect_.y = display_rect.height;

      scaled_rect_.width = clipped_width;
      scaled_rect_.height = clipped_height;
    }

    if (media_apis_wrapper_) {
      if (prev_rect_exists && video_frame_provider_vtg_->useVideoTexture())
        return;

      if (video_frame_provider_vtg_->useVideoTexture()) {
        if (video_frame_provider_vtg_->getActiveVideoRegion().isEmpty()) {
          media_apis_wrapper_->SwitchToAutoLayout();
        }
      } else {
        media_apis_wrapper_->SetDisplayWindow(scaled_rect_, scaled_in_rect_,
                                              checked_fullscreen_);
      }
    }
  }
}

void WebMediaPlayerMSEVTGImpl::suspend() {
  if (is_suspended_)
    return;

  is_suspended_ = true;
  must_suspend_ = true;

  status_on_suspended_ =
      (pipeline_.GetPlaybackRate() == 0.0f) ? PausedStatus : PlayingStatus;

  if (status_on_suspended_ == PlayingStatus) {
    pause();
  }

  if (media_apis_wrapper_.get()) {
    if (hasVideo()) {
      video_frame_provider_vtg_->setNaturalVideoSize(
          pipeline_metadata_.natural_size);
      video_frame_provider_vtg_->UpdateVideoFrame();
      video_frame_provider_vtg_->Repaint();
    }
    media_apis_wrapper_->Suspend();
  }
}

void WebMediaPlayerMSEVTGImpl::resume() {
  if (!is_suspended_)
    return;

  is_suspended_ = false;
  must_suspend_ = false;

  media::MediaAPIsWrapper::RestorePlaybackMode restore_playback_mode;

  restore_playback_mode = (status_on_suspended_ == PausedStatus)
                              ? media::MediaAPIsWrapper::RESTORE_PAUSED
                              : media::MediaAPIsWrapper::RESTORE_PLAYING;

  if (media_apis_wrapper_.get()) {
    if (hasVideo()) {
      video_frame_provider_vtg_->setNaturalVideoSize(
          pipeline_metadata_.natural_size);
      video_frame_provider_vtg_->UpdateVideoFrame();
      video_frame_provider_vtg_->Repaint();
    }

    media_apis_wrapper_->Resume(paused_time_, restore_playback_mode);

    // to force SetVideoWindow on resume
    previous_video_rect_ = blink::WebRect(-1, -1, -1, -1);
    client_->checkBounds();
  }

  if (status_on_suspended_ == PausedStatus) {
    pause();
    status_on_suspended_ = UnknownStatus;
  } else {
    play();
  }
}

void WebMediaPlayerMSEVTGImpl::OnMetadata(PipelineMetadata metadata) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  pipeline_metadata_ = metadata;

  UMA_HISTOGRAM_ENUMERATION("Media.VideoRotation", metadata.video_rotation,
                            VIDEO_ROTATION_MAX + 1);
  SetReadyState(WebMediaPlayer::ReadyStateHaveMetadata);

  if (hasVideo()) {
    video_frame_provider_vtg_->setCurrentVideoFrame(
        media::VideoFrame::CreateHoleFrame(gfx::Size(1, 1)));
    video_frame_provider_vtg_->setNaturalVideoSize(
        pipeline_metadata_.natural_size);
    video_frame_provider_vtg_->UpdateVideoFrame();

    if (pipeline_metadata_.video_rotation == VIDEO_ROTATION_90 ||
        pipeline_metadata_.video_rotation == VIDEO_ROTATION_270) {
      gfx::Size size = pipeline_metadata_.natural_size;
      pipeline_metadata_.natural_size = gfx::Size(size.height(), size.width());
    }

    if (fullscreen_ && surface_manager_)
      surface_manager_->NaturalSizeChanged(pipeline_metadata_.natural_size);

    DCHECK(!video_weblayer_);

    video_weblayer_.reset(new cc_blink::WebLayerImpl(cc::VideoLayer::Create(
        video_frame_provider_vtg_, pipeline_metadata_.video_rotation)));

    video_weblayer_->layer()->SetContentsOpaque(opaque_);
    video_weblayer_->SetContentsOpaqueIsFixed(true);
    client_->setWebLayer(video_weblayer_.get());
  }

  UpdatePlayState();
}

static void GetCurrentFrameAndSignal(VideoFrameCompositor* compositor,
                                     scoped_refptr<VideoFrame>* video_frame_out,
                                     base::WaitableEvent* event) {
  TRACE_EVENT0("media", "GetCurrentFrameAndSignal");
  *video_frame_out = compositor->GetCurrentFrameAndUpdateIfStale();
  event->Signal();
}

scoped_refptr<VideoFrame>
WebMediaPlayerMSEVTGImpl::GetCurrentFrameFromCompositor() {
  TRACE_EVENT0("media", "WebMediaPlayerMSEVTGImpl::GetCurrentFrameFromCompositor");

  if (video_frame_provider_vtg_->useVideoTexture()) {
    scoped_refptr<VideoFrame> video_frame;
    video_frame = video_frame_provider_vtg_->GetCurrentFrame();
    return video_frame;
  } else {
    return WebMediaPlayerImpl::GetCurrentFrameFromCompositor();
  }
}

void WebMediaPlayerMSEVTGImpl::OnScreenOrientationUpdate(
    ScreenOrientationType screen_orientation) {
  video_frame_provider_vtg_->setScreenOrientation(screen_orientation);
  if (video_frame_provider_vtg_->useVideoTexture()) {
    media_apis_wrapper_->SwitchToAutoLayout();
  } else {
    video_frame_provider_vtg_->setActiveVideoRegionChanged(false);
    video_frame_provider_vtg_->setActiveVideoRegion(blink::WebRect());
    media_apis_wrapper_->SetDisplayWindow(scaled_rect_, scaled_in_rect_,
                                          checked_fullscreen_);
    video_frame_provider_vtg_->createVideoFrameHole();
  }
}

}  // namespace media
