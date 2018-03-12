// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#ifndef CONTENT_RENDERER_INJECTION_DATA_MANAGER_H_
#define CONTENT_RENDERER_INJECTION_DATA_MANAGER_H_

#include <string>
#include <map>
#include <vector>
#include "content/common/content_export.h"

namespace extensions_v8 {

class CONTENT_EXPORT InjectionDataManager {
 public:
  InjectionDataManager();
  virtual ~InjectionDataManager();
  void Initialize(const std::string& json,
                  const std::vector<std::string>& data_keys);
  void SetInjectionData(const std::string& key, const std::string& value);
  bool UpdateInjectionData(const std::string& key, const std::string& value);
  bool GetInjectionData(const std::string& key, std::string& value) const;

 private:
  std::map<std::string, std::string> injection_data_;
};

}  // namespace extensions_v8

#endif  // CONTENT_RENDERER_INJECTION_DATA_MANAGER_H_