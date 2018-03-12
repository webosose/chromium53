// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef WEBOS_BROWSER_WEBOS_WEBVIEW_RENDERER_STATE_H_
#define WEBOS_BROWSER_WEBOS_WEBVIEW_RENDERER_STATE_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/singleton.h"

namespace webos {

// This class keeps track of webos webview renderer state for use on the IO
// thread.
// All methods should be called on the IO thread.
class WebOSWebViewRendererState {
 public:
  struct WebViewInfo {
    std::string app_path;
    std::string trust_level;
    std::string accept_language;
  };

  static WebOSWebViewRendererState* GetInstance();

  // Looks up the information for the webos webview for a given render
  // view, if one exists. Called on the IO thread.
  bool GetInfo(int render_process_id,
               int routing_id,
               WebViewInfo* webview_info);

  void RegisterWebViewInfo(int render_process_id,
                           int routing_id,
                           const WebViewInfo& web_view_info);
  void UnRegisterWebViewInfo(int render_process_id, int routing_id);

 private:
  friend struct base::DefaultSingletonTraits<WebOSWebViewRendererState>;

  typedef std::pair<int, int> RenderId;
  typedef std::map<RenderId, WebViewInfo> WebViewInfoMap;

  WebOSWebViewRendererState();
  ~WebOSWebViewRendererState();

  WebViewInfoMap webview_info_map_;

  DISALLOW_COPY_AND_ASSIGN(WebOSWebViewRendererState);
};

}  // namespace webos

#endif /* WEBOS_BROWSER_WEBOS_WEBVIEW_RENDERER_STATE_H_ */
