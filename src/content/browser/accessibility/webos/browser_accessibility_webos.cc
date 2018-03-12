// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2016-2017 LG Electronics, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/webos/browser_accessibility_webos.h"

using namespace blink;

namespace content {

// static
BrowserAccessibility* BrowserAccessibility::Create() {
  return new BrowserAccessibilityWebos();
}

BrowserAccessibilityWebos::BrowserAccessibilityWebos() {}

void BrowserAccessibilityWebos::OnDataChanged() {}

}  // namespace content
