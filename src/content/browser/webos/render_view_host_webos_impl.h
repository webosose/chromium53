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

#ifndef CONTENT_BROWSER_WEBOS_RENDER_VIEW_HOST_WEBOS_IMPL_H_
#define CONTENT_BROWSER_WEBOS_RENDER_VIEW_HOST_WEBOS_IMPL_H_

#include "content/browser/renderer_host/render_view_host_impl.h"

namespace content {

class CONTENT_EXPORT RenderViewHostWebosImpl : public RenderViewHostImpl {
 public:
  RenderViewHostWebosImpl(SiteInstance* instance,
                          std::unique_ptr<RenderWidgetHostImpl> widget,
                          RenderViewHostDelegate* delegate,
                          int32_t main_frame_routing_id,
                          bool swapped_out,
                          bool has_initialized_audio_host);
  ~RenderViewHostWebosImpl() override;

  // RenderViewHostWebos overridden
  void NotifyMemoryPressure(base::MemoryPressureListener::MemoryPressureLevel
                                memory_pressure_level) override;

  // RenderViewHostImpl overridden
  bool OnMessageReceived(const IPC::Message& msg) override;
  void OnWebkitPreferencesChanged() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderViewHostWebosImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBOS_RENDER_VIEW_HOST_WEBOS_IMPL_H_
