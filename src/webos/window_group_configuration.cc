// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/window_group_configuration.h"

namespace webos {

WindowGroupConfiguration::WindowGroupConfiguration(const std::string& name)
    : name_(name), is_anonymous_(false) {}

WindowGroupConfiguration::~WindowGroupConfiguration() {}

void WindowGroupConfiguration::SetIsAnonymous(bool is_anonymous) {
  is_anonymous_ = is_anonymous;
}

bool WindowGroupConfiguration::GetIsAnonymous() const {
  return is_anonymous_;
}

std::string WindowGroupConfiguration::GetName() const {
  return name_;
}

void WindowGroupConfiguration::AddLayer(
    const WindowGroupLayerConfiguration& layer_config) {
  layers_.push_back(layer_config);
}

const std::vector<WindowGroupLayerConfiguration>&
WindowGroupConfiguration::GetLayers() const {
  return layers_;
}

}  // namespace webos
