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

#ifndef WEBOS_WAYLAND_WEBOS_SURFACE_GROUP_LAYER_H_
#define WEBOS_WAYLAND_WEBOS_SURFACE_GROUP_LAYER_H_

#include "base/macros.h"
#include "ozone/wayland/group/wayland_webos_surface_group.h"

namespace ozonewayland {

class WebOSSurfaceGroupLayer
    : public wl_webos_surface_group_layer {
 public:
  WebOSSurfaceGroupLayer();
  ~WebOSSurfaceGroupLayer();

  void SetName(const std::string& name) { layer_name_ = name; }
  std::string GetName() { return layer_name_; }

  void SetLayerOrder(int z_order) { layer_z_order_ = z_order; }
  int GetLayerOrder() { return layer_z_order_; }

 private:
  std::string layer_name_;
  int layer_z_order_;

  DISALLOW_COPY_AND_ASSIGN(WebOSSurfaceGroupLayer);
};

}//namespace ozonewayland

#endif // WEBOS_WAYLAND_WEBOS_SURFACE_GROUP_LAYER_H_

