// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Copyright (c) 2016-2017 LG Electronics, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/renderer/net/webos_net_error_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/error_page/common/error_page_params.h"
#include "components/error_page/common/localized_error.h"
#include "components/error_page/common/net_error_info.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/common/content_client.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/resource_fetcher.h"
#include "grit/webos_inspector_resources.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"
#include "third_party/WebKit/public/web/WebDataSource.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "url/gurl.h"
#include "webos/common/webos_localized_error.h"
#include "webos/common/webos_view_messages.h"
#include "webos/renderer/net/template_builder.h"

using base::JSONWriter;
using content::DocumentState;
using content::RenderFrame;
using content::RenderFrameObserver;
using content::RenderThread;
using content::kUnreachableWebDataURL;
using error_page::DnsProbeStatus;
using error_page::DnsProbeStatusToString;
using error_page::ErrorPageParams;
using error_page::WebOsLocalizedError;
using error_page::NetErrorHelperCore;

namespace {

// Number of seconds to wait for the navigation correction service to return
// suggestions.  If it takes too long, just use the local error page.
const int kNavigationCorrectionFetchTimeoutSec = 3;

NetErrorHelperCore::PageType GetLoadingPageType(RenderFrame* render_frame) {
  blink::WebFrame* web_frame = render_frame->GetWebFrame();
  GURL url = web_frame->provisionalDataSource()->request().url();
  if (!url.is_valid() || url.spec() != kUnreachableWebDataURL)
    return NetErrorHelperCore::NON_ERROR_PAGE;
  return NetErrorHelperCore::ERROR_PAGE;
}

NetErrorHelperCore::FrameType GetFrameType(RenderFrame* render_frame) {
  if (render_frame->IsMainFrame())
    return NetErrorHelperCore::MAIN_FRAME;
  return NetErrorHelperCore::SUB_FRAME;
}

}  // namespace

namespace webos {
WebOsNetErrorHelper::WebOsNetErrorHelper(RenderFrame* render_frame)
    : RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<WebOsNetErrorHelper>(render_frame) {
  RenderThread::Get()->AddObserver(this);

  core_.reset(new NetErrorHelperCore(this,
                                     true,
                                     true,
                                     !render_frame->IsHidden()));
}

WebOsNetErrorHelper::~WebOsNetErrorHelper() {
  RenderThread::Get()->RemoveObserver(this);
}

void WebOsNetErrorHelper::DidStartProvisionalLoad() {
  core_->OnStartLoad(GetFrameType(render_frame()),
                     GetLoadingPageType(render_frame()));
}

void WebOsNetErrorHelper::DidCommitProvisionalLoad(bool is_new_navigation,
                                              bool is_same_page_navigation) {
  // If this is a "same page" navigation, it's not a real navigation.  There
  // wasn't a start event for it, either, so just ignore it.
  if (is_same_page_navigation)
    return;

  core_->OnCommitLoad(GetFrameType(render_frame()),
                      render_frame()->GetWebFrame()->document().url());
}

void WebOsNetErrorHelper::DidFinishLoad() {
  core_->OnFinishLoad(GetFrameType(render_frame()));
}

void WebOsNetErrorHelper::OnStop() {
  core_->OnStop();
}

void WebOsNetErrorHelper::WasShown() {
  core_->OnWasShown();
}

void WebOsNetErrorHelper::WasHidden() {
  core_->OnWasHidden();
}

bool WebOsNetErrorHelper::OnMessageReceived(const IPC::Message& message) {
  return false;
}

void WebOsNetErrorHelper::OnDestruct() {
  delete this;
}

void WebOsNetErrorHelper::NetworkStateChanged(bool enabled) {
  core_->NetworkStateChanged(enabled);
}

void WebOsNetErrorHelper::GetErrorHTML(const blink::WebURLError& error,
                                  bool is_failed_post,
                                  bool is_ignoring_cache,
                                  std::string* error_html) {
  core_->GetErrorHTML(GetFrameType(render_frame()), error, is_failed_post,
                      is_ignoring_cache, error_html);
}

bool WebOsNetErrorHelper::ShouldSuppressErrorPage(const GURL& url) {
  return core_->ShouldSuppressErrorPage(GetFrameType(render_frame()), url);
}

void WebOsNetErrorHelper::GenerateLocalizedErrorPage(
    const blink::WebURLError& error,
    bool is_failed_post,
    bool can_show_network_diagnostics_dialog,
    bool has_offline_pages,
    std::unique_ptr<ErrorPageParams> params,
    bool* reload_button_shown,
    bool* show_saved_copy_button_shown,
    bool* show_cached_copy_button_shown,
    bool* show_offline_pages_button_shown,
    std::string* error_html) const {
  error_html->clear();
  // Resource will change to net error specific page
  int resource_id = IDR_WEBOS_NETWORK_ERROR_PAGE;
  const base::StringPiece template_html(
       ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id));
   if (template_html.empty()) {
     NOTREACHED() << "unable to load template.";
   } else {
     base::DictionaryValue error_strings;
     WebOsLocalizedError::GetStrings(
         error.reason, error.domain.utf8(), error.unreachableURL,
         is_failed_post, (error.staleCopyInCache && !is_failed_post),
         RenderThread::Get()->GetLocale(),
         render_frame()->GetRenderView()->GetAcceptLanguages(),
         std::move(params), &error_strings);
     *reload_button_shown = error_strings.Get("reloadButton", nullptr);
     *show_saved_copy_button_shown =
         error_strings.Get("showSavedCopyButton", nullptr);
     *show_cached_copy_button_shown =
         error_strings.Get("cacheButton", nullptr);
     *show_offline_pages_button_shown =
         error_strings.Get("showOfflinePagesButton", nullptr);

     // "t" is the id of the template's root node.
     *error_html = webui::GetTemplatesHtml(template_html, &error_strings, "t");

     // viewport width and height
     int viewport_width = render_frame()->GetWebFrame()->viewportSize().width;
     int viewport_height = render_frame()->GetWebFrame()->viewportSize().height;

     // Add webos specific functionality
     *error_html =
         webos::GetTemplatesHtml(*error_html, &error_strings, error.reason, "t",
                                 viewport_width, viewport_height);
  }
}

void WebOsNetErrorHelper::LoadErrorPage(const std::string& html,
                                   const GURL& failed_url) {
  render_frame()->GetWebFrame()->loadHTMLString(
      html, GURL(kUnreachableWebDataURL), failed_url, true);
}

void WebOsNetErrorHelper::EnablePageHelperFunctions() {
}

void WebOsNetErrorHelper::UpdateErrorPage(const blink::WebURLError& error,
                                     bool is_failed_post,
                                     bool can_show_network_diagnostics_dialog,
                                     bool has_offline_pages) {
  base::DictionaryValue error_strings;
  WebOsLocalizedError::GetStrings(
      error.reason, error.domain.utf8(), error.unreachableURL, is_failed_post,
      (error.staleCopyInCache && !is_failed_post),
      RenderThread::Get()->GetLocale(),
      render_frame()->GetRenderView()->GetAcceptLanguages(),
      std::unique_ptr<ErrorPageParams>(), &error_strings);

  std::string json;
  JSONWriter::Write(error_strings, &json);

  std::string js = "if (window.updateForDnsProbe) "
                   "updateForDnsProbe(" + json + ");";
  base::string16 js16;
  if (!base::UTF8ToUTF16(js.c_str(), js.length(), &js16)) {
    NOTREACHED();
    return;
  }

  render_frame()->ExecuteJavaScript(js16);
}

void WebOsNetErrorHelper::FetchNavigationCorrections(
    const GURL& navigation_correction_url,
    const std::string& navigation_correction_request_body) {
  DCHECK(!correction_fetcher_.get());

  correction_fetcher_.reset(
      content::ResourceFetcher::Create(navigation_correction_url));
  correction_fetcher_->SetMethod("POST");
  correction_fetcher_->SetBody(navigation_correction_request_body);
  correction_fetcher_->SetHeader("Content-Type", "application/json");

  correction_fetcher_->Start(
      render_frame()->GetWebFrame(),
      blink::WebURLRequest::RequestContextInternal,
      blink::WebURLRequest::FrameTypeNone,
      content::ResourceFetcher::PLATFORM_LOADER,
      base::Bind(&WebOsNetErrorHelper::OnNavigationCorrectionsFetched,
                 base::Unretained(this)));

  correction_fetcher_->SetTimeout(
      base::TimeDelta::FromSeconds(kNavigationCorrectionFetchTimeoutSec));
}

void WebOsNetErrorHelper::CancelFetchNavigationCorrections() {
  correction_fetcher_.reset();
}

void WebOsNetErrorHelper::SendTrackingRequest(
    const GURL& tracking_url,
    const std::string& tracking_request_body) {
  // If there's already a pending tracking request, this will cancel it.
  tracking_fetcher_.reset(content::ResourceFetcher::Create(tracking_url));
  tracking_fetcher_->SetMethod("POST");
  tracking_fetcher_->SetBody(tracking_request_body);
  tracking_fetcher_->SetHeader("Content-Type", "application/json");

  tracking_fetcher_->Start(
      render_frame()->GetWebFrame(),
      blink::WebURLRequest::RequestContextInternal,
      blink::WebURLRequest::FrameTypeTopLevel,
      content::ResourceFetcher::PLATFORM_LOADER,
      base::Bind(&WebOsNetErrorHelper::OnTrackingRequestComplete,
                 base::Unretained(this)));
}

void WebOsNetErrorHelper::ReloadPage(bool bypass_cache) {
  render_frame()->GetWebFrame()->reload(
      bypass_cache ? blink::WebFrameLoadType::ReloadBypassingCache
                   : blink::WebFrameLoadType::Reload);
}

void WebOsNetErrorHelper::OnNavigationCorrectionsFetched(
    const blink::WebURLResponse& response,
    const std::string& data) {
  // The fetcher may only be deleted after |data| is passed to |core_|.  Move
  // it to a temporary to prevent any potential re-entrancy issues.
  std::unique_ptr<content::ResourceFetcher> fetcher(
      correction_fetcher_.release());
  bool success = (!response.isNull() && response.httpStatusCode() == 200);
  core_->OnNavigationCorrectionsFetched(success ? data : "",
                                        base::i18n::IsRTL());
}

void WebOsNetErrorHelper::LoadPageFromCache(const GURL& page_url) {
  blink::WebFrame* web_frame = render_frame()->GetWebFrame();
  DCHECK(!base::EqualsASCII(
      base::StringPiece16(web_frame->dataSource()->request().httpMethod()),
      "POST"));

  blink::WebURLRequest request(page_url);
  request.setCachePolicy(blink::WebCachePolicy::ReturnCacheDataDontLoad);

  web_frame->loadRequest(request);
}

void WebOsNetErrorHelper::DiagnoseError(const GURL& page_url) {
}

void WebOsNetErrorHelper::ShowOfflinePages() {
}

void WebOsNetErrorHelper::OnNetErrorInfo(int status_num) {
  DCHECK(status_num >= 0 && status_num < error_page::DNS_PROBE_MAX);
  DVLOG(1) << "Received status " << DnsProbeStatusToString(status_num);

  core_->OnNetErrorInfo(static_cast<DnsProbeStatus>(status_num));
}

void WebOsNetErrorHelper::OnTrackingRequestComplete(
    const blink::WebURLResponse& response,
    const std::string& data) {
  tracking_fetcher_.reset();
}

}  // namespace webos

