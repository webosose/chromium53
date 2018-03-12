// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef CHROME_BROWSER_WEBOS_LUNA_SERVICES_H_
#define CHROME_BROWSER_WEBOS_LUNA_SERVICES_H_

#include <memory>
#include <string>
#include <glib.h>

#include <lunaservice.h>
#include "base/macros.h"
#include "third_party/jsoncpp/source/include/json/json.h"

namespace base {
class DictionaryValue;
class SingleThreadTaskRunner;
class Value;
}

class LunaServices {
 public:
  LunaServices();
  ~LunaServices();

  static LunaServices* GetInstance();

  bool LunaServiceCall(const std::string& uri,
                       const base::Value* parameters,
                       LSFilterFunc callback,
                       void* context,
                       LSMessageToken* token);
  void LunaServiceCancel(LSMessageToken* token);
  bool IsIntialized() { return initialized_; }

  void RegisterNativeApp();

 private:
  enum LunaCallbackType {
    LUNA_CB_NONE = -1,
    LUNA_CB_NATIVE_APP,
    LUNA_CB_MAX,
  };

  bool Initialize();
  void Finalize();
  LSFilterFunc GetSubscriptionInfo(LunaCallbackType type,
                                   std::string* uri,
                                   base::DictionaryValue* dict);
  void CallMethod(LunaCallbackType type);

  // luna-send callback
  static bool RegisterNativeAppCb(LSHandle *sh, LSMessage *reply, void *ctx);

  static LunaServices* g_luna_service_;
  GMainLoop* g_main_loop_;
  LSHandle* ls_handle_;
  base::SingleThreadTaskRunner* task_runner_;
  std::vector<LSMessageToken> ls_token_vector_;
  bool initialized_;
  bool finalizing_;

  DISALLOW_COPY_AND_ASSIGN(LunaServices);
};

#endif  // CHROME_BROWSER_WEBOS_LUNA_SERVICES_H_
