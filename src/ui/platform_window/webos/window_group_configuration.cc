// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/platform_window/webos/window_group_configuration.h"

namespace ui {

WindowGroupLayerConfiguration::WindowGroupLayerConfiguration() : z_order(0) {}

WindowGroupLayerConfiguration::WindowGroupLayerConfiguration(
    const WindowGroupLayerConfiguration& other) = default;

WindowGroupConfiguration::WindowGroupConfiguration() : is_anonymous(false) {}

}  // namespace ui
