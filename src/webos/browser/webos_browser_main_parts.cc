// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/webos_browser_main_parts.h"

#include <sys/syscall.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "components/devtools_http_handler/devtools_http_handler.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "net/base/network_change_notifier.h"
#include "net/base/webos/network_change_notifier_factory_webos.h"
#include "ui/base/material_design/material_design_controller.h"
#include "webos/browser/devtools/webos_devtools_delegate.h"
#include "webos/browser/webos_browser_context_adapter.h"
#include "webos/browser/webos_browser_main_extra_parts.h"
#include "webos/common/webos_resource_delegate.h"
#include "webos/common/webos_watchdog.h"
#include "webos/public/runtime.h"

#if defined(ENABLE_PLUGINS)
#include "content/public/browser/plugin_service.h"
#endif

#if defined(USE_AURA)
#include "ui/aura/env.h"
#endif

namespace webos {

WebOSBrowserMainParts::WebOSBrowserMainParts()
    : BrowserMainParts(), devtools_http_handler_(0) {}

void WebOSBrowserMainParts::AddParts(WebOSBrowserMainExtraParts* parts) {
  webos_extra_parts_.push_back(parts);
}

void WebOSBrowserMainParts::PreMainMessageLoopRun() {
  default_browser_context_ = new WebOSBrowserContextAdapter("Default");

  content::RenderFrameHost::AllowInjectingJavaScriptForWebOSWebView();

#if defined(ENABLE_PLUGINS)
  plugin_service_filter_.reset(new WebOSPluginServiceFilter);
  content::PluginService::GetInstance()->SetFilter(
      plugin_service_filter_.get());
#endif

#if defined(USE_AURA)
  aura::Env::GetInstance();
#endif

  ui::MaterialDesignController::Initialize();

  // Replace the default NetworkChangeNotifierFactory with browser
  // implementation. This must be done before BrowserMainLoop calls
  // net::NetworkChangeNotifier::Create() in MainMessageLoopStart().
  net::NetworkChangeNotifier::SetFactory(
      new webos::NetworkChangeNotifierFactoryWebos());
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableWatchdog)) {
    ui_watchdog_ = new WebOSWatchdog();
    io_watchdog_ = new WebOSWatchdog();

    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();

    std::string env_timeout =
        command_line.GetSwitchValueASCII(switches::kWatchdogBrowserTimeout);
    if (!env_timeout.empty()) {
      int timeout;
      base::StringToInt(env_timeout, &timeout);
      ui_watchdog_->SetTimeout(timeout);
      io_watchdog_->SetTimeout(timeout);
    }

    std::string env_period =
        command_line.GetSwitchValueASCII(switches::kWatchdogBrowserPeriod);
    if (!env_period.empty()) {
      int period;
      base::StringToInt(env_period, &period);
      ui_watchdog_->SetPeriod(period);
      io_watchdog_->SetPeriod(period);
    }

    ui_watchdog_->StartWatchdog();
    io_watchdog_->StartWatchdog();

    content::BrowserThread::PostTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&WebOSBrowserMainParts::ArmWatchdog,
                   base::Unretained(this),
                   content::BrowserThread::UI,
                   ui_watchdog_));

    content::BrowserThread::PostTask(
        content::BrowserThread::IO,
        FROM_HERE,
        base::Bind(&WebOSBrowserMainParts::ArmWatchdog,
                   base::Unretained(this),
                   content::BrowserThread::IO,
                   io_watchdog_));
  }
}

void WebOSBrowserMainParts::PreEarlyInitialization() {
  for (size_t i = 0; i < webos_extra_parts_.size(); ++i)
    webos_extra_parts_[i]->PreEarlyInitialization();
}

void WebOSBrowserMainParts::ToolkitInitialized() {
  for (size_t i = 0; i < webos_extra_parts_.size(); ++i)
    webos_extra_parts_[i]->ToolkitInitialized();
}

int WebOSBrowserMainParts::PreCreateThreads() {
  WebosResourceDelegate::InitializeResourceBundle();

  for (size_t i = 0; i < webos_extra_parts_.size(); ++i)
    webos_extra_parts_[i]->PreCreateThreads();

  return 0;
}

void WebOSBrowserMainParts::PostDestroyThreads() {
}

bool WebOSBrowserMainParts::MainMessageLoopRun(int* result_code) {
  base::RunLoop run_loop;

  run_loop.Run();
  return true;
}

void WebOSBrowserMainParts::EnableDevTools() {
  int port;
  if (devtools_http_handler_)
      return;

  devtools_http_handler_ = initDevTools();
}

WebOSBrowserContextAdapter* WebOSBrowserMainParts::GetDefaultBrowserContext()
    const {
  return default_browser_context_;
}

void WebOSBrowserMainParts::ArmWatchdog(content::BrowserThread::ID thread,
                                        WebOSWatchdog* watchdog) {
  watchdog->Arm();
  if (!watchdog->WatchingThreadTid())
    watchdog->SetWatchingThreadTid((long int)syscall(SYS_gettid));

  content::BrowserThread::PostDelayedTask(
      thread,
      FROM_HERE,
      base::Bind(&WebOSBrowserMainParts::ArmWatchdog,
                 base::Unretained(this),
                 thread,
                 watchdog),
      base::TimeDelta::FromSeconds(watchdog->Period()));
}

} // namespace webos
