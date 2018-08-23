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

#ifndef BASE_WEBOS_LUNA_SERVICES_H_
#define BASE_WEBOS_LUNA_SERVICES_H_

#include <string>
#include <vector>

#include <glib.h>
#include <lunaservice.h>

#include "base/macros.h"

namespace Json {
class Value;
}

namespace base {

class DictionaryValue;
class SingleThreadTaskRunner;
class Value;

namespace webos {

class LunaServicesDelegate {
 public:
  LunaServicesDelegate();
  virtual ~LunaServicesDelegate();
  virtual void InitializeError() {}
  virtual void HandleMemoryInfo(const std::string& memory_info,
                                const std::string& remain_count) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(LunaServicesDelegate);
};

class LunaServices {
 public:
  explicit LunaServices(LunaServicesDelegate*);
  virtual ~LunaServices();

  bool Initialize(const std::string& app_id);
  void Finalize();
  bool LunaServiceCall(const std::string& uri,
                       const base::Value* parameters,
                       LSFilterFunc callback,
                       void* context,
                       LSMessageToken* token);
  void LunaServiceCancel(LSMessageToken* token);

  LSHandle* GetHandle() { return ls_handle_; }
  bool IsInitialized() { return initialized_; }

 private:
  enum LunaCallbackType {
    LUNA_CB_NONE = -1,
    LUNA_CB_MEMORY_INFO,
    LUNA_CB_MAX,
  };

  LSFilterFunc GetSubscriptionInfo(LunaCallbackType type,
                                   std::string& uri,
                                   base::DictionaryValue* dict);
  void CallMethod(LunaCallbackType type);
  void SubscribeMethods();
  void UnsubscribeMethods();
  static bool FindValueByKey(Json::Value item,
                             const char* key,
                             std::string& value);
  static bool ParseJsonString(std::string payload,
                              const char* key,
                              std::string& value);

  // luna-send callback
  static bool GetMemoryInfoCb(LSHandle* sh, LSMessage* reply, void* ctx);

  GMainContext* context_;
  LSHandle* ls_handle_;
  std::string app_id_;

  base::SingleThreadTaskRunner* task_runner_;
  LunaServicesDelegate* delegate_;

  bool initialized_;
  bool finalizing_;

  std::vector<LSMessageToken> ls_token_vector_;

  DISALLOW_COPY_AND_ASSIGN(LunaServices);
};

}  // namespace webos
}  // namespace base

#endif  // BASE_WEBOS_LUNA_SERVICES_H_
