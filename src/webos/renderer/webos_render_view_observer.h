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

#ifndef WEBOS_RENDERER_WEBOS_RENDER_VIEW_OBSERVER_H_
#define WEBOS_RENDERER_WEBOS_RENDER_VIEW_OBSERVER_H_

#include "base/memory/memory_pressure_listener.h"
#include "content/public/renderer/render_view_observer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/WebKit/public/platform/WebPageVisibilityState.h"

namespace content {
class RenderView;
}

namespace webos {

class WebOSRenderViewObserver : public content::RenderViewObserver {
 public:
  WebOSRenderViewObserver(content::RenderView* render_view);

  // RenderViewObserver
  void DidClearWindowObject(blink::WebLocalFrame* frame) override;

  // IPC message handlers
  void OnSetVisibilityState(blink::WebPageVisibilityState visibilityState);
  void OnSetBackgroundColor(SkColor color);
  void OnSetViewportSize(int width, int height);
  void OnDestruct();

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
};

} // namespace webos

#endif // WEBOS_RENDERER_WEBOS_RENDER_VIEW_OBSERVER_H_
