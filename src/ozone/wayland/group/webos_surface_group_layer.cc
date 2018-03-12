// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos_surface_group_layer.h"

namespace ozonewayland {

WebOSSurfaceGroupLayer::WebOSSurfaceGroupLayer()
    : wl_webos_surface_group_layer() {
}

WebOSSurfaceGroupLayer::~WebOSSurfaceGroupLayer() {
  destroy();
}

} //namespace ozonewayland

