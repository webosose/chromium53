// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/renderer/webos_content_renderer_client.h"

#include "components/cdm/renderer/webos_key_systems.h"
#include <sys/syscall.h>

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/error_page/common/localized_error.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/web_preferences.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "third_party/WebKit/public/platform/WebCachePolicy.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "ui/base/resource/resource_bundle.h"
#include "webos/common/webos_watchdog.h"
#include "webos/renderer/webos_page_load_timing_render_frame_observer.h"
#include "webos/renderer/webos_render_view_observer.h"
#include "webos/renderer/net/webos_net_error_helper.h"

using blink::WebCachePolicy;
using blink::WebURL;
using blink::WebURLError;
using blink::WebURLRequest;
using blink::WebURLResponse;

namespace webos {

void WebOSContentRendererClient::RenderThreadStarted() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableWatchdog)) {
    watchdog_ = new WebOSWatchdog();

    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();

    std::string env_timeout =
        command_line.GetSwitchValueASCII(switches::kWatchdogRenderTimeout);
    if (!env_timeout.empty()) {
      int timeout;
      base::StringToInt(env_timeout, &timeout);
      watchdog_->SetTimeout(timeout);
    }

    std::string env_period =
        command_line.GetSwitchValueASCII(switches::kWatchdogRenderPeriod);
    if (!env_period.empty()) {
      int period;
      base::StringToInt(env_period, &period);
      watchdog_->SetPeriod(period);
    }

    watchdog_->StartWatchdog();

    //Check it's currently running on RenderThread
    CHECK(content::RenderThread::Get());
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::ThreadTaskRunnerHandle::Get();
    task_runner->PostTask(FROM_HERE,
                          base::Bind(&WebOSContentRendererClient::ArmWatchdog,
                                     base::Unretained(this)));
  }
}

void WebOSContentRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  new WebOSRenderViewObserver(render_view);
}

bool WebOSContentRendererClient::ShouldSuppressErrorPage(
    content::RenderFrame* render_frame,
    const GURL& url) {
  if (render_frame &&
      WebOsNetErrorHelper::Get(render_frame)->ShouldSuppressErrorPage(url)) {
    return true;
  }
}

void WebOSContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
#if defined(USE_SPLASH_SCREEN)
  // Only attach WebOSPageLoadTimingRenderFrameObserver to the main frame, since
  // we only want to observe page load timing for the main frame.
  if (render_frame->IsMainFrame())
    new WebOSPageLoadTimingRenderFrameObserver(render_frame);
#endif
  // Create net error helper
  new WebOsNetErrorHelper(render_frame);
}

void WebOSContentRendererClient::AddSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems) {
#if defined(OS_WEBOS)
  cdm::AddWebOSKeySystems(key_systems);
#endif
}

void WebOSContentRendererClient::ArmWatchdog() {
  watchdog_->Arm();
  if (!watchdog_->WatchingThreadTid())
    watchdog_->SetWatchingThreadTid((long int)syscall(SYS_gettid));

  //Check it's currently running on RenderThread
  CHECK(content::RenderThread::Get());
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();
  task_runner->PostDelayedTask(
      FROM_HERE,
      base::Bind(&WebOSContentRendererClient::ArmWatchdog,
                 base::Unretained(this)),
      base::TimeDelta::FromSeconds(watchdog_->Period()));
}

void WebOSContentRendererClient::GetNavigationErrorStrings(
    content::RenderFrame* render_frame,
    const WebURLRequest& failed_request,
    const WebURLError& error,
    std::string* error_html,
    base::string16* error_description) {
  const GURL failed_url = error.unreachableURL;
  bool is_post = base::EqualsASCII(
      base::StringPiece16(failed_request.httpMethod()), "POST");
  bool is_ignoring_cache =
      failed_request.getCachePolicy() == WebCachePolicy::BypassingCache;
  if (error_html) {
    WebOsNetErrorHelper::Get(render_frame)
        ->GetErrorHTML(error, is_post, is_ignoring_cache, error_html);
  }

  if (error_description) {
    *error_description = error_page::LocalizedError::GetErrorDetails(
        error.domain.utf8(), error.reason, is_post);
  }
}

bool WebOSContentRendererClient::HasErrorPage(int http_status_code,
                                              std::string* error_domain) {
  // Use an internal error page, if we have one for the status code.
  if (!error_page::LocalizedError::HasStrings(
          error_page::LocalizedError::kHttpErrorDomain, http_status_code)) {
    return false;
  }

  *error_domain = error_page::LocalizedError::kHttpErrorDomain;
  return true;
}

#if defined(OS_WEBOS)
void  WebOSContentRendererClient::NotifyLocaleChanged(
    const std::string& new_locale) {

  base::i18n::SetICUDefaultLocale(new_locale);
  if (ResourceBundle::HasSharedInstance()) {
    ResourceBundle::GetSharedInstance().ReloadLocaleResources(new_locale);
    ResourceBundle::GetSharedInstance().ReloadFonts();
  }
}
#endif

} // namespace webos
