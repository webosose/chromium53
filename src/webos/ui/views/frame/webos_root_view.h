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

#ifndef WEBOS_VIEWS_FRAME_WEBOS_ROOT_VIEW_H_
#define WEBOS_VIEWS_FRAME_WEBOS_ROOT_VIEW_H_

#include "ui/views/widget/root_view.h"

class TabStrip;
class WebOSView;

namespace ui {
class OSExchangeData;
}

// RootView implementation used by WebOSWidget. This forwards drop events to
// the TabStrip. Visually the tabstrip extends to the top of the frame, but in
// actually it doesn't. The tabstrip is only as high as a tab. To enable
// dropping above the tabstrip BrowserRootView forwards drop events to the
// TabStrip.
class WebOSRootView : public views::internal::RootView {
 public:
  // Internal class name.
  static const char kViewClassName[];

  // You must call set_tabstrip before this class will accept drops.
  WebOSRootView(WebOSView* view, views::Widget* widget);

  // Overridden from views::View:
  bool GetDropFormats(
      int* formats,
      std::set<ui::Clipboard::FormatType>* format_types) override;
  bool AreDropTypesRequired() override;
  bool CanDrop(const ui::OSExchangeData& data) override;
  void OnDragEntered(const ui::DropTargetEvent& event) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  void OnDragExited() override;
  int OnPerformDrop(const ui::DropTargetEvent& event) override;
  const char* GetClassName() const override;
  bool OnMouseWheel(const ui::MouseWheelEvent& event) override;

 private:
  // ui::EventProcessor:
  ui::EventDispatchDetails OnEventFromSource(ui::Event* event) override;

  // The WebOSView.
  WebOSView* view_;

  DISALLOW_COPY_AND_ASSIGN(WebOSRootView);
};

#endif  // WEBOS_VIEWS_FRAME_WEBOS_ROOT_VIEW_H_
