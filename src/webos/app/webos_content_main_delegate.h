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

#ifndef WEBOS_APP_WEBOS_CONTENT_MAIN_DELEGATE_H_
#define WEBOS_APP_WEBOS_CONTENT_MAIN_DELEGATE_H_

#include "content/public/app/content_main_delegate.h"

#include "webos/browser/webos_content_browser_client.h"
#include "webos/common/webos_content_client.h"

namespace webos {

WebOSContentBrowserClient* GetWebOSContentBrowserClient();

class WebOSContentMainDelegate : public content::ContentMainDelegate {
 public:
  WebOSContentMainDelegate();

  // content::ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  void ProcessExiting(const std::string& process_type) override;
  void SetBrowserStartupCallback(base::Closure startup_callback) {
    startup_callback_ = startup_callback;
  }

 protected:
  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;

  WebOSContentClient content_client_;
  base::Closure startup_callback_;
};

}  // namespace webos

#endif  // WEBOS_APP_WEBOS_CONTENT_MAIN_DELEGATE_H_
