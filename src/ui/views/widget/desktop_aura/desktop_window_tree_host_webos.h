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

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_WEBOS_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_WEBOS_H_

#include "ozone/platform/window_constants.h"
#include "ui/platform_window/webos/webos_xinput_types.h"
#include "webos/common/webos_constants.h"

namespace ui {
class EventHandler;
class InputMethod;
class WindowGroupConfiguration;
};

class SkBitmap;

namespace gfx {
class Point;
}

namespace views {

class DesktopWindowTreeHostWebOS {
 public:
  virtual ~DesktopWindowTreeHostWebOS() {}

  virtual ui::InputMethod* GetInputMethod() = 0;
  virtual unsigned GetWindowHandle() = 0;
  virtual void SetCustomCursor(webos::CustomCursorType type,
                               const std::string& path,
                               int hotspot_x,
                               int hotspot_y) = 0;
  virtual void SetPlatformCursor(const SkBitmap* cursor_bitmap,
                                 const gfx::Point& location) = 0;
  virtual void SetKeyMask(webos::WebOSKeyMask key_mask, bool value) = 0;
  virtual void SetOpacity(float opacity) = 0;
  virtual void SetWindowProperty(const std::string& name,
                                 const std::string& value) = 0;
  virtual void SetWindowHostState(ui::WidgetState state) = 0;

  virtual void AddPreTargetHandler(ui::EventHandler* handler) = 0;
  virtual void Resize(int width, int height) = 0;
  virtual void ResetCompositor() = 0;
  virtual void WebAppWindowHidden() = 0;

  virtual void CreateGroup(const ui::WindowGroupConfiguration& config) = 0;
  virtual void AttachToGroup(const std::string& group,
                             const std::string& layer) = 0;
  virtual void FocusGroupOwner() = 0;
  virtual void FocusGroupLayer() = 0;
  virtual void DetachGroup() = 0;
  virtual void WebOSXInputActivate(const std::string& type) = 0;
  virtual void WebOSXInputDeactivate() = 0;
  virtual void WebOSXInputInvokeAction(
      uint32_t keysym,
      ui::WebOSXInputSpecialKeySymbolType symbol_type,
      ui::WebOSXInputEventType event_type) = 0;
};

}  // namespace webos

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_DESKTOP_WINDOW_TREE_HOST_WEBOS_H_
