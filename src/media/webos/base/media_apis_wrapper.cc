// Copyright (c) 2017-2018 LG Electronics, Inc.

#include "media/webos/base/media_apis_wrapper.h"

namespace media {

Json::Value MediaAPIsWrapper::supported_codec_mse_ = Json::Value();

MediaAPIsWrapper::BufferQueue::BufferQueue() : data_size_(0) {}

void MediaAPIsWrapper::BufferQueue::push(
    const scoped_refptr<DecoderBuffer>& buffer,
                        MediaAPIsWrapper::FeedType type) {
  queue_.push(DecoderBufferPair(buffer, type));
  data_size_ += buffer->data_size();
}

const std::pair<scoped_refptr<DecoderBuffer>, MediaAPIsWrapper::FeedType>
MediaAPIsWrapper::BufferQueue::front() {
  return queue_.front();
}

void MediaAPIsWrapper::BufferQueue::pop() {
  std::pair<scoped_refptr<DecoderBuffer>,
            MediaAPIsWrapper::FeedType> f = queue_.front();
  data_size_ -= f.first->data_size();
  queue_.pop();
}

void MediaAPIsWrapper::BufferQueue::clear() {
  DecoderBufferQueue empty_queue;
  queue_.swap(empty_queue);
}

#if !defined(USE_GST_MEDIA)
class MEDIA_EXPORT MediaAPIsWrapperStub : public MediaAPIsWrapper {
 public:
  MediaAPIsWrapperStub(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const PipelineStatusCB& error_cb)
    : MediaAPIsWrapper(task_runner, video, app_id, error_cb) {}

  virtual ~MediaAPIsWrapperStub() {}

  void Initialize(const AudioDecoderConfig& audio_config,
                  const VideoDecoderConfig& video_config,
                  const PipelineStatusCB& init_cb) override {}

  void SetDisplayWindow(const gfx::Rect& rect,
                        const gfx::Rect& in_rect,
                        bool fullscreen,
                        bool forced) override {}

  bool Loaded() override { return true; }
  bool Feed(const scoped_refptr<DecoderBuffer>& buffer,
            FeedType type) override {
    return true;
  }
  uint64_t GetCurrentTime() override { return 0; }
  bool Seek(base::TimeDelta time) override { return true; }
  void SetPlaybackRate(float playback_rate) override {}

  void Suspend() override {}
  void Resume(base::TimeDelta paused_time,
              RestorePlaybackMode restore_playback_mode) override {}
  bool IsReleasedMediaResource() override { return false; }
  bool AllowedFeedVideo() override { return true; }
  bool AllowedFeedAudio() override { return true; }
  void Finalize() override {}

  void SetPlaybackVolume(double volume) override {}
  bool IsEOSReceived() override {return true; }
};

MediaAPIsWrapper* MediaAPIsWrapper::Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const PipelineStatusCB& error_cb) {
  return new MediaAPIsWrapperStub(task_runner, video, app_id, error_cb);
}

void MediaAPIsWrapper::CheckCodecInfo(const std::string& codec_info) {
}
#endif

//
// Dummy implementation of the public interface in the case when
// there is no platform implementation available.
// This is needed to get the component compiled for the x86 emulator.
//
MediaAPIsWrapper::MediaAPIsWrapper(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const PipelineStatusCB& error_cb)
    : task_runner_(task_runner),
      app_id_(app_id),
      error_cb_(error_cb),
      state_(INVALID),
      feeded_audio_pts_(-1),
      feeded_video_pts_(-1),
      playback_volume_(1.0),
      playback_rate_(0.0f),
      received_eos_(false),
      audio_eos_received_(false),
      video_eos_received_(false),
      current_time_(0) {
  buffer_queue_.reset(new BufferQueue());

  SetState(CREATED);
}

MediaAPIsWrapper::~MediaAPIsWrapper() {}

std::string MediaAPIsWrapper::GetMediaID() {
  return std::string();
}

void MediaAPIsWrapper::SetState(State next_state) {
  state_ = next_state;
}

void MediaAPIsWrapper::UpdateCurrentTime(int64_t current_time) {
  current_time_ = current_time;
}

}  // namespace media
