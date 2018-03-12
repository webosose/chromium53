// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef MEDIA_WEBOS_BASE_MEDIA_APIS_WRAPPER_H_
#define MEDIA_WEBOS_BASE_MEDIA_APIS_WRAPPER_H_

#include <string>
#include <mutex>

#include "base/synchronization/lock.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "media/webos/base/lunaservice_client.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "ui/gfx/geometry/rect.h"
#include "base/memory/weak_ptr.h"

#if defined(USE_DIRECTMEDIA2)
#include <ndl-directmedia2/states.h>
#include <ndl-directmedia2/esplayer-api.h>
#endif

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

#if defined(USE_DIRECTMEDIA2)
class Frame : public NDL_ESP_STREAM_BUFFER {
public:
 Frame(NDL_ESP_STREAM_T type, const scoped_refptr<DecoderBuffer>& buffer)
#if defined(PLATFORM_APOLLO)
     : decoder_buffer(buffer) {
   data = (uint8_t*)buffer->data();
#else
 {
   data = new uint8_t[buffer->data_size()];
   memcpy(data, buffer->data(), buffer->data_size());
#endif
   data_len = buffer->data_size();
   offset = 0;
   stream_type = type;
   timestamp = buffer->timestamp().InMicroseconds();
   flags = buffer->eos_for_directmedia();
 }

 explicit Frame(NDL_ESP_STREAM_T type) {
   // end of frame
   data_len = 0;
   data = nullptr;
   offset = 0;
   stream_type = type;
   timestamp = 0;
   flags = NDL_ESP_FLAG_END_OF_STREAM;
 }
 ~Frame() {
#if !defined(PLATFORM_APOLLO)
   delete[] data;
#endif
 }
#if defined(PLATFORM_APOLLO)
private:
 scoped_refptr<DecoderBuffer> decoder_buffer;
#endif
};
#endif

class MEDIA_EXPORT MediaAPIsWrapper
    : public base::RefCountedThreadSafe<media::MediaAPIsWrapper> {
public:
  enum FeedType {
    Video = 1,
    Audio,
  };

  enum RestorePlaybackMode {
    RESTORE_PAUSED,
    RESTORE_PLAYING
  };

  MediaAPIsWrapper(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const PipelineStatusCB& error_cb);
  ~MediaAPIsWrapper();

  static void Callback(
      const gint type, const gint64 numValue, const gchar *strValue);
  void DispatchCallback(
      const gint type, const gint64 numValue, const std::string& strValue);

  void Initialize(const AudioDecoderConfig& audio_config,
                  const VideoDecoderConfig& video_config,
                  const PipelineStatusCB& init_cb);

  void SetDisplayWindow(const gfx::Rect& rect,
                        const gfx::Rect& in_rect, bool fullscreen);
  void SetNaturalSize(const gfx::Size& size);

  bool Loaded();
  bool Feed(const scoped_refptr<DecoderBuffer>& buffer, FeedType type);
  uint64_t GetCurrentTime();
  bool Seek(base::TimeDelta time);
  void SetPlaybackRate(float playback_rate);
  std::string GetMediaID(void);
  void Suspend();
  void Resume(base::TimeDelta paused_time,
              RestorePlaybackMode restore_playback_mode);
  bool IsReleasedMediaResource();
  bool allowedFeedVideo();
  bool allowedFeedAudio();
  void Finalize();
  static void CheckCodecInfo(const std::string& codec_info);
  static Json::Value SupportedCodecMSE() { return supported_codec_mse_; }

#if defined(ENABLE_LG_SVP)
  void setKeySystem(const std::string key_system);
#endif

#if defined(USE_DIRECTMEDIA2)
  void ESPlayerCallback(NDL_ESP_EVENT event, void* playerdata);
#endif

  void SetPlaybackVolume(double volume);
  bool IsEOSReceived() { return received_ndl_eos_; }

 private:
  enum State {
      INVALID = -1,
      CREATED = 0,
      CREATED_SUSPENDED,
      LOADED,
      PLAYING,
      PAUSED,
      SUSPENDED,
      FINALIZING,
      FINALIZED
  };

  enum RestoreDisplayWindowMode {
    DONT_RESTORE_DISPLAY_WINDOW = 0,
    RESTORE_DISPLAY_WINDOW
  };

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::string app_id_;

  PipelineStatusCB error_cb_;
  PipelineStatusCB init_cb_;
  PipelineStatusCB seek_cb_;

  std::recursive_mutex recursive_mutex_;

  State state_;

  bool can_feed_video_ {false};
  bool can_feed_audio_ {false};
  bool do_play_;
  bool resume_in_paused_state_;
  bool received_ndl_eos_;
  double volume_per_tab_;
#if defined(USE_BROADCOM)
  bool audio_port_changed_ {false};
  bool video_port_changed_ {false};
  bool audio_first_frame_feeded_ {false};
  bool video_first_frame_feeded_ {false};
#endif

  int64_t current_pts_;
  gfx::Rect window_rect_;
  gfx::Rect window_in_rect_;
  gfx::Size natural_size_;
  int int_fullscreen_;
  float playback_rate_;

  std::string media_id_;

#if defined(WEBOS_BROWSER)
  bool is_webos_browser_;
#endif

  AudioDecoderConfig audio_config_;
  VideoDecoderConfig video_config_;

#if defined(USE_DIRECTMEDIA2)
  NDL_EsplayerHandle esplayer_;
  NDL_ESP_META_DATA metadata_ = {NDL_ESP_VIDEO_CODEC_H264,
                                 NDL_ESP_AUDIO_CODEC_AAC
#if !defined(ENABLE_LG_SVP)
                                };
#else
                                ,NDL_DRM_UNKNOWN
                                };
#endif
#endif

  LunaServiceClient ls_client_;
  base::WeakPtrFactory<MediaAPIsWrapper> weak_factory_;
  // helpers

  void reset();
  void create();
  bool hasResource();
  bool load();
  void playIfNeeded();
  bool flush();
  bool play(RestoreDisplayWindowMode restore_display_window = DONT_RESTORE_DISPLAY_WINDOW,
            RestorePlaybackMode restore_playback_mode = RESTORE_PLAYING);
  bool pause();
  void enableAVFeed();
  void disableAVFeed();
  bool playerReadyForPlayOrSeek();
  std::string getMediaID(void);

  void setMetaDataVideoCodec();
  void setMetaDataAudioCodec();

  void setDisplayWindow(const gfx::Rect& rect, const gfx::Rect& in_rect, bool fullscreen);

  static Json::Value supported_codec_mse_;

  DISALLOW_COPY_AND_ASSIGN(MediaAPIsWrapper);
};

}  // namespace media

#endif  // MEDIA_WEBOS_BASE_MEDIA_APIS_WRAPPER_H_
