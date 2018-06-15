// Copyright (c) 2018 LG Electronics, Inc.

#include "media/webos/base/media_apis_wrapper_gmp.h"

#pragma GCC optimize ("rtti")
#include <gmp/MediaPlayerClient.h>
#pragma GCC reset_options

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "media/base/bind_to_current_loop.h"

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("MediaAPIsWrapperGMP", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("MediaAPIsWrapperGMP:%04d " format, __LINE__, ##__VA_ARGS__)

namespace media {

const size_t kMaxPendingFeedSize = 15 * 1024 * 1024;  // 15MB

GMP_VIDEO_CODEC video_codec[] = {
  GMP_VIDEO_CODEC_NONE,
  GMP_VIDEO_CODEC_H264,
  GMP_VIDEO_CODEC_VC1,
  GMP_VIDEO_CODEC_MPEG2,
  GMP_VIDEO_CODEC_MPEG4,
  GMP_VIDEO_CODEC_THEORA,
  GMP_VIDEO_CODEC_VP8,
  GMP_VIDEO_CODEC_VP9,
  GMP_VIDEO_CODEC_H265,
};

GMP_AUDIO_CODEC audio_codec[] = {
  GMP_AUDIO_CODEC_NONE,
  GMP_AUDIO_CODEC_AAC,
  GMP_AUDIO_CODEC_MP3,
  GMP_AUDIO_CODEC_PCM,
  GMP_AUDIO_CODEC_VORBIS,
  GMP_AUDIO_CODEC_FLAC,
  GMP_AUDIO_CODEC_AMR_NB,
  GMP_AUDIO_CODEC_AMR_WB,
  GMP_AUDIO_CODEC_PCM_MULAW,
  GMP_AUDIO_CODEC_GSM_MS,
  GMP_AUDIO_CODEC_PCM_S16BE,
  GMP_AUDIO_CODEC_PCM_S24BE,
  GMP_AUDIO_CODEC_OPUS,
  GMP_AUDIO_CODEC_EAC3,
  GMP_AUDIO_CODEC_PCM_ALAW,
  GMP_AUDIO_CODEC_ALAC,
  GMP_AUDIO_CODEC_AC3,
};

static std::set<MediaAPIsWrapperGMP*> media_apis_set;

MediaAPIsWrapper* MediaAPIsWrapper::Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const PipelineStatusCB& error_cb) {
  return new MediaAPIsWrapperGMP(task_runner, video, app_id, error_cb);
}

void MediaAPIsWrapper::CheckCodecInfo(const std::string& codec_info) {
  INFO_LOG("MediaAPIsWrapper::%s [%d] START", __FUNCTION__, __LINE__);

  Json::Value codec_capability;
  Json::Reader reader;

  if (!supported_codec_mse_.isNull())
    return;

  if (!reader.parse(codec_info, codec_capability))
    return;

  Json::Value video_codecs = codec_capability["videoCodecs"];
  Json::Value audio_codecs = codec_capability["audioCodecs"];

  for (Json::Value::iterator iter = video_codecs.begin();
       iter != video_codecs.end();
       iter++) {
    if ((*iter).isObject()) {
      if ((*iter)["name"].asString() == "H.264") {
        supported_codec_mse_["H264"] = *iter;
      } else if ((*iter)["name"].asString() == "HEVC") {
        supported_codec_mse_["H265"] = *iter;
      } else if ((*iter)["name"].asString() == "VP9") {
        supported_codec_mse_["VP9"] = *iter;
      }
    }
  }

  for (Json::Value::iterator iter = audio_codecs.begin();
       iter != audio_codecs.end();
       iter++) {
    if ((*iter).isObject()) {
      if ((*iter)["name"].asString() == "AAC")
        supported_codec_mse_["AAC"] = *iter;
    }
  }
  INFO_LOG("MediaAPIsWrapper::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::Callback(const gint type, const gint64 num_value,
                                   const gchar* str_value, void* user_data) {
  DEBUG_LOG("%s type[%d] num_value[%lld] str_value[%s] data[%p]",
            __FUNCTION__, type, num_value, str_value, user_data);

  MediaAPIsWrapperGMP* that = static_cast<MediaAPIsWrapperGMP*>(user_data);
  if (!that || media_apis_set.find(that) == media_apis_set.end()) {
    INFO_LOG("Error: Callback for erased MediaAPIsWrapperGMP[%p]", that);
    return;
  }

  std::string string_value(str_value ? str_value : std::string());
  that->task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaAPIsWrapperGMP::DispatchCallback,
                 base::Unretained(that), type, num_value, string_value));
}

MediaAPIsWrapperGMP::MediaAPIsWrapperGMP(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const PipelineStatusCB& error_cb)
    : MediaAPIsWrapper(task_runner, video, app_id, error_cb) {
  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] START", __FUNCTION__, __LINE__);

  media_apis_set.insert(this);

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

MediaAPIsWrapperGMP::~MediaAPIsWrapperGMP() {
  INFO_LOG("MediaAPIsWrapperGMP::%s [%d]", __FUNCTION__, __LINE__);

  if (HasResources())
    Finalize();

  media_apis_set.erase(this);

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::Initialize(const AudioDecoderConfig& audio_config,
                                     const VideoDecoderConfig& video_config,
                                     const PipelineStatusCB& init_cb) {
  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] START", __FUNCTION__, __LINE__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb.is_null());

  audio_config_ = audio_config;
  video_config_ = video_config;
  init_cb_ = init_cb;

  switch (state_) {
    case CREATED:
      break;
    case CREATED_SUSPENDED:
      INFO_LOG("MediaAPIsWrapperGMP::%s : prevent initialization on background",
               __FUNCTION__);
      base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
      return;
    default:
      INFO_LOG("MediaAPIsWrapperGMP::%s : Invalid state [%d] for initialization",
               __FUNCTION__, state_);
      base::ResetAndReturn(&init_cb_).Run(PIPELINE_ERROR_INITIALIZATION_FAILED);
      return;
  }

  int64_t start_pts = 0;
  if (!LoadFeed(start_pts)) {
    INFO_LOG("[%p] %s : media_player_client_->LoadFeed failed", this, __FUNCTION__);
    base::ResetAndReturn(&init_cb_).Run(PIPELINE_ERROR_INITIALIZATION_FAILED);
    return;
  }

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::SetDisplayWindow(const gfx::Rect& rect,
                                           const gfx::Rect& in_rect,
                                           bool fullscreen,
                                           bool forced) {
  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] START", __FUNCTION__, __LINE__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  if (!forced && window_rect_ == rect && window_in_rect_ == in_rect)
    return;

  if (!media_player_client_.get())
    return;

  window_rect_ = rect;
  window_in_rect_ = in_rect;

  if (window_in_rect_ == gfx::Rect() || fullscreen) {
    media_player_client_->SetDisplayWindow(window_rect_.x(),
                                           window_rect_.y(),
                                           window_rect_.width(),
                                           window_rect_.height(),
                                           fullscreen);
    INFO_LOG("SetDisplayWindow - rect[%d, %d, %d, %d], fullscreen is %s",
              rect.x(), rect.y(), rect.width(), rect.height(),
              fullscreen ? "true" : "false");
  } else {
    media_player_client_->SetCustomDisplayWindow(window_in_rect_.x(),
                                                 window_in_rect_.y(),
                                                 window_in_rect_.width(),
                                                 window_in_rect_.height(),
                                                 window_rect_.x(),
                                                 window_rect_.y(),
                                                 window_rect_.width(),
                                                 window_rect_.height(),
                                                 fullscreen);
    INFO_LOG(
        "SetCustomDisplayWindow - in_rect[%d, %d, %d, %d], \
        rect[%d, %d, %d, %d], fullscreen is %s",
        in_rect.x(), in_rect.y(), in_rect.width(), in_rect.height(), rect.x(),
        rect.y(), rect.width(), rect.height(), fullscreen ? "true" : "false");
  }

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::SetSizeChangeCb(const base::Closure& size_change_cb) {
  size_change_cb_ = size_change_cb;
}

bool MediaAPIsWrapperGMP::Loaded() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  INFO_LOG("MediaAPIsWrapperGMP::%s, state_ [ %d ]", __FUNCTION__, state_);
  return (state_ >= LOADED && state_ < RESTORING);
}

bool MediaAPIsWrapperGMP::Feed(const scoped_refptr<DecoderBuffer>& buffer,
                               FeedType type) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("MediaAPIsWrapperGMP::%s, type[ %d ]", __FUNCTION__, type);

  // When MediaAPIsWrapperGMP recovers from finalization, the all information
  // on audio & video are gone. It is required to feed key frames first.
  if (state_ == RESTORING && !buffer->is_key_frame() &&
      ((type == Audio && feeded_audio_pts_ < 0) ||
       (type == Video && feeded_video_pts_ < 0))) {
    DCHECK(buffer_queue_->empty());
    return true;
  }

  if (!HasResources()) {
    INFO_LOG("MediaAPIsWrapperGMP::%s Resources are released", __FUNCTION__);
    return false;
  }

  buffer_queue_->push(buffer, type);

  while (!buffer_queue_->empty()) {
    FeedStatus feed_status = FeedInternal(buffer_queue_->front().first,
                                          buffer_queue_->front().second);
    switch (feed_status) {
      case FeedSucceeded: {
        DEBUG_LOG("MediaAPIsWrapperGMP::%s Feed Succeeded", __FUNCTION__);
        buffer_queue_->pop();
        continue;
      }
      case FeedFailed: {
        INFO_LOG("MediaAPIsWrapperGMP::%s Feed failed", __FUNCTION__);
        return false;
      }
      case FeedOverflowed: {
        if (buffer_queue_->data_size() > kMaxPendingFeedSize) {
          INFO_LOG("MediaAPIsWrapperGMP::%s pending feed(%zu) exceeded the \
                    limit(%zu)", __FUNCTION__, buffer_queue_->data_size(),
                    kMaxPendingFeedSize);
          return false;
        } else {
          INFO_LOG("MediaAPIsWrapperGMP::%s BUFFER_FULL: pending feed size=%zu",
                    __FUNCTION__, buffer_queue_->data_size());
        }
        return true;
      }
    }
  }

  return true;
}

uint64_t MediaAPIsWrapperGMP::GetCurrentTime() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  INFO_LOG("MediaAPIsWrapperGMP::%s, current_time_[ %llu ]", __FUNCTION__,
           current_time_);
  return current_time_;
}

bool MediaAPIsWrapperGMP::Seek(base::TimeDelta time) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  int64_t pts = time.InMicroseconds() * 1000;
  INFO_LOG("MediaAPIsWrapperGMP::%s time [%lld]", __FUNCTION__, pts);

  if (!Loaded()) {
    // clear incompletely loaded pipeline
    SetState(CREATED);
    media_player_client_.reset(NULL);
    LoadFeed(pts);
    return true;
  }

  ResetFeed();

  unsigned seek_time = static_cast<unsigned>(time.InMilliseconds());
  if (media_player_client_->Seek(seek_time)) {
    SetState(SEEKING);
    return true;
  }

  return false;
}

void MediaAPIsWrapperGMP::SetPlaybackRate(float playback_rate) {
  INFO_LOG("MediaAPIsWrapperGMP::%s :rate(%f -> %f)", __FUNCTION__,
           playback_rate_, playback_rate);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  float current_playback_rate = playback_rate_;
  playback_rate_ = playback_rate;

  if (Loaded() || !IsSuspended()) {
    if (current_playback_rate == 0.0f && playback_rate != 0.0f)
      PlayFeed();
    else if (current_playback_rate != 0.0f && playback_rate == 0.0f) {
      PauseFeed();
    }
  }

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::Suspend() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  INFO_LOG("MediaAPIsWrapperGMP::%s state_[ %d ]", __FUNCTION__, state_);
  //TODO: long play_state;
  switch (state_) {
    case LOADING:
      media_player_client_.reset(NULL);
      ResetFeed();
    // intentional no-break
    case CREATED:
      SetState(CREATED_SUSPENDED);
      break;
    case SUSPENDED:
      return;
    case LOADED:
    case PLAYING:
    case SEEKING:
      media_player_client_->Pause();
    // intentional no-break
    case PAUSED:
      media_player_client_->NotifyBackground();
      SetState(SUSPENDED);
      break;
    case FINALIZED:
      break;
    default:
      break;
  }

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::Resume(base::TimeDelta paused_time,
                                 RestorePlaybackMode restore_playback_mode) {
  INFO_LOG("MediaAPIsWrapperGMP::%s (%lld) restore mode=%s", __FUNCTION__,
           paused_time.InMilliseconds(),
           restore_playback_mode == RESTORE_PLAYING ? "RESTORE_PLAYING"
                                                    : "RESTORE_PAUSED");

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!media_player_client_) {
    DCHECK(!HasResources());

    int64_t pts = paused_time.InMicroseconds() * 1000;
    if (!LoadFeed(pts)) {
      INFO_LOG("[%p] %s Failed to reinitialize", this, __FUNCTION__);
      base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
    }
    return;
  }

  media_player_client_->NotifyForeground();

  SetState(RESUMING);

  if (restore_playback_mode == RESTORE_PLAYING) {
    Seek(paused_time);
    return;
  }

  PauseFeed();

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

bool MediaAPIsWrapperGMP::IsReleasedMediaResource() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  INFO_LOG("MediaAPIsWrapperGMP::%s, state_ [ %d ]", __FUNCTION__, state_);
  return !HasResources();
}

bool MediaAPIsWrapperGMP::AllowedFeedVideo() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  bool video_feed = video_config_.IsValidConfig() &&
      (state_ == PLAYING) && buffer_queue_->empty() &&
      (audio_config_.IsValidConfig()
          ? feeded_video_pts_ - feeded_audio_pts_ < 1000000000
          : feeded_video_pts_ - current_time_ < 3000000000);
  DEBUG_LOG("%s [ video_feed = %d ]", __FUNCTION__, video_feed);
  return video_feed;
}

bool MediaAPIsWrapperGMP::AllowedFeedAudio() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  bool audio_feed = audio_config_.IsValidConfig() &&
                    (state_ == PLAYING) && buffer_queue_->empty();
  DEBUG_LOG("%s [ audio_feed = %d ]", __FUNCTION__, audio_feed);
  return audio_feed;
}

void MediaAPIsWrapperGMP::Finalize() {
  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] START", __FUNCTION__, __LINE__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  media_player_client_.reset(NULL);

  ResetFeed();

  SetState(FINALIZED);

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::SetPlaybackVolume(double volume) {
  INFO_LOG("MediaAPIsWrapperGMP::%s volume: %f -> %f", __FUNCTION__,
           playback_volume_, volume);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (playback_volume_ == volume)
    return;

  SetVolumeInternal(volume);

  playback_volume_ = volume;

  INFO_LOG("MediaAPIsWrapperGMP::%s, playback_volume_[%d]", __FUNCTION__,
           playback_volume_);
}


bool MediaAPIsWrapperGMP::IsEOSReceived() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  DEBUG_LOG("MediaAPIsWrapperGMP::%s, received_eos[ %d ]", __FUNCTION__,
           received_eos_);
  return received_eos_;
}

void MediaAPIsWrapperGMP::DispatchCallback(
      const gint type, const gint64 num_value, const std::string& str_value) {
  DEBUG_LOG("%s [this = %p] type[%d] num[%lld] str[%s]", __FUNCTION__,
            this, type, num_value, str_value.c_str());

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  DCHECK(task_runner_->BelongsToCurrentThread());

  switch (static_cast<NOTIFY_TYPE_T>(type)) {
    case NOTIFY_LOAD_COMPLETED:
      NotifyLoadComplete();
      break;
    case NOTIFY_UNLOAD_COMPLETED:
      INFO_LOG("%s, NOTIFY_UNLOAD_COMPLETED", __FUNCTION__);
      break;
    case NOTIFY_END_OF_STREAM:
      INFO_LOG("%s, NOTIFY_END_OF_STREAM", __FUNCTION__);
      received_eos_ = true;
      break;
    case NOTIFY_CURRENT_TIME:
      UpdateCurrentTime(num_value);
      break;
    case NOTIFY_SEEK_DONE:
      INFO_LOG("%s, NOTIFY_SEEK_DONE, playback_rate_[ %f ]", __FUNCTION__,
               playback_rate_);
      if (playback_rate_ > 0.0f)
        PlayFeed();
      break;
    case NOTIFY_PLAYING:
      INFO_LOG("%s, NOTIFY_PLAYING", __FUNCTION__);
      break;
    case NOTIFY_PAUSED:
      INFO_LOG("%s, NOTIFY_PAUSED", __FUNCTION__);
      break;
    case NOTIFY_NEED_DATA:
    case NOTIFY_ENOUGH_DATA:
    case NOTIFY_SEEK_DATA:
      break;
    case NOTIFY_ERROR:
      INFO_LOG("%s, ERROR(%lld)", __FUNCTION__, num_value);
      if (num_value == GMP_ERROR_STREAM) {
        if (!error_cb_.is_null())
          base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
      } else if (num_value == GMP_ERROR_ASYNC) {
        if (!error_cb_.is_null())
          base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_ABORT);
      }
      break;
    case NOTIFY_VIDEO_INFO:
      UpdateVideoInfo(str_value);
      break;
    case NOTIFY_AUDIO_INFO:
      INFO_LOG("%s, NOTIFY_AUDIO_INFO", __FUNCTION__);
      break;
    case NOTIFY_BUFFER_FULL:
    case NOTIFY_BUFFER_NEED:
      INFO_LOG("%s, BUFFER FULL/NEED", __FUNCTION__);
      break;
    default:
      INFO_LOG("%s, Default case, Type(%d)", __FUNCTION__, type);
      break;
  }
}

bool MediaAPIsWrapperGMP::LoadFeed(int64_t time) {
  INFO_LOG("MediaAPIsWrapperGMP::%s, time [ %lld ]", __FUNCTION__, time);

  if (!media_player_client_) {
    media_player_client_.reset(new gmp::player::MediaPlayerClient(app_id_));
    media_player_client_->RegisterCallback(
        &MediaAPIsWrapperGMP::Callback, this);
  }

  State next_state = INVALID;
  switch (state_) {
    case FINALIZED:
      next_state = RESTORING;
      break;
    case CREATED:
    case CREATED_SUSPENDED:
      next_state = LOADING;
      break;
    default:
      INFO_LOG("%s Invalid state %d", __FUNCTION__, state_);
      return false;
  }

  INFO_LOG("MediaAPIsWrapperGMP::%s media_player_client_[%p] [this = %p ]!!!",
           __FUNCTION__, media_player_client_.get(), this);
  media_player_client_->NotifyForeground();

  MEDIA_LOAD_DATA_T load_data;
  if (!MakeLoadData(time, &load_data)) {
    INFO_LOG("Making load data info Failed !!!");
    base::ResetAndReturn(&error_cb_).Run(DECODER_ERROR_NOT_SUPPORTED);
    return false;
  }

  if (!media_player_client_->Load(&load_data)) {
    INFO_LOG("media_player_client_->Load failed");
    return false;
  }

  current_time_ = time;

  ResetFeed();

  SetState(next_state);

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
  return true;
}

bool MediaAPIsWrapperGMP::PlayFeed() {
  INFO_LOG("MediaAPIsWrapperGMP::%s, state_ [ %d ]", __FUNCTION__, state_);

  if (state_ == PLAYING || IsSuspended() || !Loaded())
    return false;

  DCHECK(media_player_client_);

  if (media_player_client_->Play()) {
    INFO_LOG("MediaAPIsWrapper::%s [%d] END", __FUNCTION__, __LINE__);
    SetState(PLAYING);
    return true;
  }

  INFO_LOG("MediaAPIsWrapper::%s [%d] Invalid state", __FUNCTION__, __LINE__);
  return false;
}

bool MediaAPIsWrapperGMP::PauseFeed() {
  INFO_LOG("MediaAPIsWrapperGMP::%s, state_ [ %d ]", __FUNCTION__, state_);

  if (state_ == PAUSED || IsSuspended() || !Loaded())
    return false;

  DCHECK(media_player_client_);

  if (media_player_client_->Pause()) {
    INFO_LOG("MediaAPIsWrapper::%s [%d] END", __FUNCTION__, __LINE__);
    SetState(PAUSED);
    return true;
  }

  INFO_LOG("MediaAPIsWrapper::%s [%d] Invalid state", __FUNCTION__, __LINE__);
  return false;
}

void MediaAPIsWrapperGMP::ResetFeed() {
  feeded_audio_pts_ = -1;
  feeded_video_pts_ = -1;
  audio_eos_received_ = audio_config_.IsValidConfig() ? false : true;
  video_eos_received_ = video_config_.IsValidConfig() ? false : true;
  received_eos_ = false;

  buffer_queue_->clear();
}

void MediaAPIsWrapperGMP::PushEOS() {
  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] START", __FUNCTION__, __LINE__);

  if (media_player_client_)
    media_player_client_->PushEndOfStream();

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

// private helper functions
bool MediaAPIsWrapperGMP::HasResources() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  DEBUG_LOG("%s [ state_ = %d ]", __FUNCTION__, state_);
  return (state_ >= LOADING && state_ < FINALIZED);
}

bool MediaAPIsWrapperGMP::IsSuspended() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  INFO_LOG("MediaAPIsWrapperGMP::%s, state_ [ %d ]", __FUNCTION__, state_);
  return (state_ == CREATED_SUSPENDED || state_ ==SUSPENDED);
}

void MediaAPIsWrapperGMP::SetVolumeInternal(double volume) {
  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] [volume = %f]", __FUNCTION__, __LINE__,
           volume);

  if (HasResources())
    media_player_client_->SetVolume(volume * 100);

  INFO_LOG("MediaAPIsWrapperGMP::%s [%d] END", __FUNCTION__, __LINE__);
}

void MediaAPIsWrapperGMP::NotifyLoadComplete() {
  INFO_LOG("MediaAPIsWrapperGMP::%s, state_ [ %d ]", __FUNCTION__, state_);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (IsSuspended())
    return;

  SetState(LOADED);

  if (playback_rate_ > 0.0f)
    PlayFeed();

  SetVolumeInternal(playback_volume_);

  base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
}

MediaAPIsWrapperGMP::FeedStatus MediaAPIsWrapperGMP::FeedInternal(
    const scoped_refptr<DecoderBuffer>& buffer, FeedType type) {
  uint64_t pts = buffer->timestamp().InMicroseconds() * 1000;
  const uint8_t* data = buffer->data();
  size_t size = buffer->data_size();

  if (buffer->end_of_stream()) {
    DEBUG_LOG("MediaAPIsWrapperGMP::%s EOS recieved", __FUNCTION__);
    if (type == Audio)
      audio_eos_received_ = true;
    else
      video_eos_received_ = true;

    if (audio_eos_received_ && video_eos_received_)
      PushEOS();

    return FeedSucceeded;
  }

  MEDIA_DATA_CHANNEL_T es_type = MEDIA_DATA_CH_NONE;
  if (type == Video)
    es_type = MEDIA_DATA_CH_A;
  else if (type == Audio)
    es_type = MEDIA_DATA_CH_B;

  const guint8* p_buffer = static_cast<const guint8*>(data);
  DEBUG_LOG("%s [this = %p] [buffer = %p, size = %d, pts = %lld, es_type = %d]",
            __FUNCTION__, this, p_buffer, size, pts, es_type);

  MEDIA_STATUS_T media_status =
      media_player_client_->Feed(p_buffer, size, pts, es_type);
  if (media_status != MEDIA_OK) {
    INFO_LOG("MediaAPIsWrapperGMP::%s media_status = %d", __FUNCTION__,
              media_status);
    if (media_status == MEDIA_BUFFER_FULL)
      return FeedOverflowed;
    return FeedFailed;
  }

  if (type == Audio)
    feeded_audio_pts_ = pts;
  else
    feeded_video_pts_ = pts;

  return FeedSucceeded;
}

void MediaAPIsWrapperGMP::UpdateVideoInfo(const std::string& info_str) {
  DEBUG_LOG("%s video_info=%s", __FUNCTION__, info_str.c_str());

  Json::Reader reader;
  Json::Value json_value;

  if (!reader.parse(info_str, json_value))
    return;

  Json::Value video_info = json_value["videoInfo"];
  uint32_t video_width = static_cast<uint32_t>(video_info["width"].asUInt());
  uint32_t video_height = static_cast<uint32_t>(video_info["height"].asUInt());
  INFO_LOG("%s video_info [ %d, %d]", __FUNCTION__, video_width, video_height);

  gfx::Size natural_size(video_width, video_height);
  if (natural_size_ != natural_size) {
    natural_size_ = natural_size;

    if (!size_change_cb_.is_null())
      size_change_cb_.Run();
  }
}

bool MediaAPIsWrapperGMP::MakeLoadData(int64_t time,
                                       MEDIA_LOAD_DATA_T* load_data) {
  load_data->maxWidth = 1920;
  load_data->maxHeight = 1080;
  load_data->maxFrameRate = 30;

  if (video_config_.IsValidConfig()) {
    INFO_LOG("%s [video_codec = %d]", __FUNCTION__, video_config_.codec());
    switch (video_config_.codec()) {
      case media::kCodecH264:
        load_data->maxWidth = supported_codec_mse_["H264"]["maxWidth"].asInt();
        load_data->maxHeight =
            supported_codec_mse_["H264"]["maxHeight"].asInt();
        load_data->maxFrameRate =
            supported_codec_mse_["H264"]["maxFrameRate"].asInt();
        break;
      case media::kCodecVP9:
        load_data->maxWidth = supported_codec_mse_["VP9"]["maxWidth"].asInt();
        load_data->maxHeight = supported_codec_mse_["VP9"]["maxHeight"].asInt();
        load_data->maxFrameRate =
            supported_codec_mse_["VP9"]["maxFrameRate"].asInt();
        break;
      case media::kCodecHEVC:
        load_data->maxWidth = supported_codec_mse_["H265"]["maxWidth"].asInt();
        load_data->maxHeight =
            supported_codec_mse_["H265"]["maxHeight"].asInt();
        break;
      default:
        INFO_LOG("Not Supported Video Codec(%d)",
                 static_cast<int>(video_config_.codec()));
        return false;
    }
    load_data->videoCodec = video_codec[video_config_.codec()];
    load_data->width = video_config_.natural_size().width();
    load_data->height = video_config_.natural_size().height();
    load_data->frameRate = 30;
    load_data->extraData = (void *)video_config_.extra_data().data();
    load_data->extraSize = video_config_.extra_data().size();
  }

  if (audio_config_.IsValidConfig()) {
    INFO_LOG("%s [audio_codec = %d]", __FUNCTION__, audio_config_.codec());
    load_data->audioCodec = audio_codec[audio_config_.codec()];
    load_data->channels = audio_config_.channel_layout();
    load_data->sampleRate = audio_config_.samples_per_second();
    load_data->bitRate = audio_config_.bits_per_channel();
    load_data->bitsPerSample = 8 * audio_config_.bytes_per_frame();
  }
  load_data->ptsToDecode = time;

  INFO_LOG("Outgoing Codec Info[Audio = %d, Video = %d]",
           load_data->audioCodec, load_data->videoCodec);

  return true;
}

}  // namespace media
