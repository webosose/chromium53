// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef CONTENT_BROWSER_WEBOS_RENDER_VIEW_HOST_WEBOS_H_
#define CONTENT_BROWSER_WEBOS_RENDER_VIEW_HOST_WEBOS_H_

#include "base/memory/memory_pressure_listener.h"
#include "ui/base/ime/text_input_type.h"

namespace content {

class CONTENT_EXPORT RenderViewHostWebos {
 public:
  RenderViewHostWebos() {}
  virtual ~RenderViewHostWebos() {}

  virtual void NotifyMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  }

 private:
  // This interface should only be implemented inside content.
  friend class RenderViewHostImpl;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBOS_RENDER_VIEW_HOST_WEBOS_H_
