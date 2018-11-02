// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <sys/types.h>
#include <unistd.h>

#include "ozone/wayland/shell/ivi_shell_surface.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

#include "ozone/wayland/display.h"
#include "ozone/wayland/protocol/ivi-application-client-protocol.h"
#include "ozone/wayland/shell/shell.h"

namespace ozonewayland {

IVIShellSurface::IVIShellSurface()
    : WaylandShellSurface(), ivi_surface_(NULL) {}

IVIShellSurface::~IVIShellSurface() {
  ivi_surface_destroy(ivi_surface_);
}

void IVIShellSurface::InitializeShellSurface(WaylandWindow* window,
                                             WaylandWindow::ShellType type,
                                             int surface_id) {
  DCHECK(!ivi_surface_);
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  DCHECK(display);
  WaylandShell* shell = WaylandDisplay::GetInstance()->GetShell();
  DCHECK(shell && shell->GetIVIShell());

  // The window_manager on AGL handles surface_id 0 as an invalid id.
  if (surface_id == 0)
    surface_id = static_cast<int>(getpid());

  ivi_surface_ = ivi_application_surface_create(shell->GetIVIShell(),
                                                surface_id, GetWLSurface());

  DCHECK(ivi_surface_);
}

void IVIShellSurface::UpdateShellSurface(WaylandWindow::ShellType type,
                                         WaylandShellSurface* shell_parent,
                                         int x,
                                         int y) {
  WaylandShellSurface::FlushDisplay();
}

void IVIShellSurface::SetWindowTitle(const base::string16& title) {
}

void IVIShellSurface::Maximize() {
}

void IVIShellSurface::Minimize() {
}

void IVIShellSurface::Unminimize() {
}

bool IVIShellSurface::IsMinimized() const {
  return false;
}

}  // namespace ozonewayland
