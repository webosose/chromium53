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

#ifndef WEBOS_BROWSER_WEBOS_BROWSER_CONTEXT_H_
#define WEBOS_BROWSER_WEBOS_BROWSER_CONTEXT_H_

#include "content/public/browser/browser_context.h"

#include <memory>

namespace content {
class ZoomLevelDelegate;
}

namespace webos {

class WebOSBrowserContextAdapter;
class WebOSRequestContextGetter;

class WebOSBrowserContext : public content::BrowserContext {
 public:
  WebOSBrowserContext(WebOSBrowserContextAdapter* adapter);
  ~WebOSBrowserContext() override;

  net::URLRequestContextGetter* GetRequestContext();

  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  base::FilePath GetPath() const override;
  bool IsOffTheRecord() const override { return false; }
  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate*
      GetDownloadManagerDelegate() override { return NULL; }
  content::BrowserPluginGuestManager*
      GetGuestManager() override { return NULL; }
  storage::SpecialStoragePolicy*
      GetSpecialStoragePolicy() override { return NULL; }
  content::PushMessagingService*
      GetPushMessagingService() override { return NULL; }
  content::SSLHostStateDelegate*
      GetSSLHostStateDelegate() override { return NULL; }

  content::PermissionManager* GetPermissionManager() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;

  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;

  net::URLRequestContextGetter* CreateMediaRequestContext() override;
  net::URLRequestContextGetter*
      CreateMediaRequestContextForStoragePartition(
          const base::FilePath& partition_path,
          bool in_memory) override;

  void AppendExtraWebSocketHeader(const std::string& key,
                                  const std::string& value);
  void SetProxyRules(const std::string& proxy_rules);

  void FlushCookieStore();

 private:
  void UpdateRequestContextGetterExtraWebSocketHeader(const std::string& key,
                                                      const std::string& value);
  void FlushCookieStoreOnIOThread();

  class WebOSResourceContext;

  WebOSBrowserContextAdapter* adapter_;

  std::unique_ptr<WebOSResourceContext> resource_context_;
  scoped_refptr<WebOSRequestContextGetter> url_request_getter_;

  std::map<std::string, std::string> extra_websocket_headers_;
  std::string proxy_rules_;

  DISALLOW_COPY_AND_ASSIGN(WebOSBrowserContext);
};

} // namespace webos

#endif /* WEBOS_BROWSER_WEBOS_BROWSER_CONTEXT_H_ */
