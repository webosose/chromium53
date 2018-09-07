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

#ifndef MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_MSE_H_
#define MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_MSE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "media/blink/webmediaplayer_impl.h"
#include "third_party/WebKit/public/platform/WebRect.h"
#include "third_party/WebKit/public/platform/WebFloatPoint.h"


namespace media {
class MediaAPIsWrapper;

// The canonical implementation of blink::WebMediaPlayer that's backed by
// Pipeline. Handles normal resource loading, Media Source, and
// Encrypted Media.
class MEDIA_BLINK_EXPORT WebMediaPlayerMSE : public WebMediaPlayerImpl {
 public:
  // Constructs a WebMediaPlayer implementation using Chromium's media stack.
  // |delegate| may be null. |renderer_factory| must not be null.
  WebMediaPlayerMSE(
      blink::WebLocalFrame* frame,
      blink::WebMediaPlayerClient* client,
      blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
      base::WeakPtr<media::WebMediaPlayerDelegate> delegate,
      std::unique_ptr<media::RendererFactory> renderer_factory,
      linked_ptr<media::UrlIndex> url_index,
      const media::WebMediaPlayerParams& params,
      blink::WebFloatPoint additional_contents_scale,
      const blink::WebString& app_id);
  ~WebMediaPlayerMSE() override;

  void load(LoadType load_type,
            const blink::WebMediaPlayerSource& source,
            CORSMode cors_mode) override;

  void play() override;

  void pause() override;

  void setRate(double rate) override;

  void setVolume(double volume) override;

  void updateVideo(const blink::WebRect&, bool fullScreen) override;

  void setContentDecryptionModule(
      blink::WebContentDecryptionModule* cdm,
      blink::WebContentDecryptionModuleResult result) override;

  // WebMediaPlayerDelegate::Observer interface stubs
  void OnHidden() override {}
  void OnShown() override {}
  void OnSuspendRequested(bool must_suspend) override {}
  void OnPlay() override {}
  void OnPause() override {}
  void OnVolumeMultiplierUpdate(double multiplier) override {}

  void suspend() override;
  void resume() override;

 protected:
  enum StatusOnSuspended {
    UnknownStatus = 0,
    PlayingStatus,
    PausedStatus,
  };

  // Notifies blink of the video size change.
  void OnVideoSizeChange();

  // helpers
  blink::WebRect ScaleWebRect(const blink::WebRect& rect,
                              blink::WebFloatPoint scale);

  void StartPaintTimer();
  void StopPaintTimer();
  void PaintTimerFired(void);

  const blink::WebFloatPoint additional_contents_scale_;
  std::string app_id_;
  bool is_video_offscreen_;
  StatusOnSuspended status_on_suspended_;
  bool is_suspended_;
  bool suspended_by_policy_;
#if defined(PLATFORM_APOLLO)
  blink::WebRect render_view_bounds_;
#endif

  bool pending_size_change_;

  scoped_refptr<media::MediaAPIsWrapper> media_apis_wrapper_;
  blink::WebRect previous_video_rect_;
  base::RepeatingTimer paint_timer_;

  gfx::Size display_resolution_;

  DISALLOW_COPY_AND_ASSIGN(WebMediaPlayerMSE);
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_WEBMEDIAPLAYER_MSE_H_
