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

#include <mutex>
#include <queue>
#include <set>
#include <string>

#include "base/synchronization/lock.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "ui/gfx/geometry/rect.h"
#include "base/memory/weak_ptr.h"

namespace media {

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

  typedef base::Callback<void()> LoadCompletedCB;
  typedef base::Callback<void(const blink::WebRect&)> ActiveRegionCB;

  static MediaAPIsWrapper* Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const PipelineStatusCB& error_cb);

  static void CheckCodecInfo(const std::string& codec_info);
  static Json::Value SupportedCodecMSE() { return supported_codec_mse_; }

  MediaAPIsWrapper(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const PipelineStatusCB& error_cb);

  virtual ~MediaAPIsWrapper();

  virtual void Initialize(const AudioDecoderConfig& audio_config,
                          const VideoDecoderConfig& video_config,
                          const PipelineStatusCB& init_cb) = 0;
  virtual void SetDisplayWindow(const gfx::Rect& rect,
                                const gfx::Rect& in_rect,
                                bool fullscreen,
                                bool forced) = 0;

  virtual bool Loaded() = 0;
  virtual bool Feed(const scoped_refptr<DecoderBuffer>& buffer,
                    FeedType type) = 0;
  virtual uint64_t GetCurrentTime() = 0;
  virtual bool Seek(base::TimeDelta time) = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual void Suspend() = 0;
  virtual void Resume(base::TimeDelta paused_time,
                      RestorePlaybackMode restore_playback_mode) = 0;
  virtual bool IsReleasedMediaResource() = 0;
  virtual bool AllowedFeedVideo() = 0;
  virtual bool AllowedFeedAudio() = 0;
  virtual void Finalize() = 0;
  virtual void SetPlaybackVolume(double volume) = 0;
  virtual bool IsEOSReceived() = 0;

  virtual std::string GetMediaID();
  virtual void SetNaturalSize(const gfx::Size& size) {}
  virtual void Unload() {}
  virtual void SetKeySystem(const std::string& key_system) {}
  virtual void UpdateVideoConfig(const VideoDecoderConfig& video_config) {}

  virtual void SwitchToAutoLayout() {}
  virtual void SetPlayerLoadedCallback(const base::Closure& player_loaded_cb) {}
  virtual void SetActiveRegionCallback(
      const ActiveRegionCB& active_region_cb) {}
  virtual void SetSizeChangeCb(const base::Closure& size_change_cb) {}
  virtual void SetLoadCompletedCb(const LoadCompletedCB& loaded_cb) {}

  virtual void SetVisibility(bool visible);
  virtual bool Visibility();

 protected:
  class BufferQueue {
    public:
      BufferQueue();
      void push(const scoped_refptr<DecoderBuffer>& buffer, FeedType type);
      const std::pair<scoped_refptr<DecoderBuffer>, FeedType> front();
      void pop();
      void clear();

      bool empty() { return queue_.empty(); }
      size_t data_size() const { return data_size_; }

    private:
      typedef std::pair<scoped_refptr<DecoderBuffer>,
                        MediaAPIsWrapper::FeedType> DecoderBufferPair;
      typedef std::queue<DecoderBufferPair> DecoderBufferQueue;
      DecoderBufferQueue queue_;
      size_t data_size_;

      DISALLOW_COPY_AND_ASSIGN(BufferQueue);
  };

  enum State {
    INVALID = -1,
    CREATED = 0,
    CREATED_SUSPENDED,
    LOADING,
    LOADED,
    PLAYING,
    PAUSED,
    SUSPENDED,
    RESUMING,
    SEEKING,
    RESTORING,
    FINALIZED
  };

  enum RestoreDisplayWindowMode {
    DONT_RESTORE_DISPLAY_WINDOW = 0,
    RESTORE_DISPLAY_WINDOW
  };

  enum FeedStatus {
    FeedSucceeded,
    FeedOverflowed,
    FeedFailed
  };

  void SetState(State);
  void UpdateCurrentTime(int64_t current_time);

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::string app_id_;

  PipelineStatusCB error_cb_;
  PipelineStatusCB init_cb_;
  PipelineStatusCB seek_cb_;

  std::recursive_mutex recursive_mutex_;

  State state_;

  int64_t feeded_audio_pts_;
  int64_t feeded_video_pts_;
  double playback_volume_;
  float playback_rate_;
  bool received_eos_;
  bool audio_eos_received_;
  bool video_eos_received_;

  std::unique_ptr<BufferQueue> buffer_queue_;

  gfx::Rect window_rect_;
  gfx::Rect window_in_rect_;

  AudioDecoderConfig audio_config_;
  VideoDecoderConfig video_config_;

  int64_t current_time_;
  bool visible_;

  static Json::Value supported_codec_mse_;

  DISALLOW_COPY_AND_ASSIGN(MediaAPIsWrapper);
};

}  // namespace media

#endif  // MEDIA_WEBOS_BASE_MEDIA_APIS_WRAPPER_H_
