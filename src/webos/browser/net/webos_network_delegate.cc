// Copyright 2016-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/net/webos_network_delegate.h"

#include "base/files/file_util.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/public/browser/resource_request_info.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "webos/app/webos_content_main_delegate.h"
#include "webos/browser/webos_webview_renderer_state.h"

namespace webos {

WebOSNetworkDelegate::WebOSNetworkDelegate(
    std::vector<std::string>& allowed_target_paths,
    std::vector<std::string>& allowed_trusted_target_paths,
    std::map<std::string, std::string> extra_websocket_headers)
    : allowed_target_paths_(allowed_target_paths),
      allowed_trusted_target_paths_(allowed_trusted_target_paths),
      extra_websocket_headers_(std::move(extra_websocket_headers)),
      allow_all_access_(true) {
  allow_all_access_ = !allowed_target_paths_.size();
}

bool WebOSNetworkDelegate::OnCanAccessFile(const net::URLRequest& request,
                                           const base::FilePath& path) const {
  if (allow_all_access_)
    return true;

  base::File::Info file_info;
  base::GetFileInfo(path, &file_info);
  // file existance is not matter this
  // Deny directory access
  if (file_info.is_directory || path.EndsWithSeparator())
    return false;

  const base::FilePath stripped_path(path.StripTrailingSeparators());

  // 1. Resources in globally permitted paths
  for (int i = 0 ; i < allowed_target_paths_.size(); ++i) {
    const base::FilePath white_listed_path(allowed_target_paths_.at(i));
    // base::FilePath::operator== should probably handle trailing separators.
    if (white_listed_path == stripped_path ||
        white_listed_path.IsParent(path))
      return true;
  }

  // in following we handle trust level set by webOS applications

  // TODO (jani) Add support to ResourceRequestInfo::ForRequest(const
  // net::URLRequest&)
  // and replace following line with that call
  const content::ResourceRequestInfoImpl* resourceInfo =
      static_cast<content::ResourceRequestInfoImpl*>(request.GetUserData(NULL));

  WebOSWebViewRendererState::WebViewInfo web_view_info;
  if (!resourceInfo ||
      !WebOSWebViewRendererState::GetInstance()->GetInfo(
          resourceInfo->GetChildID(), resourceInfo->GetRouteID(),
          &web_view_info))
    return true;  // not a webview call, allow access???

  std::string app_path = web_view_info.app_path;
  std::string trust_level = web_view_info.trust_level;

  std::string caller_path = app_path;
  std::string target_path = path.AsUTF8Unsafe();

  // strip leading separators until only one is left
  while (!caller_path.find("//"))
    caller_path = caller_path.substr(1);

  // 2. Resources in app folder path
  const base::FilePath white_listed_path =
      base::FilePath(caller_path).StripTrailingSeparators();
  if (white_listed_path == stripped_path || white_listed_path.IsParent(path))
    return true;

  // 3. Resources for trusted app
  if (trust_level == "trusted") {
    for (int i = 0 ; i < allowed_trusted_target_paths_.size(); ++i) {
      base::FilePath config_path(allowed_trusted_target_paths_.at(i));
      // Strip trailingseparators, this allows using both /foo/ and /foo in
      // security_config file
      const base::FilePath white_listed_path =
          config_path.StripTrailingSeparators();
      // base::FilePath::operator== should probably handle trailing separators.
      if (white_listed_path == stripped_path ||
          white_listed_path.IsParent(path))
        return true;
    }
  }
  return allow_all_access_;
}

int WebOSNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  if (GetWebOSContentBrowserClient()->DoNotTrack()) {
    static const char DNTHeader[] = "DNT";
    request->SetExtraRequestHeaderByName(DNTHeader, "1", true);
  }
  bool is_websocket_request = request && request->url().SchemeIsWSOrWSS();
  if (is_websocket_request) {
    for (auto& it : extra_websocket_headers_) {
      const std::pair<std::string, std::string>& header = it;
      request->SetExtraRequestHeaderByName(header.first, header.second, true);
    }
  }

  const content::ResourceRequestInfoImpl* resourceInfo =
      static_cast<content::ResourceRequestInfoImpl*>(
          request->GetUserData(NULL));

  WebOSWebViewRendererState::WebViewInfo web_view_info;
  if (resourceInfo &&
      WebOSWebViewRendererState::GetInstance()->GetInfo(
          resourceInfo->GetChildID(), resourceInfo->GetRouteID(),
          &web_view_info)) {
    std::string accept_language = web_view_info.accept_language;
    request->SetExtraRequestHeaderByName(
        net::HttpRequestHeaders::kAcceptLanguage, accept_language, false);
  }
  return net::OK;
}

} // namespace webos
