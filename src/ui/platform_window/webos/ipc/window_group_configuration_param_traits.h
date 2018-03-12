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

#ifndef UI_PLATFORM_WINDOW_WEBOS_IPC_WINDOW_GROUP_CONFIGURATION_PARAM_TRAITS_H_
#define UI_PLATFORM_WINDOW_WEBOS_IPC_WINDOW_GROUP_CONFIGURATION_PARAM_TRAITS_H_

#include "ui/platform_window/webos/ipc/platform_window_webos_ipc_export.h"
#include "ui/platform_window/webos/window_group_configuration.h"

namespace IPC {
template <>
struct PLATFORM_WINDOW_WEBOS_IPC_EXPORT
    ParamTraits<ui::WindowGroupConfiguration> {
  typedef ui::WindowGroupConfiguration param_type;
  static void GetSize(base::PickleSizer* s, const param_type& p);
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* p);
  static void Log(const param_type& p, std::string* l);
};
}  // namespace IPC

#endif  // UI_PLATFORM_WINDOW_WEBOS_IPC_WINDOW_GROUP_CONFIGURATION_PARAM_TRAITS_H_
