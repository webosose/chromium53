// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/webmediaplayer_mse.h"

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/renderer_factory.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "media/blink/webcontentdecryptionmodule_impl.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerClient.h"

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("WebMediaPlayerMSE", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("WebMediaPlayerMSE:%04d " format, __LINE__, ##__VA_ARGS__)

namespace media {

namespace {

static const int64_t kPaintTimerInterval = 400;
static const int64_t kPaintTimerIntervalForBrowser = 100;

}  // namespace

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   BindToCurrentLoop(base::Bind(function, base::AsWeakPtr(this))))

WebMediaPlayerMSE::WebMediaPlayerMSE(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
    std::unique_ptr<media::RendererFactory> renderer_factory,
    linked_ptr<media::UrlIndex> url_index,
    const media::WebMediaPlayerParams& params,
    blink::WebFloatPoint additional_contents_scale,
    const blink::WebString& app_id)
    : media::WebMediaPlayerImpl(frame,
                                client,
                                encrypted_client,
                                delegate,
                                std::move(renderer_factory),
                                url_index,
                                params),
      additional_contents_scale_(additional_contents_scale),
      app_id_(app_id.utf8().data()),
      status_on_suspended_(UnknownStatus),
      is_suspended_(false) {
  previous_video_rect_ = blink::WebRect(-1, -1, -1, -1);

  // Use the null sink for our MSE player
  audio_source_provider_ = new media::WebAudioSourceProviderImpl(
      new media::NullAudioSink(media_task_runner_));

  // Create MediaApis Wrapper
  media_apis_wrapper_ = new media::MediaAPIsWrapper(
      media_task_runner_, client_->isVideo(), app_id_,
      BIND_TO_RENDER_LOOP(&WebMediaPlayerMSE::OnError));

  renderer_factory_->SetMediaAPIsWrapper(media_apis_wrapper_);
  pipeline_.SetMediaAPIsWrapper(media_apis_wrapper_);

#if defined(ENABLE_LG_SVP)
  if (params.initial_cdm()) {
    const std::string ks = media::ToWebContentDecryptionModuleImpl(params.initial_cdm())->GetKeySystem();
    DEBUG_LOG("Setting key_system to media APIs = '%s'", ks.c_str());
    media_apis_wrapper_->setKeySystem(ks);
  }
#endif

  display_resolution_.SetSize(
      client_->displayResolution().width * additional_contents_scale_.x,
      client_->displayResolution().height * additional_contents_scale_.y);
}

WebMediaPlayerMSE::~WebMediaPlayerMSE() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (paintTimer_.IsRunning())
      paintTimer_.Stop();

  if (media_apis_wrapper_.get())
    media_apis_wrapper_->Finalize();
}

void WebMediaPlayerMSE::load(LoadType load_type,
                             const blink::WebMediaPlayerSource& source,
                             CORSMode cors_mode) {
  // call base-class implementation
  media::WebMediaPlayerImpl::load(load_type, source, cors_mode);

  // For updating video position infomation
  if (!paintTimer_.IsRunning()) {
    uint64_t paint_interval_time = kPaintTimerInterval;
    paintTimer_.Start(FROM_HERE,
        base::TimeDelta::FromMilliseconds(paint_interval_time),
        base::Bind(&WebMediaPlayerMSE::paintTimerFired,
                   base::Unretained(this)));
  }
}

void WebMediaPlayerMSE::play() {
  DEBUG_LOG("play()");
  DVLOG(1) << __FUNCTION__;
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (must_suspend_) {
    status_on_suspended_ = PlayingStatus;
    return;
  }

  // call base-class implementation
  media::WebMediaPlayerImpl::play();
}

void WebMediaPlayerMSE::pause() {
  DEBUG_LOG("pause()");
  media::WebMediaPlayerImpl::pause();
}


void WebMediaPlayerMSE::setRate(double rate) {
  DVLOG(1) << __FUNCTION__ << "(" << rate << ")";
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  if (must_suspend_)
    return;

  // call base-class implementation
  media::WebMediaPlayerImpl::setRate(rate);
}

void WebMediaPlayerMSE::setVolume(double volume) {
  // call base-class implementation
  media::WebMediaPlayerImpl::setVolume(volume);
}

void WebMediaPlayerMSE::updateVideo(
    const blink::WebRect& rect, bool fullScreen) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());

  blink::WebRect scaled_rect = scaleWebRect(rect, additional_contents_scale_);
  if (previous_video_rect_ != scaled_rect) {
    if (pipeline_metadata_.natural_size == gfx::Size())
      return;
    previous_video_rect_ = scaled_rect;

    bool checked_fullscreen = fullScreen;
    bool clipping_rect = false;

    blink::WebRect scaled_in_rect;
    blink::WebRect display_rect(0, 0,
        display_resolution_.width(), display_resolution_.height());

    scaled_rect.y = scaled_rect.y + (delegate_?delegate_->GetRenderViewBounds().y():0);

    blink::WebRect clip_rect = gfx::IntersectRects(scaled_rect, display_rect);
    if (clip_rect != scaled_rect)
      clipping_rect = true;

    if (clipping_rect && !checked_fullscreen) {
      int moved_x = 0;
      int moved_y = 0;

      if (scaled_rect.x + scaled_rect.width < scaled_rect.width)
        moved_x = scaled_rect.x;
      else if (scaled_rect.x + scaled_rect.width > display_rect.width)
        moved_x = scaled_rect.x + scaled_rect.width - display_rect.width;

      if (scaled_rect.y + scaled_rect.height < scaled_rect.height)
        moved_y = scaled_rect.y;
      else if (scaled_rect.y + scaled_rect.height > display_rect.height)
        moved_y = scaled_rect.y + scaled_rect.height - display_rect.height;

      int clipped_x = (moved_x < 0) ? abs(moved_x) : 0;
      int clipped_y = (moved_y < 0) ? abs(moved_y) : 0;
      int clipped_width = (moved_x < scaled_rect.width) ?
          scaled_rect.width - abs(moved_x) : scaled_rect.width;
      int clipped_height = (moved_y < scaled_rect.height) ?
          scaled_rect.height - abs(moved_y) : scaled_rect.height;

      if (clipped_width > display_rect.width)
        clipped_width = display_rect.width;
      if (clipped_height > display_rect.height)
        clipped_height = display_rect.height;

      if (scaled_rect.width > 0 && scaled_rect.height > 0) {
        scaled_in_rect = gfx::Rect(
            clipped_x *
                pipeline_metadata_.natural_size.width() / scaled_rect.width,
            clipped_y *
                pipeline_metadata_.natural_size.height() / scaled_rect.height,
            clipped_width *
                pipeline_metadata_.natural_size.width() / scaled_rect.width,
            clipped_height *
                pipeline_metadata_.natural_size.height() / scaled_rect.height);
      }

      if (scaled_rect.x < 0)
        scaled_rect.x = 0;
      if (scaled_rect.y < 0)
        scaled_rect.y = 0;

      if (scaled_rect.x > display_rect.width)
        scaled_rect.x = display_rect.width;
      if (scaled_rect.y > display_rect.height)
        scaled_rect.y = display_rect.height;

      scaled_rect.width = clipped_width;
      scaled_rect.height = clipped_height;
    }

    if (media_apis_wrapper_)
      media_apis_wrapper_->SetDisplayWindow(scaled_rect, scaled_in_rect,
                                            checked_fullscreen);
  }
}

void WebMediaPlayerMSE::setContentDecryptionModule(
    blink::WebContentDecryptionModule* cdm,
    blink::WebContentDecryptionModuleResult result) {
  DEBUG_LOG("%s", __FUNCTION__);

  DCHECK(main_task_runner_->BelongsToCurrentThread());

  // set the key system for the Media APIs wrapper
#if defined(ENABLE_LG_SVP)
  if (cdm && media_apis_wrapper_.get()) {
    const std::string ks = media::ToWebContentDecryptionModuleImpl(cdm)->GetKeySystem();
    DEBUG_LOG("Setting key_system to media APIs = '%s'", ks.c_str());
    media_apis_wrapper_->setKeySystem(ks);
  }
#endif

  // call base-class implementation
  media::WebMediaPlayerImpl::setContentDecryptionModule(cdm, result);
}

void WebMediaPlayerMSE::suspend() {
  if (is_suspended_)
    return;

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  is_suspended_ = true;
  must_suspend_ = true;

  status_on_suspended_ =
      (pipeline_.GetPlaybackRate() == 0.0f) ? PausedStatus : PlayingStatus;

  if (status_on_suspended_ == PlayingStatus) {
    pause();
  }

  if (media_apis_wrapper_.get())
    media_apis_wrapper_->Suspend();
}

void WebMediaPlayerMSE::resume() {
  if (!is_suspended_)
    return;

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  is_suspended_ = false;
  must_suspend_ = false;

  media::MediaAPIsWrapper::RestorePlaybackMode restore_playback_mode;

  restore_playback_mode = (status_on_suspended_ == PausedStatus)
                        ? media::MediaAPIsWrapper::RESTORE_PAUSED
                        : media::MediaAPIsWrapper::RESTORE_PLAYING
                        ;

  if (media_apis_wrapper_.get()) {
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

// helpers
blink::WebRect WebMediaPlayerMSE::scaleWebRect(
    const blink::WebRect& rect, blink::WebFloatPoint scale) {
  blink::WebRect scaledRect;

  scaledRect.x = rect.x * scale.x;
  scaledRect.y = rect.y * scale.y;
  scaledRect.width = rect.width * scale.x;
  scaledRect.height = rect.height * scale.y;

  return scaledRect;
}

void WebMediaPlayerMSE::paintTimerFired() {
  client_->checkBounds();
}

}  // namespace media

