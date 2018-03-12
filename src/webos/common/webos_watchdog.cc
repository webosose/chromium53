// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/common/webos_watchdog.h"

#include <signal.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "content/public/common/content_switches.h"

namespace webos {

static const int kDefaultWatchdogTimeout = 100;
static const int kDefaultWatchdogPeriod = 20;
static const int kWatchdogCleanupPeriod = 10;

WebOSWatchdog::WebOSWatchdog()
    : period_(kDefaultWatchdogPeriod),
      timeout_(kDefaultWatchdogTimeout),
      watching_tid_(0) {
}

WebOSWatchdog::~WebOSWatchdog() {
  if (watchdog_thread_) {
    watchdog_thread_->Disarm();
    watchdog_thread_->Cleanup();
  }
}

void WebOSWatchdog::StartWatchdog() {
  watchdog_thread_.reset(
      new WatchdogThread(base::TimeDelta::FromSeconds(timeout_), this));
}

void WebOSWatchdog::Arm() {
  watchdog_thread_->Arm();
}

WebOSWatchdog::WatchdogThread::WatchdogThread(const base::TimeDelta& duration,
                                              WebOSWatchdog* watchdog)
    : base::Watchdog(duration, "WebOSWatchdog", true), watchdog_(watchdog) {
}

void WebOSWatchdog::WatchdogThread::Alarm() {
// kill process
  RAW_PMLOG_INFO("WatchdogThread",
                 "Stuck detected in thread %d in process %d! Kill %d process",
                 watchdog_->WatchingThreadTid(),
                 getpid(),
                 getpid());
  kill(getpid(), SIGABRT);
}

}  // namespace webos
