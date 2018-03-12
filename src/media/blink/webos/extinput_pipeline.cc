// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/extinput_pipeline.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/WebKit/public/platform/WebRect.h"

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("ExtCustom", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("ExtCustom:%04d " format, __LINE__, ##__VA_ARGS__)

namespace media {

static int64_t gExternalInputDeviceCounter = 0;

#define BIND_TO_RENDER_LOOP(function)                    \
  (DCHECK(media_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, this)))

ExtInputPipeline::ExtInputPipeline(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : loaded_(false),
      playback_rate_(0),
      media_task_runner_(task_runner),
      luna_service_client_(media::LunaServiceClient::PrivateBus),
      acb_client_(new Acb()),
      acb_load_task_id_(0),
      out_rect_(),
      source_rect_(),
      full_screen_(false),
      subscribe_key_(0) {
  acb_id_ = acb_client_->getAcbId();

  event_handler_ = std::bind(
      &ExtInputPipeline::AcbHandler, make_scoped_refptr(this), acb_id_,
      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
      std::placeholders::_4, std::placeholders::_5);
}

ExtInputPipeline::~ExtInputPipeline() {
  DEBUG_LOG("destroy ExtInput pipeline");
  luna_service_client_.unsubscribe(subscribe_key_);
  if (!external_input_id_.empty()) {
    INFO_LOG("ACB::setState(FOREGROUND, UNLOADED)");
    acb_client_->setState(ACB::AppState::FOREGROUND,
                          ACB::ExtInputState::UNLOADED);
    ClosePipeline();
  }
}

void ExtInputPipeline::Load(
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
  pipeline_message_cb_ = pipeline_message_cb;

  DEBUG_LOG("ACB::initialize(EXT_INPUT, %s)", app_id_.c_str());
  if (!acb_client_->initialize(ACB::PlayerType::EXT_INPUT, app_id_,
                               event_handler_))
    DEBUG_LOG("ACB::initialize failed");

  Json::Value root;
  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::EXTERNALDEVICE, "input/open"),
      writer.write(root),
      BIND_TO_RENDER_LOOP(&ExtInputPipeline::HandleOpenReply));
}

void ExtInputPipeline::Dispose() {
  base::AutoLock auto_lock(lock_);
  DEBUG_LOG("dispose ExtInput pipeline");
  acb_id_ = INVALID_ACB_ID;
  Release();
}

void ExtInputPipeline::HandleOpenReply(const std::string& payload) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool() &&
      response.isMember("externalInputId")) {
    external_input_id_ = response["externalInputId"].asString();
    INFO_LOG("ACB::setMediaId(%s)", external_input_id_.c_str());
    if (!acb_client_->setMediaId(external_input_id_.c_str()))
      DEBUG_LOG("ACB::setMediaId failed");
  } else {
    INFO_LOG("Failed to Open!!! (%s, %s)",
             response["errorCode"].asString().c_str(),
             response["errorText"].asString().c_str());
    response["infoType"] = "reload";

    Json::FastWriter writer;
    if (!pipeline_message_cb_.is_null())
      pipeline_message_cb_.Run(writer.write(response));
    return;
  }
  loaded_ = true;
  has_audio_ = true;
  has_video_ = true;

  if (!buffering_state_cb_.is_null()) {
    buffering_state_cb_.Run(CustomPipeline::HaveMetadata);
    buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
  }
}

void ExtInputPipeline::SetPlaybackRate(float playback_rate) {
  if (external_input_id_.empty()) {
    playback_rate_ = playback_rate;
    return;
  }

  if (playback_rate == 0.0f)
    PausePipeline();
  else if (playback_rate_ == 0.0f)
    PlayPipeline();
  playback_rate_ = playback_rate;
}

void ExtInputPipeline::PlayPipeline() {
  INFO_LOG("ACB::setState(FOREGROUND, LOADED)");
  GetCurrentVideo();
  if (acb_client_->setState(ACB::AppState::FOREGROUND,
                            ACB::ExtInputState::LOADED, &acb_load_task_id_))
    DEBUG_LOG("taskId : %ld", acb_load_task_id_);
}

void ExtInputPipeline::AcbLoadCompleted() {
  CallswitchExternalInput();
  if (!video_display_window_change_cb_.is_null())
    video_display_window_change_cb_.Run();
}

void ExtInputPipeline::CallswitchExternalInput() {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["externalInputId"] = external_input_id_;
  std::string host = url_.host();
  std::transform(host.begin(), host.end(), host.begin(), toupper);
  root["inputSourceType"] = host;
  root["portId"] = url_.port();
  root["uniqueId"] = std::to_string(gExternalInputDeviceCounter++);

  Json::FastWriter writer;
  std::string parameter = writer.write(root);
  std::string uri = media::LunaServiceClient::GetServiceURI(
      media::LunaServiceClient::EXTERNALDEVICE, "input/switchExternalInput");

  luna_service_client_.callASync(
      uri, parameter,
      BIND_TO_RENDER_LOOP(&ExtInputPipeline::HandleSwitchExternalInputReply));
}

void ExtInputPipeline::HandleSwitchExternalInputReply(
    const std::string& payload) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool()) {
    if (!buffering_state_cb_.is_null()) {
      buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
      buffering_state_cb_.Run(CustomPipeline::PipelineStarted);
    }

    INFO_LOG("ACB::setState(FOREGROUND, PLAYING)");
    acb_client_->setState(ACB::AppState::FOREGROUND,
                          ACB::ExtInputState::PLAYING);
  } else {
    INFO_LOG("Failed to switch to external input!!! (%s, %s)",
             response["errorCode"].asString().c_str(),
             response["errorText"].asString().c_str());
    response["infoType"] = "switchExternalInput";

    Json::FastWriter writer;
    if (!pipeline_message_cb_.is_null())
      pipeline_message_cb_.Run(writer.write(response));
  }
}

void ExtInputPipeline::PausePipeline() {
  DEBUG_LOG("%s", __FUNCTION__);
  if (external_input_id_.empty()) {
    DEBUG_LOG("external_input_id_ is not available.");
    return;
  }
  INFO_LOG("ACB::setState(BACKGROUND, SUSPENDED)");
  acb_client_->setState(ACB::AppState::BACKGROUND,
                        ACB::ExtInputState::SUSPENDED);

  Json::Value root;
  root["externalInputId"] = external_input_id_;
  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::EXTERNALDEVICE, "input/pause"),
      writer.write(root));
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(CustomPipeline::HaveFutureData);
}

void ExtInputPipeline::ClosePipeline() {
  Json::Value root;
  root["externalInputId"] = external_input_id_;

  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::EXTERNALDEVICE, "input/stop"),
      writer.write(root));

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::EXTERNALDEVICE, "input/close"),
      writer.write(root));
}

void ExtInputPipeline::GetCurrentVideo() {
  Json::Value root;
  root["broadcastId"] = external_input_id_;
  root["subscribe"] = true;

  Json::FastWriter writer;
  luna_service_client_.subscribe(
      media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::DISPLAY,
                                              "getCurrentVideo"),
      writer.write(root), &subscribe_key_,
      BIND_TO_RENDER_LOOP(&ExtInputPipeline::HandleGetCurrentVideoReply));
}

void ExtInputPipeline::HandleGetCurrentVideoReply(const std::string& payload) {
  DEBUG_LOG("%s", payload.c_str());
  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool() && response.isMember("width") &&
      response.isMember("height")) {
    source_rect_.width = response["width"].asInt();
    source_rect_.height = response["height"].asInt();
  }
}

void ExtInputPipeline::SetDisplayWindow(const blink::WebRect& outRect,
                                        const blink::WebRect& inRect,
                                        bool fullScreen,
                                        bool forced) {
  DEBUG_LOG("(%d, %d, %d, %d) (%d, %d, %d, %d) fullScreen: %s) forced : %s",
            inRect.x, inRect.y, inRect.width, inRect.height, outRect.x,
            outRect.y, outRect.width, outRect.height,
            fullScreen ? "true" : "false", forced ? "true" : "false");

  out_rect_ = outRect;
  full_screen_ = fullScreen;

  acb_client_->setDisplayWindow(outRect.x, outRect.y, outRect.width,
                                outRect.height, fullScreen, 0);
}

void ExtInputPipeline::AcbHandler(long acb_id,
                                  long task_id,
                                  long event_type,
                                  long app_state,
                                  long play_state,
                                  const char* reply) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&ExtInputPipeline::DispatchAcbHandler, this, acb_id,
                            task_id, event_type, app_state, play_state, reply));
}

void ExtInputPipeline::DispatchAcbHandler(long acb_id,
                                          long task_id,
                                          long event_type,
                                          long app_state,
                                          long play_state,
                                          const char* reply) {
  base::AutoLock auto_lock(lock_);
  DEBUG_LOG(
      "** dispatchAcbHandler - acbId:[%ld], taskId:[%ld], \
      eventType:[%ld], appState:[%ld], playState:[%ld], reply:[%s]",
      acb_id, task_id, event_type, app_state, play_state, reply);

  if (acb_id_ != acb_id) {
    DEBUG_LOG("wrong acbId. Ignore this message");
    return;
  }

  if (task_id == acb_load_task_id_) {
    acb_load_task_id_ = 0;
    AcbLoadCompleted();
  }
}

void ExtInputPipeline::ContentsPositionChanged(float position) {
  if (out_rect_.y + position < 0) {
    blink::WebRect out_rect = out_rect_;
    out_rect.y += position;
    blink::WebRect display_rect(0, 0, NaturalVideoSize().width(),
                                NaturalVideoSize().height());
    blink::WebRect clip_rect = gfx::IntersectRects(out_rect, display_rect);

    int y = source_rect_.height * out_rect.y / out_rect.height;
    blink::WebRect in_rect = source_rect_;
    in_rect.y = -y;
    in_rect.height += y;

    DEBUG_LOG("SetCustomDisplayWindow(%d, %d, %d, %d)(%d, %d, %d, %d",
              in_rect.x, in_rect.y, in_rect.width, in_rect.height, clip_rect.x,
              clip_rect.y, clip_rect.width, clip_rect.height);
    acb_client_->setCustomDisplayWindow(
        in_rect.x, in_rect.y, in_rect.width, in_rect.height, clip_rect.x,
        clip_rect.y, clip_rect.width, clip_rect.height, full_screen_, 0);
  } else {
    DEBUG_LOG("SetDisplayWindow(%d, %d, %d, %d)", out_rect_.x,
              out_rect_.y + (int)position, out_rect_.width, out_rect_.height);
    acb_client_->setDisplayWindow(out_rect_.x, out_rect_.y + (int)position,
                                  out_rect_.width, out_rect_.height,
                                  full_screen_, 0);
  }
}

}  // namespace media
