// Copyright 2016-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/app/webos_content_main_delegate.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "content/public/common/content_switches.h"
#include "webos/browser/webos_content_browser_client.h"
#include "webos/common/webos_resource_delegate.h"
#include "webos/renderer/webos_content_renderer_client.h"

using base::CommandLine;

base::LazyInstance<webos::WebOSContentBrowserClient>
    g_webos_content_browser_client = LAZY_INSTANCE_INITIALIZER;
base::LazyInstance<webos::WebOSContentRendererClient>
    g_webos_content_renderer_client = LAZY_INSTANCE_INITIALIZER;

namespace webos {

namespace {
bool SubprocessNeedsResourceBundle(const std::string& process_type) {
  return process_type == switches::kZygoteProcess ||
         process_type == switches::kPpapiPluginProcess ||
         process_type == switches::kPpapiBrokerProcess ||
         process_type == switches::kGpuProcess ||
         process_type == switches::kRendererProcess ||
         process_type == switches::kUtilityProcess;
}
}

WebOSContentMainDelegate::WebOSContentMainDelegate() {}

bool WebOSContentMainDelegate::BasicStartupComplete(int* exit_code) {
  base::CommandLine* parsedCommandLine = base::CommandLine::ForCurrentProcess();
  parsedCommandLine->AppendSwitch(switches::kInProcessGPU);
  parsedCommandLine->AppendSwitch(switches::kNoSandbox);
  parsedCommandLine->AppendSwitch(switches::kWebOSWAM);

  content::SetContentClient(&content_client_);

  std::string process_type =
        parsedCommandLine->GetSwitchValueASCII(switches::kProcessType);
  if (process_type.empty()) {
    startup_callback_.Run();
  }
  return false;
}

void WebOSContentMainDelegate::PreSandboxStartup() {
  base::CommandLine* parsedCommandLine = base::CommandLine::ForCurrentProcess();
  std::string process_type =
      parsedCommandLine->GetSwitchValueASCII(switches::kProcessType);
  if (SubprocessNeedsResourceBundle(process_type))
    WebosResourceDelegate::InitializeResourceBundle();
}

content::ContentBrowserClient*
WebOSContentMainDelegate::CreateContentBrowserClient() {
  return g_webos_content_browser_client.Pointer();
}

content::ContentRendererClient*
WebOSContentMainDelegate::CreateContentRendererClient() {
  return g_webos_content_renderer_client.Pointer();
}

void WebOSContentMainDelegate::ProcessExiting(const std::string& process_type) {
  if (SubprocessNeedsResourceBundle(process_type))
    ResourceBundle::CleanupSharedInstance();
}

WebOSContentBrowserClient* GetWebOSContentBrowserClient() {
  return g_webos_content_browser_client.Pointer();
}

}  // namespace webos
