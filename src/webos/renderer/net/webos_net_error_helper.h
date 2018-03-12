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

#ifndef WEBOS_RENDERER_NET_WEBOS_NET_ERROR_HELPER_H_
#define WEBOS_RENDERER_NET_WEBOS_NET_ERROR_HELPER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "components/error_page/common/net_error_info.h"
#include "components/error_page/renderer/net_error_helper_core.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "content/public/renderer/render_thread_observer.h"

class GURL;

namespace blink {
class WebFrame;
class WebURLResponse;
struct WebURLError;
}

namespace content {
class ResourceFetcher;
}

namespace error_page {
struct ErrorPageParams;
}

namespace webos {
// Listens for NetErrorInfo messages from the NetErrorTabHelper on the
// browser side and updates the error page with more details (currently, just
// DNS probe results) if/when available.
// TODO(crbug.com/578770): Should this class be moved into the error_page
// component?
class WebOsNetErrorHelper
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<WebOsNetErrorHelper>,
      public content::RenderThreadObserver,
      public error_page::NetErrorHelperCore::Delegate {
 public:
  explicit WebOsNetErrorHelper(content::RenderFrame* render_frame);
  ~WebOsNetErrorHelper() override;

  // RenderFrameObserver implementation.
  void DidStartProvisionalLoad() override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_page_navigation) override;
  void DidFinishLoad() override;
  void OnStop() override;
  void WasShown() override;
  void WasHidden() override;
  void OnDestruct() override;

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

  // RenderThreadObserver implementation.
  void NetworkStateChanged(bool online) override;

  // Initializes |error_html| with the HTML of an error page in response to
  // |error|.  Updates internals state with the assumption the page will be
  // loaded immediately.
  void GetErrorHTML(const blink::WebURLError& error,
                    bool is_failed_post,
                    bool is_ignoring_cache,
                    std::string* error_html);

  // Returns whether a load for |url| in the |frame| the WebOsNetErrorHelper is
  // attached to should have its error page suppressed.
  bool ShouldSuppressErrorPage(const GURL& url);

 private:
  // NetErrorHelperCore::Delegate implementation:
  void GenerateLocalizedErrorPage(
      const blink::WebURLError& error,
      bool is_failed_post,
      bool can_use_local_diagnostics_service,
      bool has_offline_pages,
      std::unique_ptr<error_page::ErrorPageParams> params,
      bool* reload_button_shown,
      bool* show_saved_copy_button_shown,
      bool* show_cached_copy_button_shown,
      bool* show_offline_pages_button_shown,
      std::string* html) const override;
  void LoadErrorPage(const std::string& html, const GURL& failed_url) override;
  void EnablePageHelperFunctions() override;
  void UpdateErrorPage(const blink::WebURLError& error,
                       bool is_failed_post,
                       bool can_use_local_diagnostics_service,
                       bool has_offline_pages) override;
  void FetchNavigationCorrections(
      const GURL& navigation_correction_url,
      const std::string& navigation_correction_request_body) override;
  void CancelFetchNavigationCorrections() override;
  void SendTrackingRequest(const GURL& tracking_url,
                           const std::string& tracking_request_body) override;
  void ReloadPage(bool bypass_cache) override;
  void LoadPageFromCache(const GURL& page_url) override;
  void DiagnoseError(const GURL& page_url) override;
  void ShowOfflinePages() override;

  void OnNetErrorInfo(int status);
  void OnNavigationCorrectionsFetched(const blink::WebURLResponse& response,
                                      const std::string& data);

  void OnTrackingRequestComplete(const blink::WebURLResponse& response,
                                 const std::string& data);

  std::unique_ptr<content::ResourceFetcher> correction_fetcher_;
  std::unique_ptr<content::ResourceFetcher> tracking_fetcher_;

  std::unique_ptr<error_page::NetErrorHelperCore> core_;

  DISALLOW_COPY_AND_ASSIGN(WebOsNetErrorHelper);
};

} //namespace webos
#endif  // WEBOS_RENDERER_NET_WEBOS_NET_ERROR_HELPER_H_
