// Copyright (c) 2017 LG Electronics, Inc.

#include "media/webos/base/media_apis_wrapper.h"

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#if defined(ENABLE_LG_SVP)
#if defined(WIDEVINE_CDM_AVAILABLE)
#include "widevine_cdm_version.h" // In SHARED_INTERMEDIATE_DIR.
#endif

#if defined(ENABLE_PLAYREADY_CDM)
#include "playready_cdm_version.h" // In SHARED_INTERMEDIATE_DIR.
#endif
#endif

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("MediaAPIsWrapper", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("MediaAPIsWrapper:%04d " format, __LINE__, ##__VA_ARGS__)

namespace media {

#define BIND_TO_RENDER_LOOP(function) \
  (DCHECK(task_runner_->BelongsToCurrentThread()), \
  media::BindToCurrentLoop(base::Bind(function, this)))

Json::Value MediaAPIsWrapper::supported_codec_mse_ = Json::Value();

void MediaAPIsWrapper::CheckCodecInfo(const std::string& codec_info) {
  Json::Value codec_capability;
  Json::Reader reader;

  if (!supported_codec_mse_.isNull())
    return;

  if (!reader.parse(codec_info, codec_capability))
    return;

  Json::Value videoCodecs = codec_capability["videoCodecs"];
  Json::Value audioCodecs = codec_capability["audioCodecs"];

  for (Json::Value::iterator iter = videoCodecs.begin();
       iter != videoCodecs.end();
       iter++) {
    if ((*iter).isObject()) {;
      if ((*iter)["name"].asString() == "H.264")
        supported_codec_mse_["H264"] = *iter;
      else if ((*iter)["name"].asString() == "VP9") {
        supported_codec_mse_["VP9"] = *iter;
      }
    }
  }

  for (Json::Value::iterator iter = audioCodecs.begin();
       iter != audioCodecs.end();
       iter++) {
   if ((*iter).isObject()) {
     if ((*iter)["name"].asString() == "AAC")
       supported_codec_mse_["AAC"] = *iter;
    }
  }
}

#if defined(USE_DIRECTMEDIA2)
MediaAPIsWrapper::MediaAPIsWrapper(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const PipelineStatusCB& error_cb)
    : task_runner_(task_runner),
      app_id_(app_id),
      error_cb_(error_cb),
      received_ndl_eos_(false),
      volume_per_tab_(1.0),
      ls_client_(LunaServiceClient::PrivateBus),
      weak_factory_(this) {

  DEBUG_LOG("%s", __FUNCTION__);
  reset();

  create();
}

MediaAPIsWrapper::~MediaAPIsWrapper() {
  DEBUG_LOG("%s", __FUNCTION__);
  Finalize();
}

//
// DispatchCallback and Callback are not actually used. We keep them to
// preserve the orignal public interface of the class
//
void MediaAPIsWrapper::Callback(
                    const gint type, const gint64 numValue, const gchar *strValue) {
  DEBUG_LOG("%s", __FUNCTION__);
}

void MediaAPIsWrapper::DispatchCallback(const gint type,
                    const gint64 numValue, const std::string& strValue) {
  DEBUG_LOG("%s", __FUNCTION__);
}

void MediaAPIsWrapper::Initialize(
    const AudioDecoderConfig& audio_config,
    const VideoDecoderConfig& video_config,
    const PipelineStatusCB& init_cb) {

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("%s +", __FUNCTION__);
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb.is_null());

  if (!esplayer_)
    return;

  init_cb_ = init_cb;
  audio_config_ = audio_config;
  video_config_ = video_config;

  if (state_ != CREATED && state_ != CREATED_SUSPENDED) {
    DEBUG_LOG("bad state %d for initialization", state_);
    base::ResetAndReturn(&init_cb_).Run(PIPELINE_ERROR_DECODE);
  }

  setMetaDataVideoCodec();
  setMetaDataAudioCodec();

  if (!load()) {
    INFO_LOG("esplayer_->Load Failed");
    base::ResetAndReturn(&init_cb_).Run(PIPELINE_ERROR_DECODE);
    return;
  }
#if defined(USE_BROADCOM)
  do_play_ = true;
#endif
  playIfNeeded();

  base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
  DEBUG_LOG("%s -", __FUNCTION__);
}

void MediaAPIsWrapper::SetDisplayWindow(
    const gfx::Rect& rect, const gfx::Rect& in_rect, bool fullscreen) {
  DEBUG_LOG("%s", __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  setDisplayWindow(rect, in_rect, fullscreen);
}

void MediaAPIsWrapper::SetNaturalSize(const gfx::Size& size) {
  DEBUG_LOG("%s", __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  natural_size_ = size;
}

bool MediaAPIsWrapper::Loaded() {
  DEBUG_LOG("%s", __FUNCTION__);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return (state_ >= LOADED && state_ < FINALIZING);
}

bool MediaAPIsWrapper::Feed(
    const scoped_refptr<DecoderBuffer>& buffer, FeedType type) {

#if defined(USE_BROADCOM)
  if(type == Video && video_first_frame_feeded_ == false){
    video_first_frame_feeded_ = true;
    INFO_LOG("First Video Feeded. Stop feeding before port change done");
    can_feed_video_ = false;
  }
  else if(type == Audio && audio_first_frame_feeded_ == false)
  {
    audio_first_frame_feeded_ = true;
    INFO_LOG("First Audio Feeded. Stop feeding before port change done");
    can_feed_audio_ = false;
  }
#endif
  uint32_t videoBuffersUsed = 0;

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (type == Audio)
    current_pts_ = buffer->timestamp().InMicroseconds();

  NDL_ESP_STREAM_T buffer_type;
  buffer_type = (type == Video) ? NDL_ESP_VIDEO_ES : NDL_ESP_AUDIO_ES;

  auto frame = std::make_shared<Frame>(buffer_type, buffer);

  // The A/V frames are fed in to the decoder IN buffers. The feeding will return -1 in the current
  // buffer if already full. This is possible during SEEK if the audio and
  // the video streams at the access point closest to the selected seek point have TS's different
  // by a few seconds. In such case the decoder will hold the audio queue filling the video queue.
  // If the video buffer length is not long enough to compensate the difference the frames will be
  // skipped.
  if (frame && playerReadyForPlayOrSeek()) {
    if (NDL_EsplayerFeedData(esplayer_, frame) < 0)
      INFO_LOG("feed failed, skipping %s frame",
          buffer_type == NDL_ESP_VIDEO_ES ? "video" : "audio");

    if (buffer_type == NDL_ESP_VIDEO_ES) {
      if (NDL_EsplayerGetBufferLevel(esplayer_, NDL_ESP_VIDEO_ES, &videoBuffersUsed) != 0)
        INFO_LOG("error in failed to get buffer level");
    }
  } else {
    return false;
  }

  return true;
}

uint64_t MediaAPIsWrapper::GetCurrentTime() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("%s", __FUNCTION__);

  return current_pts_;
}

bool MediaAPIsWrapper::Seek(base::TimeDelta time) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("Seeking to %u ms", time.InMilliseconds());

  if (!playerReadyForPlayOrSeek()) {
    INFO_LOG("Seek: Player not ready to seek");
    return false;
  }

  if (!flush()) {
    INFO_LOG("Seek: flush has failed");
    return false;
  }

  return true;
}

void MediaAPIsWrapper::SetPlaybackRate(float playback_rate) {

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  float current_playback_rate = playback_rate_;
  playback_rate_ = playback_rate;

  DEBUG_LOG("SetPlaybackRate(%f) [%d]", playback_rate, state_);
  if (!esplayer_)
    return;

  if (playback_rate == 0 && metadata_.video_codec == NDL_ESP_VIDEO_NONE &&
      metadata_.audio_codec != NDL_ESP_AUDIO_NONE) {
    DEBUG_LOG("Play for audio only case");
    current_playback_rate = 0;
    playback_rate = 1.0;
  }

  if (current_playback_rate == 0 && playback_rate != 0) {
    if (state_ == PLAYING)
      ; // nothing to do
    else if (state_ == LOADED || state_ == PAUSED)
      play();
    else
      do_play_ = true;
    return;
  }

  if (current_playback_rate != 0 && playback_rate == 0 && hasResource())
    pause();
}

std::string MediaAPIsWrapper::GetMediaID(void) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("%s", __FUNCTION__);

  return getMediaID();
}

void MediaAPIsWrapper::Suspend() {

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("%s - %s, state = %d", __FUNCTION__, getMediaID().c_str(), state_);
  if (NDL_EsplayerSetAppForegroundState(esplayer_,
                                        NDL_ESP_APP_STATE_BACKGROUND) != 0)
    INFO_LOG("error putting ESPlayer in background");

  disableAVFeed();

  if (CREATED == state_) {
    state_ = CREATED_SUSPENDED;
    return;
  }

  if (!hasResource() || state_ == SUSPENDED)
    return;

  pause();

  state_ = SUSPENDED;
}

void MediaAPIsWrapper::Resume(base::TimeDelta paused_time,
                              RestorePlaybackMode restore_playback_mode) {

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("%s(%lld) - %s, state = %d", __FUNCTION__,
      paused_time.InMilliseconds(), getMediaID().c_str(), state_);

  if (state_ == FINALIZED) {
    create();
    if (!load()) {
      INFO_LOG("Resume failed to create/load player");
      return;
    }
    state_ = SUSPENDED;
  }

  if (!hasResource() || state_ != SUSPENDED) {
    INFO_LOG("Resume failed : invalid state %d", state_);
    return;
  }

  Seek(paused_time);
  SetPlaybackVolume(volume_per_tab_);

  play(RESTORE_DISPLAY_WINDOW, restore_playback_mode);
}

bool MediaAPIsWrapper::IsReleasedMediaResource() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("%s", __FUNCTION__);

  return (state_ == FINALIZED);
}

bool MediaAPIsWrapper::allowedFeedVideo() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  //DEBUG_LOG("%s", __FUNCTION__);

  return can_feed_video_;
}

bool MediaAPIsWrapper::allowedFeedAudio() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  //DEBUG_LOG("%s", __FUNCTION__);

  return can_feed_audio_;
}

void MediaAPIsWrapper::Finalize() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  DEBUG_LOG("%s", __FUNCTION__);

  if (esplayer_) {
    NDL_EsplayerDestroy(esplayer_);
    esplayer_ = nullptr;
  }

  reset();

  state_ = FINALIZED;
}

void MediaAPIsWrapper::ESPlayerCallback(NDL_ESP_EVENT event, void* playerdata)
{
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  switch(event) {
#if defined(USE_BROADCOM)
    case NDL_ESP_AUDIO_PORT_CHANGED:
      audio_port_changed_ = true;
      can_feed_audio_ = true;
      DEBUG_LOG("AUDIO_PORT_CHANGED can feed next audio");
      break;
    case NDL_ESP_VIDEO_PORT_CHANGED:
      video_port_changed_ = true;
      can_feed_video_ = true;
      DEBUG_LOG("VIDEO_PORT_CHANGED can feed nex video");
      break;
#endif
    case NDL_ESP_FIRST_FRAME_PRESENTED:
      DEBUG_LOG("ESPlayer Callback -- first frame presented");
      break;
    case NDL_ESP_STREAM_DRAINED_VIDEO:
      DEBUG_LOG("ESPlayer Callback -- stream drained");
    case NDL_ESP_LOW_THRESHOLD_CROSSED_VIDEO:
      DEBUG_LOG("ESPlayer Callback -- video low threshold crossed");
      if (NDL_EsplayerGetStatus(esplayer_) == NDL_ESP_STATUS_PLAYING)
        can_feed_video_ = true;
      break;
    case NDL_ESP_HIGH_THRESHOLD_CROSSED_VIDEO:
      DEBUG_LOG("ESPlayer Callback -- video high threshold crossed");
      if (NDL_EsplayerGetStatus(esplayer_) == NDL_ESP_STATUS_PLAYING)
        can_feed_video_ = false;
      break;
    case NDL_ESP_STREAM_DRAINED_AUDIO:
      DEBUG_LOG("ESPlayer Callback -- stream drained");
    case NDL_ESP_LOW_THRESHOLD_CROSSED_AUDIO:
      DEBUG_LOG("ESPlayer Callback -- audio low threshold crossed");
      if (NDL_EsplayerGetStatus(esplayer_) == NDL_ESP_STATUS_PLAYING)
        can_feed_audio_ = true;
      break;
    case NDL_ESP_HIGH_THRESHOLD_CROSSED_AUDIO:
      DEBUG_LOG("ESPlayer Callback -- audio high threshold crossed");
      if (NDL_EsplayerGetStatus(esplayer_) == NDL_ESP_STATUS_PLAYING)
        can_feed_audio_ = false;
      break;
    case NDL_ESP_END_OF_STREAM:
      DEBUG_LOG("ESPlayer Callback -- end of stream");
      received_ndl_eos_ = true;
      SetPlaybackRate(0.0f);
      break;
    case NDL_ESP_RESOURCE_RELEASED_BY_POLICY:
      DEBUG_LOG("ESPlayer Callback -- resource released by policy");
      //
      // destroy the player if RESOURCE_RELEASED_BY_POLICY is received
      //
      if (NDL_EsplayerSetAppForegroundState(esplayer_, NDL_ESP_APP_STATE_BACKGROUND) !=0)
        INFO_LOG("error putting ESPlayer in background");

      // set state to finalizing
      state_ = FINALIZING;

      task_runner_->PostTask(FROM_HERE,
        base::Bind(&MediaAPIsWrapper::Finalize, weak_factory_.GetWeakPtr()));
      break;
    case NDL_ESP_VIDEO_INFO:
      DEBUG_LOG("ESPlayer Callback -- video info");
      break;
    default:
      DEBUG_LOG("ESPlayer Callback -- unhandled event %d", event);
      break;
  }
}

void MediaAPIsWrapper::SetPlaybackVolume(double volume) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (state_ < CREATED || state_ >= FINALIZING)
    return;

  int volume_level = (int)(volume * 100);
  int duration = 0;
  NDL_ESP_EASE_TYPE type = EASE_TYPE_LINEAR;

  if (NDL_EsplayerSetVolume(esplayer_, volume_level, duration, type) != 0)
    INFO_LOG("error in set volume - NDL_EsplayerSetVolume");
  else
    volume_per_tab_ = volume;
}

#if defined(ENABLE_LG_SVP)
void MediaAPIsWrapper::setKeySystem(const std::string key_system) {

#if defined(WIDEVINE_CDM_AVAILABLE)
  if (key_system == kWidevineKeySystem)
    metadata_.key_system = NDL_DRM_WIDEVINE_MODULAR;
#endif

#if defined(ENABLE_PLAYREADY_CDM)
  if (key_system == kPlayReadyMicrosoftKeySystem
      || key_system == kPlayReadyYouTubeKeySystem)
    metadata_.key_system = NDL_DRM_PLAYREADY;
#endif

  switch(metadata_.key_system) {
    case NDL_DRM_WIDEVINE_MODULAR:
      DEBUG_LOG("key system is widevine");
      break;
    case NDL_DRM_PLAYREADY:
      DEBUG_LOG("key system is playready");
      break;
    default:
      DEBUG_LOG("Key system is not set");
      break;
  }

}
#endif

static void ESPlayerCallback_(NDL_ESP_EVENT event, void* playerdata, void* userdata)
{
  MediaAPIsWrapper* ctxt = (MediaAPIsWrapper*) userdata;
  ctxt->ESPlayerCallback(event, playerdata);
}

void MediaAPIsWrapper::reset() {
  state_ = INVALID;
  current_pts_ = 0;
  do_play_ = false;
  playback_rate_ = 0;

  disableAVFeed();
  esplayer_ = nullptr;
#if defined(USE_BROADCOM)
  audio_port_changed_ = false;
  video_port_changed_ = false;
#endif
}

void MediaAPIsWrapper::create() {
  esplayer_ = NDL_EsplayerCreate(app_id_.c_str(), ESPlayerCallback_, this);
  if (!esplayer_) {
     INFO_LOG("failed to create player");
     return;
  }

#if defined(WEBOS_BROWSER)
  is_webos_browser_ = false;
#endif
  // TODO: kWebosBrowser is not currently defined in Chromium44. Uncomment once it is defined or remove
  // const base::CommandLine& command_line = *base::CommandLine::ForCurrentProcess();
  // is_webos_browser_ = command_line.HasSwitch(switches::kWebosBrowser);

  state_ = CREATED;
}

bool MediaAPIsWrapper::hasResource() {
    switch(state_) {
        case LOADED:
        case PLAYING:
        case PAUSED:
        case SUSPENDED:
            return true;
            break;
        default:
            break;
    }
    return false;
}

bool MediaAPIsWrapper::load() {
  DEBUG_LOG("Loading Player with codecs %d and %d",
                  metadata_.audio_codec, metadata_.video_codec);

  if (NDL_EsplayerLoadEx(esplayer_, &metadata_, NDL_ESP_PTS_MICROSECS) != 0) {
    INFO_LOG("error in NDL_EsplayerLoad");
    return false;
  }

  state_ = LOADED;

  return true;
}

void MediaAPIsWrapper::playIfNeeded() {
  if (do_play_) {
    play();
    do_play_ = false;
  }
}

bool MediaAPIsWrapper::flush() {

  if (NDL_EsplayerFlush(esplayer_) != 0) {
    INFO_LOG("error in Flush:NDL_EsplayerFlush");
    return false;
  }

  return true;
}

bool MediaAPIsWrapper::play(RestoreDisplayWindowMode restore_display_window,
                            RestorePlaybackMode restore_playback_mode) {

  // Some YouTube tests play() without calling Initialize()
  // The following will take care of it

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (state_ == CREATED) {
    if (!load()) {
      INFO_LOG("esplayer_->Load Failed");
      base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
      return false;
    }
  }

  if (state_ != LOADED && state_ != PAUSED && state_ != SUSPENDED) {
    INFO_LOG("play was called in invalid state %d", state_);
    base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
    return false;
  }

  // put the Esplayer in foreground state
  if (NDL_EsplayerSetAppForegroundState(esplayer_, NDL_ESP_APP_STATE_FOREGROUND) != 0)
    INFO_LOG("error in NDL_EsplayerSetAppForegroundState");

  // restore the display window if needed
  if (restore_display_window == RESTORE_DISPLAY_WINDOW) {
    setDisplayWindow(window_rect_, window_in_rect_, int_fullscreen_ == 1);
  }

  // play media
  received_ndl_eos_ = false;
  if (restore_playback_mode == RESTORE_PLAYING) {
    if (NDL_EsplayerPlay(esplayer_) != 0) {
      INFO_LOG("error in NDL_EsplayerPlay");
      base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
      return false;
    }

    if (playback_rate_ == 0)
      playback_rate_ = 1.0;

    INFO_LOG("MediaAPIsWrapper: Playing");

    if (state_ == SUSPENDED)
      can_feed_audio_ = true;
    else
      enableAVFeed();

    state_ = PLAYING;

  } else {
    INFO_LOG("MediaAPIsWrapper: Resumed in Paused State");

    state_ = PAUSED;
  }

  return true;
}

bool MediaAPIsWrapper::pause() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  disableAVFeed();

  if (!esplayer_ || state_ != PLAYING)
    return false;

  if (NDL_EsplayerGetStatus(esplayer_) != NDL_ESP_STATUS_PLAYING)
    return false;

  if (NDL_EsplayerPause(esplayer_) == 0) {
    state_ = PAUSED;

    if (playback_rate_ != 0)
      playback_rate_ = 0;

    return true;
  } else {
    return false;
  }
}

void MediaAPIsWrapper::enableAVFeed() {
  can_feed_video_ = true;
  can_feed_audio_ = true;
}

void MediaAPIsWrapper::disableAVFeed() {
  can_feed_video_ = false;
  can_feed_audio_ = false;
}

bool MediaAPIsWrapper::playerReadyForPlayOrSeek() {
  if (!esplayer_) {
    DEBUG_LOG("Player object destroyed");
    return false;
  }
  if (state_ < LOADED) {
    DEBUG_LOG("Player load is not completed");
    return false;
  }
  if (!hasResource()) {
    DEBUG_LOG("Player resources are released");
    return false;
  }

  NDL_ESP_STATUS status = NDL_EsplayerGetStatus(esplayer_);
  if ((status == NDL_ESP_STATUS_IDLE) || (status == NDL_ESP_STATUS_UNLOADED)
     || (status == NDL_ESP_STATUS_EOS) || (status == NDL_ESP_STATUS_FLUSHING)) {
    DEBUG_LOG("Player status is %d", status);
    return false;
  }

  return true;
}

std::string MediaAPIsWrapper::getMediaID(void) {
  char mediaId[32];
  if (esplayer_) {
    NDL_EsplayerGetConnectionId(esplayer_, mediaId, 32);
    return media_id_ = mediaId;
  }
  return media_id_;
}

void MediaAPIsWrapper::setMetaDataVideoCodec() {
  // check if the requested video codec is different from the one that is loaded

#if defined(USE_BROADCOM)
  NDL_ESP_VIDEO_CODEC codec[] = {
    NDL_ESP_VIDEO_NONE,
    NDL_ESP_VIDEO_CODEC_H264,
    NDL_ESP_VIDEO_NONE,
    NDL_ESP_VIDEO_CODEC_MPEG,
    NDL_ESP_VIDEO_CODEC_MPEG,
    NDL_ESP_VIDEO_NONE,
    NDL_ESP_VIDEO_CODEC_VP8,
    NDL_ESP_VIDEO_CODEC_VP9,
    NDL_ESP_VIDEO_CODEC_H265,
  };

  if (video_config_.IsValidConfig()) {
    metadata_.video_codec = codec[video_config_.codec()];
    metadata_.width = video_config_.natural_size().width();
    metadata_.height = video_config_.natural_size().height();
    metadata_.framerate = 0; // not defined in VideoDecoderConfig
    metadata_.extradata = (void *)video_config_.extra_data().data();
    metadata_.extrasize = video_config_.extra_data().size();
  }

  DEBUG_LOG("video_codec(%d), width(%d), height(%d), framerate(%d), extradata(%p), extrasize(%d)",
            metadata_.video_codec, metadata_.width, metadata_.height, metadata_.framerate,
            metadata_.extradata, metadata_.extrasize);
#else
  if (video_config_.IsValidConfig()) {
    if (video_config_.profile() < media::VP8PROFILE_MIN) {
      metadata_.video_codec = NDL_ESP_VIDEO_CODEC_H264;
    } else if (video_config_.profile() >= media::VP9PROFILE_MIN) {
      if (metadata_.video_codec != NDL_ESP_VIDEO_CODEC_VP9)
      metadata_.video_codec = NDL_ESP_VIDEO_CODEC_VP9;
    } else {
      if (metadata_.video_codec !=NDL_ESP_VIDEO_CODEC_VP8)
        metadata_.video_codec =NDL_ESP_VIDEO_CODEC_VP8;
      }
  } else {
    metadata_.video_codec = NDL_ESP_VIDEO_NONE;
  }
#endif
}

void MediaAPIsWrapper::setMetaDataAudioCodec() {
  // The original starfish player allowed two audio codecs: AAC and Vorbis.
  // The directmedia ES player does not support Vorbis.

#if defined(USE_BROADCOM)
  NDL_ESP_AUDIO_CODEC codec[] = {
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_CODEC_AAC,
    NDL_ESP_AUDIO_CODEC_MP3,
    NDL_ESP_AUDIO_CODEC_PCM_44100_2CH,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_CODEC_PCM_44100_2CH,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_CODEC_EAC3,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_NONE,
    NDL_ESP_AUDIO_CODEC_AC3,
  };

  if (audio_config_.IsValidConfig()) {
    metadata_.audio_codec = codec[audio_config_.codec()];
    metadata_.channels = audio_config_.channel_layout();
    metadata_.samplerate = audio_config_.samples_per_second();
    metadata_.bitrate = audio_config_.bits_per_channel();
    metadata_.bitspersample = 8 * audio_config_.bytes_per_frame();
  }

  DEBUG_LOG("audio_codec(%d), channels(%d), samplerate(%d), bitrate(%d), bitspersample(%d)",
            metadata_.audio_codec, metadata_.channels, metadata_.samplerate, metadata_.bitrate,
            metadata_.bitspersample);
#else
  if (audio_config_.IsValidConfig()) {
    if (audio_config_.codec() == media::kCodecAAC) {
      metadata_.audio_codec = NDL_ESP_AUDIO_CODEC_AAC;
    } else {
      if (metadata_.audio_codec != NDL_ESP_AUDIO_NONE)
      metadata_.audio_codec = NDL_ESP_AUDIO_NONE;
    }
  } else {
    metadata_.audio_codec = NDL_ESP_AUDIO_NONE;
  }
#endif
}

void MediaAPIsWrapper::setDisplayWindow(
    const gfx::Rect& rect, const gfx::Rect& in_rect, bool fullscreen) {

  window_rect_ = rect;
  window_in_rect_ = in_rect;
  int_fullscreen_ = !fullscreen ? 0 :1;
  int result = -1;

  if (!window_in_rect_.IsEmpty() && !window_rect_.IsEmpty())
    result = NDL_EsplayerSetVideoCustomDisplayWindow(esplayer_,
                                           window_in_rect_.x(), window_in_rect_.y(),
                                           window_in_rect_.width(), window_in_rect_.height(),
                                           window_rect_.x(), window_rect_.y(),
                                           window_rect_.width(), window_rect_.height(),
                                           int_fullscreen_);
  else if (!window_rect_.IsEmpty() || fullscreen)
    result = NDL_EsplayerSetVideoDisplayWindow(esplayer_,
                                           window_rect_.x(), window_rect_.y(),
                                           window_rect_.width(), window_rect_.height(),
                                           int_fullscreen_);

  if (result != 0)
    INFO_LOG("error in setDisplayWindow:result [%d]", result);
}

#else // #if defined(USE_DIRECTMEDIA2)
//
// Dummy implementation of the public interface in the case then the directmedia is not used.
// This is needed to get the component compiled for the x86 emulator. The application most likely
// will crash if attempted to run on the emulator
//
MediaAPIsWrapper::MediaAPIsWrapper(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const PipelineStatusCB& error_cb)
    : task_runner_(task_runner),
      app_id_(app_id),
      error_cb_(error_cb),
      ls_client_(LunaServiceClient::PrivateBus),
      weak_factory_(this) {}
MediaAPIsWrapper::~MediaAPIsWrapper() {}

void MediaAPIsWrapper::Callback(
      const gint type, const gint64 numValue, const gchar *strValue) {}
void MediaAPIsWrapper::DispatchCallback(
      const gint type, const gint64 numValue, const std::string& strValue) {}

void MediaAPIsWrapper::Initialize(const AudioDecoderConfig& audio_config,
                                  const VideoDecoderConfig& video_config,
                                  const PipelineStatusCB& init_cb) {}

void MediaAPIsWrapper::SetDisplayWindow(const gfx::Rect& rect,
                                        const gfx::Rect& in_rect,
                                        bool fullscreen) {}
void MediaAPIsWrapper::SetNaturalSize(const gfx::Size& size) {}

bool MediaAPIsWrapper::Loaded() { return false; }
bool MediaAPIsWrapper::Feed(const scoped_refptr<DecoderBuffer>& buffer,
                            FeedType type) { return false; }
uint64_t MediaAPIsWrapper::GetCurrentTime() { return 0; }
bool MediaAPIsWrapper::Seek(base::TimeDelta time) { return false; }
void MediaAPIsWrapper::SetPlaybackRate(float playback_rate) {}
std::string MediaAPIsWrapper::GetMediaID(void) { std::string ret(""); return ret; }
void MediaAPIsWrapper::Suspend() {}
void MediaAPIsWrapper::Resume(base::TimeDelta paused_time, RestorePlaybackMode restore_playback_mode) {}
bool MediaAPIsWrapper::IsReleasedMediaResource() { return false; }
bool MediaAPIsWrapper::allowedFeedVideo() { return false; }
bool MediaAPIsWrapper::allowedFeedAudio() { return false; }
void MediaAPIsWrapper::Finalize() {}
void MediaAPIsWrapper::SetPlaybackVolume(double volume) {}

#endif // #if defined(USE_DIRECTMEDIA2)

}  // namespace media
