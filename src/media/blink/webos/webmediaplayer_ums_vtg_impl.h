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

#ifndef MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_UMS_VTG_IMPL_H_
#define MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_UMS_VTG_IMPL_H_

#include "media/blink/webos/webmediaplayer_ums.h"

#include "content/renderer/media/webos/stream_texture_factory.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "media/blink/screen_orientation_type.h"
#include "media/blink/webos/video_frame_provider_vtg_impl.h"

namespace gpu {
struct SyncToken;
}

namespace content {
class StreamTextureFactory;
class RenderThreadImpl;
}

namespace media {

class WebMediaPlayerUMSVTGImpl : public media::WebMediaPlayerUMS {
 public:
  // Constructs a WebMediaPlayer implementation
  // using uMediaServer WebOS media stack.
  // |delegate| may be null.
  WebMediaPlayerUMSVTGImpl(blink::WebLocalFrame* frame,
                           blink::WebMediaPlayerClient* client,
                           base::WeakPtr<WebMediaPlayerDelegate> delegate,
                           const WebMediaPlayerParams& params,
                           blink::WebFloatPoint additionalContentsScale,
                           const blink::WebString& app_id);
  virtual ~WebMediaPlayerUMSVTGImpl();

  virtual bool copyVideoTextureToPlatformTexture(
      gpu::gles2::GLES2Interface* web_graphics_context,
      unsigned int texture,
      unsigned int internal_format,
      unsigned int type,
      bool premultiply_alpha,
      bool flip_y) override;

  virtual void OnPipelineBufferingState(
      UMediaClientImpl::BufferingState buffering_state) override;

  virtual void updateVideo(const blink::WebRect&, bool fullscreen) override;
  virtual void suspend() override;
  virtual void resume() override;

  // WebMediaPlayerDelegate::Observer implementation
  virtual void OnScreenOrientationUpdate(
      ScreenOrientationType screen_orientation) override;

 protected:
  virtual void LoadMedia() override;

  virtual void OnVideoDisplayWindowChange() override;

  const scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;
  VideoFrameProviderVTGImpl* video_frame_provider_vtg_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerUMSVTGImpl);
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_UMS_VTG_IMPL_H_
