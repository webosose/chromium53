// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef BASE_MEMORY_MEMORY_PRESSURE_MONITOR_WEBOS_H_
#define BASE_MEMORY_MEMORY_PRESSURE_MONITOR_WEBOS_H_

#include "base/base_export.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/timer/timer.h"
#include "base/webos/luna_services.h"

namespace base {
namespace webos {

class BASE_EXPORT MemoryPressureMonitor : public base::MemoryPressureMonitor,
                                          public LunaServicesDelegate {
 public:
  using MemoryPressureLevel = base::MemoryPressureListener::MemoryPressureLevel;

  MemoryPressureMonitor(const std::string& app_id);
  virtual ~MemoryPressureMonitor();

  MemoryPressureLevel GetCurrentPressureLevel() const override {
    return current_level_;
  }
  void NotifyCurrentMemoryPressure();

  // LunaServicesDelegate
  void HandleMemoryInfo(const std::string& memory_info,
                        const std::string& remain_count) override;

 private:
  MemoryPressureLevel current_level_ =
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
  base::RepeatingTimer timer_;
  LunaServices luna_services_;
  DISALLOW_COPY_AND_ASSIGN(MemoryPressureMonitor);
};

}  // namespace webos
}  // namespace base

#endif  // BASE_MEMORY_MEMORY_PRESSURE_MONITOR_WEBOS_H_
