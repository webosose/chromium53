// Copyright (c) 2014 LG Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/injection/injection_observer.h"

#include "content/common/injection_messages.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/injection/injection_loader_extension.h"
#include "content/renderer/render_view_impl.h"

namespace content {

InjectionObserver::InjectionObserver(RenderViewImpl* render_view)
    : RenderViewObserver(render_view) {
}

InjectionObserver::~InjectionObserver() {
}

bool InjectionObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(InjectionObserver, message)
    IPC_MESSAGE_HANDLER(InjectionMsg_LoadExtension, OnLoadExtension)
    IPC_MESSAGE_HANDLER(InjectionMsg_ClearExtensions, OnClearExtensions)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void InjectionObserver::OnLoadExtension(const std::string& extension) {
  RenderThread* thread = RenderThread::Get();
  if (thread) {
    v8::Extension* const ext =
        extensions_v8::InjectionLoaderExtension::Get(extension);
    if (ext)
      thread->RegisterExtension(ext);
    else
      LOG(ERROR) << "The extension for injection is not created";
  }
  else
    LOG(ERROR) << "RenderThread is NULL";
}

void InjectionObserver::OnClearExtensions() {
  RenderThread* thread = RenderThread::Get();
  if (thread) {
    thread->ClearExtensions();
  }
}

void InjectionObserver::OnDestruct() {
  // TODO: This function probably should be implemented
  // see render_view_observer.h comment on its declaration.
}

} // namespace content
