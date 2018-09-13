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

#ifndef WEBOS_BROWSER_WEBOS_URL_REQUEST_CONTEXT_GETTER_h
#define WEBOS_BROWSER_WEBOS_URL_REQUEST_CONTEXT_GETTER_h

#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/url_constants.h"
#include "net/http/http_network_session.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory_impl.h"

namespace net {
class HostResolver;
class ChannelIDService;
class CertVerifier;
class CTVerifier;
class CTPolicyEnforcer;
class SSLConfigService;
class TransportSecurityState;
class ProxyService;
class HttpAuthHandlerFactory;
class HttpServerProperties;
class HttpUserAgentSettings;
class HttpTransactionFactory;
class CookieStore;
class CodeCache;
}

namespace webos {

class WebOSBrowserContext;
class WebOSNetworkDelegate;

class WebOSRequestContextGetter : public net::URLRequestContextGetter {
 public:
  WebOSRequestContextGetter(
      WebOSBrowserContext* browser_context,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors,
      std::map<std::string, std::string> extra_websocket_headers,
      std::string proxy_rules);

  net::URLRequestContext* GetURLRequestContext() override;
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override;

  void AppendExtraWebSocketHeader(const std::string& key,
                                  const std::string& value);

 private:
  virtual ~WebOSRequestContextGetter();

  void InitializeSystemContextDependencies();
  void InitializeMainContextDependencies(
      net::HttpTransactionFactory* factory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors);
  void PopulateNetworkSessionParams(bool ignore_certificate_errors,
                                    net::HttpNetworkSession::Params* params);

  net::URLRequestContext* CreateRequestContext(
      WebOSBrowserContext* browser_context,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors);

  WebOSBrowserContext* const browser_context_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;
  std::map<std::string, std::string> extra_websocket_headers_;
  std::string proxy_rules_;
  std::unique_ptr<net::URLRequestContext> request_context_;
  std::unique_ptr<WebOSNetworkDelegate> network_delegate_;

  std::unique_ptr<net::HostResolver> host_resolver_;
  std::unique_ptr<net::ChannelIDService> channel_id_service_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::CTVerifier> cert_transparency_verifier_;
  std::unique_ptr<net::CTPolicyEnforcer> policy_enforcer_;
  scoped_refptr<net::SSLConfigService> ssl_config_service_;
  std::unique_ptr<net::TransportSecurityState> transport_security_state_;
  std::unique_ptr<net::ProxyService> proxy_service_;
  std::unique_ptr<net::HttpAuthHandlerFactory> http_auth_handler_factory_;
  std::unique_ptr<net::HttpServerProperties> http_server_properties_;
  std::unique_ptr<net::HttpUserAgentSettings> http_user_agent_settings_;
  std::unique_ptr<net::HttpTransactionFactory> system_transaction_factory_;

  std::unique_ptr<net::HttpTransactionFactory> main_transaction_factory_;
  std::unique_ptr<net::URLRequestJobFactory> main_job_factory_;
  std::unique_ptr<net::CookieStore> main_cookie_store_;
  std::unique_ptr<net::CodeCache> code_cache_;

  DISALLOW_COPY_AND_ASSIGN(WebOSRequestContextGetter);
};

}  // namespace webos

#endif  // WEBOS_BROWSER_WEBOS_URL_REQUEST_CONTEXT_GETTER_h
