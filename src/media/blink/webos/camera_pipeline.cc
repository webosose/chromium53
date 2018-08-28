// Copyright (c) 2015-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webos/camera_pipeline.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "third_party/WebKit/public/platform/WebRect.h"

#define INFO_LOG(format, ...) \
  RAW_PMLOG_INFO("CameraPipe", ":%04d " format, __LINE__, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("CameraPipe:%04d " format, __LINE__, ##__VA_ARGS__)

namespace media {

#define BIND_TO_RENDER_LOOP(function)                    \
  (DCHECK(media_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, this)))

CameraPipeline::CameraPipeline(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const std::string& app_id)
    : loaded_(false),
      playback_rate_(0),
      media_task_runner_(task_runner),
      luna_service_client_(app_id) {}

CameraPipeline::~CameraPipeline() {
  DEBUG_LOG("destroy Camera pipeline");
  if (!media_id_.empty()) {
    uMediaServer::uMediaClient::unload();
  }
}

void CameraPipeline::Dispose() {
  DEBUG_LOG("dispose Camera pipeline");
  Release();
}

void CameraPipeline::Load(
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

  Json::Value root;
  root["streamType"] = "MPEGTS";
  root["width"] = 1920;
  root["height"] = 1080;
  root["pipelineMode"] = "webkit";
  root["option"]["appId"] = app_id_;

  if (!payload.empty()) {
    Json::Value response;
    Json::Reader reader;

    reader.parse(payload, response);
    if (response.isMember("streamType"))
      root["streamType"] = response["streamType"].asString();
    if (response.isMember("format"))
      root["format"] = response["format"].asString();
    if (response.isMember("width"))
      root["width"] = response["width"].asString();
    if (response.isMember("height"))
      root["height"] = response["height"].asString();
    if (response.isMember("frameRate"))
      root["frameRate"] = response["frameRate"].asString();
    if (response.isMember("pipelineMode"))
      root["pipelineMode"] = response["pipelineMode"].asString();
    if (response.isMember("recordMode"))
      root["recordMode"] = response["recordMode"].asString();
  }

  Json::FastWriter writer;
  DEBUG_LOG("call uMediaClient::loadAsync(%s)(%s)", url_.spec().c_str(),
            writer.write(root).c_str());
  uMediaServer::uMediaClient::loadAsync(url_.spec().c_str(), kCamera,
                                        writer.write(root).c_str());
}

bool CameraPipeline::onLoadCompleted() {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CameraPipeline::DispatchLoadCompleted, this));
  uMediaServer::uMediaClient::play();
  return true;
}

void CameraPipeline::DispatchLoadCompleted() {
  media_id_ = uMediaServer::uMediaClient::media_id;
  DEBUG_LOG("loadCompleted [%s]", media_id_.c_str());

  mdc_content_provider_.reset(new MDCContentProvider(media_id_));
  loaded_ = true;

  if (!buffering_state_cb_.is_null()) {
    buffering_state_cb_.Run(CustomPipeline::HaveMetadata);
    buffering_state_cb_.Run(CustomPipeline::HaveEnoughData);
  }
}

bool CameraPipeline::onUnloadCompleted() {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CameraPipeline::DispatchUnloadCompleted, this));
  return true;
}

void CameraPipeline::DispatchUnloadCompleted() {
  DEBUG_LOG("UnloadCompleted [%s]", media_id_.c_str());
}

void CameraPipeline::SetPlaybackRate(float playback_rate) {
  if (media_id_.empty()) {
    playback_rate_ = playback_rate;
    return;
  }

  if (playback_rate == 0.0f)
    PausePipeline();
  else if (playback_rate_ == 0.0f)
    PlayPipeline();

  playback_rate_ = playback_rate;
}

void CameraPipeline::PlayPipeline() {}

bool CameraPipeline::onPlaying() {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CameraPipeline::DispatchPlaying, this));
  return true;
}

void CameraPipeline::DispatchPlaying() {
  DEBUG_LOG("onPlaying");
  if (!video_display_window_change_cb_.is_null())
    video_display_window_change_cb_.Run();

  Json::Value root;
  root["cameraState"] = "playing";

  Json::FastWriter writer;
  pipeline_message_cb_.Run(writer.write(root));
}

void CameraPipeline::PausePipeline() {
  DEBUG_LOG("%s", __FUNCTION__);
}

bool CameraPipeline::onPaused() {
  DEBUG_LOG("%s", __FUNCTION__);
  return true;
}

bool CameraPipeline::onError(long long errorCode,
                             const std::string& errorText) {
  DEBUG_LOG("onError : (%lld) %s", errorCode, errorText.c_str());

  Json::Value root;
  Json::Reader reader;

  reader.parse(errorText, root);

  if (errorCode == ImageDecodeError || errorCode == ImageDisplayError) {
    if (root.isMember("id"))
      camera_id_ = root["id"].asString();
    root["infoType"] = "acquire";
  } else {
    root["infoType"] = "error";
    root["errorCode"] = (double)errorCode;
    root["errorText"] = errorText;
    root["mediaId"] = media_id_;
  }

  Json::FastWriter writer;
  pipeline_message_cb_.Run(writer.write(root));
  return true;
}

bool CameraPipeline::onSourceInfo(
    const uMediaServer::source_info_t& sourceInfo) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["infoType"] = "sourceInfo";
  root["container"] = sourceInfo.container;
  root["numPrograms"] = sourceInfo.numPrograms;

  return true;
}

bool CameraPipeline::onAudioInfo(const uMediaServer::audio_info_t& audioInfo) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["infoType"] = "audioInfo";
  root["sampleRate"] = audioInfo.sampleRate;
  root["channels"] = audioInfo.channels;

  Json::FastWriter writer;
  if (!pipeline_message_cb_.is_null())
    pipeline_message_cb_.Run(writer.write(root));
  return true;
}

bool CameraPipeline::onVideoInfo(const uMediaServer::video_info_t& videoInfo) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CameraPipeline::DispatchVideoInfo, this, videoInfo));
  return true;
}

void CameraPipeline::DispatchVideoInfo(
    const struct uMediaServer::video_info_t& videoInfo) {
  DEBUG_LOG("onVideoInfo");

  has_video_ = true;
  Json::Value root;
  root["infoType"] = "videoInfo";
  root["width"] = videoInfo.width;
  root["height"] = videoInfo.height;
  root["aspectRatio"] = videoInfo.aspectRatio;
  root["frameRate"] = videoInfo.frameRate;
  root["mode3D"] = videoInfo.mode3D;

  Json::FastWriter writer;
  if (!pipeline_message_cb_.is_null())
    pipeline_message_cb_.Run(writer.write(root));
  SetMediaVideoData(videoInfo);
}

bool CameraPipeline::onEndOfStream() {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["infoType"] = "endOfStream";
  root["mediaId"] = media_id_;

  Json::FastWriter writer;
  if (!pipeline_message_cb_.is_null())
    pipeline_message_cb_.Run(writer.write(root));
  return true;
}

bool CameraPipeline::onFileGenerated() {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["infoType"] = "fileGenerated";
  root["mediaId"] = media_id_;

  Json::FastWriter writer;
  if (!pipeline_message_cb_.is_null())
    pipeline_message_cb_.Run(writer.write(root));
  return true;
}

bool CameraPipeline::onRecordInfo(
    const uMediaServer::record_info_t& recordInfo) {
  DEBUG_LOG("%s", __FUNCTION__);

  Json::Value root;
  root["infoType"] = "recordInfo";
  root["recordState"] = recordInfo.recordState;
  root["elapsedMiliSecond"] = recordInfo.elapsedMiliSecond;
  root["bitRate"] = recordInfo.bitRate;
  root["fileSize"] = recordInfo.fileSize;
  root["fps"] = recordInfo.fps;

  Json::FastWriter writer;
  if (!pipeline_message_cb_.is_null())
    pipeline_message_cb_.Run(writer.write(root));
  return true;
}

bool CameraPipeline::onUserDefinedChanged(const char* message) {
  DEBUG_LOG("%s : %s", __FUNCTION__, message);

  Json::Value root;
  Json::Reader reader;
  std::string para;
  Json::FastWriter writer;

  reader.parse(message, root);

  root["infoType"] = "acquire";
  if (root.isMember("cameraId")) {
    camera_id_ = root["cameraId"].asString();
    DEBUG_LOG("cameraId received - %s", camera_id_.c_str());
    para = writer.write(root);
  } else if (root.isMember("cameraServiceEvent")) {
    root["cameraServiceEvent"]["infoType"] = "acquire";
    para = writer.write(root["cameraServiceEvent"]);
  } else
    para = writer.write(root);

  DEBUG_LOG("para - %s", para.c_str());
  if (!pipeline_message_cb_.is_null())
    pipeline_message_cb_.Run(para);
  return true;
}

struct CameraPipeline::Media3DInfo CameraPipeline::getMedia3DInfo(
    const std::string& media_3dinfo) {
  struct Media3DInfo res;
  res.type = "LR";

  if (media_3dinfo.find("RL") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "RL";
  } else if (media_3dinfo.find("LR") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "LR";
  } else if (media_3dinfo == "bottom_top") {
    res.pattern = "top_bottom";
    res.type = "RL";
  } else {
    res.pattern = media_3dinfo;
  }

  return res;
}

gfx::Size CameraPipeline::getResoultionFromPAR(const std::string& par) {
  gfx::Size res(1, 1);

  size_t pos = par.find(":");
  if (pos == std::string::npos)
    return res;

  std::string w;
  std::string h;
  w.assign(par, 0, pos);
  h.assign(par, pos + 1, par.size() - pos - 1);
  return gfx::Size(std::stoi(w), std::stoi(h));
}

void CameraPipeline::SetMediaVideoData(
    const struct uMediaServer::video_info_t& video_info) {
  uMediaServer::mdc::video_info_t video_info_;

  video_info_.content = "camera";
  video_info_.frame_rate = video_info.frameRate;
  video_info_.scan_type = "progressive";
  video_info_.width = video_info.width;
  video_info_.height = video_info.height;
  video_info_.adaptive = true;
  video_info_.pixel_aspect_ratio.width = video_info.pixelAspectRatio[0];
  video_info_.pixel_aspect_ratio.height = video_info.pixelAspectRatio[1];

  struct Media3DInfo media_3dinfo;
  media_3dinfo = getMedia3DInfo(video_info.mode3D);
  video_info_.video3d.current = media_3dinfo.pattern;
  video_info_.video3d.type_lr = media_3dinfo.type;
  media_3dinfo = getMedia3DInfo(video_info.actual3D);
  video_info_.video3d.original = media_3dinfo.pattern;

  mdc_content_provider_->setVideoInfo(video_info_);
  mdc_content_provider_->mediaContentReady(true);
}

void CameraPipeline::SetDisplayWindow(const blink::WebRect& outRect,
                                      const blink::WebRect& inRect,
                                      bool fullScreen,
                                      bool forced) {
  using namespace uMediaServer;
  DEBUG_LOG("(%d, %d, %d, %d) (%d, %d, %d, %d) fullScreen: %s) forced : %s",
            inRect.x, inRect.y, inRect.width, inRect.height, outRect.x,
            outRect.y, outRect.width, outRect.height,
            fullScreen ? "true" : "false", forced ? "true" : "false");
  if (outRect.isEmpty())
    return;
  uMediaClient::setDisplayWindow(
      rect_t(outRect.x, outRect.y, outRect.width, outRect.height));
}

}  // namespace media
