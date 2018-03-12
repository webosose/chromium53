// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#ifndef MEDIA_WEBOS_BASE_LUNASERVICE_CLIENT_H_
#define MEDIA_WEBOS_BASE_LUNASERVICE_CLIENT_H_

#include <lunaservice.h>
#include <map>
#include <string>

#include "base/callback.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT LunaServiceClient {
 public:
  enum URIType {
    VSM = 0,
    DISPLAY,
    AVBLOCK,
    AUDIO,
    BROADCAST,
    CHANNEL,
    EXTERNALDEVICE,
    DVR,
    PHOTORENDERER,
    URITypeMax = PHOTORENDERER,
  };
  enum BusType {
    PrivateBus = 0,
    PublicBus,
  };
  typedef base::Callback<void(const std::string&)> ResponseCB;

  struct ResponseHandlerWrapper {
    LunaServiceClient::ResponseCB callback;
    std::string uri;
    std::string param;
  };

  explicit LunaServiceClient(BusType type);
  ~LunaServiceClient();
  bool callASync(const std::string& uri, const std::string& param);
  bool callASync(const std::string& uri,
                 const std::string& param,
                 const ResponseCB& callback);
  bool subscribe(const std::string& uri,
                 const std::string& param,
                 LSMessageToken* subscribeKey,
                 const ResponseCB& callback);
  bool unsubscribe(LSMessageToken subscribeKey);

  static std::string GetServiceURI(URIType type, const std::string& action);

 private:
  LSHandle* handle;
  GMainContext* context;
  std::map<LSMessageToken, std::unique_ptr<ResponseHandlerWrapper>> handlers;

  DISALLOW_COPY_AND_ASSIGN(LunaServiceClient);
};

}  // namespace media

#endif  // MEDIA_WEBOS_BASE_LUNASERVICE_CLIENT_H_
