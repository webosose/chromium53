#ifndef LUNASERVICEMGR_H
#define LUNASERVICEMGR_H

#include "base/macros.h"
#include <lunaservice.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

struct LunaServiceManagerListener {
  LunaServiceManagerListener()
      : listener_token_(LSMESSAGE_TOKEN_INVALID) { }
  virtual ~LunaServiceManagerListener() { }
  virtual void ServiceResponse(const char* body) = 0;

  LSMessageToken GetListenerToken() { return listener_token_; }
  void SetListenerToken(LSMessageToken token) { listener_token_ = token; }
 private:
  LSMessageToken listener_token_;
};

class LunaServiceManager {
 public:
  static std::shared_ptr<LunaServiceManager> GetManager(const std::string&);
  ~LunaServiceManager();

  unsigned long Call(const char* uri,
                     const char* payload,
                     LunaServiceManagerListener*);
  void Cancel(LunaServiceManagerListener*);
  bool Initialized() { return initialized_; }

 private:
  LunaServiceManager(const std::string&);
  void Init();

  LSHandle* sh_;
  std::string identifier_;
  bool initialized_;
  static std::mutex storage_lock_;
  static std::unordered_map<std::string, std::weak_ptr<LunaServiceManager>> storage_;

  DISALLOW_COPY_AND_ASSIGN(LunaServiceManager);
};

#endif
