// Copyright (c) 2017 LG Electronics, Inc.

#include "chrome/browser/webos/luna_services.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/values.h"

const char kBrowserAppId[] = "com.webos.app.browser";
const char kSubscribe[] = "subscribe";
const char kReturnValue[] = "returnValue";

const char kRegisterNativeAppCmd[] =
    "palm://com.webos.applicationManager/registerNativeApp";

bool FindValueByKey(Json::Value item, const char* key, std::string &value) {
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
    if(tempItem.isObject()){
      if(FindValueByKey(tempItem, key, value)){
        return true;
      }
    }
  }

  return false;
}

bool ParseJsonString(
    std::string payload, const char* key, std::string &value) {
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(payload, root))
    return false;

  if (root.get("returnValue", std::string()) != std::string() &&
      !root.get("returnValue", std::string()).asBool()) {
    RAW_PMLOG_DEBUG("%s returnValue FALSE!!! \n", __FUNCTION__);
    return false;
  }

  if (!FindValueByKey(root, key, value)) {
    RAW_PMLOG_DEBUG("%s  don't find Value by Key \n", __FUNCTION__);
    return false;
  }

  return true;
}

LunaServices* LunaServices::g_luna_service_ = NULL;

LunaServices* LunaServices::GetInstance() {
  if (!g_luna_service_) {
    g_luna_service_ = new LunaServices();
    g_luna_service_->Initialize();
  }

  return g_luna_service_;
}

LunaServices::LunaServices()
  : g_main_loop_(NULL),
    ls_handle_(NULL),
    task_runner_(NULL),
    initialized_(false),
    finalizing_(false) {
}

LunaServices::~LunaServices() {
  if (initialized_)
    Finalize();
}

bool LunaServices::Initialize() {
  RAW_PMLOG_DEBUG("LunaServices initilize!!!\n");

  task_runner_ = base::MessageLoopForUI::current()->task_runner().get();

  g_main_loop_ = g_main_loop_new(NULL, false);
  if (!g_main_loop_) {
    g_critical("g_main_loop_new() failed: Unable to create main loop");
    RAW_PMLOG_DEBUG("g_main_loop_new() failed: Unable to create main loop\n");
    return false;
  }

  LSError lserror;
  LSErrorInit(&lserror);

  if (!LSRegister(kBrowserAppId, &ls_handle_, &lserror)) {
    RAW_PMLOG_DEBUG("LSRegister failed -> Terminate Browser!! \n");

    LSErrorPrint(&lserror, stderr);
    LSErrorFree(&lserror);
    return false;
  }

  if (!LSGmainAttach(ls_handle_, g_main_loop_, &lserror)) {
    RAW_PMLOG_DEBUG("LSGmainAttach failed\n");

    LSErrorPrint(&lserror, stderr);
    LSErrorFree(&lserror);

    return false;
  }

  initialized_ = true;

  return true;
}

void LunaServices::Finalize() {
  if (!initialized_)
    return;

  finalizing_ = true;

  if (ls_handle_) {
    LSError lserror;
    LSErrorInit(&lserror);
    LSUnregister(ls_handle_, &lserror);
    ls_handle_ = NULL;
  }

  if (g_main_loop_) {
    g_main_loop_unref(g_main_loop_);
    g_main_loop_ = NULL;
  }

  initialized_ = false;
}

bool LunaServices::LunaServiceCall(const std::string& uri,
                                   const base::Value* parameters,
                                   LSFilterFunc callback,
                                   void *context,
                                   LSMessageToken* token) {
  RAW_PMLOG_DEBUG("%s:%d>>>%s [START] \n", __FILE__, __LINE__, __FUNCTION__);

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

  RAW_PMLOG_DEBUG("%s:%d>>>%s uri[%s] payload[%s] \n",
      __FILE__, __LINE__, __FUNCTION__, uri.c_str(), payload.c_str());
  LSError lserror;
  LSErrorInit(&lserror);
  if (subscribe) {
    if (!LSCallFromApplication(ls_handle_, uri.c_str(), payload.c_str(),
            kBrowserAppId, callback, context, token, &lserror)) {
      RAW_PMLOG_DEBUG("LSCallFromApplication failed\n");
      LSErrorPrint(&lserror, stderr);
      LSErrorFree(&lserror);

      return false;
    }

    return true;
  }

  if (!LSCallFromApplicationOneReply(ls_handle_, uri.c_str(), payload.c_str(),
          kBrowserAppId, callback, context, token, &lserror)) {
    RAW_PMLOG_DEBUG("LSCallFromApplicationOneReply failed\n");
    LSErrorPrint(&lserror, stderr);
    LSErrorFree(&lserror);

    return false;
  }

  return true;
}

void LunaServices::LunaServiceCancel(LSMessageToken* token) {
  RAW_PMLOG_DEBUG("%s:%d>>>%s [START] \n", __FILE__, __LINE__, __FUNCTION__);

  if (!ls_handle_ || !token || *token == LSMESSAGE_TOKEN_INVALID)
    return;

  LSError lserror;
  LSErrorInit(&lserror);
  if (!LSCallCancel(ls_handle_, *token, &lserror)) {
    LSErrorPrint(&lserror, stderr);
    LSErrorFree(&lserror);

    return;
  }

  *token = LSMESSAGE_TOKEN_INVALID;
}

void LunaServices::RegisterNativeApp() {
  CallMethod(LUNA_CB_NATIVE_APP);
}

////////////////////////////////////////////////////////////////////////////
//Private APIs : LunaServices

LSFilterFunc LunaServices::GetSubscriptionInfo(LunaCallbackType type,
                                               std::string* uri,
                                               base::DictionaryValue* dict) {
  LSFilterFunc callback = NULL;
  std::string list_name = "keys";
  const std::unique_ptr<base::ListValue> list(new base::ListValue());

  switch (type) {
    case LUNA_CB_NATIVE_APP:
      *uri = kRegisterNativeAppCmd;
      callback = &LunaServices::RegisterNativeAppCb;
      break;
    default:
      RAW_PMLOG_DEBUG("Wrong callback type!!!\n");
      break;
  }

  dict->SetBoolean(kSubscribe, true);
  if (!list->empty())
    dict->Set(list_name, list->DeepCopy());

  return callback;
}

void LunaServices::CallMethod(LunaCallbackType type) {
  RAW_PMLOG_DEBUG("%s:%d>>>%s [START] LunaCallbackType = %d \n",
      __FILE__, __LINE__, __FUNCTION__, type);

  std::string uri;
  const std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  LSFilterFunc callback = GetSubscriptionInfo(type, &uri, dict.get());

  if (uri.empty() || !callback)
    return;

  LSMessageToken token;
  if (LunaServiceCall(uri, dict.get(), callback, this, &token))
    ls_token_vector_.push_back(token);
}

bool LunaServices::RegisterNativeAppCb(
    LSHandle *sh, LSMessage *reply, void *ctx) {
  std::string payload = const_cast<char*> (LSMessageGetPayload(reply));
  RAW_PMLOG_DEBUG("%s Call : [%s]\n", __FUNCTION__, payload.c_str());

  LunaServices* luna_service = static_cast<LunaServices*>(ctx);
  if (!luna_service)
    return true;

  std::string ret_value = std::string();
  if (!ParseJsonString(payload, kReturnValue, ret_value))
    return false;

  if (ret_value != "true")
    return false;

  // Apollo: There is no relaunch when the browser is background
  // Thus, it has to be handled relaunch case. Just return.
  return true;
}
