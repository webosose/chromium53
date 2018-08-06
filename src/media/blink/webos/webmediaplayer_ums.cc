// Copyright (c) 2014-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/webmediaplayer_ums.h"

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
#include "content/renderer/media/render_media_log.h"
#include "cc/blink/web_layer_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"
#include "media/base/video_frame.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerSource.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/platform/WebURL.h"

/* uncomment the following line to turn on debug traces for webmediaplayer_ums */
/* #define DEBUG_WEBMEDIAPLAYER_UMS */
#ifdef DEBUG_WEBMEDIAPLAYER_UMS
#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("WebMediaPlayerUMS", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("WebMediaPlayerUMS:%04d " format, __LINE__, ##__VA_ARGS__)
#else
#define INFO_LOG(format, ...)
#define DEBUG_LOG(format, ...)
#endif /* #ifdef DEBUG_WEBMEDIAPLAYER_UMS */

using blink::WebCanvas;
using blink::WebMediaPlayer;
using blink::WebRect;
using blink::WebSize;
using blink::WebString;
using blink::WebMediaPlayerClient;
using media::PipelineStatus;
using media::WebMediaPlayerParams;

namespace {

// Limits the range of playback rate.
const double kMinRate = -16.0;
const double kMaxRate = 16.0;

static const int64_t kPaintTimerInterval = 400;

static const int64_t kPaintTimerIntervalForBrowser = 100;

}  // namespace

namespace media {

#define BIND_TO_RENDER_LOOP(function) \
  (DCHECK(main_loop_->task_runner()->BelongsToCurrentThread()), \
  media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

WebMediaPlayerUMS::WebMediaPlayerUMS(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    base::WeakPtr<WebMediaPlayerDelegate> delegate,
    const WebMediaPlayerParams& params,
    blink::WebFloatPoint additional_contents_scale,
    const blink::WebString& app_id)
    : frame_(frame),
      network_state_(WebMediaPlayer::NetworkStateEmpty),
      ready_state_(WebMediaPlayer::ReadyStateHaveNothing),
      main_loop_(base::MessageLoop::current()),
      media_task_runner_(params.media_task_runner()),
      paused_(true),
      seeking_(false),
      playback_rate_(0.0f),
      pending_seek_(false),
      pending_seek_seconds_(0.0f),
      client_(client),
      delegate_(delegate),
      defer_load_cb_(params.defer_load_cb()),
      media_log_(params.media_log()),
      is_local_source_(false),
      supports_save_(true),
      starting_(false),
      is_suspended_(false),
      status_on_suspended_(UnknownStatus),
      pending_size_change_(false),
      video_frame_provider_client_(NULL),
      fullscreen_(false),
      additional_contents_scale_(additional_contents_scale),
      is_video_offscreen_(false),
      app_id_(app_id.utf8().data()) {
  if (delegate_)
    delegate_id_ = delegate_->AddObserver(this);

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_CREATED));

  // We want to be notified of |main_loop_| destruction.
  base::MessageLoop::current()->AddDestructionObserver(this);

  // Use the null sink if no sink was provided.
  audio_source_provider_ = new WebAudioSourceProviderImpl(
      params.audio_renderer_sink().get()
      ? params.audio_renderer_sink()
      : new media::NullAudioSink(media_task_runner_));

  umedia_client_ = new UMediaClientImpl(media_task_runner_);

  display_resolution_.SetSize(
      GetClient()->displayResolution().width * additional_contents_scale_.x,
      GetClient()->displayResolution().height * additional_contents_scale_.y);
}

WebMediaPlayerUMS::~WebMediaPlayerUMS() {
  SetVideoFrameProviderClient(NULL);
  GetClient()->setWebLayer(NULL);

  if (paintTimer_.IsRunning())
    paintTimer_.Stop();

  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_DESTROYED));

  if (delegate_) {
    delegate_->PlayerGone(delegate_id_);
    delegate_->RemoveObserver(delegate_id_);
  }

  // Remove destruction observer if we're being destroyed but the main thread is
  // still running.
  if (base::MessageLoop::current())
    base::MessageLoop::current()->RemoveDestructionObserver(this);

  delete umedia_client_;
}

void WebMediaPlayerUMS::load(LoadType load_type, const blink::WebMediaPlayerSource& src,
                             CORSMode cors_mode) {
  if (!defer_load_cb_.is_null()) {
    defer_load_cb_.Run(base::Bind(
        &WebMediaPlayerUMS::DoLoad, AsWeakPtr(), load_type, src.getAsURL(), cors_mode));
    return;
  }
  DoLoad(load_type, src.getAsURL(), cors_mode);
}

void WebMediaPlayerUMS::DoLoad(LoadType load_type,
                                const blink::WebURL& url,
                                CORSMode cors_mode) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  // We should use MediaInfoLoader for all URLs but because of missing
  // scheme handlers in WAM we use it only for file scheme for now.
  // By using MediaInfoLoader url gets passed to network delegate which
  // does proper whitelist filtering for local file access.
  GURL mediaUrl(url);

  DEBUG_LOG("Loading %s", mediaUrl.spec().c_str());

  if (mediaUrl.SchemeIsFile() || mediaUrl.SchemeIsFileSystem()) {
    info_loader_.reset(new MediaInfoLoader(
        mediaUrl,
        base::Bind(&WebMediaPlayerUMS::DidLoadMediaInfo, AsWeakPtr())));
    info_loader_->Start(frame_);

    DEBUG_LOG("file url");
    SetNetworkState(WebMediaPlayer::NetworkStateLoading);
    SetReadyState(WebMediaPlayer::ReadyStateHaveNothing);
  } else {
    DEBUG_LOG("non-file url");
    SetNetworkState(WebMediaPlayer::NetworkStateLoading);
    SetReadyState(WebMediaPlayer::ReadyStateHaveNothing);
    DidLoadMediaInfo(true, mediaUrl);
  }
}

void WebMediaPlayerUMS::DidLoadMediaInfo(bool ok, const GURL& url) {

  DEBUG_LOG("%s - %d %s", __FUNCTION__, ok, url.spec().c_str());

  if (!ok) {
    info_loader_.reset();
    DEBUG_LOG("%s - Network Error", __FUNCTION__);
    SetNetworkState(WebMediaPlayer::NetworkStateNetworkError);
    return;
  }

  media_log_->AddEvent(media_log_->CreateLoadEvent(url.spec()));
  paused_ = false;
  is_local_source_ = url.SchemeIs("file");
  url_ = url.spec();

  LoadMedia();

  if (!paintTimer_.IsRunning()) {
    uint64_t paint_interval_time = kPaintTimerInterval;
    paintTimer_.Start(FROM_HERE,
        base::TimeDelta::FromMilliseconds(paint_interval_time),
        base::Bind(&WebMediaPlayerUMS::paintTimerFired,
                   base::Unretained(this)));
  }
}

void WebMediaPlayerUMS::LoadMedia() {

  DEBUG_LOG("%s", __FUNCTION__);

  umedia_client_->load(
      GetClient()->isVideo(), is_local_source_, app_id_, url_,
      std::string(GetClient()->contentMIMEType().utf8().data()),
      std::string(GetClient()->referrer().utf8().data()),
      std::string(GetClient()->userAgent().utf8().data()),
      std::string(GetClient()->cookies().utf8().data()),
      std::string(GetClient()->contentMediaOption().utf8().data()),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnPipelinePlaybackStateChanged),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnPipelineEnded),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnPipelineSeek),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnPipelineError),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnPipelineBufferingState),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnDurationChange),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnVideoSizeChange),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnVideoDisplayWindowChange),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::updateUMSInfo),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::focusChanged));
}

void WebMediaPlayerUMS::updateUMSInfo(const std::string& detail) {

  DEBUG_LOG("%s - %s", __FUNCTION__, detail.c_str());

  if (!detail.empty())
    GetClient()->updateUMSMediaInfo(WebString::fromUTF8(detail.c_str()));
}

void WebMediaPlayerUMS::focusChanged() {
  GetClient()->focusChanged();
}

void WebMediaPlayerUMS::play() {
  DEBUG_LOG("%s - playmedia", __FUNCTION__);
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  if (is_suspended_) {
    DEBUG_LOG("block to play on suspended");
    status_on_suspended_ = PlayingStatus;
    return;
  }

  paused_ = false;
  umedia_client_->SetPlaybackRate(playback_rate_);

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));

  if (delegate_) {
    bool is_remote = false;  // assumming remote sorce; TODO: figure this out
    delegate_->DidPlay(delegate_id_, hasVideo(), hasAudio(), is_remote,
                       base::TimeDelta::FromSecondsD(duration()));
  }
}

void WebMediaPlayerUMS::pause() {

  DEBUG_LOG("%s", __FUNCTION__);

  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  paused_ = true;
  umedia_client_->SetPlaybackRate(0.0f);
  paused_time_ = base::TimeDelta::FromSecondsD(umedia_client_->currentTime());

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PAUSE));

  if (delegate_) {
    delegate_->DidPause(delegate_id_, false); // TODO: capture EOS
  }
}

bool WebMediaPlayerUMS::supportsFullscreen() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return true;
}

bool WebMediaPlayerUMS::supportsSave() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return supports_save_;
}

void WebMediaPlayerUMS::seek(double seconds) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  base::TimeDelta seek_time = base::TimeDelta::FromSecondsD(seconds);

  if (starting_ || seeking_) {
    pending_seek_ = true;
    pending_seek_seconds_ = seconds;
    return;
  }

  media_log_->AddEvent(media_log_->CreateSeekEvent(seconds));

  // Update our paused time.
  if (paused_)
    paused_time_ = seek_time;

  seeking_ = true;

  // Kick off the asynchronous seek!
  umedia_client_->Seek(seek_time,
    BIND_TO_RENDER_LOOP(&WebMediaPlayerUMS::OnPipelineSeek));
}

void WebMediaPlayerUMS::setRate(double rate) {

  DEBUG_LOG("%s - rate=%f", __FUNCTION__, rate);

  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  if (!umedia_client_->isSupportedBackwardTrickPlay() && rate < 0.0)
    return;

  // Limit rates to reasonable values by clamping.
  if (rate != 0.0) {
    if (rate < kMinRate)
      rate = kMinRate;
    else if (rate > kMaxRate)
      rate = kMaxRate;
  }

  playback_rate_ = rate;
  if (is_suspended_) {
    DEBUG_LOG("block to setRate on suspended");
    return;
  }

  if (!paused_)
    umedia_client_->SetPlaybackRate(rate);
}

void WebMediaPlayerUMS::setVolume(double volume) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  umedia_client_->SetPlaybackVolume(volume);
}

void WebMediaPlayerUMS::setPreload(Preload preload) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
#if UMS_INTERNAL_API_VERSION == 2
  /* RMP does not support preloading media, hence adding this temporary workaround */
  umedia_client_->SetPreload(static_cast<UMediaClientImpl::Preload>(PreloadNone));
#else
  umedia_client_->SetPreload(static_cast<UMediaClientImpl::Preload>(preload));
#endif
}

bool WebMediaPlayerUMS::hasVideo() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  bool ret_val = umedia_client_->hasVideo();
  DEBUG_LOG("%s - hasVideo=%d", __FUNCTION__, ret_val);
  return ret_val;
}

bool WebMediaPlayerUMS::hasAudio() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  bool ret_val = umedia_client_->hasAudio();
  DEBUG_LOG("%s - hasAudio=%d", __FUNCTION__, ret_val);
  return ret_val;
}

bool WebMediaPlayerUMS::selectTrack(std::string& type, int32_t index) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  bool ret_val = umedia_client_->selectTrack(type, index);
  DEBUG_LOG("%s - type=%s index=%d ret_val=%d", __FUNCTION__, type.c_str(), index, ret_val);
  return ret_val;
}

blink::WebSize WebMediaPlayerUMS::naturalSize() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  gfx::Size size;
  size = umedia_client_->naturalVideoSize();
  return blink::WebSize(size);
}

bool WebMediaPlayerUMS::paused() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return paused_;
}

bool WebMediaPlayerUMS::seeking() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing)
    return false;

  return seeking_;
}

double WebMediaPlayerUMS::duration() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing)
    return std::numeric_limits<double>::quiet_NaN();

  return umedia_client_->duration();
}

double WebMediaPlayerUMS::currentTime() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return paused_ ? paused_time_.InSecondsF() : umedia_client_->currentTime();
}

WebMediaPlayer::NetworkState WebMediaPlayerUMS::getNetworkState() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayerUMS::getReadyState() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return ready_state_;
}

blink::WebTimeRanges WebMediaPlayerUMS::buffered() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  blink::WebTimeRanges web_ranges((size_t)1);
  web_ranges[0].start = 0.0f;
  web_ranges[0].end = umedia_client_->bufferEnd();

  return web_ranges;
}

blink::WebTimeRanges WebMediaPlayerUMS::seekable() const {
  if (ready_state_ < WebMediaPlayer::ReadyStateHaveMetadata)
    return blink::WebTimeRanges();

  const blink::WebTimeRange seekable_range(0.0, duration());
  return blink::WebTimeRanges(&seekable_range, 1);
}

double WebMediaPlayerUMS::maxTimeSeekable() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  // If we haven't even gotten to ReadyStateHaveMetadata yet then just
  // return 0 so that the seekable range is empty.
  if (ready_state_ < WebMediaPlayer::ReadyStateHaveMetadata)
    return 0.0;

  // If media type is streaming media(live), shoud be return 0.

  return duration();
}

bool WebMediaPlayerUMS::didLoadingProgress() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return umedia_client_->DidLoadingProgress();
}

void WebMediaPlayerUMS::paint(WebCanvas* canvas,
                               const WebRect& rect,
                               unsigned char alpha,
                               SkXfermode::Mode mode) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return;
}

bool WebMediaPlayerUMS::hasSingleSecurityOrigin() const {
  return true;
}

bool WebMediaPlayerUMS::didPassCORSAccessCheck() const {
  return false;
}

double WebMediaPlayerUMS::mediaTimeForTimeValue(double timeValue) const {
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayerUMS::decodedFrameCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

unsigned WebMediaPlayerUMS::droppedFrameCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

unsigned WebMediaPlayerUMS::audioDecodedByteCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

unsigned WebMediaPlayerUMS::videoDecodedByteCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

void WebMediaPlayerUMS::SetVideoFrameProviderClient(
    cc::VideoFrameProvider::Client* client) {
  // This is called from both the main renderer thread and the compositor
  // thread (when the main thread is blocked).
  if (video_frame_provider_client_)
    video_frame_provider_client_->StopUsingProvider();
  video_frame_provider_client_ = client;
}

void WebMediaPlayerUMS::PutCurrentFrame(){
}

scoped_refptr<media::VideoFrame> WebMediaPlayerUMS::GetCurrentFrame() {
  base::AutoLock auto_lock(lock_);
  return current_frame_;
}

bool WebMediaPlayerUMS::HasCurrentFrame() {
  base::AutoLock auto_lock(lock_);
  return current_frame_ != nullptr;
}

bool WebMediaPlayerUMS::copyVideoTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* web_graphics_context,
    unsigned int texture,
    unsigned int internal_format,
    unsigned int type,
    bool premultiply_alpha,
    bool flip_y) {
  return true;
}


blink::WebAudioSourceProvider* WebMediaPlayerUMS::getAudioSourceProvider() {
  return audio_source_provider_.get();
}

void WebMediaPlayerUMS::WillDestroyCurrentMessageLoop() {
}

void WebMediaPlayerUMS::Repaint() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  GetClient()->repaint();
}

blink::WebRect WebMediaPlayerUMS::ScaleWebRect(
    const blink::WebRect& rect, blink::WebFloatPoint scale) {
  blink::WebRect scaledRect;

  scaledRect.x = rect.x * scale.x;
  scaledRect.y = rect.y * scale.y;
  scaledRect.width = rect.width * scale.x;
  scaledRect.height = rect.height * scale.y;

  return scaledRect;
}

void WebMediaPlayerUMS::updateVideo(const blink::WebRect& rect,
                                    bool fullscreen) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  if (pending_size_change_ || previous_video_rect_ != rect) {
    bool forced = pending_size_change_;
    pending_size_change_ = false;
    previous_video_rect_ = rect;

    blink::WebRect inRect;
    display_window_in_rect_ = ScaleWebRect(inRect, additional_contents_scale_);
    display_window_out_rect_ = ScaleWebRect(rect, additional_contents_scale_);

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
      if (natural_size == gfx::Size() && !forced) {
        previous_video_rect_ = blink::WebRect();
        return;
      }

      if (display_window_out_rect.width > 0 &&
          display_window_out_rect.height > 0) {
        display_window_in_rect_ = WebRect(
            clipped_x * natural_size.width() / display_window_out_rect.width,
            clipped_y * natural_size.height() / display_window_out_rect.height,
            clipped_width *
                natural_size.width() / display_window_out_rect.width,
            clipped_height *
                natural_size.height() / display_window_out_rect.height);
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

    umedia_client_->setDisplayWindow(display_window_out_rect,
                                     display_window_in_rect_,
                                     checked_fullscreen, forced);
  }
}

void WebMediaPlayerUMS::paintTimerFired() {
  if (pending_size_change_) {
    GetClient()->repaint();
    GetClient()->sizeChanged();
  }
  GetClient()->checkBounds();
}

bool WebMediaPlayerUMS::UpdateCurrentFrame(base::TimeTicks deadline_min,
                                               base::TimeTicks deadline_max) {
  NOTIMPLEMENTED();
  return false;
}

void WebMediaPlayerUMS::OnPipelinePlaybackStateChanged(bool playing) {

  DEBUG_LOG("%s - playing=%s", __FUNCTION__, playing?"true":"false");

  if (playing == paused()) {
    paused_ = !playing;

    auto delegate = delegate_.get();

    if (playing) {
      if (delegate) {
        bool is_remote =
            false;  // assumming remote sorce; TODO: figure this out
        delegate_->DidPlay(delegate_id_, hasVideo(), hasAudio(), is_remote,
                           base::TimeDelta::FromSecondsD(duration()));
      }
    } else {
      if (delegate)
        delegate_->DidPause(delegate_id_, false);
    }

    GetClient()->playbackStateChanged();
  }
}

void WebMediaPlayerUMS::OnPipelineSeek(PipelineStatus status) {

  DEBUG_LOG("%s - PipelineStatus=%d", __FUNCTION__, status);

  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  starting_ = false;
  seeking_ = false;
  if (pending_seek_) {
    pending_seek_ = false;
    seek(pending_seek_seconds_);
    return;
  }

  if (status != media::PIPELINE_OK) {
    OnPipelineError(status);
    return;
  }

  // Update our paused time.
  if (paused_)
    paused_time_ = base::TimeDelta::FromSecondsD(umedia_client_->currentTime());

  GetClient()->timeChanged();
}

void WebMediaPlayerUMS::OnPipelineEnded() {
  DEBUG_LOG("WebMediaPlayerUMS::OnPipelineEnded()...");
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  GetClient()->timeChanged();
}

void WebMediaPlayerUMS::OnPipelineError(PipelineStatus error) {

  DEBUG_LOG("%s - PipelineStatus=%d", __FUNCTION__, error);

  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DCHECK_NE(error, media::PIPELINE_OK);

  // use uMediaServer's automatic reload
  // but this function is experimental method
  // so we should be implemented reload media.
  if (error == media::DECODER_ERROR_RESOURCE_IS_RELEASED)
    return;

  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing) {
    // Any error that occurs before reaching ReadyStateHaveMetadata should
    // be considered a format error.
    SetNetworkState(WebMediaPlayer::NetworkStateFormatError);
    Repaint();
    return;
  }

  SetNetworkState(PipelineErrorToNetworkState(error));

  // Repaint to trigger UI update.
  Repaint();
}

void WebMediaPlayerUMS::OnPipelineBufferingState(
    UMediaClientImpl::BufferingState buffering_state) {

  DEBUG_LOG("%s - BufferingState=%d", __FUNCTION__, buffering_state);

  DVLOG(1) << __FUNCTION__ << "(" << buffering_state << ")";

  switch (buffering_state) {
    case UMediaClientImpl::kHaveMetadata:
      if (ready_state_ >= WebMediaPlayer::ReadyStateHaveMetadata)
        break;

      SetReadyState(WebMediaPlayer::ReadyStateHaveMetadata);

      if (hasVideo()) {
        if (!video_weblayer_) {
          video_weblayer_.reset(new cc_blink::WebLayerImpl(
              cc::VideoLayer::Create(this, media::VIDEO_ROTATION_0)));
          GetClient()->setWebLayer(video_weblayer_.get());
        }
      }

#if UMS_INTERNAL_API_VERSION == 2
      /* we don't get kPreloadCompleted or kLoadCompleted in the RMP yet,
      hence as a woraround, we just force ready state to ReadyStateHaveEnoughData
      (which will also make the network state as NetworkStateLoaded), and we
      will just make the Hole frame if needed to start showing the video
      */
      DEBUG_LOG("Force WebMediaPlayer::ReadyStateHaveEnoughData");
      SetReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
#if !defined(USE_VIDEO_TEXTURE)
      current_frame_ = media::VideoFrame::CreateHoleFrame(gfx::Size(1, 1));
#endif /* #if !defined(USE_VIDEO_TEXTURE) */
      Repaint();
#endif /* #if UMS_INTERNAL_API_VERSION == 2 */
      break;
    case UMediaClientImpl::kPreloadCompleted:
      SetReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
      break;
    case UMediaClientImpl::kLoadCompleted:
      current_frame_ = media::VideoFrame::CreateHoleFrame(gfx::Size(1, 1));
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

void WebMediaPlayerUMS::SetNetworkState(WebMediaPlayer::NetworkState state) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DVLOG(1) << "SetNetworkState: " << state;

  DEBUG_LOG("%s - NetworkState=%d", __FUNCTION__, state);

  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing &&
      (state == WebMediaPlayer::NetworkStateNetworkError ||
       state == WebMediaPlayer::NetworkStateDecodeError)) {
    // Any error that occurs before reaching ReadyStateHaveMetadata should
    // be considered a format error.
    network_state_ = WebMediaPlayer::NetworkStateFormatError;
  } else {
    network_state_ = state;
  }
  // Always notify to ensure client has the latest value.
  GetClient()->networkStateChanged();
}

void WebMediaPlayerUMS::SetReadyState(WebMediaPlayer::ReadyState state) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DVLOG(1) << "SetReadyState: " << state;

  DEBUG_LOG("%s - ReadyState=%d", __FUNCTION__, state);

  if (state == WebMediaPlayer::ReadyStateHaveEnoughData &&
      is_local_source_ &&
      network_state_ == WebMediaPlayer::NetworkStateLoading) {
    DEBUG_LOG("SetNetworkState to NetworkStateLoaded");
    SetNetworkState(WebMediaPlayer::NetworkStateLoaded);
  }

  if (state >= WebMediaPlayer::ReadyStateHaveMetadata &&
      ready_state_ < WebMediaPlayer::ReadyStateHaveMetadata) {
    for (int i = 0; i < umedia_client_->numAudioTracks(); i++) {
      WebMediaPlayer::TrackId id = GetClient()->addAudioTrack("audio",
          blink::WebMediaPlayerClient::AudioTrackKindMain,
          "Audio Track", "", false);
      if (id != 0)
        audio_track_ids_.push_back(id);
    }
  }
  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  GetClient()->readyStateChanged();
}

blink::WebMediaPlayerClient* WebMediaPlayerUMS::GetClient() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DCHECK(client_);
  return client_;
}

void WebMediaPlayerUMS::OnDurationChange() {

  DEBUG_LOG("%s - ready_state_=%d", __FUNCTION__, ready_state_);

  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing)
    return;

  GetClient()->durationChanged();
}

void WebMediaPlayerUMS::OnVideoSizeChange() {
  DEBUG_LOG("%s", __FUNCTION__);
  pending_size_change_ = true;
}

void WebMediaPlayerUMS::OnVideoDisplayWindowChange() {
  bool forced = true;

  DEBUG_LOG("%s", __FUNCTION__);

  blink::WebRect display_window_out_rect = display_window_out_rect_;

  umedia_client_->setDisplayWindow(
      display_window_out_rect, display_window_in_rect_, fullscreen_, forced);
}

WebString WebMediaPlayerUMS::mediaId() const {
  return WebString::fromUTF8(umedia_client_->mediaId().c_str());
}

#if defined(USE_WEBOS_MEDIA_FOCUS_EXTENSION)
bool WebMediaPlayerUMS::mediaFocus() const {
  bool ret_val = umedia_client_ ? umedia_client_->focus() : false;
  DEBUG_LOG("query mediaFocus - returns %s", ret_val?"true":"false");
  return ret_val;
}

void WebMediaPlayerUMS::setMediaFocus(bool focus) {
  DEBUG_LOG("setMediaFocus = %d", focus);
  if (umedia_client_ && focus && !umedia_client_->focus()) {
    umedia_client_->setFocus();
  }
}

bool WebMediaPlayerUMS::visible() const {
  bool ret_val = umedia_client_->visibility();
  DEBUG_LOG("%s - return=%d", __FUNCTION__, ret_val);
  return ret_val;
}

void WebMediaPlayerUMS::setVisible(bool visible) {
  DEBUG_LOG("setVisible = %d is_video_offscreen_ = %d", visible,
            is_video_offscreen_);
  if (!is_video_offscreen_)
    umedia_client_->setVisibility(visible);
}
#endif

bool WebMediaPlayerUMS::usesIntrinsicSize() {
  return true;
}

void WebMediaPlayerUMS::suspend() {

  DEBUG_LOG("%s - is_suspended_=%d", __FUNCTION__, is_suspended_);

  if (is_suspended_)
    return;

  is_suspended_ = true;
  status_on_suspended_ = paused() ? PausedStatus : PlayingStatus;
  if (status_on_suspended_ == PlayingStatus) {
    pause();
    GetClient()->playbackStateChanged();
  }
  if (hasVideo()) {
    current_frame_ = media::VideoFrame::CreateBlackFrame(gfx::Size(1, 1));
    Repaint();
  }
  umedia_client_->suspend();
}

void WebMediaPlayerUMS::resume() {

  DEBUG_LOG("%s - is_suspended_=%d", __FUNCTION__, is_suspended_);

  if (!is_suspended_)
    return;

  is_suspended_ = false;
  umedia_client_->resume();

  if (hasVideo()) {
    current_frame_ = media::VideoFrame::CreateHoleFrame(gfx::Size(1, 1));
    Repaint();
  }

  if (status_on_suspended_ == PlayingStatus) {
    play();
    GetClient()->playbackStateChanged();
    status_on_suspended_ = UnknownStatus;
  }
}

void WebMediaPlayerUMS::enabledAudioTracksChanged(
    const blink::WebVector<TrackId>& enabledTrackIds) {
  if (enabledTrackIds.size()) {
    for (size_t j = 0; j < audio_track_ids_.size(); j++) {
      if (enabledTrackIds[enabledTrackIds.size() - 1] == audio_track_ids_[j]) {
        std::string type = "audio";
        selectTrack(type, j);
        break;
      }
    }
  }
}

}  // namespace media
