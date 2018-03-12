// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos_surface_group.h"

#include "base/logging.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/group/webos_surface_group_layer.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/shell/webos_shell_surface.h"
#include "ozone/wayland/window.h"

namespace ozonewayland {

WebOSSurfaceGroup::WebOSSurfaceGroup()
    : wl_webos_surface_group()
    , attached_surface_(false)
    , window_handle_(0) {
}

WebOSSurfaceGroup::WebOSSurfaceGroup(unsigned handle)
    : wl_webos_surface_group()
    , attached_surface_(false)
    , window_handle_(handle) {
}


WebOSSurfaceGroup::~WebOSSurfaceGroup() {
  if (!named_layer_.empty()) {
    std::list<WebOSSurfaceGroupLayer*>::iterator iter;
    for (iter = named_layer_.begin(); iter != named_layer_.end(); ++iter)
      delete *iter;
  }

  wl_webos_surface_group_destroy(object());
}

void WebOSSurfaceGroup::SetAllowAnonymousLayers(bool allow) {
  allow_anonymous_layers(allow);
}

void WebOSSurfaceGroup::AttachAnonymousSurface(ZHint hint) {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  WaylandWindow* window = display->GetWindow(window_handle_);

  attach_anonymous(window->ShellSurface()->GetWLSurface(), (uint32_t)hint);
}

WebOSSurfaceGroupLayer* WebOSSurfaceGroup::CreateNamedLayer(const std::string& name, int z_order) {
  struct ::wl_webos_surface_group_layer* layer = create_layer(name, z_order);
  WebOSSurfaceGroupLayer* group_layer = new WebOSSurfaceGroupLayer();
  named_layer_.push_back(group_layer);
  group_layer->init(layer);
  group_layer->SetName(name);
  group_layer->SetLayerOrder(z_order);
  return group_layer;
}

void WebOSSurfaceGroup::AttachSurface(const std::string& layer) {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  WaylandWindow* window = display->GetWindow(window_handle_);

  if (!window || !window->ShellSurface()) {
    RAW_PMLOG_DEBUG(
        "WebOSWebView",
        "[Error]WaylandWindow(window:%s, handle:%d) or ShellSurface not found.\
        Can not attach %s layer.",
        window ? "true" : "false", window_handle_, layer.c_str());
    return;
  }

  attach(window->ShellSurface()->GetWLSurface(), layer);
  attached_surface_ = true;
  RAW_PMLOG_DEBUG("WebOSWebView", "Window(handle:%d) surface(layer:%s) attached",
                  window_handle_, layer.c_str());
}

void WebOSSurfaceGroup::DetachSurface() {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  WaylandWindow* window = display->GetWindow(window_handle_);

  if (!window || !window->ShellSurface()) {
    RAW_PMLOG_DEBUG(
        "WebOSWebView",
        "[Error]WaylandWindow(window:%s, handle:%d) or ShellSurface not found.\
        Can not detach and need to check whether group owner app is closed",
        window ? "true" : "false", window_handle_);
    return;
  }

  detach(window->ShellSurface()->GetWLSurface());
  attached_surface_ = false;
  RAW_PMLOG_DEBUG("WebOSWebView", "Window(handle:%d) surface detached",
                  window_handle_);
}

void WebOSSurfaceGroup::FocusOwner() {
  focus_owner();
}

void WebOSSurfaceGroup::FocusLayer(const std::string& layer) {
  if (!layer.empty())
    focus_layer(layer);
}

void WebOSSurfaceGroup::webos_surface_group_owner_destroyed() {
  WaylandDisplay* display = WaylandDisplay::GetInstance();
  WaylandWindow* window = display->GetWindow(window_handle_);

  detach(window->ShellSurface()->GetWLSurface());
  WebosShellSurface::HandleClose(window, NULL);
}

} //namespace ozonewayland
