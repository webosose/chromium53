// Copyright (c) 2014-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/umediaclient_impl.h"

#include <cmath>
#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "media/base/bind_to_current_loop.h"
#include "media/webos/base/starfish_media_pipeline_error.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("UMediaClientImpl", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("UMediaClientImpl:%04d " format, __LINE__, ##__VA_ARGS__)

static const char* kUdpUrl = "udp://";
static const char* kRtpUrl = "rtp://";
static const char* kRtspUrl = "rtsp://";

namespace media {

#define BIND_TO_RENDER_LOOP(function) \
  (DCHECK(media_task_runner_->BelongsToCurrentThread()), \
  media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

UMediaClientImpl::UMediaClientImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : duration_(0.),
      current_time_(0.),
      buffer_end_(0),
      buffer_end_at_last_didLoadingProgress_(0),
      video_(false),
      loaded_(false),
      preloaded_(false),
      load_started_(false),
      num_audio_tracks_(0),
      is_local_source_(false),
      is_usb_file_(false),
      is_seeking_(false),
      is_suspended_(false),
      use_umsinfo_(false),
      use_pipeline_preload_(false),
      updated_source_info_(false),
      buffering_(false),
      requests_play_(false),
      requests_pause_(false),
      playback_rate_(0),
      playback_rate_on_eos_(0),
      playback_rate_on_paused_(0),
      volume_(1.0),
      media_task_runner_(task_runner),
      ls_client_(media::LunaServiceClient::PrivateBus),
      preload_(PreloadNone) {
}

UMediaClientImpl::~UMediaClientImpl() {
  if (!mediaId().empty() && loaded_) {
    uMediaServer::uMediaClient::unload();
  }
}

void UMediaClientImpl::load(bool video,
                            bool is_local_source,
                            const std::string& app_id,
                            const std::string& url,
                            const std::string& mime_type,
                            const std::string& referrer,
                            const std::string& user_agent,
                            const std::string& cookies,
                            const std::string& payload,
                            const PlaybackStateCB& playback_state_cb,
                            const base::Closure& ended_cb,
                            const media::PipelineStatusCB& seek_cb,
                            const media::PipelineStatusCB& error_cb,
                            const BufferingStateCB& buffering_state_cb,
                            const base::Closure& duration_change_cb,
                            const base::Closure& video_size_change_cb,
                            const base::Closure& video_display_window_change_cb,
                            const UpdateUMSInfoCB& update_ums_info_cb,
                            const base::Closure& focus_cb) {
  DEBUG_LOG("url - %s", url.c_str());
  DEBUG_LOG("payload - %s", payload.empty()?"{}":payload.c_str());

  video_ = video;
  is_local_source_ = is_local_source;
  app_id_ = app_id;
  url_ = url;
  mime_type_ = mime_type;
  referrer_ = referrer;
  user_agent_ = user_agent;
  cookies_ = cookies;

  playback_state_cb_ = playback_state_cb;
  ended_cb_ = ended_cb;
  seek_cb_ = seek_cb;
  error_cb_ = error_cb;
  buffering_state_cb_ = buffering_state_cb;
  duration_change_cb_ = duration_change_cb;
  video_size_change_cb_ = video_size_change_cb;
  video_display_window_change_cb_ = video_display_window_change_cb;
  update_ums_info_cb_ = update_ums_info_cb;
  focus_cb_ = focus_cb;

  updated_payload_ = updateMediaOption(payload, current_time_);

  using std::placeholders::_1;
  set_source_info_callback(std::bind(&UMediaClientImpl::onSourceInfoCb, this, _1));

  if (use_pipeline_preload_) {
    uMediaServer::uMediaClient::preload(url_.c_str(), kMedia,
                                        updated_payload_.c_str());
  } else {
    LoadInternal();
  }
}

void UMediaClientImpl::LoadInternal() {
  AudioStreamClass stream_type = kMedia;

  if (use_pipeline_preload_ && !is_suspended_)
    uMediaServer::uMediaClient::notifyForeground();

  if(media_transport_type_ == "GAPLESS")
    stream_type = kGapless;

  uMediaServer::uMediaClient::loadAsync(url_.c_str(), stream_type,
                                        updated_payload_.c_str());
  load_started_ = true;

  if (!use_pipeline_preload_ && !is_suspended_)
    uMediaServer::uMediaClient::notifyForeground();
}

void UMediaClientImpl::Seek(
    base::TimeDelta time,
    const media::PipelineStatusCB& seek_cb) {
  current_time_ = time.InSecondsF();
  seek_cb_ = seek_cb;
  is_seeking_ = true;
  uMediaServer::uMediaClient::seek(time.InSecondsF() * 1000);
}

void UMediaClientImpl::SetPlaybackRate(float playback_rate) {
  if (mediaId().empty()) {
    playback_rate_ = playback_rate;
    return;
  }

  if (playback_rate == 0.0f) {
    requests_pause_ = true;
    playback_rate_on_paused_ = playback_rate_;
    uMediaServer::uMediaClient::pause();
  } else if (playback_rate_ == 0.0f) {
    requests_play_ = true;
    if (buffering_)
      dispatchBufferingEnd();
    if (playback_rate_on_paused_ != 1.0f || playback_rate != 1.0f)
      uMediaServer::uMediaClient::setPlayRate(
          playback_rate, CheckAudioOutput(playback_rate));
    if (use_pipeline_preload_ && !load_started_)
      LoadInternal();
    else
      uMediaServer::uMediaClient::play();
    playback_rate_on_eos_ = 0.0f;
  } else if (playback_rate != 0.0f && playback_rate != playback_rate_) {
    uMediaServer::uMediaClient::setPlayRate(
        playback_rate, CheckAudioOutput(playback_rate));
  }

  playback_rate_ = playback_rate;
}

void UMediaClientImpl::SetPreload(Preload preload) {
  INFO_LOG("%s(%d)", __FUNCTION__, preload);
  if (use_pipeline_preload_ && !load_started_ && preload_ == PreloadMetaData &&
      preload == PreloadAuto) {
    LoadInternal();
    use_pipeline_preload_ = false;
  }
  preload_ = preload;
}

bool UMediaClientImpl::CheckAudioOutput(float playback_rate) {
  if (playback_rate == 1.0f)
    return true;

  if (!isSupportedAudioOutputOnTrickPlaying())
    return false;

  if (playback_rate != 0.5f && playback_rate != 2.0f)
    return false;

  if (playback_rate == 2.0f && is2kVideoAndOver())
    return false;

  return true;
}

float UMediaClientImpl::GetPlaybackRate() const {
  return playback_rate_;
}

void UMediaClientImpl::SetPlaybackVolume(double volume, bool forced) {
  if (!forced && (volume_ == volume))
    return;

  volume_ = volume;

  if (mediaId().empty() || !loaded_)
    return;

  int volume_level = (int)(volume * 100);
  int duration = 0;
  EaseType type = kEaseTypeLinear;

  uMediaServer::uMediaClient::setVolume(volume_level, duration, type);
}

std::string UMediaClientImpl::mediaId() const {
  return uMediaServer::uMediaClient::media_id;
}

bool UMediaClientImpl::onLoadCompleted() {
  if (loaded_)
    return true;

  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchLoadCompleted, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchLoadCompleted() {
  if (loaded_) {
    DEBUG_LOG("ignore duplicated loadCompleted event");
    return;
  }

  DEBUG_LOG("%s", __FUNCTION__);
  loaded_ = true;

  update_ums_info_cb_.Run(mediaInfoToJson(NotifyLoadCompleted));

  if (isNotSupportedSourceInfo()) {
    has_audio_ = true;
    has_video_ = video_;
    updated_source_info_ = true;
    buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);
  }

  if (updated_source_info_ && !buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kLoadCompleted);

  if (use_pipeline_preload_)
    uMediaServer::uMediaClient::play();
}

bool UMediaClientImpl::onPreloadCompleted() {
  if (loaded_)
    return true;

  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchPreloadCompleted, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchPreloadCompleted() {
  DEBUG_LOG("%s", __FUNCTION__);

  preloaded_ = true;

  update_ums_info_cb_.Run(mediaInfoToJson(NotifyPreloadCompleted));

  if (isNotSupportedSourceInfo()) {
    has_audio_ = true;
    has_video_ = video_;
    updated_source_info_ = true;
    buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);
  }

  if (updated_source_info_ && !buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kPreloadCompleted);
}

bool UMediaClientImpl::onPlaying() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchPlaying, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchPlaying() {
  if (!requests_play_) {
    DEBUG_LOG("ignore %s", __FUNCTION__);
    return;
  }

  DEBUG_LOG("%s", __FUNCTION__);
  SetPlaybackVolume(volume_, true);
  requests_play_ = false;

  playback_state_cb_.Run(true);
  update_ums_info_cb_.Run(mediaInfoToJson(NotifyPlaying));
}

bool UMediaClientImpl::onPaused() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchPaused, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchPaused() {
  DEBUG_LOG("%s", __FUNCTION__);
  requests_pause_ = false;
  playback_rate_on_paused_ = playback_rate_;
  playback_rate_ = 0.f;

  playback_state_cb_.Run(false);
}

bool UMediaClientImpl::onEndOfStream() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchEndOfStream, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchEndOfStream() {
  DEBUG_LOG("%s", __FUNCTION__);

  playback_rate_on_eos_ = playback_rate_;
  if (duration_ > 0.)
    current_time_ = (playback_rate_ < 0) ? 0. : duration_;

  playback_rate_ = 0.0f;
  ended_cb_.Run();
  update_ums_info_cb_.Run(mediaInfoToJson(NotifyEndOfStream));
}

bool UMediaClientImpl::onSeekDone() {
  if (!is_seeking_)
    return true;

  is_seeking_ = false;
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchSeekDone, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchSeekDone() {
  if (!seek_cb_.is_null())
    base::ResetAndReturn(&seek_cb_).Run(media::PIPELINE_OK);
  update_ums_info_cb_.Run(mediaInfoToJson(NotifySeekDone));
}

bool UMediaClientImpl::onCurrentTime(int64_t currentTime) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchCurrentTime,
                 AsWeakPtr(), currentTime));
  return true;
}

void UMediaClientImpl::dispatchCurrentTime(int64_t currentTime) {
  current_time_ = currentTime / 1000.;
  if (!seek_cb_.is_null())
    base::ResetAndReturn(&seek_cb_).Run(media::PIPELINE_OK);
}

bool UMediaClientImpl::onBufferRange(
    const struct uMediaServer::buffer_range_t& bufferRange) {
  media_task_runner_->PostTask(FROM_HERE, base::Bind(
        &UMediaClientImpl::dispatchBufferRange, AsWeakPtr(), bufferRange));
  return true;
}

void UMediaClientImpl::dispatchBufferRange(
    const struct uMediaServer::buffer_range_t& bufferRange) {
  buffer_end_ = bufferRange.endTime * 1000;
  if (duration_ > 0) {
    if (bufferRange.remainingTime == -1 || buffer_end_ > 1000 * duration_)
      buffer_end_ = 1000 * duration_;
  }
}

bool UMediaClientImpl::onVideoInfo(
    const struct uMediaServer::video_info_t& videoInfo) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchVideoInfo,
                 AsWeakPtr(), videoInfo));
  return true;
}

void UMediaClientImpl::dispatchVideoInfo(
    const struct uMediaServer::video_info_t& videoInfo) {
  DEBUG_LOG("%s", __FUNCTION__);
  has_video_ = true;
  gfx::Size naturalVideoSize(videoInfo.width, videoInfo.height);

  if (natural_video_size_ != naturalVideoSize) {
    natural_video_size_ = naturalVideoSize;
    if (!video_size_change_cb_.is_null())
      video_size_change_cb_.Run();
  }
  update_ums_info_cb_.Run(mediaInfoToJson(videoInfo));
}

bool UMediaClientImpl::onAudioInfo(
    const struct uMediaServer::audio_info_t& audioInfo) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchAudioInfo,
                 AsWeakPtr(), audioInfo));
  return true;
}

void UMediaClientImpl::dispatchAudioInfo(
    const struct uMediaServer::audio_info_t& audioInfo) {
  has_audio_ = true;
  update_ums_info_cb_.Run(mediaInfoToJson(audioInfo));
}

#if UMS_INTERNAL_API_VERSION == 2
bool UMediaClientImpl::onSourceInfoCb(const struct ums::source_info_t& sourceInfo) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchSourceInfo2,
                 AsWeakPtr(), sourceInfo));
  return true;
}
#else
bool UMediaClientImpl::onSourceInfoCb(
    const struct uMediaServer::source_info_t& sourceInfo) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchSourceInfo,
                 AsWeakPtr(), sourceInfo));
  return true;
}
#endif


#if UMS_INTERNAL_API_VERSION == 2
void UMediaClientImpl::dispatchSourceInfo2(
    const struct ums::source_info_t& sourceInfo) {
  DEBUG_LOG("%s", __FUNCTION__);

  if (sourceInfo.programs.size() > 0) {
    if (sourceInfo.duration >= 0) {
      double updated_duration = sourceInfo.duration / 1000.;
      if (duration_ != updated_duration) {
        duration_ = updated_duration;
        if (!duration_change_cb_.is_null())
          duration_change_cb_.Run();
      }
    }

    has_video_ = (sourceInfo.programs[0].video_stream > 0) ? true : false;
    has_audio_ = (sourceInfo.programs[0].audio_stream > 0) ? true : false;
    num_audio_tracks_ = has_audio_ ? 1 : 0;

    if (isInsufficientSourceInfo()) {
      has_audio_ = true;
      has_video_ = video_;
    }

    if (has_video_) {
      uint32_t video_stream = sourceInfo.programs[0].video_stream;
      if (video_stream > 0 && video_stream < sourceInfo.video_streams.size()) {
        ums::video_info_t video_info = sourceInfo.video_streams[video_stream];
        gfx::Size naturalVideoSize(video_info.width, video_info.height);

        if (natural_video_size_ != naturalVideoSize) {
          natural_video_size_ = naturalVideoSize;
          if (!video_size_change_cb_.is_null())
            video_size_change_cb_.Run();
        }
      }
    }
  }
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);

  if (!updated_source_info_) {
    if (loaded_)
      buffering_state_cb_.Run(UMediaClientImpl::kLoadCompleted);
    else if (preloaded_)
      buffering_state_cb_.Run(UMediaClientImpl::kPreloadCompleted);
  }
  updated_source_info_ = true;

  update_ums_info_cb_.Run(mediaInfoToJson(sourceInfo));
}
#else
void UMediaClientImpl::dispatchSourceInfo(
    const struct uMediaServer::source_info_t& sourceInfo) {
  DEBUG_LOG("%s", __FUNCTION__);
  // Now numPrograms is always 1. if the value is 0, this case is invalid.
  if (sourceInfo.numPrograms > 0 && sourceInfo.programInfo.size()) {
    if (sourceInfo.programInfo[0].duration >= 0) {
      double updated_duration = sourceInfo.programInfo[0].duration / 1000.;
      if (duration_ != updated_duration) {
        duration_ = updated_duration;
        if (!duration_change_cb_.is_null())
          duration_change_cb_.Run();
      }
    }

    has_video_ = sourceInfo.programInfo[0].numVideoTracks?true:false;
    has_audio_ = sourceInfo.programInfo[0].numAudioTracks?true:false;
    num_audio_tracks_ = has_audio_ ?
        sourceInfo.programInfo[0].numAudioTracks : 0;

    if (isInsufficientSourceInfo()) {
      has_audio_ = true;
      has_video_ = video_;
    }

    if (has_video_
        && sourceInfo.programInfo[0].videoTrackInfo.size()
        && sourceInfo.programInfo[0].videoTrackInfo[0].width
        && sourceInfo.programInfo[0].videoTrackInfo[0].height) {
      gfx::Size naturalVideoSize(
          sourceInfo.programInfo[0].videoTrackInfo[0].width,
          sourceInfo.programInfo[0].videoTrackInfo[0].height);

      if (natural_video_size_ != naturalVideoSize) {
        natural_video_size_ = naturalVideoSize;
        if (!video_size_change_cb_.is_null())
          video_size_change_cb_.Run();
      }
    }
  }
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);

  if (!updated_source_info_) {
    if (loaded_)
      buffering_state_cb_.Run(UMediaClientImpl::kLoadCompleted);
    else if (preloaded_)
      buffering_state_cb_.Run(UMediaClientImpl::kPreloadCompleted);
  }
  updated_source_info_ = true;

  update_ums_info_cb_.Run(mediaInfoToJson(sourceInfo));
}
#endif

bool UMediaClientImpl::onExternalSubtitleTrackInfo(
    const struct uMediaServer::external_subtitle_track_info_t& trackInfo) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchExternalSubtitleTrackInfo,
                 AsWeakPtr(), trackInfo));
  return true;
}

void UMediaClientImpl::dispatchExternalSubtitleTrackInfo(
    const struct uMediaServer::external_subtitle_track_info_t& trackInfo) {
  update_ums_info_cb_.Run(mediaInfoToJson(trackInfo));
}

bool UMediaClientImpl::onError(int64_t errorCode, const std::string& errorText) {
  // ignore buffer full/low error
  if (errorCode == SMP_BUFFER_FULL || errorCode == SMP_BUFFER_LOW)
    return true;

  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchError,
                 AsWeakPtr(), errorCode, errorText));
  return true;
}

void UMediaClientImpl::dispatchError(
    int64_t errorCode, const std::string& errorText) {
  INFO_LOG("%s - %s error(%lld) - %s", __FUNCTION__, mediaId().c_str(),
      errorCode, errorText.c_str());

  update_ums_info_cb_.Run(mediaInfoToJson(errorCode, errorText));

  media::PipelineStatus status = media::PIPELINE_OK;

  if (SMP_STATUS_IS_100_GENERAL_ERROR(errorCode)) {
    status = media::PIPELINE_ERROR_INITIALIZATION_FAILED;
    // Ignore 101(Command Not Supported) status
    if (errorCode == SMP_COMMAND_NOT_SUPPORTED)
      status = media::PIPELINE_OK;
  } else if (SMP_STATUS_IS_200_PLAYBACK_ERROR(errorCode)) {
    // Playback related statuss (200 range)
    status = media::PIPELINE_ERROR_ABORT;

    // Ignore 200(Audio Codec Not Supported) status
    // when there is no audio track.
    if (errorCode == SMP_AUDIO_CODEC_NOT_SUPPORTED &&
        (!has_audio_ || has_video_))
      status = media::PIPELINE_OK;
    // Ignore 201(Video Codec Not Supported) status
    // when there is no video track.
    if (errorCode == SMP_VIDEO_CODEC_NOT_SUPPORTED && !has_video_)
      status = media::PIPELINE_OK;
    // Ignore 210(Unknown Subtitle) status
    if (errorCode == SMP_UNKNOWN_SUBTITLE)
      status = media::PIPELINE_OK;
  } else if (SMP_STATUS_IS_300_NETWORK_ERROR(errorCode)) {
    // Network related statuss (300 range)
    status = media::PIPELINE_ERROR_NETWORK;
  } else if (SMP_STATUS_IS_400_SERVER_ERROR(errorCode)) {
    // Server related statuss (400 range)
    status = media::PIPELINE_ERROR_NETWORK;
  } else if (SMP_STATUS_IS_500_DRM_ERROR(errorCode)) {
    // DRM related statuss (500 range)
    status = media::PIPELINE_ERROR_DECRYPT;
  } else if (SMP_STATUS_IS_600_RM_ERROR(errorCode)) {
    // resource is released by policy action
    if (errorCode == SMP_RM_RELATED_ERROR) {
      status = media::DECODER_ERROR_RESOURCE_IS_RELEASED;
      // force paused playback state
      pause();
      dispatchPaused();
    }
    // allocation resources status
    if (errorCode == SMP_RESOURCE_ALLOCATION_ERROR)
      status = media::PIPELINE_ERROR_ABORT;
  } else if (SMP_STATUS_IS_700_API_ERROR(errorCode)) {
    // API functionality failure,
    // but not critical status for playback (700 range)
    status = media::PIPELINE_OK;
  } else if (SMP_STATUS_IS_40000_STREAMING_ERROR(errorCode)) {
    // Streaming Protocol related statuss (40000 ~ 49999 range)
    status = media::DEMUXER_ERROR_NO_SUPPORTED_STREAMS;
  }

  if (status != media::PIPELINE_OK &&  !error_cb_.is_null())
    base::ResetAndReturn(&error_cb_).Run(status);
}

bool UMediaClientImpl::onUserDefinedChanged(const char* message) {
  std::string msg = std::string(message);
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchUserDefinedChanged,
                 AsWeakPtr(), msg));
  return true;
}

void UMediaClientImpl::dispatchUserDefinedChanged(const std::string& message) {
  update_ums_info_cb_.Run(mediaInfoToJson(message));
}

bool UMediaClientImpl::onBufferingStart() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchBufferingStart, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchBufferingStart() {
  if (!current_time_ && requests_play_)
    return;

  DEBUG_LOG("%s", __FUNCTION__);
  buffering_ = true;
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kWebOSBufferingStart);
}

bool UMediaClientImpl::onBufferingEnd() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchBufferingEnd, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchBufferingEnd() {
  DEBUG_LOG("%s", __FUNCTION__);
  buffering_ = false;
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kWebOSBufferingEnd);
}

bool UMediaClientImpl::onFocusChanged(bool) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::dispatchFocusChanged, AsWeakPtr()));
  return true;
}

void UMediaClientImpl::dispatchFocusChanged() {
  if (!focus_cb_.is_null()) {
    focus_cb_.Run();
  }
}

bool UMediaClientImpl::onActiveRegion(
    const uMediaServer::rect_t& active_region) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::dispatchActiveRegion,
                            AsWeakPtr(), active_region));
  return true;
}

void UMediaClientImpl::dispatchActiveRegion(
    const uMediaServer::rect_t& active_region) {
  if (!active_region_cb_.is_null()) {
    active_region_cb_.Run(blink::WebRect(active_region.x, active_region.y,
                                         active_region.w,
                                         active_region.h));
  }
}

void UMediaClientImpl::setDisplayWindow(const blink::WebRect& outRect,
    const blink::WebRect& inRect, bool fullScreen, bool forced) {
  using namespace uMediaServer;

  if (outRect.isEmpty())
    return;

  if (!forced && previous_display_window_ == outRect)
    return;
  previous_display_window_ = outRect;

  if (fullScreen)
    switchToFullscreen();
  else if (inRect.isEmpty())
    uMediaClient::setDisplayWindow(rect_t(outRect.x, outRect.y, outRect.width, outRect.height));
  else
    uMediaClient::setDisplayWindow(rect_t(inRect.x, inRect.y, inRect.width, inRect.height),
                                   rect_t(outRect.x, outRect.y, outRect.width, outRect.height));
}

gfx::Size UMediaClientImpl::getResoultionFromPAR(const std::string& par) {
  gfx::Size res(1, 1);

  size_t pos = par.find(":");
  if (pos == std::string::npos)
    return res;

  std::string w;
  std::string h;
  w.assign(par, 0, pos);
  h.assign(par, pos + 1, par.size() - pos - 1);
  return gfx::Size(std::stoi(w), std::stoi(h));
}

struct UMediaClientImpl::Media3DInfo UMediaClientImpl::getMedia3DInfo(
    const std::string& media_3dinfo) {
  struct Media3DInfo res;
  res.type = "LR";

  if (media_3dinfo.find("RL") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "RL";
  } else if (media_3dinfo.find("LR") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "LR";
  } else if (media_3dinfo == "bottom_top") {
    res.pattern = "top_bottom";
    res.type = "RL";
  } else {
    res.pattern = media_3dinfo;
  }

  return res;
}

std::string UMediaClientImpl::mediaInfoToJson(
    const PlaybackNotification notification) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "playbackNotificationInfo";
  eventInfo["mediaId"] = mediaId().c_str();

  switch (notification) {
    case NotifySeekDone:
      eventInfo["info"] = "seekDone";
      break;
    case NotifyPlaying:
      eventInfo["info"] = "playing";
      break;
    case NotifyPaused:
      eventInfo["info"] = "paused";
      break;
    case NotifyPreloadCompleted:
      eventInfo["info"] = "preloadCompleted";
      break;
    case NotifyLoadCompleted:
      eventInfo["info"] = "loadCompleted";
      break;
    case NotifyEndOfStream:
      eventInfo["info"] = "endOfStream";
      if (playback_rate_on_eos_ > 0.0f)
        eventInfo["direction"] = "forward";
      else if (playback_rate_on_eos_ < 0.0f)
        eventInfo["direction"] = "backward";
      break;
    default:
      return std::string("");
  }

  res = writer.write(eventInfo);

  return res;
}

#if UMS_INTERNAL_API_VERSION == 2
std::string UMediaClientImpl::mediaInfoToJson(
    const struct ums::source_info_t& value) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value sourceInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "sourceInfo";
  eventInfo["mediaId"] = mediaId().c_str();

  sourceInfo["container"] = value.container.c_str();
  sourceInfo["seekable"] = value.seekable;
  sourceInfo["numPrograms"] = value.programs.size();

  Json::Value programInfos(Json::arrayValue);
  for (int i = 0; i < value.programs.size(); i++) {
    Json::Value programInfo;
    programInfo["duration"] = static_cast<double>(value.duration);

    int numAudioTracks = 0;
    if (value.programs[i].audio_stream > 0 &&
        value.programs[i].audio_stream < value.audio_streams.size()) {
      numAudioTracks = 1;
    }
    programInfo["numAudioTracks"] = numAudioTracks;
    Json::Value audioTrackInfos(Json::arrayValue);
    for (int j = 0; j < numAudioTracks; j++) {
      Json::Value audioTrackInfo;
      int asi = value.programs[i].audio_stream;

      audioTrackInfo["codec"] =
        value.audio_streams[asi].codec.c_str();
      audioTrackInfo["bitRate"] =
        value.audio_streams[asi].bit_rate;
      audioTrackInfo["sampleRate"] =
        value.audio_streams[asi].sample_rate;

      audioTrackInfos.append(audioTrackInfo);
    }
    if (numAudioTracks)
      programInfo["audioTrackInfo"] = audioTrackInfos;

    int numVideoTracks = 0;
    if (value.programs[i].video_stream > 0 &&
        value.programs[i].video_stream < value.video_streams.size()) {
      numVideoTracks = 1;
    }

    Json::Value videoTrackInfos(Json::arrayValue);
    for (int j = 0; j < numVideoTracks; j++) {
      Json::Value videoTrackInfo;
      int vsi = value.programs[i].video_stream;

      float frame_rate = ((float)value.video_streams[vsi].frame_rate.num) /
                         ((float)value.video_streams[vsi].frame_rate.den);

      videoTrackInfo["codec"] =
        value.video_streams[vsi].codec.c_str();
      videoTrackInfo["width"] =
        value.video_streams[vsi].width;
      videoTrackInfo["height"] =
        value.video_streams[vsi].height;
      videoTrackInfo["frameRate"] = frame_rate;

      videoTrackInfo["bitRate"] =
        value.video_streams[vsi].bit_rate;

      videoTrackInfos.append(videoTrackInfo);
    }
    if (numVideoTracks)
      programInfo["videoTrackInfo"] = videoTrackInfos;

    programInfos.append(programInfo);
  }
  sourceInfo["programInfo"] = programInfos;

  eventInfo["info"] = sourceInfo;
  res = writer.write(eventInfo);

  if (previous_source_info_ == res)
    return std::string();

  previous_source_info_ = res;

  return res;
}
#else
std::string UMediaClientImpl::mediaInfoToJson(
    const struct uMediaServer::source_info_t& value) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value sourceInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "sourceInfo";
  eventInfo["mediaId"] = mediaId().c_str();

  sourceInfo["container"] = value.container.c_str();
  sourceInfo["seekable"] = value.seekable;
  sourceInfo["trickable"] = value.trickable;
  sourceInfo["numPrograms"] = value.numPrograms;

  Json::Value programInfos(Json::arrayValue);
  for (int i = 0; i < value.numPrograms; i++) {
    Json::Value programInfo;
    programInfo["duration"] =
      static_cast<double>(value.programInfo[i].duration);

    programInfo["numAudioTracks"] = value.programInfo[i].numAudioTracks;
    Json::Value audioTrackInfos(Json::arrayValue);
    for (int j = 0; j < value.programInfo[i].numAudioTracks; j++) {
      Json::Value audioTrackInfo;
      audioTrackInfo["language"] =
        value.programInfo[i].audioTrackInfo[j].language.c_str();
      audioTrackInfo["codec"] =
        value.programInfo[i].audioTrackInfo[j].codec.c_str();
      audioTrackInfo["profile"] =
        value.programInfo[i].audioTrackInfo[j].profile.c_str();
      audioTrackInfo["level"] =
        value.programInfo[i].audioTrackInfo[j].level.c_str();
      audioTrackInfo["bitRate"] =
        value.programInfo[i].audioTrackInfo[j].bitRate;
      audioTrackInfo["sampleRate"] =
        value.programInfo[i].audioTrackInfo[j].sampleRate;
      audioTrackInfo["channels"] =
        value.programInfo[i].audioTrackInfo[j].channels;
      audioTrackInfos.append(audioTrackInfo);
    }
    if (value.programInfo[i].numAudioTracks)
      programInfo["audioTrackInfo"] = audioTrackInfos;

    programInfo["numVideoTracks"] = value.programInfo[i].numVideoTracks;
    Json::Value videoTrackInfos(Json::arrayValue);
    for (int j = 0; j < value.programInfo[i].numVideoTracks; j++) {
      Json::Value videoTrackInfo;
      videoTrackInfo["angleNumber"] =
        value.programInfo[i].videoTrackInfo[j].angleNumber;
      videoTrackInfo["codec"] =
        value.programInfo[i].videoTrackInfo[j].codec.c_str();
      videoTrackInfo["profile"] =
        value.programInfo[i].videoTrackInfo[j].profile.c_str();
      videoTrackInfo["level"] =
        value.programInfo[i].videoTrackInfo[j].level.c_str();
      videoTrackInfo["width"] =
        value.programInfo[i].videoTrackInfo[j].width;
      videoTrackInfo["height"] =
        value.programInfo[i].videoTrackInfo[j].height;
      videoTrackInfo["aspectRatio"] =
        value.programInfo[i].videoTrackInfo[j].aspectRatio.c_str();
      videoTrackInfo["frameRate"] =
        value.programInfo[i].videoTrackInfo[j].frameRate;
      videoTrackInfo["bitRate"] =
        value.programInfo[i].videoTrackInfo[j].bitRate;
      videoTrackInfo["progressive"] =
        value.programInfo[i].videoTrackInfo[j].progressive;
      videoTrackInfos.append(videoTrackInfo);
    }
    if (value.programInfo[i].numVideoTracks)
      programInfo["videoTrackInfo"] = videoTrackInfos;

    programInfo["numSubtitleTracks"] = value.programInfo[i].numSubtitleTracks;
    Json::Value subtitleTrackInfos(Json::arrayValue);
    for (int j = 0; j < value.programInfo[i].numSubtitleTracks; j++) {
      Json::Value subtitleTrackInfo;
      subtitleTrackInfo["language"] =
        value.programInfo[i].subtitleTrackInfo[j].language.c_str();
      subtitleTrackInfos.append(subtitleTrackInfo);
    }
    if (value.programInfo[i].numSubtitleTracks)
      programInfo["subtitleTrackInfo"] = subtitleTrackInfos;

    programInfos.append(programInfo);
  }
  sourceInfo["programInfo"] = programInfos;

  eventInfo["info"] = sourceInfo;
  res = writer.write(eventInfo);

  if (previous_source_info_ == res)
    return std::string();

  previous_source_info_ = res;

  return res;
}
#endif

std::string UMediaClientImpl::mediaInfoToJson(
    const struct uMediaServer::video_info_t& value) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value videoInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "videoInfo";
  eventInfo["mediaId"] = mediaId().c_str();

  videoInfo["width"] = value.width;
  videoInfo["height"] = value.height;
  videoInfo["aspectRatio"] = value.aspectRatio.c_str();
  videoInfo["frameRate"] = value.frameRate;
  videoInfo["mode3D"] = value.mode3D.c_str();

  eventInfo["info"] = videoInfo;
  res = writer.write(eventInfo);

  if (previous_video_info_ == res)
    return std::string();

  previous_video_info_ = res;

  return res;
}

std::string UMediaClientImpl::mediaInfoToJson(
    const struct uMediaServer::audio_info_t& value) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value audioInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "audioInfo";
  eventInfo["mediaId"] = mediaId().c_str();

  audioInfo["sampleRate"] = value.sampleRate;
  audioInfo["channels"] = value.channels;

  eventInfo["info"] = audioInfo;
  res = writer.write(eventInfo);

  return res;
}

std::string UMediaClientImpl::mediaInfoToJson(
    const struct uMediaServer::external_subtitle_track_info_t& value) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value externalSubtitleTrackInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "externalSubtitleTrackInfo";
  eventInfo["mediaId"] = mediaId().c_str();

  externalSubtitleTrackInfo["uri"] = value.uri.c_str();
  externalSubtitleTrackInfo["hitEncoding"] = value.hitEncoding.c_str();
  externalSubtitleTrackInfo["numSubtitleTracks"] = value.numSubtitleTracks;

  Json::Value tracks(Json::arrayValue);
  for (int i = 0; i < value.numSubtitleTracks; i++) {
    Json::Value track;
    track["description"] = value.tracks[i].description.c_str();
    tracks.append(track);
  }
  externalSubtitleTrackInfo["tracks"] = tracks;

  eventInfo["info"] = externalSubtitleTrackInfo;
  res = writer.write(eventInfo);

  return res;
}

std::string UMediaClientImpl::mediaInfoToJson(
    int64_t errorCode, const std::string& errorText) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value error;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "error";
  eventInfo["mediaId"] = mediaId().c_str();

  error["errorCode"] = errorCode;
  error["errorText"] = errorText;

  eventInfo["info"] = error;
  res = writer.write(eventInfo);

  return res;
}

std::string UMediaClientImpl::mediaInfoToJson(const std::string& message) {
  if (!isRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value userDefinedChanged;
  Json::Reader reader;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "userDefinedChanged";
  eventInfo["mediaId"] = mediaId().c_str();

  if (!reader.parse(message, userDefinedChanged)) {
    DEBUG_LOG("json_reader.parse error");
    return std::string();
  }

  eventInfo["info"] = userDefinedChanged;
  res = writer.write(eventInfo);

  if (previous_user_defined_changed_ == res)
    return std::string();

  previous_user_defined_changed_ = res;

  return res;
}

bool UMediaClientImpl::DidLoadingProgress() {
  if (!duration_)
    return false;

  int64_t current_buffer_end = buffer_end_;
  bool did_loading_progress =
    current_buffer_end != buffer_end_at_last_didLoadingProgress_;
  buffer_end_at_last_didLoadingProgress_ = current_buffer_end;

  return did_loading_progress;
}

bool UMediaClientImpl::usesIntrinsicSize() {
  return !isRequiredUMSInfo();
}

std::string UMediaClientImpl::updateMediaOption(
    const std::string& mediaOption, int64_t start) {
  Json::Reader reader;
  Json::FastWriter writer;
  Json::Value media_option;
  Json::Value http_header;
  std::string res;
  bool use_pipeline_preload = false;

  if (!mediaOption.empty()) {
    if (!reader.parse(mediaOption, media_option)) {
      DEBUG_LOG("json_reader.parse error");
    } else if (media_option.isObject()) {
      if (media_option.isMember("htmlMediaOption")) {
        // Parse
        if (media_option["htmlMediaOption"].isMember("useUMSMediaInfo"))
          use_umsinfo_ =
            media_option["htmlMediaOption"]["useUMSMediaInfo"].asBool();
        if (media_option["htmlMediaOption"].isMember("usePipelinePreload"))
          use_pipeline_preload =
              media_option["htmlMediaOption"]["usePipelinePreload"].asBool();
        media_option.removeMember("htmlMediaOption");
      }
      if (media_option.isMember("mediaTransportType")) {
        media_transport_type_ = media_option["mediaTransportType"].asString();

        // support legacy spec for smartshare
        if (media_option["mediaTransportType"].asString() == "USB") {
          media_option["mediaTransportType"] = "URI";
          is_usb_file_ = true;
        }
      }
    }
  }

  if (preload_ == PreloadMetaData) {
    use_pipeline_preload_ = true;
    if (!preloaded_ && is_usb_file_ && !use_pipeline_preload)
      use_pipeline_preload_ = false;
    if (!isSupportedPreload())
      use_pipeline_preload_ = false;
  }

  http_header["referer"] = referrer_;
  http_header["userAgent"] = user_agent_;
  http_header["cookies"] = cookies_;
  media_option["option"]["transmission"]["httpHeader"] = http_header;
  media_option["option"]["bufferControl"]["userBufferCtrl"] = false;
  media_option["option"]["appId"] = app_id_;
  media_option["option"]["preload"] =
      (use_pipeline_preload_) ? "true" : "false";

  if (start)
    media_option["option"]["transmission"]["playTime"]["start"] = start;

  // check contents type
  if (!media_option.isMember("mediaTransportType")) {
    if (!mime_type_.empty()) {
      if (mime_type_ == "application/vnd.apple.mpegurl"
          || mime_type_ == "application/mpegurl"
          || mime_type_ == "application/x-mpegurl"
          || mime_type_ == "application/vnd.apple.mpegurl.audio"
          || mime_type_ == "audio/mpegurl"
          || mime_type_ == "audio/x-mpegurl")
        media_transport_type_ = "HLS";
      else if (mime_type_ == "application/dash+xml")
        media_transport_type_ = "MPEG-DASH";
      else if (mime_type_ == "application/vnd.ms-sstr+xml")
        media_transport_type_ = "MSIIS";
      else if(mime_type_ == "application/vnd.lge.gapless")
        media_transport_type_ = "GAPLESS";
    } else if (url_.find("m3u8") != std::string::npos) {
      media_transport_type_ = "HLS";
    }

    if (url_.find(kUdpUrl) != std::string::npos)
      media_transport_type_ = "UDP";
    else if (url_.find(kRtpUrl) != std::string::npos)
      media_transport_type_ = "RTP";
    else if (url_.find(kRtspUrl) != std::string::npos)
      media_transport_type_ = "RTSP";

    if (!media_transport_type_.empty())
      media_option["mediaTransportType"] = media_transport_type_;
  }

  if (media_option.empty())
    return std::string();

  res = writer.write(media_option);

  return res;
}

bool UMediaClientImpl::isRequiredUMSInfo() {
  if (media_transport_type_ == "DLNA"
      || media_transport_type_ == "HLS-LG"
      || media_transport_type_ == "USB"
      || media_transport_type_ == "MIRACAST"
      || media_transport_type_ == "DPS"
      || use_umsinfo_)
    return true;
  return false;
}

bool UMediaClientImpl::isInsufficientSourceInfo() {
  if (media_transport_type_ == "HLS"
      || media_transport_type_ == "MSIIS"
      || media_transport_type_ == "WIDEVINE"
      || media_transport_type_ == "DPS")
    return true;
  return false;
}

bool UMediaClientImpl::isAdaptiveStreaming() {
  if (media_transport_type_.compare(0, 3, "HLS") == 0
      || media_transport_type_ == "MSIIS"
      || media_transport_type_ == "MPEG-DASH"
      || media_transport_type_ == "WIDEVINE")
    return true;
  return false;
}

bool UMediaClientImpl::isNotSupportedSourceInfo() {
  if (media_transport_type_ == "MIRACAST" || media_transport_type_ == "RTP")
    return true;
  return false;
}

bool UMediaClientImpl::isSupportedBackwardTrickPlay() {
  if (media_transport_type_ == "DLNA"
      || media_transport_type_ == "HLS-LG"
      || media_transport_type_ == "USB"
      || media_transport_type_ == "DPS"
      || media_transport_type_.empty())
    return true;
  return false;
}

bool UMediaClientImpl::isSupportedPreload() {
  // HLS, MPEG-DASH, URI is supported preload
  if (media_transport_type_.compare(0, 3, "HLS") == 0 ||
      media_transport_type_ == "MPEG-DASH" || media_transport_type_ == "URI" ||
      media_transport_type_ == "USB" || media_transport_type_.empty())
    return true;
  return false;
}

bool UMediaClientImpl::is2kVideoAndOver() {
  if (!video_)
    return false;

  if (natural_video_size_ == gfx::Size())
    return false;

  float macro_blocks_of_2k = 8704.0;
  float macro_blocks_of_video =
    ceilf((float)natural_video_size_.width() / 16.0) *
    ceilf((float)natural_video_size_.height() / 16.0);

  DEBUG_LOG("macro_blocks_of_2k %f, macro_blocks_of_video %f",
      macro_blocks_of_2k, macro_blocks_of_video);

  if (macro_blocks_of_video >= macro_blocks_of_2k)
    return true;

  return false;
}

bool UMediaClientImpl::isSupportedAudioOutputOnTrickPlaying() {
  if (media_transport_type_ == "DLNA"
      || media_transport_type_ == "HLS-LG"
      || media_transport_type_ == "USB"
      || media_transport_type_ == "DPS")
    return true;
  return false;
}

void UMediaClientImpl::suspend() {
  DEBUG_LOG("%s - %s", __FUNCTION__, mediaId().c_str());
  is_suspended_ = true;
  uMediaServer::uMediaClient::notifyBackground();
}

void UMediaClientImpl::resume() {
  DEBUG_LOG("%s - %s", __FUNCTION__, mediaId().c_str());
  is_suspended_ = false;

  if (use_pipeline_preload_ && !loaded_)
    return;

  uMediaServer::uMediaClient::notifyForeground();
}

}  // namespace media
