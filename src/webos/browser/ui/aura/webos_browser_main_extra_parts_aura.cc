// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/ui/aura/webos_browser_main_extra_parts_aura.h"

#include "ozone/ui/webui/ozone_webui.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"

namespace webos {
WebOSBrowserMainExtraPartsAura::WebOSBrowserMainExtraPartsAura() {
}

WebOSBrowserMainExtraPartsAura::~WebOSBrowserMainExtraPartsAura() {
}

void WebOSBrowserMainExtraPartsAura::PreEarlyInitialization() {
  views::LinuxUI::SetInstance(BuildWebUI());
}

void WebOSBrowserMainExtraPartsAura::ToolkitInitialized() {
  views::LinuxUI::instance()->Initialize();
}

void WebOSBrowserMainExtraPartsAura::PreCreateThreads() {
  display::Screen::SetScreenInstance(views::CreateDesktopScreen());
}

} // namespace webos
