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

#ifndef WEBOS_BROWSER_WEBOS_HTTP_USER_AGENT_SETTINGS_H_
#define WEBOS_BROWSER_WEBOS_HTTP_USER_AGENT_SETTINGS_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/url_request/http_user_agent_settings.h"

namespace webos {

class WebOSHttpUserAgentSettings : public net::HttpUserAgentSettings {
 public:
  WebOSHttpUserAgentSettings();
  virtual ~WebOSHttpUserAgentSettings();

  // net::HttpUserAgentSettings implementation:
  std::string GetAcceptLanguage() const override;
  std::string GetUserAgent() const override;

 private:
  mutable std::string accept_language_;

  DISALLOW_COPY_AND_ASSIGN(WebOSHttpUserAgentSettings);
};

}  // namespace webos
#endif  // WEBOS_BROWSER_WEBOS_HTTP_USER_AGENT_SETTINGS_H_
