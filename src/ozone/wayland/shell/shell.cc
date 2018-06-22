// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/shell/shell.h"

#include "base/logging.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/protocol/ivi-application-client-protocol.h"
#include "ozone/wayland/protocol/xdg-shell-client-protocol.h"
#include "ozone/wayland/shell/ivi_shell_surface.h"
#if defined(OS_WEBOS)
#include "ozone/wayland/shell/webos_shell_surface.h"
#endif
#include "ozone/wayland/shell/wl_shell_surface.h"
#include "ozone/wayland/shell/xdg_shell_surface.h"

namespace ozonewayland {

WaylandShell::WaylandShell()
    : shell_(NULL),
#if defined(OS_WEBOS)
      webos_shell_(NULL),
#endif
      xdg_shell_(NULL),
      ivi_application_(NULL) {
}

WaylandShell::~WaylandShell() {
  if (shell_)
    wl_shell_destroy(shell_);
#if defined(OS_WEBOS)
  if (webos_shell_)
    wl_webos_shell_destroy(webos_shell_);
#endif
  if (xdg_shell_)
    xdg_shell_destroy(xdg_shell_);
  if (ivi_application_)
    ivi_application_destroy(ivi_application_);
}

WaylandShellSurface*
WaylandShell::CreateShellSurface(WaylandWindow* window,
                                 WaylandWindow::ShellType type) {
  DCHECK(shell_ || xdg_shell_ || ivi_application_);
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  DCHECK(display);
  WaylandShellSurface* surface = NULL;
  if (ivi_application_)
    surface = new IVIShellSurface();
  if (xdg_shell_ && !surface)
    surface = new XDGShellSurface();
#if defined(OS_WEBOS)
  if (webos_shell_ && !surface)
    surface = new WebosShellSurface();
#endif
  if (!surface)
    surface = new WLShellSurface();

  DCHECK(surface);
  surface->InitializeShellSurface(window, type);
  wl_surface_set_user_data(surface->GetWLSurface(), window);
  display->FlushDisplay();

  return surface;
}

void WaylandShell::Initialize(struct wl_registry *registry,
                              uint32_t name,
                              const char *interface,
                              uint32_t version) {
  if (strcmp(interface, "wl_shell") == 0) {
    DCHECK(!shell_);
    shell_ = static_cast<wl_shell*>(
        wl_registry_bind(registry, name, &wl_shell_interface, 1));
#if defined(OS_WEBOS)
  } else if (strcmp(interface, "wl_webos_shell") == 0) {
    DCHECK(!webos_shell_);
    webos_shell_ = static_cast<wl_webos_shell*>(
        wl_registry_bind(registry, name, &wl_webos_shell_interface, 1));
#endif
  } else if ((strcmp(interface, "xdg_shell") == 0) &&
                getenv("OZONE_WAYLAND_USE_XDG_SHELL")) {
      DCHECK(!xdg_shell_);
      xdg_shell_ = static_cast<xdg_shell*>(
          wl_registry_bind(registry, name, &xdg_shell_interface, 1));
      xdg_shell_use_unstable_version(xdg_shell_, XDG_SHELL_VERSION_CURRENT);

      static const xdg_shell_listener xdg_shell_listener = {
        WaylandShell::XDGHandlePing
      };
      xdg_shell_add_listener(xdg_shell_, &xdg_shell_listener, NULL);
  } else if (strcmp(interface, "ivi_application") == 0) {
      DCHECK(!ivi_application_);
      ivi_application_ = static_cast<ivi_application*>(
          wl_registry_bind(registry, name, &ivi_application_interface, 1));
  }
}

void WaylandShell::XDGHandlePing(void* data,
                                 struct xdg_shell* xdg_shell,
                                 uint32_t serial) {
  xdg_shell_pong(xdg_shell, serial);
}

}  // namespace ozonewayland
