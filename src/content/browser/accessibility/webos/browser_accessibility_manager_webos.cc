// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright (c) 2016-2017 LG Electronics, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/webos/browser_accessibility_manager_webos.h"

#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/webos/browser_accessibility_webos.h"

namespace content {

// static
BrowserAccessibilityManager* BrowserAccessibilityManager::Create(
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory) {
  return new BrowserAccessibilityManagerWebos(initial_tree, delegate, factory);
}

BrowserAccessibilityManagerWebos::BrowserAccessibilityManagerWebos(
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory)
    : BrowserAccessibilityManager(initial_tree, delegate, factory) {
}

BrowserAccessibilityManagerWebos::~BrowserAccessibilityManagerWebos() {}

void BrowserAccessibilityManagerWebos::NotifyAccessibilityEvent(
    BrowserAccessibilityEvent::Source source,
    ui::AXEvent event_type,
    BrowserAccessibility* node) {}

}  // namespace content
