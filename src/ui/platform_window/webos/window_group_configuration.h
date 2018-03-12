// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef UI_PLATFORM_WINDOW_WEBOS_WINDOW_GROUP_CONFIGURATION_H_
#define UI_PLATFORM_WINDOW_WEBOS_WINDOW_GROUP_CONFIGURATION_H_

#include <string>
#include <vector>

namespace ui {

struct WindowGroupLayerConfiguration {
  WindowGroupLayerConfiguration();
  WindowGroupLayerConfiguration(const WindowGroupLayerConfiguration& other);

  std::string name;
  int z_order = 0;
};

struct WindowGroupConfiguration {
  WindowGroupConfiguration();

  std::string name;
  bool is_anonymous = false;

  std::vector<WindowGroupLayerConfiguration> layers;
};
}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_WEBOS_WINDOW_GROUP_CONFIGURATION_H_
