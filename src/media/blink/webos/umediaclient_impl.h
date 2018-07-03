// Copyright (c) 2014-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MEDIA_BLINK_WEBOS_UMEDIACLIENT_IMPL_H_
#define MEDIA_BLINK_WEBOS_UMEDIACLIENT_IMPL_H_

#include <string>
#include <cstdint>
#include <memory>

#include "base/time/time.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "media/base/pipeline.h"
#include "media/blink/webmediaplayer_util.h"
#include "media/webos/base/lunaservice_client.h"
#include "third_party/WebKit/public/platform/WebRect.h"

#include <uMediaClient.h>

namespace media {

class UMediaClientImpl
    : public uMediaServer::uMediaClient,
      public base::SupportsWeakPtr<UMediaClientImpl> {
 public:
  enum PlaybackNotification {
    NotifyPreloadCompleted,
    NotifyLoadCompleted,
    NotifyUnloadCompleted,
    NotifyEndOfStream,
    NotifyPaused,
    NotifyPlaying,
    NotifySeekDone,
  };
  enum BufferingState {
    kHaveMetadata,
    kLoadCompleted,
    kPreloadCompleted,
    kPrerollCompleted,
    kWebOSBufferingStart,
    kWebOSBufferingEnd,
  };
  enum Preload {
    PreloadNone,
    PreloadMetaData,
    PreloadAuto,
  };
  typedef base::Callback<void(const std::string& detail)> UpdateUMSInfoCB;
  typedef base::Callback<void(BufferingState)> BufferingStateCB;
  typedef base::Callback<void(bool playing)> PlaybackStateCB;
  typedef base::Callback<void(const blink::WebRect&)> ActiveRegionCB;

  UMediaClientImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  ~UMediaClientImpl();

  void load(bool video,
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
            const base::Closure& focus_cb);
  void Seek(base::TimeDelta time, const media::PipelineStatusCB& seek_cb);
  void SetPlaybackRate(float playback_rate);
  float GetPlaybackRate() const;
  void SetPlaybackVolume(double volume, bool forced = false);
  void SetPreload(Preload preload);
  std::string mediaId() const;

  // implement ums virtual functions
  bool onPlaying() override;
  bool onPaused() override;
  bool onSeekDone() override;
  bool onEndOfStream() override;
  bool onLoadCompleted() override;
  bool onPreloadCompleted() override;
  bool onCurrentTime(int64_t currentTime) override;
#if UMS_INTERNAL_API_VERSION == 2
  bool onSourceInfo(const struct ums::source_info_t& sourceInfo) override;
  bool onAudioInfo(const struct ums::audio_info_t&) override;
  bool onVideoInfo(const struct ums::video_info_t&) override;
#else
  bool onSourceInfo(
      const struct uMediaServer::source_info_t& sourceInfo) override;
  bool onAudioInfo(const struct uMediaServer::audio_info_t&) override;
  bool onVideoInfo(const struct uMediaServer::video_info_t&) override;
#endif
  bool onBufferRange(const struct uMediaServer::buffer_range_t&) override;
  bool onError(int64_t errorCode, const std::string& errorText) override;
  bool onExternalSubtitleTrackInfo(
      const struct uMediaServer::external_subtitle_track_info_t&) override;
  bool onUserDefinedChanged(const char* message) override;
  bool onBufferingStart() override;
  bool onBufferingEnd() override;
  bool onFocusChanged(bool) override;
  bool onActiveRegion(const uMediaServer::rect_t&) override;

  // dispatch event
  void dispatchPlaying();
  void dispatchPaused();
  void dispatchSeekDone();
  void dispatchEndOfStream();
  void dispatchLoadCompleted();
  void dispatchPreloadCompleted();
  void dispatchCurrentTime(int64_t currentTime);
#if UMS_INTERNAL_API_VERSION == 2
  void dispatchSourceInfo(const struct ums::source_info_t&);
  void dispatchAudioInfo(const struct ums::audio_info_t&);
  void dispatchVideoInfo(const struct ums::video_info_t&);
#else
  void dispatchSourceInfo(const struct uMediaServer::source_info_t&);
  void dispatchAudioInfo(const struct uMediaServer::audio_info_t&);
  void dispatchVideoInfo(const struct uMediaServer::video_info_t&);
#endif
  void dispatchBufferRange(const struct uMediaServer::buffer_range_t&);
  void dispatchError(int64_t errorCode, const std::string& errorText);
  void dispatchExternalSubtitleTrackInfo(
      const struct uMediaServer::external_subtitle_track_info_t&);
  void dispatchUserDefinedChanged(const std::string& message);
  void dispatchBufferingStart();
  void dispatchBufferingEnd();
  void dispatchFocusChanged();
  void dispatchActiveRegion(const uMediaServer::rect_t&);

  double duration() const { return duration_; }
  void SetDuration(double duration) { duration_ = duration; }
  double currentTime() { return current_time_; }
  void SetCurrentTime(double time) { current_time_ = time; }

  float bufferEnd() const { return static_cast<float>(buffer_end_ / 1000.0); }
  bool hasVideo() { return has_video_; }
  bool hasAudio() { return has_audio_; }
  int numAudioTracks() { return num_audio_tracks_; }
  gfx::Size naturalVideoSize() { return natural_video_size_; }
  void SetNaturalVideoSize(const gfx::Size& size) {
    natural_video_size_ = size; }

  void setDisplayWindow(const blink::WebRect&, const blink::WebRect&,
      bool fullScreen, bool forced = false);

  bool DidLoadingProgress();
  bool usesIntrinsicSize();
  void suspend();
  void resume();
  bool isSupportedBackwardTrickPlay();
  bool isSupportedPreload();
  void SetActiveRegionCB(const ActiveRegionCB& active_region_cb) {
    active_region_cb_ = active_region_cb;
  }

 private:
  struct Media3DInfo {
    std::string pattern;
    std::string type;
  };
  std::string mediaInfoToJson(const PlaybackNotification);

#if UMS_INTERNAL_API_VERSION == 2
  std::string mediaInfoToJson(const struct ums::source_info_t&);
  std::string mediaInfoToJson(const struct ums::video_info_t&);
  std::string mediaInfoToJson(const struct ums::audio_info_t&);
#else
  std::string mediaInfoToJson(const struct uMediaServer::source_info_t&);
  std::string mediaInfoToJson(const struct uMediaServer::video_info_t&);
  std::string mediaInfoToJson(const struct uMediaServer::audio_info_t&);
#endif
  std::string mediaInfoToJson(
      const struct uMediaServer::external_subtitle_track_info_t&);
  std::string mediaInfoToJson(
      int64_t errorCode, const std::string& errorText);
  std::string mediaInfoToJson(const std::string& message);

  std::string updateMediaOption(const std::string& mediaOption, int64_t start);
  bool isRequiredUMSInfo();
  bool isInsufficientSourceInfo();
  bool isAdaptiveStreaming();
  bool isNotSupportedSourceInfo();
  bool is2kVideoAndOver();
  bool isSupportedAudioOutputOnTrickPlaying();

  gfx::Size getResoultionFromPAR(const std::string& par);
  struct Media3DInfo getMedia3DInfo(const std::string& media_3dinfo);

  bool CheckAudioOutput(float playback_rate);
  void LoadInternal();

  PlaybackStateCB playback_state_cb_;
  base::Closure ended_cb_;
  media::PipelineStatusCB seek_cb_;
  media::PipelineStatusCB error_cb_;
  base::Closure duration_change_cb_;
  base::Closure video_size_change_cb_;
  base::Closure video_display_window_change_cb_;
  UpdateUMSInfoCB update_ums_info_cb_;
  BufferingStateCB buffering_state_cb_;
  base::Closure focus_cb_;
  ActiveRegionCB active_region_cb_;
  double duration_;
  double current_time_;
  int64_t buffer_end_;
  int64_t buffer_end_at_last_didLoadingProgress_;
  bool video_;
  bool loaded_;
  bool preloaded_;
  bool load_started_;
  bool has_video_;
  bool has_audio_;
  int num_audio_tracks_;
  bool is_local_source_;
  bool is_usb_file_;
  bool is_seeking_;
  bool is_suspended_;
  bool use_umsinfo_;
  bool use_pipeline_preload_;
  bool updated_source_info_;
  bool buffering_;
  bool requests_play_;
  bool requests_pause_;
  std::string media_transport_type_;
  gfx::Size natural_video_size_;
  float playback_rate_;
  float playback_rate_on_eos_;
  float playback_rate_on_paused_;
  double volume_;
  const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  LunaServiceClient ls_client_;
  std::string app_id_;
  std::string url_;
  std::string mime_type_;
  std::string referrer_;
  std::string user_agent_;
  std::string cookies_;
  std::string previous_source_info_;
  std::string previous_video_info_;
  std::string previous_user_defined_changed_;
  std::string previous_media_video_data_;
  std::string updated_payload_;
  blink::WebRect previous_display_window_;
  Preload preload_;

  DISALLOW_COPY_AND_ASSIGN(UMediaClientImpl);
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_UMEDIACLIENT_IMPL_H_
