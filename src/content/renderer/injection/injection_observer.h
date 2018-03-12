// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#ifndef CONTENT_RENDERER_INJECTION_OBSERVER_H_
#define CONTENT_RENDERER_INJECTION_OBSERVER_H_

#include <string>
#include "base/macros.h"
#include "content/public/renderer/render_view_observer.h"

namespace content {

class InjectionObserver : public RenderViewObserver {
 public:
  explicit InjectionObserver(RenderViewImpl* render_view);
  virtual ~InjectionObserver();

  // RenderViewObserver overrides:
  virtual bool OnMessageReceived(const IPC::Message& message) override;

  virtual void OnDestruct();
 private:
  // IPC Message handlers:
  void OnLoadExtension(const std::string& extension);
  void OnClearExtensions();

  DISALLOW_COPY_AND_ASSIGN(InjectionObserver);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INJECTION_OBSERVER_H_

