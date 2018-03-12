// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/display.h"
#include "ozone/wayland/group/webos_surface_group.h"
#include "ozone/wayland/group/webos_surface_group_compositor.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/window.h"

namespace ozonewayland {

WebOSSurfaceGroupCompositor::WebOSSurfaceGroupCompositor(
    wl_registry* registry, uint32_t id)
    : wl_webos_surface_group_compositor(registry, id) {
}

WebOSSurfaceGroupCompositor::~WebOSSurfaceGroupCompositor() {
}

WebOSSurfaceGroup* WebOSSurfaceGroupCompositor::CreateGroup(
    unsigned handle, const std::string& name) {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  WaylandWindow* window = display->GetWindow(handle);
  struct ::wl_webos_surface_group* grp = create_surface_group(
    window->ShellSurface()->GetWLSurface(), name);
  if (!grp)
    return NULL;

  WebOSSurfaceGroup* group = new WebOSSurfaceGroup(handle);
  group->init(grp);
  return group;
}

WebOSSurfaceGroup* WebOSSurfaceGroupCompositor::GetGroup(
    unsigned handle, const std::string& name) {
  struct ::wl_webos_surface_group* grp = get_surface_group(name);
  if (!grp)
    return NULL;

  WebOSSurfaceGroup* group = new WebOSSurfaceGroup(handle);
  group->init(grp);
  return group;
}

} //namcespace ozonewayland
