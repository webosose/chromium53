#include "luna_service_mgr.h"

#ifdef USE_BASE_LOGGING
#include "base/logging.h"
#endif
#include "v8/include/v8.h"
#include <glib.h>
#include <lunaservice.h>
#include <stdlib.h>
#include <unistd.h>

// static
std::mutex LunaServiceManager::storage_lock_;
std::unordered_map<std::string, std::weak_ptr<LunaServiceManager>> LunaServiceManager::storage_;

// static
static bool message_filter(LSHandle* sh, LSMessage* reply, void* ctx) {
  const char* payload = LSMessageGetPayload(reply);

  LunaServiceManagerListener* listener =
    static_cast<LunaServiceManagerListener*>(ctx);

  if (listener) {
    listener->ServiceResponse(payload);
    return true;
  }

  return false;
}

class LSErrorSafe: public LSError {
 public:
  LSErrorSafe() {
    LSErrorInit(this);
  };
  ~LSErrorSafe() {
    LSErrorFree(this);
  }
};

LunaServiceManager::LunaServiceManager(const std::string& id)
    : sh_(NULL),
      initialized_(false),
      identifier_(id) {
  Init();
}

LunaServiceManager::~LunaServiceManager() {
  if (sh_) {
    bool retVal;
    LSErrorSafe lserror;
    retVal = LSUnregister(sh_, &lserror);
    if (!retVal) {
#ifdef USE_BASE_LOGGING
      LOG(ERROR) << "LSUnregisterPalmService ERROR "
        << lserror.error_code << ": " << lserror.message << " ("
        << lserror.func << " @ " << lserror.file << ":" << lserror.line << ")";
#endif
    }
  }
}

std::shared_ptr<LunaServiceManager> LunaServiceManager::GetManager(const std::string& identifier) {
  std::lock_guard<std::mutex> lock(storage_lock_);
  std::shared_ptr<LunaServiceManager> manager;
  auto iter = storage_.find(identifier);
  if (iter != storage_.end())
    manager = iter->second.lock();
  if (!manager) {
    manager.reset(new LunaServiceManager(identifier));
    if (manager->Initialized())
      storage_[identifier] = manager;
    else
      manager.reset();
  }
  return manager;
}

void log_error(LSErrorSafe& lserror) {
#ifdef USE_BASE_LOGGING
  LOG(ERROR) << "Cannot initialize LunaServiceManager ERROR "
    << lserror.error_code << ": " << lserror.message << " ("
    << lserror.func << " @ " << lserror.file << ":" << lserror.line << ")";
#endif
}

void LunaServiceManager::Init() {
  bool init;
  LSErrorSafe lserror;

  if (initialized_)
    return;

  bool is_phone = identifier_.find("com.palm.app.phone") == 0;

  std::string identifier = identifier_ + '-' + std::to_string(getpid());

  init = LSRegisterApplicationService(identifier.c_str(), identifier_.c_str(), &sh_, &lserror);
  if (!init) {
    log_error(lserror);
    return;
  }

  init = LSGmainContextAttach(sh_, g_main_context_default(), &lserror);
  if (!init) {
    log_error(lserror);
    return;
  }

  init = LSGmainSetPriority(sh_, is_phone ? G_PRIORITY_HIGH : G_PRIORITY_DEFAULT, &lserror);
  if (!init) {
    log_error(lserror);
    return;
  }

  initialized_ = true;
  return;
}

unsigned long LunaServiceManager::Call(const char* uri,
                                       const char* payload,
                                       LunaServiceManagerListener* inListener) {
  bool retVal;
  LSErrorSafe lserror;
  LSMessageToken token = 0;

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Value> val = v8::JSON::Parse(v8::String::NewFromUtf8(isolate, payload));
  bool subscription = false;
  if (!val.IsEmpty() && val->IsObject()) {
    v8::Local<v8::Object> obj = val->ToObject();
    v8::Local<v8::Value> subscribe = obj->Get(v8::String::NewFromUtf8(isolate, "subscribe"));
    v8::Local<v8::Value> watch = obj->Get(v8::String::NewFromUtf8(isolate, "watch"));

    if (!subscribe.IsEmpty() && subscribe->IsBoolean())
      subscription = subscription || subscribe->ToBoolean()->Value();

    if (!watch.IsEmpty() && watch->IsBoolean())
      subscription = subscription || watch->ToBoolean()->Value();
  }

  if (subscription)
    retVal = LSCall(sh_, uri, payload, message_filter, inListener, &token, &lserror);
  else
    retVal = LSCallOneReply(sh_, uri, payload, message_filter, inListener, &token, &lserror);

  if (retVal)
    inListener->SetListenerToken(token);
  else
    token = 0;

  return token;
}

void LunaServiceManager::Cancel(LunaServiceManagerListener* inListener) {
  bool retVal;
  LSErrorSafe lserror;

  if (!inListener || !inListener->GetListenerToken())
    return;

  if (!LSCallCancel(sh_, inListener->GetListenerToken(), &lserror)) {
#ifdef USE_BASE_LOGGING
    LOG(ERROR) << "LSCallCancel ERROR "
      << lserror.error_code << ": " << lserror.message << " ("
      << lserror.func << " @ " << lserror.file << ":" << lserror.line << ")";
#endif
  }

  inListener->SetListenerToken(0);
}
