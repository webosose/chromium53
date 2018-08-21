// Copyright (c) 2018 LG Electronics, Inc.

#include "media/webos/base/media_apis_wrapper_gmp.h"

#pragma GCC optimize("rtti")
#include <gmp/MediaPlayerClient.h>
#pragma GCC reset_options

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "media/base/bind_to_current_loop.h"

#define INFO_LOG(format, ...)                                      \
  RAW_PMLOG_INFO("MediaAPIsWrapperGmp", ":%04d " format, __LINE__, \
                 ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("MediaAPIsWrapperGmp:%04d " format, __LINE__, ##__VA_ARGS__)

namespace media {

const size_t kMaxPendingFeedSize = 15 * 1024 * 1024;  // 15MB

GMP_VIDEO_CODEC video_codec[] = {
    GMP_VIDEO_CODEC_NONE,  GMP_VIDEO_CODEC_H264,  GMP_VIDEO_CODEC_VC1,
    GMP_VIDEO_CODEC_MPEG2, GMP_VIDEO_CODEC_MPEG4, GMP_VIDEO_CODEC_THEORA,
    GMP_VIDEO_CODEC_VP8,   GMP_VIDEO_CODEC_VP9,   GMP_VIDEO_CODEC_H265,
};

GMP_AUDIO_CODEC audio_codec[] = {
    GMP_AUDIO_CODEC_NONE,      GMP_AUDIO_CODEC_AAC,
    GMP_AUDIO_CODEC_MP3,       GMP_AUDIO_CODEC_PCM,
    GMP_AUDIO_CODEC_VORBIS,    GMP_AUDIO_CODEC_FLAC,
    GMP_AUDIO_CODEC_AMR_NB,    GMP_AUDIO_CODEC_AMR_WB,
    GMP_AUDIO_CODEC_PCM_MULAW, GMP_AUDIO_CODEC_GSM_MS,
    GMP_AUDIO_CODEC_PCM_S16BE, GMP_AUDIO_CODEC_PCM_S24BE,
    GMP_AUDIO_CODEC_OPUS,      GMP_AUDIO_CODEC_EAC3,
    GMP_AUDIO_CODEC_PCM_ALAW,  GMP_AUDIO_CODEC_ALAC,
    GMP_AUDIO_CODEC_AC3,
};

static std::set<MediaAPIsWrapperGmp*> media_apis_set;

MediaAPIsWrapper* MediaAPIsWrapper::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const PipelineStatusCB& error_cb) {
  return new MediaAPIsWrapperGmp(task_runner, video, app_id, error_cb);
}

void MediaAPIsWrapper::CheckCodecInfo(const std::string& codec_info) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value codec_capability;
  Json::Reader reader;

  if (!supported_codec_mse_.isNull())
    return;

  if (!reader.parse(codec_info, codec_capability))
    return;

  Json::Value video_codecs = codec_capability["videoCodecs"];
  Json::Value audio_codecs = codec_capability["audioCodecs"];

  for (Json::Value::iterator iter = video_codecs.begin();
       iter != video_codecs.end(); iter++) {
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
       iter != audio_codecs.end(); iter++) {
    if ((*iter).isObject()) {
      if ((*iter)["name"].asString() == "AAC")
        supported_codec_mse_["AAC"] = *iter;
    }
  }
}

void MediaAPIsWrapperGmp::Callback(const gint type,
                                   const gint64 num_value,
                                   const gchar* str_value,
                                   void* user_data) {
  DEBUG_LOG("%s type[%d] num_value[%lld] str_value[%s] data[%p]", __FUNCTION__,
            type, num_value, str_value, user_data);
  MediaAPIsWrapperGmp* that = static_cast<MediaAPIsWrapperGmp*>(user_data);
  if (that && that->is_finalized_)
    return;

  if (!that || media_apis_set.find(that) == media_apis_set.end()) {
    INFO_LOG("Error: Callback for erased [%p]", that);
    return;
  }

  std::string string_value(str_value ? str_value : std::string());
  that->task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaAPIsWrapperGmp::DispatchCallback, base::Unretained(that),
                 type, num_value, string_value));
}

MediaAPIsWrapperGmp::MediaAPIsWrapperGmp(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const PipelineStatusCB& error_cb)
    : MediaAPIsWrapper(task_runner, video, app_id, error_cb),
      ls_client_(media::LunaServiceClient::PrivateBus),
      play_internal_(false),
      released_media_resource_(false),
      is_destructed_(false),
      is_suspended_(false),
      load_completed_(false),
      is_finalized_(false) {
  INFO_LOG("MediaAPIsWrapper Created[%p]::%s", this, __FUNCTION__);

  media_player_client_.reset(new gmp::player::MediaPlayerClient(app_id));
  media_player_client_->RegisterCallback(&MediaAPIsWrapperGmp::Callback, this);

  media_apis_set.insert(this);

  INFO_LOG("%s [%d] END", __FUNCTION__, __LINE__);
}

MediaAPIsWrapperGmp::~MediaAPIsWrapperGmp() {
  INFO_LOG("MediaAPIsWrapper Destroyed[%p]::%s", this, __FUNCTION__);
}

void MediaAPIsWrapperGmp::Initialize(const AudioDecoderConfig& audio_config,
                                     const VideoDecoderConfig& video_config,
                                     const PipelineStatusCB& init_cb) {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  DCHECK(task_runner_->BelongsToCurrentThread());

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DCHECK(!init_cb.is_null());

  audio_config_ = audio_config;
  video_config_ = video_config;
  init_cb_ = init_cb;

  MEDIA_LOAD_DATA_T load_data;
  if (!MakeLoadData(0, &load_data)) {
    INFO_LOG("[%p] %s ->Making load data info Failed !!!", this, __FUNCTION__);
    return;
  }

  if (is_suspended_) {
    INFO_LOG("[%p] %s -> prevent background init", this, __FUNCTION__);
    released_media_resource_ = true;
    base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
    return;
  }

  DEBUG_LOG("[%p] %s-> call NotifyForeground", this, __FUNCTION__);
  media_player_client_->NotifyForeground();

  if (!media_player_client_->Load(&load_data)) {
    INFO_LOG("[%p] %s : media_player_client_->Load Failed", this, __FUNCTION__);
    base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
    return;
  }

  ResetFeedInfo();

  base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
}

bool MediaAPIsWrapperGmp::Loaded() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return load_completed_;
}

void MediaAPIsWrapperGmp::SetDisplayWindow(const gfx::Rect& rect,
                                           const gfx::Rect& in_rect,
                                           bool fullscreen,
                                           bool forced) {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  if (!forced && window_rect_ == rect && window_in_rect_ == in_rect)
    return;

  if (!media_player_client_.get())
    return;

  window_rect_ = rect;
  window_in_rect_ = in_rect;

  if (window_in_rect_ == gfx::Rect() || fullscreen) {
    media_player_client_->SetDisplayWindow(window_rect_.x(), window_rect_.y(),
                                           window_rect_.width(),
                                           window_rect_.height(), fullscreen);
    INFO_LOG("[%p] %s -> rect[%d, %d, %d, %d], fullscreen is %s", this,
             __FUNCTION__, rect.x(), rect.y(), rect.width(), rect.height(),
             fullscreen ? "true" : "false");
  } else {
    media_player_client_->SetCustomDisplayWindow(
        window_in_rect_.x(), window_in_rect_.y(), window_in_rect_.width(),
        window_in_rect_.height(), window_rect_.x(), window_rect_.y(),
        window_rect_.width(), window_rect_.height(), fullscreen);
    INFO_LOG(
        "[%p] %s -> in_rect[%d, %d, %d, %d], rect[%d, %d, %d, %d], \
             fullscreen [%s]",
        this, __FUNCTION__, in_rect.x(), in_rect.y(), in_rect.width(),
        in_rect.height(), rect.x(), rect.y(), rect.width(), rect.height(),
        fullscreen ? "true" : "false");
  }
}

bool MediaAPIsWrapperGmp::Feed(const scoped_refptr<DecoderBuffer>& buffer,
                               FeedType type) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (is_finalized_ || released_media_resource_)
    return true;

  buffer_queue_->push(buffer, type);

  while (!buffer_queue_->empty()) {
    FeedStatus feed_status = FeedInternal(buffer_queue_->front().first,
                                          buffer_queue_->front().second);
    switch (feed_status) {
      case FeedSucceeded: {
        buffer_queue_->pop();
        continue;
      }
      case FeedFailed: {
        INFO_LOG("[%p] %s Feed failed", this, __FUNCTION__);
        return false;
      }
      case FeedOverflowed: {
        if (buffer_queue_->data_size() > kMaxPendingFeedSize) {
          INFO_LOG("[%p] %s pending feed(%zu) exceeded the limit(%zu)", this,
                   __FUNCTION__, buffer_queue_->data_size(),
                   kMaxPendingFeedSize);
          return false;
        }
        DEBUG_LOG("[%p] %s BUFFER_FULL: pending feed size=%zu", this,
                  __FUNCTION__, buffer_queue_->data_size());
        return true;
      }
    }
  }

  return true;
}

uint64_t MediaAPIsWrapperGmp::GetCurrentTime() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return current_time_;
}

bool MediaAPIsWrapperGmp::Seek(base::TimeDelta time) {
  DEBUG_LOG("[%p] %s to %lldms", this, __FUNCTION__, time.InMilliseconds());

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!media_player_client_)
    return true;

  ResetFeedInfo();

  if (!load_completed_) {
    // clear incompletely loaded pipeline
    if (resume_time_ != time) {
      media_player_client_.reset(NULL);

      INFO_LOG("[%p] %s : Load not finished, Trye to ReInitialize..", this,
               __FUNCTION__);
      ReInitialize(time);
    }
    return true;
  }

  SetState(SEEKING);

  unsigned seek_time = static_cast<unsigned>(time.InMilliseconds());
  return media_player_client_->Seek(seek_time);
}

void MediaAPIsWrapperGmp::SetPlaybackRate(float playback_rate) {
  DEBUG_LOG("[%p] %s : rate(%f -> %f)", this, __FUNCTION__, playback_rate_,
            playback_rate);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  float current_playback_rate = playback_rate_;
  playback_rate_ = playback_rate;

  DEBUG_LOG("[%p] %s : media_player_client_[ %p ]", this, __FUNCTION__,
            media_player_client_.get());
  if (!media_player_client_)
    return;

  if (playback_rate == 0.0f && !video_config_.IsValidConfig() &&
      audio_config_.IsValidConfig()) {
    INFO_LOG("[%p] %s -> Play for audio only case", this, __FUNCTION__);
    current_playback_rate = 0.0f;
    playback_rate = 1.0;
  }

  if (current_playback_rate == 0.0f && playback_rate != 0.0f) {
    INFO_LOG("[%p] %s -> load_completed_ = %d", this, __FUNCTION__,
             load_completed_);
    if (load_completed_)
      PlayInternal();
    else
      play_internal_ = true;
    return;
  }

  if (current_playback_rate != 0.0f && playback_rate == 0.0f) {
    DEBUG_LOG("[%p] %s -> Call PauseInternal", this, __FUNCTION__);
    PauseInternal();
  }
}

void MediaAPIsWrapperGmp::Suspend() {
  DEBUG_LOG("[%p] %s -> media_player_client_[%p] is_finalized_[%d]", this,
            __FUNCTION__, media_player_client_.get(), is_finalized_);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!media_player_client_)
    return;

  is_suspended_ = true;
  if (load_completed_ && is_finalized_)
    media_player_client_->NotifyBackground();
}

void MediaAPIsWrapperGmp::Resume(base::TimeDelta paused_time,
                                 RestorePlaybackMode restore_playback_mode) {
  DEBUG_LOG("[%p] %s (%lldms) restore mode=%s", this, __FUNCTION__,
            paused_time.InMilliseconds(),
            restore_playback_mode == RESTORE_PLAYING ? "RESTORE_PLAYING"
                                                     : "RESTORE_PAUSED");

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  is_suspended_ = false;
  if (released_media_resource_) {
    if (playback_rate_ == 0.0f)
      play_internal_ = false;
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MediaAPIsWrapperGmp::ReInitialize, this, paused_time));
    return;
  }

  if (load_completed_)
    media_player_client_->NotifyForeground();
}

bool MediaAPIsWrapperGmp::IsReleasedMediaResource() {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return released_media_resource_;
}

bool MediaAPIsWrapperGmp::AllowedFeedVideo() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return video_config_.IsValidConfig() && state_ == PLAYING &&
         buffer_queue_->empty() &&
         (audio_config_.IsValidConfig()
              ?  // preventing video overrun
              feeded_video_pts_ - feeded_audio_pts_ < 1000000000
              : feeded_video_pts_ - current_time_ < 3000000000);
}

bool MediaAPIsWrapperGmp::AllowedFeedAudio() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return audio_config_.IsValidConfig() && state_ == PLAYING &&
         buffer_queue_->empty();
}

void MediaAPIsWrapperGmp::Finalize() {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  is_destructed_ = true;
  is_finalized_ = true;

  media_apis_set.erase(this);

  if (media_player_client_.get())
    media_player_client_.reset(NULL);

  ResetFeedInfo();
}

void MediaAPIsWrapperGmp::SetPlaybackVolume(double volume) {
  DEBUG_LOG("[%p] %s volume: %f -> %f", this, __FUNCTION__, playback_volume_,
            volume);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!load_completed_) {
    playback_volume_ = volume;
    return;
  }

  if (playback_volume_ == volume)
    return;

  SetVolumeInternal(volume);

  playback_volume_ = volume;
}

bool MediaAPIsWrapperGmp::IsEOSReceived() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return received_eos_;
}

std::string MediaAPIsWrapperGmp::GetMediaID(void) {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!media_player_client_)
    return std::string();

  const char* media_id = media_player_client_->GetMediaID();
  return std::string(media_id ? media_id : std::string());
}

void MediaAPIsWrapperGmp::Unload() {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (load_completed_) {
    load_completed_ = false;
    released_media_resource_ = true;

    if (media_player_client_) {
      is_finalized_ = true;
      INFO_LOG("[%p] %s - destroy media client id ( %s )", this, __FUNCTION__,
               GetMediaID().c_str());
      media_player_client_.reset(NULL);
    }
  }
}

void MediaAPIsWrapperGmp::SetSizeChangeCb(const base::Closure& size_change_cb) {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);
  size_change_cb_ = size_change_cb;
}

void MediaAPIsWrapperGmp::SetLoadCompletedCb(
    const LoadCompletedCB& load_completed_cb) {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);
  load_completed_cb_ = load_completed_cb;
}

void MediaAPIsWrapperGmp::DispatchCallback(const gint type,
                                           const gint64 num_value,
                                           const std::string& str_value) {
  DEBUG_LOG("[%p] %s : [type=%d numValue=%lld strValue=%s]", this, __FUNCTION__,
            type, num_value, str_value.c_str());

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  switch (static_cast<NOTIFY_TYPE_T>(type)) {
    case NOTIFY_LOAD_COMPLETED:
      NotifyLoadComplete();
      break;
    case NOTIFY_END_OF_STREAM:
      INFO_LOG("%s, NOTIFY_END_OF_STREAM", __FUNCTION__);
      received_eos_ = true;
      break;
    case NOTIFY_CURRENT_TIME:
      if (state_ != SEEKING)
        UpdateCurrentTime(num_value);
      break;
    case NOTIFY_SEEK_DONE: {
      INFO_LOG("[%p] %s, NOTIFY_SEEK_DONE, playback_rate_[ %f ]", this,
               __FUNCTION__, playback_rate_);
      if (playback_rate_ > 0.0f)
        PlayInternal();
      else
        SetState(PLAYING);
      break;
    }
    case NOTIFY_UNLOAD_COMPLETED:
    case NOTIFY_PLAYING:
    case NOTIFY_PAUSED:
    case NOTIFY_NEED_DATA:
    case NOTIFY_ENOUGH_DATA:
    case NOTIFY_SEEK_DATA:
      break;
    case NOTIFY_ERROR:
      INFO_LOG("%s, ERROR(%lld)", __FUNCTION__, num_value);
      if (num_value == GMP_ERROR_RES_ALLOC) {
        is_finalized_ = true;
        released_media_resource_ = true;
        load_completed_ = false;
        media_player_client_.reset(NULL);
      } else if (num_value == GMP_ERROR_STREAM) {
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
    case NOTIFY_BUFFER_FULL:
    case NOTIFY_BUFFER_NEED:
      break;
    default:
      INFO_LOG("[%p] %s : Default case, Type(%d)", this, __FUNCTION__, type);
      break;
  }
}

void MediaAPIsWrapperGmp::PlayInternal() {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  if (load_completed_) {
    media_player_client_->Play();
    SetState(PLAYING);
  }

  play_internal_ = true;
}

void MediaAPIsWrapperGmp::PauseInternal(bool update_media) {
  DEBUG_LOG("[%p] %s : media_player_client[ %p ]", this, __FUNCTION__,
            media_player_client_.get());

  if (!media_player_client_)
    return;

  if (update_media)
    media_player_client_->Pause();

  SetState(PAUSED);
}

void MediaAPIsWrapperGmp::SetVolumeInternal(double volume) {
  DEBUG_LOG("[%p] %s, volume = %f", this, __FUNCTION__, volume);

  Json::Value root;
  Json::FastWriter writer;

  root["volume"] = (int)(volume * 100);
  std::string param = writer.write(root);

  // Send input volume to audiod
  ls_client_.callASync("luna://com.webos.service.audio/media/setVolume", param);
}

MediaAPIsWrapperGmp::FeedStatus MediaAPIsWrapperGmp::FeedInternal(
    const scoped_refptr<DecoderBuffer>& buffer,
    FeedType type) {
  if (is_finalized_ || released_media_resource_)
    return FeedSucceeded;

  uint64_t pts = buffer->timestamp().InMicroseconds() * 1000;
  const uint8_t* data = buffer->data();
  size_t size = buffer->data_size();

  if (buffer->end_of_stream()) {
    INFO_LOG("[%p] %s : EOS received !!!", this, __FUNCTION__);
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
  MEDIA_STATUS_T media_status =
      media_player_client_->Feed(p_buffer, size, pts, es_type);
  if (media_status != MEDIA_OK) {
    INFO_LOG("[%p] %s : media_status = %d", this, __FUNCTION__, media_status);
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

void MediaAPIsWrapperGmp::PushEOS() {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  if (is_finalized_ || released_media_resource_)
    return;

  if (media_player_client_)
    media_player_client_->PushEndOfStream();
}

void MediaAPIsWrapperGmp::ResetFeedInfo() {
  feeded_audio_pts_ = -1;
  feeded_video_pts_ = -1;
  audio_eos_received_ = !audio_config_.IsValidConfig();
  video_eos_received_ = !video_config_.IsValidConfig();
  received_eos_ = false;

  buffer_queue_->clear();
}

void MediaAPIsWrapperGmp::UpdateVideoInfo(const std::string& info_str) {
  DEBUG_LOG("[%p] %s : video_info[ %s ]", this, __FUNCTION__, info_str.c_str());

  Json::Reader reader;
  Json::Value json_value;

  if (!reader.parse(info_str, json_value))
    return;

  Json::Value video_info = json_value["videoInfo"];
  uint32_t video_width = static_cast<uint32_t>(video_info["width"].asUInt());
  uint32_t video_height = static_cast<uint32_t>(video_info["height"].asUInt());
  INFO_LOG("[this] %s : video_info [ %d, %d]", this, __FUNCTION__, video_width,
           video_height);

  gfx::Size natural_size(video_width, video_height);
  if (natural_size_ != natural_size) {
    natural_size_ = natural_size;

    if (!size_change_cb_.is_null())
      size_change_cb_.Run();
  }
}

void MediaAPIsWrapperGmp::ReInitialize(base::TimeDelta start_time) {
  DEBUG_LOG("[%p] %s", this, __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (is_destructed_)
    return;

  if (is_finalized_)
    is_finalized_ = false;

  uint64_t pts = start_time.InMicroseconds() * 1000;
  resume_time_ = start_time;

  if (media_player_client_)
    media_player_client_.reset(NULL);

  media_player_client_.reset(new gmp::player::MediaPlayerClient(app_id_));
  media_player_client_->RegisterCallback(&MediaAPIsWrapperGmp::Callback, this);

  MEDIA_LOAD_DATA_T load_data;
  if (!MakeLoadData(pts, &load_data)) {
    INFO_LOG("[%p] %s ->Making load data info Failed !!!", this, __FUNCTION__);
    return;
  }

  DEBUG_LOG("[%p] %s-> call NotifyForeground", this, __FUNCTION__);
  media_player_client_->NotifyForeground();

  if (!media_player_client_->Load(&load_data)) {
    INFO_LOG("[%p] %s : media_player_client_->Load Failed", this, __FUNCTION__);
    base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
    return;
  }

  released_media_resource_ = false;
}

void MediaAPIsWrapperGmp::NotifyLoadComplete() {
  INFO_LOG("[%p] %s : state_ [ %d ]", this, __FUNCTION__, state_);

  load_completed_ = true;

  // Run LoadCompletedCb to TvRenderer.
  // It's necessary to callback on loading complete after feeding 10 secs.
  if (!load_completed_cb_.is_null())
    base::ResetAndReturn(&load_completed_cb_).Run();

  DEBUG_LOG("[%p] %s : play_internal_[%d]", this, __FUNCTION__, play_internal_);
  if (play_internal_)
    PlayInternal();

  SetVolumeInternal(playback_volume_);
}

bool MediaAPIsWrapperGmp::MakeLoadData(int64_t start_time,
                                       MEDIA_LOAD_DATA_T* load_data) {
  load_data->maxWidth = 1920;
  load_data->maxHeight = 1080;
  load_data->maxFrameRate = 30;

  if (video_config_.IsValidConfig()) {
    INFO_LOG("[%p] %s : [video_codec = %d]", this, __FUNCTION__,
             video_config_.codec());
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
        INFO_LOG("[%] %s : Not Supported Video Codec(%d)", this, __FUNCTION__,
                 static_cast<int>(video_config_.codec()));
        return false;
    }

    load_data->videoCodec = video_codec[video_config_.codec()];
    load_data->width = video_config_.natural_size().width();
    load_data->height = video_config_.natural_size().height();
    load_data->frameRate = 30;
    load_data->extraData = (void*)video_config_.extra_data().data();
    load_data->extraSize = video_config_.extra_data().size();
  }

  if (audio_config_.IsValidConfig()) {
    INFO_LOG("[%p] %s : [audio_codec = %d]", this, __FUNCTION__,
             audio_config_.codec());
    load_data->audioCodec = audio_codec[audio_config_.codec()];
    load_data->channels = audio_config_.channel_layout();
    load_data->sampleRate = audio_config_.samples_per_second();
    load_data->bitRate = audio_config_.bits_per_channel();
    load_data->bitsPerSample = 8 * audio_config_.bytes_per_frame();
  }
  load_data->ptsToDecode = start_time;

  INFO_LOG("[%p] %s : Outgoing Codec Info[Audio = %d, Video = %d]", this,
           __FUNCTION__, load_data->audioCodec, load_data->videoCodec);

  return true;
}

}  // namespace media
