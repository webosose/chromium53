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

#include "base/logging.h"
#include "base/memory/memory_pressure_monitor_webos.h"

#define INFO_LOG(format, ...)                                       \
  RAW_PMLOG_INFO("MemoryPressureMonitorWebOS", "[%s:%04d] " format, \
                 __FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace base {
namespace webos {

MemoryPressureMonitor::MemoryPressureMonitor(const std::string& app_id)
    : luna_services_(this) {
  luna_services_.Initialize(app_id);
}

MemoryPressureMonitor::~MemoryPressureMonitor() {
  luna_services_.Finalize();
}

void MemoryPressureMonitor::NotifyCurrentMemoryPressure() {
  INFO_LOG();
  base::MemoryPressureListener::NotifyMemoryPressure(current_level_);
}

void MemoryPressureMonitor::HandleMemoryInfo(const std::string& memory_info,
                                             const std::string& remain_count) {
  INFO_LOG("webOS memory pressure level %s", memory_info.c_str());
  TimeDelta timer_delta;
  if (memory_info == "low" || memory_info == "critical") {
    current_level_ =
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
    timer_delta = TimeDelta::FromSeconds(5);
  } else if (memory_info == "medium") {
    current_level_ =
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;
    timer_delta = TimeDelta::FromSeconds(30);
  } else if (memory_info == "normal") {
    current_level_ = base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
  }

  timer_.Stop();
  if (current_level_ !=
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE) {
    INFO_LOG("notifying memory pressure");
    base::MemoryPressureListener::NotifyMemoryPressure(current_level_);
    timer_.Start(FROM_HERE, timer_delta, this,
                 &MemoryPressureMonitor::NotifyCurrentMemoryPressure);
  }
}

}  // namespace webos
}  // namespace base
