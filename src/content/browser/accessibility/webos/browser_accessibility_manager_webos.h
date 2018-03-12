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

#ifndef CONTENT_BROWSER_ACCESSIBILITY_WEBOS_BROWSER_ACCESSIBILITY_MANAGER_WEBOS_H_
#define CONTENT_BROWSER_ACCESSIBILITY_WEBOS_BROWSER_ACCESSIBILITY_MANAGER_WEBOS_H_

#include "base/memory/ptr_util.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"

namespace content {

// Manages a tree of BrowserAccessibilityWebos objects.
class CONTENT_EXPORT BrowserAccessibilityManagerWebos
    : public BrowserAccessibilityManager {
 public:
  BrowserAccessibilityManagerWebos(
      const ui::AXTreeUpdate& initial_tree,
      BrowserAccessibilityDelegate* delegate,
      BrowserAccessibilityFactory* factory = new BrowserAccessibilityFactory());

  virtual ~BrowserAccessibilityManagerWebos();

  // BrowserAccessibilityManager methods
  virtual void NotifyAccessibilityEvent(
      BrowserAccessibilityEvent::Source source,
      ui::AXEvent event_type,
      BrowserAccessibility* node) override;

  // Give BrowserAccessibilityManager::Create access to our constructor.
  friend class BrowserAccessibilityManager;

  DISALLOW_COPY_AND_ASSIGN(BrowserAccessibilityManagerWebos);
};
}  // namespace content

#endif  // CONTENT_BROWSER_ACCESSIBILITY_WEBOS_BROWSER_ACCESSIBILITY_MANAGER_WEBOS_H_
