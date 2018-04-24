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

#ifndef WEBOS_RENDERER_WEBOS_CONTENT_RENDERER_CLIENT_H_
#define WEBOS_RENDERER_WEBOS_CONTENT_RENDERER_CLIENT_H_

#include "content/public/renderer/content_renderer_client.h"

namespace content {
class RenderView;
class RenderFrame;
}

namespace webos {

class WebOSWatchdog;

class WebOSContentRendererClient : public content::ContentRendererClient {
 public:
  void RenderThreadStarted() override;
  void RenderViewCreated(content::RenderView* render_view) override;
  bool ShouldSuppressErrorPage(content::RenderFrame* render_frame,
                               const GURL& url) override;
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems)
      override;
  void GetNavigationErrorStrings(content::RenderFrame* render_frame,
                                 const blink::WebURLRequest& failed_request,
                                 const blink::WebURLError& error,
                                 std::string* error_html,
                                 base::string16* error_description) override;
  bool HasErrorPage(int http_status_code, std::string* error_domain) override;
#if defined(OS_WEBOS)
  void NotifyLocaleChanged(const std::string& new_locale) override;
#endif

 private:
  void ArmWatchdog();
  WebOSWatchdog* watchdog_;
};

} // namespace webos

#endif // WEBOS_RENDERER_WEBOS_CONTENT_RENDERER_CLIENT_H_
