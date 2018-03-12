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

#ifndef CONTENT_BROWSER_ACCESSIBILITY_WEBOS_ACCESSIBILITY_LOCALIZATION_WEBOS_H_
#define CONTENT_BROWSER_ACCESSIBILITY_WEBOS_ACCESSIBILITY_LOCALIZATION_WEBOS_H_

#include "base/memory/singleton.h"
#include "content/common/content_export.h"

namespace content {

class CONTENT_EXPORT AccessibilityLocalizationWebos {
 public:
  static AccessibilityLocalizationWebos* GetInstance();

  void SetLocale(std::string language);

   private:
  AccessibilityLocalizationWebos();
  friend struct base::DefaultSingletonTraits<AccessibilityLocalizationWebos>;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityLocalizationWebos);
};
}

#endif  // CONTENT_BROWSER_ACCESSIBILITY_WEBOS_ACCESSIBILITY_LOCALIZATION_WEBOS_H_
