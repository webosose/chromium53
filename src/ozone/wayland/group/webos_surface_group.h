// Copyright (c) 2016-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef WEBOS_WAYLAND_WEBOS_SURFACE_GROUP_H_
#define WEBOS_WAYLAND_WEBOS_SURFACE_GROUP_H_

#include <list>

#include "base/macros.h"
#include "ozone/platform/ozone_export_wayland.h"
#include "ozone/wayland/group/wayland_webos_surface_group.h"

namespace ozonewayland {

class WebOSSurfaceGroupLayer;

class OZONE_WAYLAND_EXPORT WebOSSurfaceGroup
    : public wl_webos_surface_group {
 public:
  enum ZHint {
    ZHintBelow = 0,
    ZHintAbove,
    ZHintTop,
  };
  WebOSSurfaceGroup();
  WebOSSurfaceGroup(unsigned handle);
  ~WebOSSurfaceGroup();

  void SetAllowAnonymousLayers(bool allow);
  void AttachAnonymousSurface(ZHint hint = ZHint::ZHintAbove);

  WebOSSurfaceGroupLayer* CreateNamedLayer(const std::string& name, int z_order);
  void AttachSurface(const std::string& layer);
  void DetachSurface();

  void FocusOwner();
  void FocusLayer(const std::string& layer);

  void webos_surface_group_owner_destroyed();

 private:
  std::list<WebOSSurfaceGroupLayer*> named_layer_;

  bool attached_surface_;
  unsigned window_handle_;

  DISALLOW_COPY_AND_ASSIGN(WebOSSurfaceGroup);
};

}//namespace ozonewayland

#endif // WEBOS_WAYLAND_WEBOS_SURFACE_GROUP_H_
