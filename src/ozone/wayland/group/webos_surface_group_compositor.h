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

#ifndef WEBOS_SURFACE_GROUP_COMPOSITOR_H_
#define WEBOS_SURFACE_GROUP_COMPOSITOR_H_

#include "ozone/platform/ozone_export_wayland.h"
#include "ozone/wayland/group/wayland_webos_surface_group.h"

namespace ozonewayland {

class WebOSSurfaceGroup;

class OZONE_WAYLAND_EXPORT WebOSSurfaceGroupCompositor
    : public wl_webos_surface_group_compositor {
 public:
  WebOSSurfaceGroupCompositor(wl_registry* registry, uint32_t id);
  ~WebOSSurfaceGroupCompositor();

  WebOSSurfaceGroup* CreateGroup(unsigned handle, const std::string& name);
  WebOSSurfaceGroup* GetGroup(unsigned handle, const std::string& name);

private:

  DISALLOW_COPY_AND_ASSIGN(WebOSSurfaceGroupCompositor);
};

} //namespace ozonewayland
#endif // WEBOS_SURFACE_GROUP_COMPOSITOR_H_
