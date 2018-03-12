// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/devtools/webos_devtools_delegate.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/devtools_discovery/basic_target_descriptor.h"
#include "components/devtools_discovery/devtools_discovery_manager.h"
#include "components/devtools_discovery/devtools_target_descriptor.h"
#include "components/devtools_http_handler/devtools_http_handler.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/common/content_switches.h"
#include "grit/webos_inspector_resources.h"
#include "net/base/net_errors.h"
#include "net/socket/tcp_server_socket.h"
#include "ui/base/resource/resource_bundle.h"

#include <memory>

namespace webos {

namespace {

const int kBackLog = 10;
const int kDefaultRemoteDebuggingPort = 9998;
const int kMinRemoteDebuggingPort = 0;
const int kMaxRemoteDebuggingPort = 65535;

class WebOSDevToolsServerSocketFactory
    : public devtools_http_handler::DevToolsHttpHandler::ServerSocketFactory {
 public:
  WebOSDevToolsServerSocketFactory(int port) : port_(port) {}
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    std::unique_ptr<net::ServerSocket> socket(
        new net::TCPServerSocket(nullptr, net::NetLog::Source()));
    if (socket->ListenWithAddressAndPort("0.0.0.0", port_, kBackLog) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    return socket;
  }

 private:
  int port_;
};

}  // namespace

devtools_discovery::DevToolsTargetDescriptor::List
WebOSDevToolsDiscoveryProvider::GetDescriptors() {
  std::vector<scoped_refptr<content::DevToolsAgentHost>> wc_list =
      content::DevToolsAgentHost::GetOrCreateAll();
  devtools_discovery::DevToolsTargetDescriptor::List targets;

  for (std::vector<scoped_refptr<content::DevToolsAgentHost>>::iterator it =
           wc_list.begin();
       it != wc_list.end(); ++it) {
    targets.push_back(new devtools_discovery::BasicTargetDescriptor(*it));
  }

  return targets;
}

WebOSDevToolsDelegate::WebOSDevToolsDelegate() {
}

WebOSDevToolsDelegate::~WebOSDevToolsDelegate() {
}

std::string WebOSDevToolsDelegate::GetDiscoveryPageHTML() {
  return ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_WEBOS_INSPECTOR_DISCOVERY_PAGE).as_string();
}

std::string WebOSDevToolsDelegate::GetFrontendResource(
    const std::string& path) {
  return content::DevToolsFrontendHost::GetFrontendResource(path).as_string();
}

std::string WebOSDevToolsDelegate::GetPageThumbnailData(const GURL& url) {
  return "";
}

content::DevToolsExternalAgentProxyDelegate*
    WebOSDevToolsDelegate::HandleWebSocketConnection(const std::string& path) {
  return nullptr;
}

base::DictionaryValue* WebOSDevToolsManagerDelegate::HandleCommand(
    content::DevToolsAgentHost*,
    base::DictionaryValue*) {
  return 0;
}

devtools_http_handler::DevToolsHttpHandler* initDevTools() {
  int port;

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kRemoteDebuggingPort)) {
    std::string port_str =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kRemoteDebuggingPort);
    if (!(base::StringToInt(port_str, &port) &&
          port >= kMinRemoteDebuggingPort && port < kMaxRemoteDebuggingPort)) {
#if 0 // FIXME(jb) uncomment after PMLOG patch
      RAW_PMLOG_INFO("WPL_WAR", "Invalid Port! Use DefaultPort %d",
                     kDefaultRemoteDebuggingPort);
#endif
      port = kDefaultRemoteDebuggingPort;
    }
  } else
    port = kDefaultRemoteDebuggingPort;

  devtools_discovery::DevToolsDiscoveryManager::GetInstance()->AddProvider(
      std::unique_ptr<devtools_discovery::DevToolsDiscoveryManager::Provider>(
          new WebOSDevToolsDiscoveryProvider()));

  std::unique_ptr<devtools_http_handler::DevToolsHttpHandler::ServerSocketFactory>
      factory(new WebOSDevToolsServerSocketFactory(port));
  return new devtools_http_handler::DevToolsHttpHandler(
      std::move(factory), std::string(), new WebOSDevToolsDelegate(),
      base::FilePath(), base::FilePath(), std::string(), std::string());
}

}  // namespace webos
