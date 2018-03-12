// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_WEBOS_CAMERA_PIPELINE_H_
#define MEDIA_BLINK_WEBOS_CAMERA_PIPELINE_H_

#include <string>
#include <cstdint>
#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "media/base/pipeline.h"
#include "media/blink/webmediaplayer_util.h"
#include "media/blink/webos/custom_pipeline.h"
#include "media/webos/base/lunaservice_client.h"
#include <uMediaClient.h>
#include <MDCContentProvider.h>

class GURL;

namespace media {

class CameraPipeline : public CustomPipeline,
                       public uMediaServer::uMediaClient,
                       public base::RefCountedThreadSafe<CameraPipeline> {
 public:
  enum ErrorCode {
    ImageDecodeError = 500,
    ImageDisplayError = 501,
  };
  CameraPipeline(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  void Load(bool is_video,
            const std::string& app_id,
            const GURL& url,
            const std::string& mime_type,
            const std::string& payload,
            const CustomPipeline::BufferingStateCB& buffering_state_cb,
            const base::Closure& video_display_window_change_cb,
            const CustomPipeline::PipelineMsgCB& pipeline_message_cb) override;
  void Dispose() override;
  void SetPlaybackRate(float playback_rate) override;
  float GetPlaybackRate() const override { return playback_rate_; }
  std::string CameraId() const override { return camera_id_; }
  std::string MediaId() const override { return media_id_; }

  double Duration() override { return static_cast<double>(0); }
  base::TimeDelta CurrentTime() override {
    return base::TimeDelta::FromMilliseconds(0);
  }
  float BufferEnd() override { return static_cast<float>(0); }
  bool HasVideo() override { return has_video_; }
  bool HasAudio() override { return has_audio_; }
  gfx::Size NaturalVideoSize() override { return gfx::Size(1920, 1080); }
  void SetDisplayWindow(const blink::WebRect&,
                        const blink::WebRect&,
                        bool fullScreen,
                        bool forced = false) override;

  void SetMediaVideoData(const struct uMediaServer::video_info_t& video_info);
  bool DidLoadingProgress() override { return false; }

  bool onLoadCompleted() override;
  bool onUnloadCompleted() override;
  bool onPlaying() override;
  bool onPaused() override;
  bool onError(long long errorCode, const std::string& errorText) override;
  bool onSourceInfo(const uMediaServer::source_info_t& sourceInfo) override;
  bool onAudioInfo(const uMediaServer::audio_info_t& audioInfo) override;
  bool onVideoInfo(const uMediaServer::video_info_t&) override;
  bool onEndOfStream() override;
  bool onFileGenerated() override;
  bool onRecordInfo(const uMediaServer::record_info_t& recordInfo) override;
  bool onUserDefinedChanged(const char* message) override;

  void DispatchLoadCompleted();
  void DispatchUnloadCompleted();
  void DispatchVideoInfo(const struct uMediaServer::video_info_t& videoInfo);
  void DispatchPlaying();

 private:
  friend class base::RefCountedThreadSafe<CameraPipeline>;
  ~CameraPipeline();

  struct Media3DInfo {
    std::string pattern;
    std::string type;
  };

  bool loaded_;
  bool has_video_;
  bool has_audio_;
  float playback_rate_;
  const scoped_refptr<base::SingleThreadTaskRunner> media_task_runner_;
  media::LunaServiceClient luna_service_client_;
  std::unique_ptr<MDCContentProvider> mdc_content_provider_;
  std::string app_id_;
  GURL url_;
  std::string mime_type_;
  base::Closure video_display_window_change_cb_;
  BufferingStateCB buffering_state_cb_;
  PipelineMsgCB pipeline_message_cb_;
  std::string camera_id_;
  std::string media_id_;

  void PlayPipeline();
  void PausePipeline();
  gfx::Size getResoultionFromPAR(const std::string& par);
  struct Media3DInfo getMedia3DInfo(const std::string& media_3dinfo);
  DISALLOW_COPY_AND_ASSIGN(CameraPipeline);
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_CAMERA_PIPELINE_H_
