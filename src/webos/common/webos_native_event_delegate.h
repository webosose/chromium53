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

#ifndef WEBOS_UI_WEBOS_NATIVE_EVENT_DELEGATE_H_
#define WEBOS_UI_WEBOS_NATIVE_EVENT_DELEGATE_H_

#include "ozone/platform/window_constants.h"

namespace webos {

class WebOSNativeEventDelegate {
 public:
  WebOSNativeEventDelegate() {}
  virtual ~WebOSNativeEventDelegate() {}

  virtual void CompositorBuffersSwapped() {}
  virtual void CursorVisibilityChange(bool visible) {}
  virtual void InputPanelHidden() {}
  virtual void InputPanelShown() {}
  virtual void InputPanelRectChanged() {}
  virtual bool IsEnableResize() { return true; }
  virtual void KeyboardEnter() {}
  virtual void KeyboardLeave() {}
  virtual void WindowHostClose() {}
  virtual void WindowHostExposed() {}
  virtual void WindowHostStateChanged(ui::WidgetState new_state) {}
  virtual void WindowHostStateAboutToChange(ui::WidgetState state) {}
};

}  // namespace webos

#endif  // WEBOS_UI_WEBOS_NATIVE_EVENT_DELEGATE_H_
