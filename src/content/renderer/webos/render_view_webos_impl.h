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

#ifndef CONTENT_RENDERER_WEBOS_RENDER_VIEW_WEBOS_IMPL_H_
#define CONTENT_RENDERER_WEBOS_RENDER_VIEW_WEBOS_IMPL_H_

#include "base/timer/timer.h"
#include "content/renderer/render_view_impl.h"

namespace content {

class CONTENT_EXPORT RenderViewWebosImpl : public RenderViewImpl {
 public:
  RenderViewWebosImpl(CompositorDependencies* compositor_deps,
                      const ViewMsg_New_Params& params);
  ~RenderViewWebosImpl() override;

  // RenderViewWebos overridden
  void Initialize(const ViewMsg_New_Params& params,
                  bool was_created_by_renderer) override;

  // RenderViewImpl overridden
  bool OnMessageReceived(const IPC::Message& msg) override;

  // blink::WebViewClient overridden
  blink::WebView* createView(blink::WebLocalFrame* creator,
                             const blink::WebURLRequest& request,
                             const blink::WebWindowFeatures& features,
                             const blink::WebString& frame_name,
                             blink::WebNavigationPolicy policy,
                             bool suppress_opener) override;
  blink::WebWidget* createPopupMenu(blink::WebPopupType popup_type) override;

 private:
  void ChangePriority(bool raise_nice);
  void ChangePriorityDone();

  DISALLOW_COPY_AND_ASSIGN(RenderViewWebosImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_WEBOS_RENDER_VIEW_WEBOS_IMPL_H_
