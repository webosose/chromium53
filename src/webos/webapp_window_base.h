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

#ifndef WEBOS_WEBAPP_WINDOW_BASE_H_
#define WEBOS_WEBAPP_WINDOW_BASE_H_

#include "base/macros.h"
#include "webos/common/webos_constants.h"
#include "webos/webapp_window_delegate.h"
#include "webos/window_group_configuration.h"

#include <memory>

namespace content {
class WebContents;
}

namespace webos {

class WebAppWindow;
class WindowGroupConfiguration;

class WEBOS_EXPORT WebAppWindowBase : public WebAppWindowDelegate {
 public:
  WebAppWindowBase();
  virtual ~WebAppWindowBase();

  virtual void Show();
  virtual void Hide();

#if defined(OS_WEBOS)
  virtual void SetCustomCursor(CustomCursorType type,
                               const std::string& path,
                               int hotspot_x,
                               int hotspot_y);
#endif
  int DisplayWidth();
  int DisplayHeight();

  virtual void AttachWebContents(void* web_contents);
  virtual void DetachWebContents();
  virtual void RecreatedWebContents();

  NativeWindowState GetWindowHostState() const;
  NativeWindowState GetWindowHostStateAboutToChange() const;
  void SetWindowHostState(NativeWindowState state);
#if defined(OS_WEBOS)
  unsigned GetWindowHandle();
#endif
  void SetKeyMask(WebOSKeyMask key_mask, bool value);
  void SetWindowProperty(const std::string& name, const std::string& value);
  void SetWindowSurfaceId(int surface_id);
  void SetOpacity(float opacity);
  void Resize(int width, int height);
  void SetScaleFactor(float scale);
  void SetUseVirtualKeyboard(bool enable);
  void InitWindow(int width, int height);

  void CreateWindowGroup(const WindowGroupConfiguration& config);
  void AttachToWindowGroup(const std::string& name, const std::string& layer);
  void FocusWindowGroupOwner();
  void FocusWindowGroupLayer();
  void DetachWindowGroup();

#if defined(OS_WEBOS)
  void XInputActivate(const std::string& type = std::string());
  void XInputDeactivate();
  void XInputInvokeAction(uint32_t keysym,
                          SpecialKeySymbolType symType = QT_KEY_SYMBOL,
                          XInputEventType eventType = XINPUT_PRESS_AND_RELEASE);
#endif

 private:
  std::unique_ptr<WebAppWindow> webapp_window_;
  int pending_surface_id_;

  DISALLOW_COPY_AND_ASSIGN(WebAppWindowBase);
};

}  // namespace webos

#endif  // WEBOS_WEBAPP_WINDOW_BASE_H_
