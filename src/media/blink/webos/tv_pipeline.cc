// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/tv_pipeline.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/WebKit/public/platform/WebRect.h"

#define INFO_LOG(format, ...)                                          \
  RAW_PMLOG_INFO("TvCustom", "%s:%04d:%s:" format, __FILE__, __LINE__, \
                 __FUNCTION__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...)                                          \
  RAW_PMLOG_DEBUG("TvCustom", "%s:%04d:%s:" format, __FILE__, __LINE__, \
                  __FUNCTION__, ##__VA_ARGS__)

namespace media {

#define BIND_TO_RENDER_LOOP(function)                    \
  (DCHECK(media_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, this)))

TvPipeline::TvPipeline(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : loaded_(false),
      need_change_channel_(true),
      is_first_use_(false),
      is_suspended_(false),
      playback_rate_(0),
      media_task_runner_(task_runner),
      luna_service_client_(media::LunaServiceClient::PrivateBus),
      acb_client_(new Acb()),
      acb_load_task_id_(0),
      acb_suspend_task_id_(0),
      out_rect_(),
      source_rect_(),
      full_screen_(false),
      subscribe_key_(0) {
  acb_id_ = acb_client_->getAcbId();

  event_handler_ = std::bind(&TvPipeline::AcbHandler, make_scoped_refptr(this),
                             acb_id_, std::placeholders::_1,
                             std::placeholders::_2, std::placeholders::_3,
                             std::placeholders::_4, std::placeholders::_5);
}

TvPipeline::~TvPipeline() {
  DEBUG_LOG();
  luna_service_client_.unsubscribe(subscribe_key_);
  if (!broadcast_id_.empty()) {
    DEBUG_LOG("ACB::setState(FOREGROUND, UNLOADED)");
    acb_client_->setState(ACB::AppState::FOREGROUND, ACB::TvState::UNLOADED);
    ClosePipeline();
  }
}

void TvPipeline::Load(
    bool is_video,
    const std::string& app_id,
    const GURL& url,
    const std::string& mime_type,
    const std::string& payload,
    const BufferingStateCB& buffering_state_cb,
    const base::Closure& video_display_window_change_cb,
    const CustomPipeline::PipelineMsgCB& pipeline_message_cb) {
  DEBUG_LOG("url : %s, host : %s, port : %s, payload - %s", url.spec().c_str(),
            url.host().c_str(), url.port().c_str(), payload.c_str());

  app_id_ = app_id;
  url_ = url;
  mime_type_ = mime_type;
  buffering_state_cb_ = buffering_state_cb;
  video_display_window_change_cb_ = video_display_window_change_cb;
  pipeline_message_cb_ = pipeline_message_cb;

  DEBUG_LOG("ACB::initialize(TV, %s)", app_id_.c_str());
  if (!acb_client_->initialize(ACB::PlayerType::TV, app_id_, event_handler_))
    DEBUG_LOG("ACB::initialize failed");

  Json::Value root;
  Json::FastWriter writer;

  if (mime_type_ == "service/webos-broadcast")
    broadcast_type_ = BroadcastTV;
  else if (mime_type_ == "service/webos-broadcast-cable")
    broadcast_type_ = BroadcastCable;

  if (!payload.empty()) {
    Json::Value response;
    Json::Reader reader;

    reader.parse(payload, response);
    if (response.isMember("firstUse")) {
      if (response["firstUse"].asBool()) {
        is_first_use_ = true;
        DEBUG_LOG("It's the first launching after booting.");
      }
      response.removeMember("firstUse");
    }
    if (broadcast_type_ == BroadcastCable) {
      last_channel_id_ = writer.write(response);
      DEBUG_LOG("Cable channelId : %s", last_channel_id_.c_str());
    }
  }

  Json::Value options;
  options["appId"] = app_id_;
  root["options"] = options;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::BROADCAST, "open"),
      writer.write(root), BIND_TO_RENDER_LOOP(&TvPipeline::HandleOpenReply));
}

void TvPipeline::Dispose() {
  base::AutoLock auto_lock(lock_);
  DEBUG_LOG("dispose TV pipeline");
  acb_id_ = INVALID_ACB_ID;
  Release();
}

void TvPipeline::HandleOpenReply(const std::string& payload) {
  DEBUG_LOG("%s", payload.c_str());

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool() && response.isMember("broadcastId")) {
    broadcast_id_ = response["broadcastId"].asString();
    DEBUG_LOG("ACB::setMediaId(%s)", broadcast_id_.c_str());
    if (!acb_client_->setMediaId(broadcast_id_.c_str()))
      DEBUG_LOG("ACB::setMediaId failed");
    GetVideoInfo();
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

  if (is_suspended_) {
    INFO_LOG("App has been suspended");
    return;
  }

  if (!buffering_state_cb_.is_null()) {
    buffering_state_cb_.Run(CustomPipeline::HaveMetadata);
    buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
  }
}

void TvPipeline::SetPlaybackRate(float playback_rate) {
  if (broadcast_id_.empty()) {
    if (playback_rate == 0.0f)
      is_suspended_ = true;
    playback_rate_ = playback_rate;
    return;
  }

  DEBUG_LOG("playback_rate : %f", playback_rate);
  if (playback_rate == 0.0f)
    PausePipeline();
  else if (playback_rate_ == 0.0f)
    PlayPipeline();

  playback_rate_ = playback_rate;
}

void TvPipeline::PlayPipeline() {
  DEBUG_LOG();
  if (broadcast_id_.empty()) {
    DEBUG_LOG("broadcast_id_ is not available.");
    return;
  }

  if (is_suspended_) {
    need_change_channel_ = true;
    if (broadcast_type_ == BroadcastTV)
      GetLastChannelId();
    else if (broadcast_type_ == BroadcastCable)
      ChangeAcbLoaded(ACB::TvState::LOADED);
  } else {
    if (broadcast_type_ == BroadcastCable) {
      if (is_first_use_) {
        need_change_channel_ = false;
        ChangeAcbLoaded(ACB::TvState::LOADED_FIRST);
      } else {
        need_change_channel_ = true;
        ChangeAcbLoaded(ACB::TvState::LOADED);
      }
    } else if (broadcast_type_ == BroadcastTV) {
      need_change_channel_ = true;
      if (url_.host().empty())
        GetLastChannelId();
      else {
        last_channel_id_ = url_.host();
        ChangeAcbLoaded(ACB::TvState::LOADED);
      }
    }
  }
  is_suspended_ = false;
}

void TvPipeline::ChangeAcbLoaded(const long& playState) {
  DEBUG_LOG("ACB::setState(FOREGROUND, %s)",
            playState == ACB::TvState::LOADED ? "LOADED" : "LOADED_FIRST");
  if (acb_client_->setState(ACB::AppState::FOREGROUND, playState,
                            &acb_load_task_id_))
    DEBUG_LOG("taskId : %ld", acb_load_task_id_);
}

void TvPipeline::AcbLoadCompleted() {
  DEBUG_LOG();
  if (!video_display_window_change_cb_.is_null())
    video_display_window_change_cb_.Run();

  if (!last_channel_id_.empty() && need_change_channel_) {
    need_change_channel_ = false;
    if (broadcast_type_ == BroadcastTV)
      ChangeChannel(last_channel_id_);
    else if (broadcast_type_ == BroadcastCable)
      ChangeVolatileChannel(last_channel_id_);
  } else {
    INFO_LOG("Don't call changeChannel. (First launch or No channelId)");
    if (!buffering_state_cb_.is_null())
      buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);

    INFO_LOG("ACB::setState(FOREGROUND, PLAYING)");
    acb_client_->setState(ACB::AppState::FOREGROUND, ACB::TvState::PLAYING);
  }
}

void TvPipeline::GetLastChannelId() {
  DEBUG_LOG();
  Json::Value root;
  root["sourceType"] = "INPUTSOURCE_NONE";

  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::CHANNEL,
                                              "getLastChannelId"),
      writer.write(root),
      BIND_TO_RENDER_LOOP(&TvPipeline::HandleGetLastChannelIdReply));
}

void TvPipeline::HandleGetLastChannelIdReply(const std::string& payload) {
  DEBUG_LOG("%s", payload.c_str());

  if (is_suspended_) {
    INFO_LOG("App has been suspended");
    playback_rate_ = 0.0f;
    return;
  }

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool() && response.isMember("channelId")) {
    last_channel_id_ = response["channelId"].asString();
    ChangeAcbLoaded(ACB::TvState::LOADED);
  } else {
    INFO_LOG("Failed to get last channel ID!!! (%s, %s)",
             response["errorCode"].asString().c_str(),
             response["errorText"].asString().c_str());
    response["infoType"] = "getLastChannelId";

    Json::FastWriter writer;
    if (!pipeline_message_cb_.is_null())
      pipeline_message_cb_.Run(writer.write(response));
  }
}

void TvPipeline::ChangeChannel(const std::string& channel_id) {
  Json::Value root;
  root["broadcastId"] = broadcast_id_;
  root["channelId"] = channel_id;

  Json::FastWriter writer;

  INFO_LOG("call ChangeChannel(%s) [%s]", channel_id.c_str(),
           broadcast_id_.c_str());
  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::BROADCAST, "changeChannel"),
      writer.write(root),
      BIND_TO_RENDER_LOOP(&TvPipeline::HandleChangeChannelReply));
}

void TvPipeline::ChangeVolatileChannel(const std::string& channel_info) {
  Json::Value channel_object;
  Json::Reader reader;

  reader.parse(channel_info, channel_object);
  channel_object["broadcastId"] = broadcast_id_;

  Json::FastWriter writer;

  INFO_LOG("call changeVolatileChannel(%s) [%s]", channel_info.c_str(),
           broadcast_id_.c_str());
  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::BROADCAST, "changeVolatileChannel"),
      writer.write(channel_object),
      BIND_TO_RENDER_LOOP(&TvPipeline::HandleChangeChannelReply));
}

void TvPipeline::HandleChangeChannelReply(const std::string& payload) {
  DEBUG_LOG("%s", payload.c_str());

  if (is_suspended_) {
    INFO_LOG("App has been suspended");
    return;
  }

  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      !response["returnValue"].asBool()) {
    INFO_LOG("Failed to change channel!!! (%s, %s)",
             response["errorCode"].asString().c_str(),
             response["errorText"].asString().c_str());
    response["infoType"] = "changeChannel";

    Json::FastWriter writer;
    if (!pipeline_message_cb_.is_null())
      pipeline_message_cb_.Run(writer.write(response));
  }

  if (!buffering_state_cb_.is_null()) {
    buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
    buffering_state_cb_.Run(CustomPipeline::PipelineStarted);
  }

  INFO_LOG("ACB::setState(FOREGROUND, PLAYING)");
  acb_client_->setState(ACB::AppState::FOREGROUND, ACB::TvState::PLAYING);
}

void TvPipeline::PausePipeline() {
  DEBUG_LOG();
  if (broadcast_id_.empty()) {
    DEBUG_LOG("broadcast_id_ is not available.");
    return;
  }

  is_suspended_ = true;
  INFO_LOG("ACB::setState(BACKGROUND, SUSPENDED)");
  acb_client_->setState(ACB::AppState::BACKGROUND, ACB::TvState::SUSPENDED,
                        &acb_suspend_task_id_);
}

void TvPipeline::AcbSuspendCompleted() {
  DEBUG_LOG("call broadcast pause");
  Json::Value root;
  root["broadcastId"] = broadcast_id_;
  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::BROADCAST, "pause"),
      writer.write(root));

  is_suspended_ = true;
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(CustomPipeline::HaveFutureData);
}

void TvPipeline::ClosePipeline() {
  DEBUG_LOG();
  Json::Value root;
  root["broadcastId"] = broadcast_id_;

  Json::FastWriter writer;

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::BROADCAST, "stop"),
      writer.write(root));

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::BROADCAST, "close"),
      writer.write(root));
}

void TvPipeline::GetVideoInfo() {
  DEBUG_LOG();
  Json::Value root;
  root["broadcastId"] = broadcast_id_;
  root["subscribe"] = true;

  Json::FastWriter writer;
  luna_service_client_.subscribe(
      media::LunaServiceClient::GetServiceURI(
          media::LunaServiceClient::BROADCAST, "getVideoInfo"),
      writer.write(root), &subscribe_key_,
      BIND_TO_RENDER_LOOP(&TvPipeline::HandleGetVideoInfoReply));
}

void TvPipeline::HandleGetVideoInfoReply(const std::string& payload) {
  DEBUG_LOG("%s", payload.c_str());
  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool()) {
    source_rect_.width = response["videoInfo"]["video"]["width"].asInt();
    source_rect_.height = response["videoInfo"]["video"]["height"].asInt();
  }
}

void TvPipeline::SetDisplayWindow(const blink::WebRect& outRect,
                                  const blink::WebRect& inRect,
                                  bool fullScreen,
                                  bool forced) {
  DEBUG_LOG("(%d, %d, %d, %d) (%d, %d, %d, %d) fullScreen: %s) forced : %s",
            inRect.x, inRect.y, inRect.width, inRect.height, outRect.x,
            outRect.y, outRect.width, outRect.height,
            fullScreen ? "true" : "false", forced ? "true" : "false");
  if (outRect.isEmpty())
    return;

  out_rect_ = outRect;
  full_screen_ = fullScreen;
  acb_client_->setDisplayWindow(outRect.x, outRect.y, outRect.width,
                                outRect.height, fullScreen, 0);
}

void TvPipeline::AcbHandler(long acb_id,
                            long task_id,
                            long event_type,
                            long app_state,
                            long play_state,
                            const char* reply) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&TvPipeline::DispatchAcbHandler, this, acb_id,
                            task_id, event_type, app_state, play_state, reply));
}

void TvPipeline::DispatchAcbHandler(long acb_id,
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
  } else if (task_id == acb_suspend_task_id_) {
    acb_suspend_task_id_ = 0;
    AcbSuspendCompleted();
  }
}

void TvPipeline::ContentsPositionChanged(float position) {
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
