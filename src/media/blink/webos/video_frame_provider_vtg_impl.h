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

#ifndef MEDIA_BLINK_WEBOS_VIDEO_FRAME_PROVIDER_VTG_IMPL_H_
#define MEDIA_BLINK_WEBOS_VIDEO_FRAME_PROVIDER_VTG_IMPL_H_

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "cc/layers/video_frame_provider.h"
#include "content/renderer/media/webos/stream_texture_factory.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "media/base/pipeline_metadata.h"
#include "media/base/renderer_factory.h"
#include "media/base/video_frame.h"
#include "media/blink/media_blink_export.h"
#include "media/blink/screen_orientation_type.h"
#include "media/blink/webmediaplayer_delegate.h"
#include "media/blink/webmediaplayer_params.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/platform/WebMediaPlayer.h"

namespace blink {
class WebLocalFrame;
class WebMediaPlayerClient;
class WebMediaPlayerEncryptedMediaClient;
}

namespace content {
class RenderThreadImpl;
class StreamTextureFactory;
}

namespace gpu {
struct SyncToken;
}

using gpu::gles2::GLES2Interface;

namespace media {

class MEDIA_BLINK_EXPORT VideoFrameProviderVTGImpl
    : public NON_EXPORTED_BASE(cc::VideoFrameProvider),
      public base::SupportsWeakPtr<VideoFrameProviderVTGImpl> {
 public:
  VideoFrameProviderVTGImpl(const scoped_refptr<base::SingleThreadTaskRunner>&
                                compositor_task_runner);

  virtual ~VideoFrameProviderVTGImpl() override;

  // cc::VideoFrameProvider implementation.
  virtual void SetVideoFrameProviderClient(
      cc::VideoFrameProvider::Client* client) override;
  virtual scoped_refptr<media::VideoFrame> GetCurrentFrame() override;
  virtual void PutCurrentFrame() override;
  virtual bool UpdateCurrentFrame(base::TimeTicks deadline_min,
                                  base::TimeTicks deadline_max) override;
  virtual bool HasCurrentFrame() override;

  // Getter method to |client_|.
  virtual blink::WebMediaPlayerClient* GetClient();

  virtual bool useVideoTexture() const;

  virtual void activeRegionChanged(const blink::WebRect&);

  virtual void Repaint();

  virtual void setCurrentVideoFrame(
      scoped_refptr<media::VideoFrame> current_frame) {
    current_frame_ = current_frame;
  }

  virtual scoped_refptr<media::VideoFrame>& getCurrentVideoFrame() {
    return current_frame_;
  }

  virtual base::Lock& getLock() { return lock_; }

  virtual void setWebLocalFrame(blink::WebLocalFrame* frame) { frame_ = frame; }

  virtual void setWebMediaPlayerClient(blink::WebMediaPlayerClient* client) {
    client_ = client;
  }

  virtual void UpdateVideoFrame();

  virtual blink::WebRect& getActiveVideoRegion() {
    return active_video_region_;
  }

  virtual void setActiveVideoRegion(blink::WebRect active_video_region) {
    active_video_region_ = active_video_region;
  }

  virtual bool& getActiveVideoRegionChanged() {
    return active_video_region_changed_;
  }

  virtual void setActiveVideoRegionChanged(bool active_video_region_changed) {
    active_video_region_changed_ = active_video_region_changed;
  }

  virtual gfx::Size getNaturalVideoSize() { return natural_video_size_; }

  virtual void setNaturalVideoSize(gfx::Size natural_video_size);

  virtual void createVideoFrameHole();

  virtual void setScreenOrientation(ScreenOrientationType screen_orientation) {
    screen_orientation_ = screen_orientation;
  }

  virtual ScreenOrientationType screenOrientation() {
    return screen_orientation_;
  }

 protected:
  virtual scoped_refptr<VideoFrame> CreateVideoFrame(
      media::VideoFrame::StorageType frame_storage_type);

  virtual void CreateStreamTexture();
  virtual void DeleteStreamTexture();
  virtual void CreateStreamTextureProxyIfNeeded();

  static void OnReleaseTexture(
      const scoped_refptr<content::StreamTextureFactory>& factories,
      const gpu::SyncToken& release_sync_token);

  blink::WebLocalFrame* frame_;
  blink::WebMediaPlayerClient* client_;
  bool is_suspended_;

  gfx::Size natural_video_size_;

  // Video frame rendering members.
  //
  // |lock_| protects |current_frame_| since new frames arrive on the video
  // rendering thread, yet are accessed for rendering on either the main thread
  // or compositing thread depending on whether accelerated compositing is used.
  base::Lock lock_;
  scoped_refptr<media::VideoFrame> current_frame_;

  // A pointer back to the compositor to inform it about state changes. This is
  // not NULL while the compositor is actively using this webmediaplayer.
  cc::VideoFrameProvider::Client* video_frame_provider_client_;

  // Message loops for posting tasks on Chrome's main thread. Also used
  // for DCHECKs so methods calls won't execute in the wrong thread.
  base::MessageLoop* main_loop_;

  blink::WebRect active_video_region_;
  bool active_video_region_changed_;

  const scoped_refptr<base::SingleThreadTaskRunner> compositor_task_runner_;

  // Object for allocating stream textures
  scoped_refptr<content::StreamTextureFactory> stream_texture_factory_;

  // Object for calling back the compositor thread to repaint the video when a
  // frame available. It should be initialized on the compositor thread.
  // Accessed on main thread and on compositor thread when main thread is
  // blocked.
  content::ScopedStreamTextureProxy stream_texture_proxy_;

  // Whether |OnReleaseTexture| event is received.
  static bool stream_texture_busy_;

  // GL texture ID allocated to the video.
  unsigned int texture_id_;

  // GL texture mailbox for texture_id_ to provide in the VideoFrame, and sync
  // point for when the mailbox was produced.
  gpu::Mailbox texture_mailbox_;

  // Stream texture ID allocated to the video.
  unsigned int stream_id_;

  ScreenOrientationType screen_orientation_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameProviderVTGImpl);
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_VIDEO_FRAME_PROVIDER_VTG_IMPL_H_
