// Copyright (c) 2018 LG Electronics, Inc.
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

#include "base/webos/luna_services.h"

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/values.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#define INFO_LOG(format, ...)                                                 \
  RAW_PMLOG_INFO("LunaServices", "[%s:%04d] " format, __FUNCTION__, __LINE__, \
                 ##__VA_ARGS__)

namespace base {
namespace webos {

static const char kCategory[] = "category";
static const char kCurrent[] = "current";
static const char kMemMgrAddMatchCmd[] = "palm://com.palm.bus/signal/addmatch";
static const char kMemoryInfoCategory[] = "/com/webos/memorymanager";
static const char kMemoryInfoMethod[] = "thresholdChanged";
static const char kMethod[] = "method";
static const char kRemainCount[] = "remainCount";
static const char kSubscribe[] = "subscribe";

class LunaServiceError {
 public:
  LunaServiceError() { LSErrorInit(&ls_error_); }
  ~LunaServiceError() {
    if (LSErrorIsSet(&ls_error_))
      LSErrorFree(&ls_error_);
  }
  LSError* GetLSError() { return &ls_error_; }
  void PmLog() {
    RAW_PMLOG_INFO("LunaServices", "%s [%s:%04d] %s", ls_error_.func,
                   ls_error_.file, ls_error_.line, ls_error_.message);
  }

 private:
  LSError ls_error_;
};

LunaServicesDelegate::LunaServicesDelegate() {
}
LunaServicesDelegate::~LunaServicesDelegate() {}

LunaServices::LunaServices(LunaServicesDelegate* delegate)
    : context_(nullptr),
      ls_handle_(nullptr),
      task_runner_(nullptr),
      delegate_(delegate),
      initialized_(false),
      finalizing_(false) {}

LunaServices::~LunaServices() {
  if (initialized_)
    Finalize();
}

bool LunaServices::Initialize(const std::string& app_id) {
  app_id_ = app_id;
  LunaServiceError error;

  if (!delegate_) {
    LOG(ERROR) << "Invalid NULL delegate passed to LunaServices";
    return false;
  }

  if (!LSRegister(app_id.c_str(), &ls_handle_, error.GetLSError())) {
    LOG(ERROR) << "LSRegister failed -> Terminate Browser!!";
    error.PmLog();
    delegate_->InitializeError();
    return false;
  }

  task_runner_ = base::MessageLoopForUI::current()->task_runner().get();

  context_ = g_main_context_ref(g_main_context_default());
  if (!LSGmainContextAttach(ls_handle_, g_main_context_default(),
                            error.GetLSError())) {
    LOG(ERROR) << "LSGmainAttach failed";
    error.PmLog();
    LSUnregister(ls_handle_, error.GetLSError());
    ls_handle_ = nullptr;
    delegate_->InitializeError();
    return false;
  }

  SubscribeMethods();
  initialized_ = true;

  return true;
}

void LunaServices::Finalize() {
  if (!initialized_)
    return;

  finalizing_ = true;

  UnsubscribeMethods();

  if (ls_handle_) {
    LunaServiceError error;
    LSUnregister(ls_handle_, error.GetLSError());
    ls_handle_ = nullptr;
  }

  if (context_) {
    g_main_context_unref(context_);
    context_ = nullptr;
  }

  initialized_ = false;
}

bool LunaServices::LunaServiceCall(const std::string& uri,
                                   const base::Value* parameters,
                                   LSFilterFunc callback,
                                   void* context,
                                   LSMessageToken* token) {
  if (!ls_handle_ || !parameters || finalizing_)
    return false;

  bool subscribe = false;
  if (parameters->IsType(base::Value::TYPE_DICTIONARY)) {
    const base::DictionaryValue* dict =
        static_cast<const base::DictionaryValue*>(parameters);
    dict->GetBoolean(kSubscribe, &subscribe);
  }

  std::string payload;
  base::JSONWriter::Write(*parameters, &payload);

  LunaServiceError error;
  if (subscribe) {
    if (!LSCallFromApplication(ls_handle_, uri.c_str(), payload.c_str(),
                               app_id_.c_str(), callback, context, token,
                               error.GetLSError())) {
      LOG(ERROR) << "LSCallFromApplication failed";
      error.PmLog();
      return false;
    }

    return true;
  }

  if (!LSCallFromApplicationOneReply(ls_handle_, uri.c_str(), payload.c_str(),
                                     app_id_.c_str(), callback, context, token,
                                     error.GetLSError())) {
    LOG(ERROR) << "LSCallFromApplicationOneReply failed";
    error.PmLog();
    return false;
  }

  return true;
}

void LunaServices::LunaServiceCancel(LSMessageToken* token) {
  if (!ls_handle_ || !token || *token == LSMESSAGE_TOKEN_INVALID)
    return;

  LunaServiceError error;
  if (!LSCallCancel(ls_handle_, *token, error.GetLSError())) {
    error.PmLog();
    return;
  }

  *token = LSMESSAGE_TOKEN_INVALID;
}

LSFilterFunc LunaServices::GetSubscriptionInfo(LunaCallbackType type,
                                               std::string& uri,
                                               base::DictionaryValue* dict) {
  LSFilterFunc callback = nullptr;

  std::string list_name = "keys";
  const std::unique_ptr<base::ListValue> list(new base::ListValue());
  switch (type) {
    case LUNA_CB_MEMORY_INFO:
      uri = kMemMgrAddMatchCmd;
      dict->SetString(kCategory, kMemoryInfoCategory);
      dict->SetString(kMethod, kMemoryInfoMethod);
      callback = &LunaServices::GetMemoryInfoCb;
      break;
    default:
      LOG(ERROR) << "Wrong callback type!!!";
      break;
  }

  dict->SetBoolean(kSubscribe, true);
  if (!list->empty())
    dict->Set(list_name, list->DeepCopy());

  return callback;
}

void LunaServices::CallMethod(LunaCallbackType type) {
  std::string uri;
  const std::unique_ptr<base::DictionaryValue> dict(
      new base::DictionaryValue());
  LSFilterFunc callback = GetSubscriptionInfo(type, uri, dict.get());

  if (uri.empty() || !callback) {
    LOG(ERROR) << "Could not get uri or callback";
    return;
  }

  LSMessageToken token;
  if (!LunaServiceCall(uri, dict.get(), callback, this, &token)) {
    LOG(ERROR) << "Failed to call " << uri;
    return;
  }
  ls_token_vector_.push_back(token);
}

void LunaServices::SubscribeMethods() {
  for (int i = LUNA_CB_MEMORY_INFO; i != LUNA_CB_MAX; ++i) {
    CallMethod(static_cast<LunaCallbackType>(i));
  }
}

void LunaServices::UnsubscribeMethods() {
  LSMessageToken token;
  for (std::vector<LSMessageToken>::iterator it = ls_token_vector_.begin();
       it != ls_token_vector_.end(); ++it) {
    token = *it;
    LunaServiceCancel(&token);
  }

  if (ls_token_vector_.size() > 0)
    ls_token_vector_.clear();
}

bool LunaServices::GetMemoryInfoCb(LSHandle* sh, LSMessage* reply, void* ctx) {
  std::string payload = const_cast<char*>(LSMessageGetPayload(reply));

  std::string current, remain_count;
  if (!ParseJsonString(payload, kCurrent, current)) {
    LOG(ERROR) << "Failed to get memory info current level.";
    return false;
  }

  if (!ParseJsonString(payload, kRemainCount, remain_count)) {
    LOG(ERROR) << "Failed to get memory info remain count.";
    return false;
  }

  LunaServices* luna_service = static_cast<LunaServices*>(ctx);
  if (!luna_service)
    return false;

  luna_service->task_runner_->PostTask(
      FROM_HERE, base::Bind(&LunaServicesDelegate::HandleMemoryInfo,
                            base::Unretained(luna_service->delegate_), current,
                            remain_count));

  return true;
}

bool LunaServices::FindValueByKey(Json::Value item,
                                  const char* key,
                                  std::string& value) {
  std::string temp_str;
  if (!item.get(key, std::string()).isArray() &&
      !item.get(key, std::string()).isObject()) {
    temp_str = item.get(key, std::string()).asString();
    if (temp_str != std::string()) {
      value = temp_str;
      return true;
    }
  }

  for (Json::Value::iterator iter = item.begin(); iter != item.end(); iter++) {
    Json::Value tempItem = *iter;
    if (tempItem.isObject() && FindValueByKey(tempItem, key, value))
      return true;
  }

  return false;
}

bool LunaServices::ParseJsonString(std::string payload,
                                   const char* key,
                                   std::string& value) {
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(payload, root))
    return false;

  if (root.get("returnValue", std::string()) != std::string() &&
      !root.get("returnValue", std::string()).asBool()) {
    return false;
  }

  return FindValueByKey(root, key, value);
}

}  // namespace webos
}  // namespace base
