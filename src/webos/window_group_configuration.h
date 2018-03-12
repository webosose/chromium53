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

#ifndef WEBOS_WINDOW_GROUP_CONFIGURATION_H_
#define WEBOS_WINDOW_GROUP_CONFIGURATION_H_

#include "base/macros.h"
#include "webos/common/webos_export.h"

#include <string>
#include <vector>

namespace webos {

class WEBOS_EXPORT WindowGroupLayerConfiguration {
 public:
  WindowGroupLayerConfiguration(const std::string& name, int z_order)
      : name_(name), z_order_(z_order) {}
  std::string GetName() const { return name_; }
  int GetZOrder() const { return z_order_; }

 private:
  std::string name_;
  int z_order_;
};

class WEBOS_EXPORT WindowGroupConfiguration {
 public:
  typedef std::pair<std::string, int> LayerEntry;
  typedef std::vector<LayerEntry> LayerList;

  WindowGroupConfiguration(const std::string& name);
  virtual ~WindowGroupConfiguration();

  std::string GetName() const;
  void SetIsAnonymous(bool is_anonymous);
  bool GetIsAnonymous() const;

  void AddLayer(const WindowGroupLayerConfiguration& layer_config);
  const std::vector<WindowGroupLayerConfiguration>& GetLayers() const;

 private:
  std::string name_;
  bool is_anonymous_;

  std::vector<WindowGroupLayerConfiguration> layers_;
  DISALLOW_COPY_AND_ASSIGN(WindowGroupConfiguration);
};

}  // namespace webos

#endif  // WEBOS_WINDOW_GROUP_CONFIGURATION_H_
