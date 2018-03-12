// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef WEBOS_PUBLIC_RUNTIME_DELEGATES_H_
#define WEBOS_PUBLIC_RUNTIME_DELEGATES_H_

#include <string>

#include "webos/common/webos_constants.h"
#include "webos/common/webos_export.h"

class LSHandle;

namespace webos {

class WEBOS_EXPORT LunaServiceDelegate {
 public:
  virtual ~LunaServiceDelegate() {}

  virtual LSHandle* GetLSHandle() = 0;
};

class WEBOS_EXPORT CookieStoreUtilDelegate {
 public:
  virtual ~CookieStoreUtilDelegate() {}

  virtual void FlushStoreCookie(PowerOffState power_off_state,
                                std::string timestamp) = 0;
};

class WEBOS_EXPORT PlatformDelegate {
 public:
  virtual ~PlatformDelegate() {}

  virtual void OnCursorVisibilityChanged(bool visible) = 0;
  virtual void OnNetworkStateChanged(bool is_connected) = 0;
  virtual void OnLocaleInfoChanged(std::string language) = 0;
};

}  // namespace webos

#endif  // WEBOS_PUBLIC_RUNTIME_DELEGATES_H_
