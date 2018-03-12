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

#ifndef WEBOS_WEBOS_DEV_TOOLS_DELEGATE_H_
#define WEBOS_WEBOS_DEV_TOOLS_DELEGATE_H_

#include "components/devtools_http_handler/devtools_http_handler_delegate.h"
#include "components/devtools_discovery/devtools_discovery_manager.h"
#include "components/devtools_discovery/devtools_target_descriptor.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace devtools_http_handler {
class DevToolsHttpHandler;
};

namespace webos {

devtools_http_handler::DevToolsHttpHandler* initDevTools();

class WebOSDevToolsDelegate
    : public devtools_http_handler::DevToolsHttpHandlerDelegate {
 public:
  WebOSDevToolsDelegate();
  virtual ~WebOSDevToolsDelegate();

  // DevToolsHttpHandlerDelegate implementation.
  std::string GetDiscoveryPageHTML() override;
  std::string GetFrontendResource(const std::string& path) override;
  std::string GetPageThumbnailData(const GURL& url) override;

  content::DevToolsExternalAgentProxyDelegate*
      HandleWebSocketConnection(const std::string& path) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebOSDevToolsDelegate);
};

class WebOSDevToolsManagerDelegate : public content::DevToolsManagerDelegate {
 public:
  void Inspect(content::BrowserContext* browser_context,
               content::DevToolsAgentHost* agent_host) override {}
  void DevToolsAgentStateChanged(content::DevToolsAgentHost* agent_host,
                                 bool attached) override {}
  base::DictionaryValue* HandleCommand(content::DevToolsAgentHost* agent_host,
                                       base::DictionaryValue* command) override;
};

class WebOSDevToolsDiscoveryProvider
    : public devtools_discovery::DevToolsDiscoveryManager::Provider {
 public:
  devtools_discovery::DevToolsTargetDescriptor::List GetDescriptors() override;
};

}  // namespace webos

#endif  // WEBOS_WEBOS_DEV_TOOLS_DELEGATE_H_
