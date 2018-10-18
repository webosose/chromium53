// Copyright (c) 2014 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/webos/base/lunaservice_client.h"

#include <glib.h>

#include "base/callback_helpers.h"
#include "base/logging.h"

#if defined(OS_WEBOS)
#include "base/base_switches.h"
#endif

#define DEBUG_LOG(format, ...) \
  RAW_PMLOG_DEBUG("LunaServiceClient " format, ##__VA_ARGS__)

namespace media {

static const char* const luna_service_uris[] = {
    "luna://com.webos.service.videosinkmanager",   // VSM
    "luna://com.webos.service.tv.display",         // DISPLAY
    "luna://com.webos.service.tv.avblock",         // AVBLOCK
    "luna://com.webos.audio",                      // AUDIO
    "luna://com.webos.service.utp.broadcast",      // BROADCAST
    "luna://com.webos.service.utp.channel",        // CHANNEL
    "luna://com.webos.service.tv.externaldevice",  // EXTERNALDEVICE
    "luna://com.webos.service.tv.dvr",             // DVR
    "luna://com.webos.service.photorenderer",      // PHOTORENDERER
};

// LunaServiceClient implematation
LunaServiceClient::LunaServiceClient(const std::string& identifier)
    : handle_(NULL), context_(NULL) {
  if (!RegisterService(identifier)) {
    DEBUG_LOG("Failed to register service (%s)", identifier.c_str());
  }
}

LunaServiceClient::~LunaServiceClient() {
  UnregisterService();
}

bool handleAsync(LSHandle* sh, LSMessage* reply, void* ctx) {
  LunaServiceClient::ResponseHandlerWrapper* wrapper =
      static_cast<LunaServiceClient::ResponseHandlerWrapper*>(ctx);

  LSMessageRef(reply);
  std::string dump = LSMessageGetPayload(reply);
  DEBUG_LOG("[RES] - %s %s", wrapper->uri.c_str(), dump.c_str());
  if (!wrapper->callback.is_null())
    base::ResetAndReturn(&wrapper->callback).Run(dump);

  LSMessageUnref(reply);

  delete wrapper;

  return true;
}

bool handleSubscribe(LSHandle* sh, LSMessage* reply, void* ctx) {
  LunaServiceClient::ResponseHandlerWrapper* wrapper =
      static_cast<LunaServiceClient::ResponseHandlerWrapper*>(ctx);

  LSMessageRef(reply);
  std::string dump = LSMessageGetPayload(reply);
  DEBUG_LOG("[SUB-RES] - %s %s", wrapper->uri.c_str(), dump.c_str());
  if (!wrapper->callback.is_null())
    wrapper->callback.Run(dump);

  LSMessageUnref(reply);

  return true;
}

bool LunaServiceClient::callASync(const std::string& uri,
                                  const std::string& param) {
  ResponseCB nullcb;

  return callASync(uri, param, nullcb);
}

bool LunaServiceClient::callASync(const std::string& uri,
                                  const std::string& param,
                                  const ResponseCB& callback) {
  AutoLSError error;
  ResponseHandlerWrapper* wrapper = new ResponseHandlerWrapper;
  wrapper->callback = callback;
  wrapper->uri = uri;
  wrapper->param = param;

  if (!wrapper)
    return false;

  if (!handle_)
    return false;

  DEBUG_LOG("[REQ] - %s %s", uri.c_str(), param.c_str());
  bool retval = LSCallOneReply(handle_, uri.c_str(), param.c_str(), handleAsync,
                               wrapper, NULL, &error);
  if (!retval) {
    base::ResetAndReturn(&wrapper->callback).Run("");
    delete wrapper;
    return retval;
  }

  return retval;
}

bool LunaServiceClient::subscribe(const std::string& uri,
                                  const std::string& param,
                                  LSMessageToken* subscribeKey,
                                  const ResponseCB& callback) {
  AutoLSError error;
  ResponseHandlerWrapper* wrapper = new ResponseHandlerWrapper;
  wrapper->callback = callback;
  wrapper->uri = uri;
  wrapper->param = param;

  if (!wrapper)
    return false;

  if (!handle_)
    return false;

  bool retval = LSCall(handle_, uri.c_str(), param.c_str(), handleSubscribe,
                       wrapper, subscribeKey, &error);
  if (!retval) {
    DEBUG_LOG("[SUB] %s:[%s] fail[%s]", uri.c_str(), param.c_str(),
              error.message);
    delete wrapper;
    return retval;
  }

  handlers_[*subscribeKey] = std::unique_ptr<ResponseHandlerWrapper>(wrapper);

  return retval;
}

bool LunaServiceClient::unsubscribe(LSMessageToken subscribeKey) {
  AutoLSError error;

  if (!handle_)
    return false;

  bool retval = LSCallCancel(handle_, subscribeKey, &error);
  if (!retval) {
    DEBUG_LOG("[UNSUB] %u fail[%s]", subscribeKey, error.message);
    handlers_.erase(subscribeKey);
    return retval;
  }

  if (handlers_[subscribeKey])
    handlers_[subscribeKey]->callback.Reset();

  handlers_.erase(subscribeKey);

  return retval;
}

std::string LunaServiceClient::GetServiceURI(URIType type,
                                             const std::string& action) {
  if (type < 0 || type > LunaServiceClient::URITypeMax)
    return std::string();

  std::string uri = luna_service_uris[type];
  uri.append("/");
  uri.append(action);
  return uri;
}

bool LunaServiceClient::RegisterService(const std::string& identifier) {
  bool retval;
#if defined(OS_WEBOS)
  AutoLSError error;
  std::string service_name = identifier + '-' + std::to_string(getpid());

  retval = LSRegisterApplicationService(service_name.c_str(),
                                        identifier.c_str(), &handle_, &error);
  if (!retval) {
    LogError("Fail to register a web application to LS2", error);
    return retval;
  }

  context_ = g_main_context_ref(g_main_context_default());
  retval = LSGmainContextAttach(handle_, context_, &error);
  if (!retval) {
    UnregisterService();
    LogError("Fail to attach a service to a mainloop", error);
    return retval;
  }
#endif  // OS_WEBOS

  return retval;
}

bool LunaServiceClient::UnregisterService() {
  bool retval;
  AutoLSError error;

  if (!handle_)
    return false;

  retval = LSUnregister(handle_, &error);
  if (!retval) {
    LogError("Fail to unregister service", error);
    return retval;
  }
  g_main_context_unref(context_);
  return retval;
}

void LunaServiceClient::LogError(const std::string& message,
                                 AutoLSError& lserror) {
  LOG(ERROR) << message.c_str() << " " << lserror.error_code << " : "
             << lserror.message << "(" << lserror.func << " @ " << lserror.file
             << ":" << lserror.line << ")";
}

}  // namespace media
