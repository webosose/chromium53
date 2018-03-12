// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/photo_pipeline.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/WebKit/public/platform/WebRect.h"

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("PhotoPipeline", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("PhotoPipeline:%04d " format, __LINE__, ##__VA_ARGS__)

namespace media {

#define BIND_TO_RENDER_LOOP(function)                    \
  (DCHECK(media_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, this)))

PhotoPipeline::PhotoPipeline(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : is_loaded_(false),
      is_suspended_(false),
      playback_rate_(0),
      pipeline_state_(Close),
      media_task_runner_(task_runner),
      luna_service_client_(media::LunaServiceClient::PrivateBus),
      subscribe_key_(0),
      osd_size_(0, 0, 1920, 1080) {}

PhotoPipeline::~PhotoPipeline() {
  DEBUG_LOG("destroy Photo pipeline");
  if (!pipeline_id_.empty()) {
    luna_service_client_.unsubscribe(subscribe_key_);
    ClosePipeline();
  }
}

void PhotoPipeline::Dispose() {
  DEBUG_LOG("dispose Photo pipeline");
  Release();
}

void PhotoPipeline::Load(
    bool is_video,
    const std::string& app_id,
    const GURL& url,
    const std::string& mime_type,
    const std::string& payload,
    const CustomPipeline::BufferingStateCB& buffering_state_cb,
    const base::Closure& video_display_window_change_cb,
    const CustomPipeline::PipelineMsgCB& pipeline_message_cb) {
  DEBUG_LOG("url : %s, host : %s port : %s", url.spec().c_str(),
            url.host().c_str(), url.port().c_str());
  DEBUG_LOG("payload - %s", payload.empty() ? "{}" : payload.c_str());

  app_id_ = app_id;
  url_ = url;
  mime_type_ = mime_type;
  buffering_state_cb_ = buffering_state_cb;
  video_display_window_change_cb_ = video_display_window_change_cb;

  Json::Value root;
  Json::FastWriter writer;

  if (mime_type_ == "service/webos-photo")
    root["type"] = "photo";
  else if (mime_type_ == "service/webos-photo-camera")
    root["type"] = "photo-camera";

  root["appId"] = app_id_;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::PHOTORENDERER, "open"),
      writer.write(root), BIND_TO_RENDER_LOOP(&PhotoPipeline::HandleOpenReply));
}

void PhotoPipeline::HandleOpenReply(const std::string& payload) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool() && response.isMember("pipelineID")) {
    pipeline_id_ = response["pipelineID"].asString();
    pipeline_state_ = Open;
    CallPhotoRendererSubscribe();
  }
  is_loaded_ = true;
  has_audio_ = true;
  has_video_ = true;

  if (!buffering_state_cb_.is_null()) {
    buffering_state_cb_.Run(CustomPipeline::HaveMetadata);
    buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
  }
}

void PhotoPipeline::CallPhotoRendererSubscribe() {
  Json::Value root;
  root["pipelineID"] = pipeline_id_;
  root["subscribe"] = true;

  Json::FastWriter writer;

  luna_service_client_.subscribe(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::PHOTORENDERER, "subscribe"),
      writer.write(root), &subscribe_key_,
      BIND_TO_RENDER_LOOP(&PhotoPipeline::HandleSubscribeReply));
}

void PhotoPipeline::HandleSubscribeReply(const std::string& payload) {
  DEBUG_LOG("%s : %s", __FUNCTION__, payload.c_str());

  Json::Value response;
  Json::Reader reader;

  reader.parse(payload, response);
  if (response.isMember("status")) {
    std::string status = response["status"].asString();
    if (status == "registered") {
      CallPhotoRendererPlay();
    } else if (status == "playing") {
      pipeline_state_ = Playing;
      ChangePlayingState();
    } else if (status == "paused") {
      pipeline_state_ = Paused;
    }
  }
  if (response.isMember("sinkInfo")) {
    std::string pattern = response["sinkInfo"]["pattern"].asString();
    blink::WebRect in_rect(
        response["sinkInfo"]["drawRect"]["sink"]["input"]["x"].asInt(),
        response["sinkInfo"]["drawRect"]["sink"]["input"]["y"].asInt(),
        response["sinkInfo"]["drawRect"]["sink"]["input"]["w"].asInt(),
        response["sinkInfo"]["drawRect"]["sink"]["input"]["h"].asInt());
    blink::WebRect out_rect(
        response["sinkInfo"]["drawRect"]["sink"]["output"]["x"].asInt(),
        response["sinkInfo"]["drawRect"]["sink"]["output"]["y"].asInt(),
        response["sinkInfo"]["drawRect"]["sink"]["output"]["w"].asInt(),
        response["sinkInfo"]["drawRect"]["sink"]["output"]["h"].asInt());

    Json::Value root;
    Json::FastWriter writer;

    root["pipelineID"] = pipeline_id_;
    root["width"] = out_rect.width;
    root["height"] = out_rect.height;
    root["horz"] = out_rect.x;
    root["vert"] = out_rect.y;

    root["in_width"] = in_rect.width;
    root["in_height"] = in_rect.height;
    root["in_horz"] = in_rect.x;
    root["in_vert"] = in_rect.y;

    luna_service_client_.callASync(
        media::LunaServiceClient::GetServiceURI(
            media::LunaServiceClient::PHOTORENDERER, "setWindow"),
        writer.write(root),
        BIND_TO_RENDER_LOOP(&PhotoPipeline::HandleSetWindowReply));
  }
}

void PhotoPipeline::HandleSetWindowReply(const std::string& payload) {
  DEBUG_LOG("%s : Display window set", __FUNCTION__);
}

void PhotoPipeline::SetPlaybackRate(float playback_rate) {
  if (pipeline_id_.empty()) {
    playback_rate_ = playback_rate;
    return;
  }

  if (playback_rate == 0.0f)
    PausePipeline();
  else if (playback_rate_ == 0.0f)
    PlayPipeline();
  playback_rate_ = playback_rate;
}

void PhotoPipeline::PlayPipeline() {
  if (is_suspended_)
    CallPhotoRendererResume();

  is_suspended_ = false;
}

void PhotoPipeline::CallPhotoRendererResume() {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["pipelineID"] = pipeline_id_;
  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::PHOTORENDERER, "resume"),
      writer.write(root),
      BIND_TO_RENDER_LOOP(&PhotoPipeline::HandleResumeReply));
}

void PhotoPipeline::HandleResumeReply(const std::string& payload) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool()) {
  } else {
    INFO_LOG("payload - %s", payload.empty() ? "{}" : payload.c_str());
    // TODO(sooho): Report error.
  }
}

void PhotoPipeline::CallPhotoRendererPlay() {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["pipelineID"] = pipeline_id_;
  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::PHOTORENDERER, "play"),
      writer.write(root));
}

void PhotoPipeline::ChangePlayingState() {
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
}

void PhotoPipeline::PausePipeline() {
  DEBUG_LOG("%s", __FUNCTION__);
  if (pipeline_id_.empty()) {
    DEBUG_LOG("pipeline_id_ is not available.");
    return;
  }

  is_suspended_ = true;
  if (pipeline_state_ != Paused)
    CallPhotoRendererPause();

  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(CustomPipeline::HaveFutureData);
}

void PhotoPipeline::CallPhotoRendererPause() {
  Json::Value root;
  root["pipelineID"] = pipeline_id_;

  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::PHOTORENDERER, "pause"),
      writer.write(root));
}

void PhotoPipeline::ClosePipeline() {
  Json::Value root;
  root["pipelineID"] = pipeline_id_;

  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::PHOTORENDERER, "close"),
      writer.write(root));
}

void PhotoPipeline::SetDisplayWindow(const blink::WebRect& outRect,
                                     const blink::WebRect& inRect,
                                     bool fullScreen,
                                     bool forced) {
  DEBUG_LOG("(%d, %d, %d, %d) (%d, %d, %d, %d) fullScreen: %s) forced : %s",
            inRect.x, inRect.y, inRect.width, inRect.height, outRect.x,
            outRect.y, outRect.width, outRect.height,
            fullScreen ? "true" : "false", forced ? "true" : "false");
  if (outRect.isEmpty())
    return;

  osd_size_ = outRect;
}

}  // namespace media
