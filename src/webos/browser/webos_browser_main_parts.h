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

#ifndef WEBOS_BROWSER_WEBOS_BROWSER_MAIN_PARTS_H_
#define WEBOS_BROWSER_WEBOS_BROWSER_MAIN_PARTS_H_

#include <vector>

#include "content/public/browser/browser_main_parts.h"

#include "content/public/browser/browser_thread.h"

#if defined(ENABLE_PLUGINS)
#include "webos/browser/webos_plugin_service_filter.h"
#endif

namespace devtools_http_handler {
class DevToolsHttpHandler;
}  // namespace devtools_http_handler

namespace webos {

class WebOSBrowserContextAdapter;
class WebOSBrowserMainExtraParts;
class WebOSRemoteDebuggingServer;
class WebOSWatchdog;

class WebOSBrowserMainParts : public content::BrowserMainParts {
 public:
  WebOSBrowserMainParts();

  void AddParts(WebOSBrowserMainExtraParts* parts);

  // content::BrowserMainParts overrides.
  void PreEarlyInitialization() override;
  void ToolkitInitialized() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  void PostDestroyThreads() override;
  bool MainMessageLoopRun(int* result_code) override;

  void EnableDevTools();
  void ArmWatchdog(content::BrowserThread::ID thread, WebOSWatchdog* watchdog);

  WebOSBrowserContextAdapter* GetDefaultBrowserContext() const;

 private:
  WebOSBrowserContextAdapter* default_browser_context_;
  devtools_http_handler::DevToolsHttpHandler* devtools_http_handler_;
  WebOSWatchdog* ui_watchdog_;
  WebOSWatchdog* io_watchdog_;

#if defined(ENABLE_PLUGINS)
  std::unique_ptr<WebOSPluginServiceFilter> plugin_service_filter_;
#endif

  std::vector<WebOSBrowserMainExtraParts*> webos_extra_parts_;
};

} // namespace webos

#endif /* WEBOS_BROWSER_WEBOS_BROWSER_MAIN_PARTS_H_ */
