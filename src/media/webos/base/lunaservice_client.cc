// Copyright (c) 2014 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/webos/base/lunaservice_client.h"

#include <glib.h>

#include "base/callback_helpers.h"
#include "base/logging.h"

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

struct AutoLSError : LSError {
  AutoLSError() { LSErrorInit(this); }
  ~AutoLSError() { LSErrorFree(this); }
};

// LunaServiceClient implematation
LunaServiceClient::LunaServiceClient(BusType type)
    : handle(NULL), context(NULL) {
  AutoLSError error;
  if (LSRegisterPubPriv(NULL, &handle, type, &error)) {
    context = g_main_context_ref(g_main_context_default());
    LSGmainContextAttach(handle, context, &error);
  }
}

LunaServiceClient::~LunaServiceClient() {
  AutoLSError error;
  LSUnregister(handle, &error);
  g_main_context_unref(context);
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

  DEBUG_LOG("[REQ] - %s %s", uri.c_str(), param.c_str());
  if (!LSCallOneReply(handle, uri.c_str(), param.c_str(), handleAsync, wrapper,
                      NULL, &error)) {
    base::ResetAndReturn(&wrapper->callback).Run("");
    delete wrapper;
    return false;
  }

  return true;
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

  if (!LSCall(handle, uri.c_str(), param.c_str(), handleSubscribe, wrapper,
              subscribeKey, &error)) {
    DEBUG_LOG("[SUB] %s:[%s] fail[%s]", uri.c_str(), param.c_str(),
              error.message);
    delete wrapper;
    return false;
  }

  handlers[*subscribeKey] = std::unique_ptr<ResponseHandlerWrapper>(wrapper);

  return true;
}

bool LunaServiceClient::unsubscribe(LSMessageToken subscribeKey) {
  AutoLSError error;

  if (!LSCallCancel(handle, subscribeKey, &error)) {
    DEBUG_LOG("[UNSUB] %u fail[%s]", subscribeKey, error.message);
    handlers.erase(subscribeKey);
    return false;
  }

  if (handlers[subscribeKey])
    handlers[subscribeKey]->callback.Reset();

  handlers.erase(subscribeKey);

  return true;
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

}  // namespace media
