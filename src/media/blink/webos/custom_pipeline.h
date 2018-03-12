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

#ifndef MEDIA_BLINK_WEBOS_CUSTOM_PIPELINE_H_
#define MEDIA_BLINK_WEBOS_CUSTOM_PIPELINE_H_

#include <string>

#include "media/blink/webmediaplayer_util.h"
#include "media/base/pipeline.h"

class GURL;

namespace media {

class CustomPipeline {
 public:
  enum BufferingState {
    HaveMetadata,
    HaveFutureData,
    HaveEnoughData,
    PipelineStarted,
    WebOSBufferingStart,
    WebOSBufferingEnd,
  };

  // FIXME - Acb does not define any value for invalid handle in Acb.h
  // however, AcbCore.cpp shows that valid handles go from 1 to MAX_ACB_ID
  // and is of type long. The value of MAX_ACB_ID is #defined as 1000000000
  // the following const should be moved to Acb.h because it belongs there
  // The const is defined here because multiple derived classes use it
  const long INVALID_ACB_ID = -1;

  typedef base::Callback<void(const std::string& detail)> PipelineMsgCB;
  typedef base::Callback<void(BufferingState)> BufferingStateCB;

  CustomPipeline() {}
  virtual ~CustomPipeline() {}
  virtual void Load(
      bool is_video,
      const std::string& app_id,
      const GURL& url,
      const std::string& mime_type,
      const std::string& payload,
      const CustomPipeline::BufferingStateCB& buffering_state_cb,
      const base::Closure& video_display_window_change_cb,
      const CustomPipeline::PipelineMsgCB& pipeline_message_cb) = 0;
  virtual void Dispose() = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual float GetPlaybackRate() const = 0;
  virtual std::string CameraId() const { return std::string(); }
  virtual std::string MediaId() const = 0;

  virtual double Duration() = 0;
  virtual base::TimeDelta CurrentTime() = 0;
  virtual float BufferEnd() = 0;
  virtual bool HasVideo() = 0;
  virtual bool HasAudio() = 0;
  virtual gfx::Size NaturalVideoSize() = 0;
  virtual void SetDisplayWindow(const blink::WebRect&,
                                const blink::WebRect&,
                                bool fullScreen,
                                bool forced = false) = 0;
  virtual bool DidLoadingProgress() = 0;
  virtual void ContentsPositionChanged(float position) {}
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_CUSTOM_PIPELINE_H_
