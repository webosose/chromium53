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

#ifndef MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_MSE_VTG_IMPL_H_
#define MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_MSE_VTG_IMPL_H_

#include "media/blink/screen_orientation_type.h"
#include "media/blink/webos/webmediaplayer_mse.h"
#include "media/blink/webos/video_frame_provider_vtg_impl.h"

namespace media {

class MEDIA_BLINK_EXPORT WebMediaPlayerMSEVTGImpl : public WebMediaPlayerMSE {
 public:
  // Constructs a WebMediaPlayer implementation using Chromium's media stack.
  // |delegate| may be null. |renderer_factory| must not be null.
  WebMediaPlayerMSEVTGImpl(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
      base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
      std::unique_ptr<media::RendererFactory> renderer_factory,
      linked_ptr<media::UrlIndex> url_index,
      const media::WebMediaPlayerParams& params,
      blink::WebFloatPoint additional_contents_scale,
      const blink::WebString& app_id);

  ~WebMediaPlayerMSEVTGImpl() override;

  void updateVideo(const blink::WebRect&, bool fullScreen) override;

  void suspend() override;
  void resume() override;

  // WebMediaPlayerDelegate::Observer implementation
  virtual void OnScreenOrientationUpdate(
      ScreenOrientationType screen_orientation) override;

  scoped_refptr<VideoFrame> GetCurrentFrameFromCompositor() override;

 protected:
  void OnMetadata(PipelineMetadata metadata) override;

  VideoFrameProviderVTGImpl* video_frame_provider_vtg_;

  bool checked_fullscreen_;
  blink::WebRect scaled_rect_;
  blink::WebRect scaled_in_rect_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerMSEVTGImpl);
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_MSE_VTG_IMPL_H_
