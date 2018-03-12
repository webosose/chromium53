// Copyright (c) 2015 -2017 LG Electronics, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/webos/accessibility_localization_webos.h"

namespace content {

// static
AccessibilityLocalizationWebos* AccessibilityLocalizationWebos::GetInstance() {
  return base::Singleton<AccessibilityLocalizationWebos>::get();
}

AccessibilityLocalizationWebos::AccessibilityLocalizationWebos() {
}

void AccessibilityLocalizationWebos::SetLocale(std::string language) {
}

}
