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

#ifndef MEDIA_WEBOS_FILTERS_TV_AUDIO_DECODER_H_
#define MEDIA_WEBOS_FILTERS_TV_AUDIO_DECODER_H_

#include <list>
#include <memory>

#include "base/callback.h"
#include "base/time/time.h"
#include "media/base/audio_decoder.h"
#include "media/base/demuxer_stream.h"
#include "media/base/media_log.h"
#include "media/base/sample_format.h"
#include "media/webos/base/media_apis_wrapper.h"

struct AVCodecContext;
struct AVFrame;

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class AudioDiscardHelper;
class DecoderBuffer;

class MEDIA_EXPORT TvAudioDecoder : public AudioDecoder {
 public:
  TvAudioDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      const scoped_refptr<MediaAPIsWrapper>& media_apis_wrapper);
  virtual ~TvAudioDecoder();

  // AudioDecoder implementation.
  std::string GetDisplayName() const override;
  virtual void Initialize(const AudioDecoderConfig& config,
                          CdmContext * cdm_context,
                          const InitCB& init_cb,
                          const OutputCB& output_cb) override;
  virtual void Decode(const scoped_refptr<DecoderBuffer>& buffer,
                      const DecodeCB& decode_cb) override;
  virtual void Reset(const base::Closure& closure) override;

 private:
  // There are four states the decoder can be in:
  //
  // - kUninitialized: The decoder is not initialized.
  // - kNormal: This is the normal state. The decoder is idle and ready to
  //            decode input buffers, or is decoding an input buffer.
  // - kDecodeFinished: EOS buffer received, codec flushed and decode finished.
  //                    No further Decode() call should be made.
  // - kError: Unexpected error happened.
  //
  // These are the possible state transitions.
  //
  // kUninitialized -> kNormal:
  //     The decoder is successfully initialized and is ready to decode buffers.
  // kNormal -> kDecodeFinished:
  //     When buffer->end_of_stream() is true and avcodec_decode_audio4()
  //     returns 0 data.
  // kNormal -> kError:
  //     A decoding error occurs and decoding needs to stop.
  // (any state) -> kNormal:
  //     Any time Reset() is called.
  enum DecoderState {
    kUninitialized,
    kNormal,
    kDecodeFinished,
    kError
  };

  // Reset decoder and call |reset_cb_|.
  void DoReset();

  // Handles decoding an unencrypted encoded buffer.
  void DecodeBuffer(const scoped_refptr<DecoderBuffer>& buffer,
                    const DecodeCB& decode_cb);
  // Handles (re-)initializing the decoder with a (new) config.
  // Returns true if initialization was successful.
  bool ConfigureDecoder();

  // Releases resources associated with |codec_context_| and |av_frame_|
  // and resets them to NULL.
  void ResetTimestampState();
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  OutputCB output_cb_;
  InitCB inut_cb_;

  DecoderState state_;

  AudioDecoderConfig config_;

  // AVSampleFormat initially requested; not Chrome's SampleFormat.
  int av_sample_format_;

  std::unique_ptr<AudioDiscardHelper> discard_helper_;

  scoped_refptr<MediaAPIsWrapper> media_apis_wrapper_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(TvAudioDecoder);
};

}  // namespace media

#endif  // MEDIA_WEBOS_FILTERS_TV_AUDIO_DECODER_H_
