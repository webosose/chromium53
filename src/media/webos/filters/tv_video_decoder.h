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

#ifndef MEDIA_WEBOS_FILTERS_TV_VIDEO_DECODER_H_
#define MEDIA_WEBOS_FILTERS_TV_VIDEO_DECODER_H_

#include <list>

#include "base/callback.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame_pool.h"
#include "media/webos/base/media_apis_wrapper.h"

struct AVCodecContext;
struct AVFrame;

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class DecoderBuffer;

class MEDIA_EXPORT TvVideoDecoder : public VideoDecoder {
 public:
  TvVideoDecoder(
          const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
          const scoped_refptr<MediaAPIsWrapper>& media_apis_wrapper);
  virtual ~TvVideoDecoder();

  // VideoDecoder implementation.
  std::string GetDisplayName() const override;
  virtual void Initialize(const VideoDecoderConfig& config,
                          bool low_delay,
                          CdmContext * cdm_context,
                          const InitCB& init_cb,
                          const OutputCB& output_cb) override;
  virtual void Decode(const scoped_refptr<DecoderBuffer>& buffer,
                      const DecodeCB& decode_cb) override;
  virtual void Reset(const base::Closure& closure) override;

 private:
  enum DecoderState {
    kUninitialized,
    kNormal,
    kDecodeFinished,
    kError
  };

  // Handles (re-)initializing the decoder with a (new) config.
  // Returns true if initialization was successful.
  bool ConfigureDecoder(bool low_delay);
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DecoderState state_;

  OutputCB output_cb_;

  VideoDecoderConfig config_;

  VideoFramePool frame_pool_;

  bool decode_nalus_;
  scoped_refptr<MediaAPIsWrapper> media_apis_wrapper_;

  DISALLOW_COPY_AND_ASSIGN(TvVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_WEBOS_FILTERS_TV_VIDEO_DECODER_H_
