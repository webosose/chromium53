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

#ifndef WEBOS_COMMON_WEBOS_WATCHDOG_H_
#define WEBOS_COMMON_WEBOS_WATCHDOG_H_

#include <memory>

#include "base/threading/watchdog.h"
#include "base/time/time.h"

namespace webos {

class WebOSWatchdog {
 public:
  WebOSWatchdog();
  virtual ~WebOSWatchdog();

  void StartWatchdog();
  void Arm();

  void SetPeriod(int period) { period_ = period; }
  int Period() { return period_; }

  void SetTimeout(int timeout) { timeout_ = timeout; }

  int WatchingThreadTid() { return watching_tid_; }
  void SetWatchingThreadTid(int tid) { watching_tid_ = tid; }

 private:
  class WatchdogThread : public base::Watchdog {
   public:
    WatchdogThread(const base::TimeDelta& duration, WebOSWatchdog* watchdog);

    void Alarm() override;

   private:
    WebOSWatchdog* watchdog_;
  };

  std::unique_ptr<base::Watchdog> watchdog_thread_;
  int watching_tid_;
  int period_;
  int timeout_;
};

}  // namespace webos

#endif /* WEBOS_COMMON_WEBOS_WATCHDOG_H_ */
