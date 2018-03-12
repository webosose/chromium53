// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/webmediaplayer_custom.h"

#include <algorithm>
#include <limits>
#include <string>
#include <cstdint>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "cc/layers/video_layer.h"
#include "media/blink/webaudiosourceprovider_impl.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_params.h"
#include "media/blink/webmediaplayer_util.h"
#include "content/renderer/media/render_media_log.h"
#include "cc/blink/web_layer_impl.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "media/audio/null_audio_sink.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_log.h"
#include "media/base/video_frame.h"
#include "third_party/WebKit/public/platform/WebMediaPlayerSource.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/platform/WebURL.h"

using blink::WebCanvas;
using blink::WebMediaPlayer;
using blink::WebRect;
using blink::WebSize;
using blink::WebString;
using media::PipelineStatus;

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("WmpCustom", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("WmpCustom:%04d " format, __LINE__, ##__VA_ARGS__)

namespace {

// Limits the range of playback rate.
// We think 1/16x to 16x is a safe and useful range for now.
const double kMinRate = 0.0625;
const double kMaxRate = 16.0;

static const int64_t kPaintTimerInterval = 400;
}  // namespace

namespace media {

#define BIND_TO_RENDER_LOOP(function)                           \
  (DCHECK(main_loop_->task_runner()->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

WebMediaPlayerCustom::WebMediaPlayerCustom(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
    const media::WebMediaPlayerParams& params,
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
      pending_size_change_(false),
      video_frame_provider_client_(NULL),
      fullscreen_(false),
      additional_contents_scale_(additional_contents_scale),
      app_id_(app_id.utf8().data()) {
  if (delegate_)
    delegate_id_ = delegate_->AddObserver(this);

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  media_log_->AddEvent(
      media_log_->CreateEvent(media::MediaLogEvent::WEBMEDIAPLAYER_CREATED));

  // We want to be notified of |main_loop_| destruction.
  if (main_loop_)
    main_loop_->AddDestructionObserver(this);

  // Use the null sink if no sink was provided.
  audio_source_provider_ = new media::WebAudioSourceProviderImpl(
      params.audio_renderer_sink().get()
          ? params.audio_renderer_sink()
          : new media::NullAudioSink(media_task_runner_));

  if (client->contentMIMEType() == WebString("service/webos-broadcast") ||
      client->contentMIMEType() == WebString("service/webos-broadcast-cable"))
    custom_pipeline_ = new TvPipeline(media_task_runner_);
  else if (client->contentMIMEType() == WebString("service/webos-camera"))
    custom_pipeline_ = new CameraPipeline(media_task_runner_);
  else if (client->contentMIMEType() == WebString("service/webos-external"))
    custom_pipeline_ = new ExtInputPipeline(media_task_runner_);
  else if (client->contentMIMEType() == WebString("service/webos-dvr"))
    custom_pipeline_ = new DvrPipeline(media_task_runner_);
  else if (client->contentMIMEType() == WebString("service/webos-photo") ||
           client->contentMIMEType() == WebString("service/webos-photo-camera"))
    custom_pipeline_ = new PhotoPipeline(media_task_runner_);
  else
    DCHECK(false) << "Unexpected mimetype";
}

WebMediaPlayerCustom::~WebMediaPlayerCustom() {
  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

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

  Destroy();

  // Remove destruction observer if we're being destroyed but the main thread is
  // still running.
  if (main_loop_)
    main_loop_->RemoveDestructionObserver(this);
}

void WebMediaPlayerCustom::load(LoadType load_type,
                                const blink::WebMediaPlayerSource& src,
                                CORSMode cors_mode) {
  if (!defer_load_cb_.is_null()) {
    defer_load_cb_.Run(base::Bind(&WebMediaPlayerCustom::DoLoad, AsWeakPtr(),
                                  load_type, src.getAsURL(), cors_mode));
    return;
  }
  DoLoad(load_type, src.getAsURL(), cors_mode);
}

void WebMediaPlayerCustom::DoLoad(LoadType load_type,
                                  const blink::WebURL& url,
                                  CORSMode cors_mode) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  url::Parsed parsed;
  url::ParseStandardURL(url.string().utf8().c_str(),
                        url.string().utf8().length(), &parsed);
  GURL gurl(url.string().utf8(), parsed, url.isValid());

  SetNetworkState(WebMediaPlayer::NetworkStateLoading);
  SetReadyState(WebMediaPlayer::ReadyStateHaveNothing);
  media_log_->AddEvent(media_log_->CreateLoadEvent(url.string().utf8()));
  custom_pipeline_->Load(
      GetClient()->isVideo(), app_id_, gurl,
      std::string(GetClient()->contentMIMEType().utf8().data()),
      std::string(GetClient()->contentCustomOption().utf8().data()),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerCustom::OnPipelineBufferingState),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerCustom::OnVideoDisplayWindowChange),
      BIND_TO_RENDER_LOOP(&WebMediaPlayerCustom::OnPipelineMessage));

  is_local_source_ = gurl.SchemeIs("file");

  if (!paintTimer_.IsRunning()) {
    paintTimer_.Start(FROM_HERE,
                      base::TimeDelta::FromMilliseconds(kPaintTimerInterval),
                      base::Bind(&WebMediaPlayerCustom::paintTimerFired,
                                 base::Unretained(this)));
  }
}

void WebMediaPlayerCustom::play() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  if (paused_)
    paused_ = false;

  custom_pipeline_->SetPlaybackRate(playback_rate_);

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PLAY));

  if (delegate_) {
    bool is_remote = true;  // assumming remote sorce; TODO: figure this out
    delegate_->DidPlay(delegate_id_, hasVideo(), hasAudio(), is_remote,
                       base::TimeDelta::FromSecondsD(duration()));
  }
}

void WebMediaPlayerCustom::pause() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  paused_ = true;
  custom_pipeline_->SetPlaybackRate(0.0f);
  paused_time_ = custom_pipeline_->CurrentTime();

  media_log_->AddEvent(media_log_->CreateEvent(media::MediaLogEvent::PAUSE));

  if (delegate_)
    delegate_->DidPause(delegate_id_, false);  // TODO: capture EOS
}

bool WebMediaPlayerCustom::supportsFullscreen() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return true;
}

bool WebMediaPlayerCustom::supportsSave() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return supports_save_;
}

void WebMediaPlayerCustom::seek(double seconds) {
  // Never use this function in Custom Player.
  NOTREACHED();
}

void WebMediaPlayerCustom::setRate(double rate) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  // TODO(kylep): Remove when support for negatives is added. Also, modify the
  // following checks so rewind uses reasonable values also.
  if (rate < 0.0)
    return;

  // Limit rates to reasonable values by clamping.
  if (rate != 0.0) {
    if (rate < kMinRate)
      rate = kMinRate;
    else if (rate > kMaxRate)
      rate = kMaxRate;
  }

  playback_rate_ = rate;
  if (!paused_)
    custom_pipeline_->SetPlaybackRate(rate);
}

void WebMediaPlayerCustom::setVolume(double volume) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
}

bool WebMediaPlayerCustom::hasVideo() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return custom_pipeline_->HasVideo();
}

bool WebMediaPlayerCustom::hasAudio() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return custom_pipeline_->HasAudio();
}

blink::WebSize WebMediaPlayerCustom::naturalSize() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  gfx::Size size;
  size = custom_pipeline_->NaturalVideoSize();
  return blink::WebSize(size);
}

bool WebMediaPlayerCustom::paused() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return paused_;
}

bool WebMediaPlayerCustom::seeking() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing)
    return false;

  return seeking_;
}

double WebMediaPlayerCustom::duration() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  if (ready_state_ == WebMediaPlayer::ReadyStateHaveNothing)
    return std::numeric_limits<double>::quiet_NaN();

  return custom_pipeline_->Duration();
}

double WebMediaPlayerCustom::currentTime() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return (paused_ ? paused_time_ : custom_pipeline_->CurrentTime())
      .InSecondsF();
}

WebMediaPlayer::NetworkState WebMediaPlayerCustom::getNetworkState() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return network_state_;
}

WebMediaPlayer::ReadyState WebMediaPlayerCustom::getReadyState() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return ready_state_;
}

blink::WebTimeRanges WebMediaPlayerCustom::buffered() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  blink::WebTimeRanges web_ranges((size_t)1);
  web_ranges[0].start = 0.0f;
  web_ranges[0].end = custom_pipeline_->BufferEnd();

  return web_ranges;
}

blink::WebTimeRanges WebMediaPlayerCustom::seekable() const {
  if (ready_state_ < WebMediaPlayer::ReadyStateHaveMetadata)
    return blink::WebTimeRanges();

  const blink::WebTimeRange seekable_range(0.0, duration());
  return blink::WebTimeRanges(&seekable_range, 1);
}

double WebMediaPlayerCustom::maxTimeSeekable() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  // If we haven't even gotten to ReadyStateHaveMetadata yet then just
  // return 0 so that the seekable range is empty.
  if (ready_state_ < WebMediaPlayer::ReadyStateHaveMetadata)
    return 0.0;

  return duration();
}

bool WebMediaPlayerCustom::didLoadingProgress() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return custom_pipeline_->DidLoadingProgress();
}

void WebMediaPlayerCustom::paint(WebCanvas* canvas,
                                 const WebRect& rect,
                                 unsigned char alpha,
                                 SkXfermode::Mode mode) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return;
}

bool WebMediaPlayerCustom::hasSingleSecurityOrigin() const {
  return true;
}

bool WebMediaPlayerCustom::didPassCORSAccessCheck() const {
  return false;
}

double WebMediaPlayerCustom::mediaTimeForTimeValue(double timeValue) const {
  return base::TimeDelta::FromSecondsD(timeValue).InSecondsF();
}

unsigned WebMediaPlayerCustom::decodedFrameCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

unsigned WebMediaPlayerCustom::droppedFrameCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

unsigned WebMediaPlayerCustom::audioDecodedByteCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

unsigned WebMediaPlayerCustom::videoDecodedByteCount() const {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  return 0;
}

void WebMediaPlayerCustom::SetVideoFrameProviderClient(
    cc::VideoFrameProvider::Client* client) {
  // This is called from both the main renderer thread and the compositor
  // thread (when the main thread is blocked).
  if (video_frame_provider_client_)
    video_frame_provider_client_->StopUsingProvider();
  video_frame_provider_client_ = client;
}

scoped_refptr<media::VideoFrame> WebMediaPlayerCustom::GetCurrentFrame() {
  base::AutoLock auto_lock(lock_);
  return current_frame_;
}

void WebMediaPlayerCustom::PutCurrentFrame() {}

bool WebMediaPlayerCustom::HasCurrentFrame() {
  base::AutoLock auto_lock(lock_);
  return current_frame_ != nullptr;
}

bool WebMediaPlayerCustom::copyVideoTextureToPlatformTexture(
    gpu::gles2::GLES2Interface* web_graphics_context,
    unsigned int texture,
    unsigned int internal_format,
    unsigned int type,
    bool premultiply_alpha,
    bool flip_y) {
#if 0  // TODO: reenable textured video
  scoped_refptr<media::VideoFrame> video_frame;
  {
    base::AutoLock auto_lock(lock_);
    video_frame = current_frame_;
  }

  if (!video_frame.get())
    return false;
  if (video_frame->format() != media::VideoFrame::NATIVE_TEXTURE)
    return false;
  const gpu::MailboxHolder& mailbox_holder = video_frame->mailbox_holder(0);
  if (mailbox_holder.texture_target != GL_TEXTURE_2D)
      return false;

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

  uint32 source_texture = web_graphics_context->createTexture();

  web_graphics_context->waitSyncPoint(mailbox_holder.sync_point);
  web_graphics_context->bindTexture(GL_TEXTURE_2D, source_texture);
  web_graphics_context->consumeTextureCHROMIUM(GL_TEXTURE_2D,
                                               mailbox_holder.mailbox.name);

  // The video is stored in a unmultiplied format, so premultiply
  // if necessary.
  web_graphics_context->pixelStorei(GL_UNPACK_PREMULTIPLY_ALPHA_CHROMIUM,
                                    premultiply_alpha);
  // Application itself needs to take care of setting the right flip_y
  // value down to get the expected result.
  // flip_y==true means to reverse the video orientation while
  // flip_y==false means to keep the intrinsic orientation.
  web_graphics_context->pixelStorei(GL_UNPACK_FLIP_Y_CHROMIUM, flip_y);
  web_graphics_context->copyTextureCHROMIUM(GL_TEXTURE_2D,
                                            source_texture,
                                            texture,
                                            internal_format,
                                            type);
  web_graphics_context->pixelStorei(GL_UNPACK_FLIP_Y_CHROMIUM, false);
  web_graphics_context->pixelStorei(GL_UNPACK_PREMULTIPLY_ALPHA_CHROMIUM,
                                    false);

  // Restore the state for TEXTURE_2D binding point as mentioned above.
  web_graphics_context->bindTexture(GL_TEXTURE_2D, texture);

  web_graphics_context->deleteTexture(source_texture);

  // The flush() operation is not necessary here. It is kept since the
  // performance will be better when it is added than not.
  web_graphics_context->flush();
#endif
  return true;
}

blink::WebAudioSourceProvider* WebMediaPlayerCustom::getAudioSourceProvider() {
  return audio_source_provider_.get();
}

void WebMediaPlayerCustom::WillDestroyCurrentMessageLoop() {
  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  Destroy();
}

void WebMediaPlayerCustom::Repaint() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  GetClient()->repaint();
}

blink::WebRect WebMediaPlayerCustom::ScaleWebRect(const blink::WebRect& rect,
                                                  blink::WebFloatPoint scale) {
  blink::WebRect scaledRect;
  scaledRect.x = rect.x * scale.x;
  scaledRect.y = rect.y * scale.y;
  scaledRect.width = rect.width * scale.x;
  scaledRect.height = rect.height * scale.y;
  return scaledRect;
}

void WebMediaPlayerCustom::updateVideo(const blink::WebRect& rect,
                                       bool fullScreen) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());

  if (previous_video_rect_ != rect) {
    previous_video_rect_ = rect;

    blink::WebRect inRect;
    fullscreen_ = fullScreen;
    display_window_in_rect_ = ScaleWebRect(inRect, additional_contents_scale_);
    display_window_out_rect_ = ScaleWebRect(rect, additional_contents_scale_);
    custom_pipeline_->SetDisplayWindow(display_window_out_rect_,
                                       display_window_in_rect_, fullScreen);
  }
}

void WebMediaPlayerCustom::paintTimerFired() {
  // Trigger HTMLMediaElement::mediaPlayerSizeChanged which will
  // chain to the ::synchronizeBackendVideoPosition method.
  bool size_changed = false;
  {
    base::AutoLock auto_lock(lock_);
    std::swap(pending_size_change_, size_changed);
  }

  if (size_changed) {
    GetClient()->repaint();
    GetClient()->sizeChanged();
  }
  GetClient()->checkBounds();
}

bool WebMediaPlayerCustom::UpdateCurrentFrame(base::TimeTicks deadline_min,
                                              base::TimeTicks deadline_max) {
  NOTIMPLEMENTED();
  return false;
}

void WebMediaPlayerCustom::OnPipelineBufferingState(
    CustomPipeline::BufferingState buffering_state) {
  DVLOG(1) << __FUNCTION__ << "(" << buffering_state << ")";

  switch (buffering_state) {
    case CustomPipeline::HaveMetadata:
      if (ready_state_ >= WebMediaPlayer::ReadyStateHaveMetadata)
        break;

      current_frame_ = media::VideoFrame::CreateHoleFrame(gfx::Size(1, 1));

      SetReadyState(WebMediaPlayer::ReadyStateHaveMetadata);

      if (hasVideo()) {
        DCHECK(!video_weblayer_);
        video_weblayer_.reset(new cc_blink::WebLayerImpl(
            cc::VideoLayer::Create(this, media::VIDEO_ROTATION_0)));
        GetClient()->setWebLayer(video_weblayer_.get());
      }
      break;
    case CustomPipeline::HaveFutureData:
      SetReadyState(WebMediaPlayer::ReadyStateHaveFutureData);
      break;
    case CustomPipeline::HaveEnoughData:
      // Only transition to ReadyStateHaveEnoughData if we don't have
      // any pending seeks because the transition can cause Blink to
      // report that the most recent seek has completed.
      if (!pending_seek_)
        SetReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
      break;
    case CustomPipeline::PipelineStarted:
      // Pipeline Start Complete.
      // Let App Know this moment by event.
      GetClient()->pipelineStarted();
      break;
    case CustomPipeline::WebOSBufferingStart:
      SetReadyState(WebMediaPlayer::ReadyStateHaveCurrentData);
      break;
    case CustomPipeline::WebOSBufferingEnd:
      SetReadyState(WebMediaPlayer::ReadyStateHaveEnoughData);
      break;
  }

  // Repaint to trigger UI update.
  Repaint();
}

void WebMediaPlayerCustom::OnPipelineMessage(const std::string& detail) {
  if (!detail.empty())
    GetClient()->sendPipelineMessage(WebString::fromUTF8(detail.c_str()));
}

void WebMediaPlayerCustom::SetNetworkState(WebMediaPlayer::NetworkState state) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DVLOG(1) << "SetNetworkState: " << state;
  network_state_ = state;
  // Always notify to ensure client has the latest value.
  GetClient()->networkStateChanged();
}

void WebMediaPlayerCustom::SetReadyState(WebMediaPlayer::ReadyState state) {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DVLOG(1) << "SetReadyState: " << state;

  if (state == WebMediaPlayer::ReadyStateHaveEnoughData && is_local_source_ &&
      network_state_ == WebMediaPlayer::NetworkStateLoading)
    SetNetworkState(WebMediaPlayer::NetworkStateLoaded);

  ready_state_ = state;
  // Always notify to ensure client has the latest value.
  GetClient()->readyStateChanged();
}

void WebMediaPlayerCustom::Destroy() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  if (custom_pipeline_) {
    custom_pipeline_->Dispose();
    custom_pipeline_ = NULL;
  }
}

blink::WebMediaPlayerClient* WebMediaPlayerCustom::GetClient() {
  DCHECK(main_loop_->task_runner()->BelongsToCurrentThread());
  DCHECK(client_);
  return client_;
}

void WebMediaPlayerCustom::OnVideoDisplayWindowChange() {
  custom_pipeline_->SetDisplayWindow(display_window_out_rect_,
                                     display_window_in_rect_, false, true);
}

WebString WebMediaPlayerCustom::cameraId() const {
  return WebString::fromUTF8(custom_pipeline_->CameraId());
}

WebString WebMediaPlayerCustom::mediaId() const {
  return WebString::fromUTF8(custom_pipeline_->MediaId());
}

bool WebMediaPlayerCustom::usesIntrinsicSize() {
  return false;
}
void WebMediaPlayerCustom::suspend() {
  if (is_suspended_)
    return;

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  is_suspended_ = true;
  custom_pipeline_->SetPlaybackRate(0.0f);
}

void WebMediaPlayerCustom::resume() {
  if (!is_suspended_)
    return;

  DEBUG_LOG("%s - 0x%x", __FUNCTION__, this);

  is_suspended_ = false;
  custom_pipeline_->SetPlaybackRate(playback_rate_);
}

void WebMediaPlayerCustom::contentsPositionChanged(float position) {
  custom_pipeline_->ContentsPositionChanged(position);
}

}  // namespace media
