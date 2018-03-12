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

#ifndef WEBOS_WAYLAND_PLATFORM_WEBOS_PLATFORM_H_
#define WEBOS_WAYLAND_PLATFORM_WEBOS_PLATFORM_H_

#include <vector>

#include "base/macros.h"
#include "webos/common/webos_export.h"
#include "ui/gfx/geometry/rect.h"
#include "webos/common/webos_constants.h"
#include "webos/public/runtime_delegates.h"

namespace webos {

class WEBOS_EXPORT InputPointer {
 public:
  InputPointer() : visible_(false) { }
  virtual ~InputPointer() { }

  void SetVisible(bool visible) { visible_ = visible; }
  bool IsVisible() { return visible_; }

  virtual void OnCursorVisibilityChanged(bool visible) = 0;

 private:
  bool visible_;
};

class WEBOS_EXPORT WebOSPlatform : public PlatformDelegate {
 public:
  static WebOSPlatform* GetInstance();
  ~WebOSPlatform();

  // Overridden from PlatformDelegate:
  void OnCursorVisibilityChanged(bool visible) override;
  void OnNetworkStateChanged(bool is_connected) override;
  void OnLocaleInfoChanged(std::string language) override;

  void SetInputPointer(InputPointer* input_pointer);
  InputPointer* GetInputPointer();

  void SetInputRegion(unsigned handle, std::vector<gfx::Rect>region);
  void SetKeyMask(unsigned handle, webos::WebOSKeyMask keyMask);

 private:
  WebOSPlatform();

  InputPointer* input_pointer_;
  static WebOSPlatform* webos_platform_;

  DISALLOW_COPY_AND_ASSIGN(WebOSPlatform);
};

}  // namespace webos

#endif  // WEBOS_WAYLAND_PLATFORM_WEBOS_PLATFORM_H_
