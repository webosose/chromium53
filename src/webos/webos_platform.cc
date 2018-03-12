// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos_platform.h"

#include "net/base/webos/network_change_notifier_factory_webos.h"
#include "net/base/webos/network_change_notifier_webos.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/shell/webos_shell_surface.h"
#include "ozone/wayland/window.h"
#include "webos/app/webos_content_main_delegate.h"
#include "webos/common/webos_locales_mapping.h"
#include "webos/public/runtime.h"

namespace webos {

WebOSPlatform* WebOSPlatform::webos_platform_ = new WebOSPlatform;

WebOSPlatform* WebOSPlatform::GetInstance() {
  return webos_platform_;
}

WebOSPlatform::WebOSPlatform() : input_pointer_(NULL) {
  Runtime::GetInstance()->InitializePlatformDelegate(this);
}

WebOSPlatform::~WebOSPlatform() {
}

void WebOSPlatform::OnCursorVisibilityChanged(bool visible) {
  if (input_pointer_)
    input_pointer_->OnCursorVisibilityChanged(visible);
}

void WebOSPlatform::OnNetworkStateChanged(bool is_connected) {
  webos::NetworkChangeNotifierWebos* network_change_notifier =
      NetworkChangeNotifierFactoryWebos::GetInstance();
  if (network_change_notifier)
    network_change_notifier->OnNetworkStateChanged(is_connected);
}

void WebOSPlatform::OnLocaleInfoChanged(std::string language) {
  std::string locale = webos::MapWebOsToChromeLocales(language);
  GetWebOSContentBrowserClient()->SetApplicationLocale(locale);
}

void WebOSPlatform::SetInputPointer(InputPointer* input_pointer) {
  input_pointer_ = input_pointer;
}

InputPointer* WebOSPlatform::GetInputPointer() {
  return input_pointer_;
}

void WebOSPlatform::SetInputRegion(unsigned handle,
                                   std::vector<gfx::Rect>region) {
  ozonewayland::WaylandDisplay* display = ozonewayland::WaylandDisplay::GetInstance();
  ozonewayland::WaylandWindow* window = display->GetWindow(handle);

  ozonewayland::WebosShellSurface* shellSurface =
      static_cast<ozonewayland::WebosShellSurface*>(window->ShellSurface());
  shellSurface->SetInputRegion(region);
}

void WebOSPlatform::SetKeyMask(unsigned handle, webos::WebOSKeyMask keyMask) {
  ozonewayland::WaylandDisplay* display = ozonewayland::WaylandDisplay::GetInstance();
  ozonewayland::WaylandWindow* window = display->GetWindow(handle);

  ozonewayland::WebosShellSurface* shellSurface =
      static_cast<ozonewayland::WebosShellSurface*>(window->ShellSurface());
  shellSurface->SetKeyMask(keyMask);
}

} //namespace ozonewayland
