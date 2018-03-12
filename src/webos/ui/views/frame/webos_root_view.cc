// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/ui/views/frame/webos_root_view.h"

#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "webos/ui/views/frame/webos_view.h"

// static
const char WebOSRootView::kViewClassName[] =
    "webos/ui/views/frame/WebOSRootView";

WebOSRootView::WebOSRootView(WebOSView* view,
                             views::Widget* widget)
    : views::internal::RootView(widget),
      view_(view) {
}

bool WebOSRootView::GetDropFormats(
    int* formats,
    std::set<ui::Clipboard::FormatType>* format_types) {
  NOTIMPLEMENTED();
  return false;
}

bool WebOSRootView::AreDropTypesRequired() {
  return true;
}

bool WebOSRootView::CanDrop(const ui::OSExchangeData& data) {
  NOTIMPLEMENTED();
  return false;
}

void WebOSRootView::OnDragEntered(const ui::DropTargetEvent& event) {
  NOTIMPLEMENTED();
  return;
}

int WebOSRootView::OnDragUpdated(const ui::DropTargetEvent& event) {
  NOTIMPLEMENTED();
  return ui::DragDropTypes::DRAG_NONE;
}

void WebOSRootView::OnDragExited() {
  NOTIMPLEMENTED();
}

int WebOSRootView::OnPerformDrop(const ui::DropTargetEvent& event) {
  NOTIMPLEMENTED();
  return ui::DragDropTypes::DRAG_NONE;
}

const char* WebOSRootView::GetClassName() const {
  return kViewClassName;
}

bool WebOSRootView::OnMouseWheel(const ui::MouseWheelEvent& event) {
  return RootView::OnMouseWheel(event);
}

ui::EventDispatchDetails WebOSRootView::OnEventFromSource(ui::Event* event) {
  return RootView::OnEventFromSource(event);
}
