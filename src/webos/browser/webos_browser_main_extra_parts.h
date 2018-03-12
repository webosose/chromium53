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

#ifndef WEBOS_BROWSER_WEBOS_BROWSER_MAIN_EXTRA_PARTS_H_
#define WEBOS_BROWSER_WEBOS_BROWSER_MAIN_EXTRA_PARTS_H_

// Interface class for Parts owned by WebOSBrowserMainParts.
// The default implementation for all methods is empty.

// Most of these map to content::BrowserMainParts methods. This interface is
// separate to allow stages to be further subdivided for WebOS specific
// initialization stages (e.g. browser process init, profile init).

// While WebOSBrowserMainParts are platform-specific,
// WebOSBrowserMainExtraParts are used to do further initialization for various
// WebOS toolkits (e.g., AURA, etc.;)
namespace webos {

class WebOSBrowserMainExtraParts {
 public:
  virtual ~WebOSBrowserMainExtraParts() {}

  // EarlyInitialization methods.
  virtual void PreEarlyInitialization() {}
  virtual void PostEarlyInitialization() {}

  // ToolkitInitialized methods.
  virtual void ToolkitInitialized() {}

  // MainMessageLoopStart methods.
  virtual void PreMainMessageLoopStart() {}
  virtual void PostMainMessageLoopStart() {}

  // MainMessageLoopRun methods.
  virtual void PreCreateThreads() {}
  virtual void PreProfileInit() {}
  virtual void PostProfileInit() {}
  virtual void PreBrowserStart() {}
  virtual void PostBrowserStart() {}
  virtual void PreMainMessageLoopRun() {}
  virtual void PostMainMessageLoopRun() {}
};

} // namespace webos

#endif  // WEBOS_BROWSER_WEBOS_BROWSER_MAIN_EXTRA_PARTS_H_
