// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/dvr_pipeline.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "media/base/bind_to_current_loop.h"
#include "net/base/escape.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/WebKit/public/platform/WebRect.h"

#define INFO_LOG(format, ...)                                             \
  RAW_PMLOG_INFO("DVRpipe", ":%04d::%s: " format, __LINE__, __FUNCTION__, \
                 ##__VA_ARGS__)
#define DEBUG_LOG(format, ...)                                             \
  RAW_PMLOG_DEBUG("DVRpipe", ":%04d::%s: " format, __LINE__, __FUNCTION__, \
                  ##__VA_ARGS__)

namespace media {

#define BIND_TO_RENDER_LOOP(function)                    \
  (DCHECK(media_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, this)))

DvrPipeline::DvrPipeline(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const std::string& app_id)
    : playback_rate_(0),
      media_task_runner_(task_runner),
      luna_service_client_(app_id),
      acb_client_(new Acb()),
      subscribe_key_(0),
      iscontentplaying_key_(0),
      acb_load_task_id_(0),
      is_paused_(false),
      is_suspended_(false) {
  acb_id_ = acb_client_->getAcbId();

  event_handler_ = std::bind(&DvrPipeline::AcbHandler, make_scoped_refptr(this),
                             acb_id_, std::placeholders::_1,
                             std::placeholders::_2, std::placeholders::_3,
                             std::placeholders::_4, std::placeholders::_5);
}

DvrPipeline::~DvrPipeline() {
  INFO_LOG("destroy DVR pipeline");
  luna_service_client_.unsubscribe(subscribe_key_);
  if (iscontentplaying_key_)
    luna_service_client_.unsubscribe(iscontentplaying_key_);
  ClosePipeline();
}

void DvrPipeline::Load(
    bool is_video,
    const std::string& app_id,
    const GURL& url,
    const std::string& mime_type,
    const std::string& payload,
    const CustomPipeline::BufferingStateCB& buffering_state_cb,
    const base::Closure& video_display_window_change_cb,
    const CustomPipeline::PipelineMsgCB& pipeline_message_cb) {
  INFO_LOG("url : %s, host : %s port : %s", url.spec().c_str(),
           url.host().c_str(), url.port().c_str());
  INFO_LOG("payload - %s", payload.empty() ? "{}" : payload.c_str());

  buffering_state_cb_ = buffering_state_cb;
  video_display_window_change_cb_ = video_display_window_change_cb;
  pipeline_message_cb_ = pipeline_message_cb;

  INFO_LOG("ACB::initialize(DVR, %s)", app_id.c_str());
  if (!acb_client_->initialize(ACB::PlayerType::DVR, app_id, event_handler_))
    DEBUG_LOG("ACB::initialize failed");

  if (!video_display_window_change_cb_.is_null())
    video_display_window_change_cb_.Run();

  std::string decodedURL = net::UnescapeURLComponent(
      url.host(),
      net::UnescapeRule::SPACES | net::UnescapeRule::PATH_SEPARATORS |
          net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);

  if (payload == "contentPlay") {
    base::DictionaryValue res;
    res.SetString("contentId", decodedURL);
    std::string para;
    base::JSONWriter::Write(res, &para);

    INFO_LOG("Call Open");
    luna_service_client_.callASync(
        media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::DVR,
                                                "play/openContentPlayInstance"),
        para, BIND_TO_RENDER_LOOP(&DvrPipeline::HandleOpenReply));
  }
}

void DvrPipeline::Dispose() {
  base::AutoLock auto_lock(lock_);
  DEBUG_LOG("dispose DVR pipeline");
  acb_id_ = INVALID_ACB_ID;
  Release();
}

void DvrPipeline::HandleOpenReply(const std::string& payload) {
  INFO_LOG();
  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("returnValue") &&
      response["returnValue"].asBool() && response.isMember("playInstanceId")) {
    play_instance_id_ = response["playInstanceId"].asString();
    INFO_LOG("ACB::setMediaId(%s)", play_instance_id_.c_str());
    if (!acb_client_->setMediaId(play_instance_id_.c_str()))
      DEBUG_LOG("ACB::setMediaId failed");
  } else {
    INFO_LOG("Failed to Open!!! (%s, %s)",
             response["errorCode"].asString().c_str(),
             response["errorText"].asString().c_str());
    response["errorType"] = "open";

    Json::FastWriter writer;
    if (!pipeline_message_cb_.is_null())
      pipeline_message_cb_.Run(writer.write(response));
    return;
  }
  has_audio_ = true;
  has_video_ = true;

  if (is_suspended_)
    PausePipeline();
  else
    ChangeAcbLoaded();
}

void DvrPipeline::ChangeAcbLoaded() {
  INFO_LOG("ACB::setState(FOREGROUND, LOADED)");
  if (acb_client_->setState(ACB::AppState::FOREGROUND, ACB::DVRState::LOADED,
                            &acb_load_task_id_))
    DEBUG_LOG("taskId : %ld", acb_load_task_id_);
}

void DvrPipeline::AcbLoadCompleted() {
  INFO_LOG();
  if (!buffering_state_cb_.is_null()) {
    buffering_state_cb_.Run(CustomPipeline::HaveMetadata);
    buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
  }
}

void DvrPipeline::SetPlaybackRate(float playback_rate) {
  if (play_instance_id_.empty()) {
    if (playback_rate == 0.0f)
      is_suspended_ = true;
    playback_rate_ = playback_rate;
    return;
  }
  if (playback_rate == 0.0f)
    PausePipeline();
  else if (playback_rate_ == 0.0f)
    PlayPipeline();

  playback_rate_ = playback_rate;
}

void DvrPipeline::PlayPipeline() {
  GetPlayingState();
  IsContentPlaying();
}

void DvrPipeline::PausePipeline() {
  INFO_LOG();
  if (!play_instance_id_.empty()) {
    INFO_LOG("ACB::setState(BACKGROUND, SUSPENDED)");
    acb_client_->setState(ACB::AppState::BACKGROUND, ACB::DVRState::SUSPENDED);
    ClosePipeline();
  }
}

void DvrPipeline::ClosePipeline() {
  INFO_LOG();
  if (play_instance_id_.empty()) {
    DEBUG_LOG("play_instance_id_ is not available.");
    return;
  }
  INFO_LOG("ACB::setState(FOREGROUND, UNLOADED)");
  acb_client_->setState(ACB::AppState::FOREGROUND, ACB::DVRState::UNLOADED);
  base::DictionaryValue res;
  res.SetString("playInstanceId", play_instance_id_);
  std::string para;
  base::JSONWriter::Write(res, &para);

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::DVR,
                                              "play/stopPlay"),
      para);

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::DVR,
                                              "play/stopPlayInstance"),
      para);

  luna_service_client_.callASync(
      media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::DVR,
                                              "play/closePlayInstance"),
      para);
  play_instance_id_.clear();
}

void DvrPipeline::IsContentPlaying() {
  INFO_LOG();
  Json::Value root;
  root["subscribe"] = true;

  Json::FastWriter writer;
  luna_service_client_.subscribe(
      media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::DVR,
                                              "play/isContentPlaying"),
      writer.write(root), &iscontentplaying_key_,
      BIND_TO_RENDER_LOOP(&DvrPipeline::HandleIsContentPlayingReply));
}

void DvrPipeline::HandleIsContentPlayingReply(const std::string& payload) {
  INFO_LOG("%s", payload.c_str());
  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) &&
      response.isMember("isContentPlaying")) {
    bool is_content_playing = response["isContentPlaying"].asBool();

    if (is_content_playing) {
      INFO_LOG("ACB::setState(FOREGROUND, PLAYING)");
      acb_client_->setState(ACB::AppState::FOREGROUND, ACB::DVRState::PLAYING);
      luna_service_client_.unsubscribe(iscontentplaying_key_);
      iscontentplaying_key_ = 0;
    }
  }
}

void DvrPipeline::GetPlayingState() {
  INFO_LOG();
  Json::Value root;
  root["playInstanceId"] = play_instance_id_;
  root["subscribe"] = true;

  Json::FastWriter writer;
  luna_service_client_.subscribe(
      media::LunaServiceClient::GetServiceURI(media::LunaServiceClient::DVR,
                                              "play/getPlayingState"),
      writer.write(root), &subscribe_key_,
      BIND_TO_RENDER_LOOP(&DvrPipeline::HandleGetPlayingStateReply));
}

void DvrPipeline::HandleGetPlayingStateReply(const std::string& payload) {
  INFO_LOG("%s", payload.c_str());
  Json::Value response;
  Json::Reader reader;

  if (reader.parse(payload, response) && response.isMember("playingState")) {
    std::string playing_state = response["playingState"].asString();

    // play_state_play : set ACB Loaded
    // play_state_pause : set ACB Paused
    // play_state_fast_forward : set ACB Loaded
    // play_state_fast_rewind : set ACB Loaded
    // play_state_seek : do nothing
    // play_state_slow : set ACB Loaded
    // play_state_partial_repeat : do nothing
    // play_state_stop : do nothing

    if (is_paused_) {
      if (playing_state == "play_state_play" ||
          playing_state == "play_state_slow" ||
          playing_state == "play_state_fast_forward" ||
          playing_state == "play_state_fast_rewind") {
        is_paused_ = false;
        INFO_LOG("ACB::setState(FOREGROUND, PLAYING)");
        acb_client_->setState(ACB::AppState::FOREGROUND,
                              ACB::DVRState::PLAYING);
      }
    } else if (playing_state == "play_state_pause") {
      is_paused_ = true;
      INFO_LOG("ACB::setState(FOREGROUND, PAUSED)");
      acb_client_->setState(ACB::AppState::FOREGROUND, ACB::DVRState::PAUSED);
    }
  }
}

void DvrPipeline::SetDisplayWindow(const blink::WebRect& outRect,
                                   const blink::WebRect& inRect,
                                   bool fullScreen,
                                   bool forced) {
  INFO_LOG("(%d, %d, %d, %d) (%d, %d, %d, %d) fullScreen: %s) forced : %s",
           inRect.x, inRect.y, inRect.width, inRect.height, outRect.x,
           outRect.y, outRect.width, outRect.height,
           fullScreen ? "true" : "false", forced ? "true" : "false");
  if (outRect.isEmpty()) {
    INFO_LOG("outrect is empty. Set NaturalVideoSize width : %d, hight : %d",
             NaturalVideoSize().width(), NaturalVideoSize().height());
    acb_client_->setDisplayWindow(0, 0, NaturalVideoSize().width(),
                                  NaturalVideoSize().height(), true, 0);
    return;
  }

  acb_client_->setDisplayWindow(outRect.x, outRect.y, outRect.width,
                                outRect.height, fullScreen, 0);
}

void DvrPipeline::AcbHandler(long acb_id,
                             long task_id,
                             long event_type,
                             long app_state,
                             long play_state,
                             const char* reply) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DvrPipeline::DispatchAcbHandler, this, acb_id,
                            task_id, event_type, app_state, play_state, reply));
}

void DvrPipeline::DispatchAcbHandler(long acb_id,
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

}  // namespace media
