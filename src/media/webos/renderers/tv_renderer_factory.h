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

#ifndef MEDIA_WEBOS_RENDERERS_TV_RENDERER_FACTORY_H_
#define MEDIA_WEBOS_RENDERERS_TV_RENDERER_FACTORY_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_vector.h"
#include "media/base/media_export.h"
#include "media/base/renderer_factory.h"

namespace media {

class AudioDecoder;
class AudioHardwareConfig;
class AudioRendererSink;
class DecoderFactory;
class GpuVideoAcceleratorFactories;
class MediaLog;
class VideoDecoder;
class VideoRendererSink;
class MediaAPIsWrapper;

// The default factory class for creating RendererImpl.
class MEDIA_EXPORT TvRendererFactory : public RendererFactory {
 public:
  using GetGpuFactoriesCB = base::Callback<GpuVideoAcceleratorFactories*()>;

  TvRendererFactory(const scoped_refptr<MediaLog>& media_log,
                       DecoderFactory* decoder_factory,
                       const GetGpuFactoriesCB& get_gpu_factories_cb);
  ~TvRendererFactory() final;

  std::unique_ptr<Renderer> CreateRenderer(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const scoped_refptr<base::TaskRunner>& worker_task_runner,
      AudioRendererSink* audio_renderer_sink,
      VideoRendererSink* video_renderer_sink,
      const RequestSurfaceCB& request_surface_cb) final;

  void SetMediaAPIsWrapper(
    const scoped_refptr<MediaAPIsWrapper>& media_apis_wrapper);

 private:
  ScopedVector<AudioDecoder> CreateAudioDecoders(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner);
  ScopedVector<VideoDecoder> CreateVideoDecoders(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const RequestSurfaceCB& request_surface_cb,
      GpuVideoAcceleratorFactories* gpu_factories);

  scoped_refptr<MediaLog> media_log_;

  // Factory to create extra audio and video decoders.
  // Could be nullptr if not extra decoders are available.
  DecoderFactory* decoder_factory_;

  // Creates factories for supporting video accelerators. May be null.
  GetGpuFactoriesCB get_gpu_factories_cb_;

  scoped_refptr<MediaAPIsWrapper> media_apis_wrapper_;

  DISALLOW_COPY_AND_ASSIGN(TvRendererFactory);
};

}  // namespace media

#endif  // MEDIA_WEBOS_RENDERERS_TV_RENDERER_FACTORY_H_
