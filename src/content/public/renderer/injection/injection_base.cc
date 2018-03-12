// Copyright (c) 2015 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "content/common/browser_control_messages.h"
#include "content/public/renderer/injection/injection_base.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace extensions_v8 {

InjectionBase::InjectionBase() {
}

void InjectionBase::NotImplemented(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
#ifdef USE_BASE_LOGGING
  LOG(ERROR) << "error: 'NotImplemented' function call";
#endif
}

void InjectionBase::SendCommand(
    const std::string& function) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "command=" << function;
#endif
   std::vector<std::string> arguments;
   SendCommand(function, arguments);
}

void InjectionBase::SendCommand(
    const std::string& function,
    const std::vector<std::string>& arguments) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "command=" << function;
#endif
  blink::WebFrame* frame = blink::WebLocalFrame::frameForCurrentContext();
  if(!frame)
    return;
  content::RenderView* view = content::RenderView::FromWebView(frame->view());
  if(!view)
    return;
  view->Send(new BrowserControlMsg_Command(view->GetRoutingID(), function, arguments));
}

std::string InjectionBase::CallFunction(
    const std::string& function) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "function=" << function;
#endif
  std::vector<std::string> arguments;
  return CallFunction(function, arguments);
}

std::string InjectionBase::CallFunction(
    const std::string& function,
    const std::vector<std::string>& arguments) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "function=" << function;
#endif
  std::string result;
  blink::WebFrame* frame = blink::WebLocalFrame::frameForCurrentContext();
  if(!frame)
    return result;
  content::RenderView* view = content::RenderView::FromWebView(frame->view());
  if(!view)
    return result;
  view->Send(new BrowserControlMsg_Function(view->GetRoutingID(), function, arguments, &result));
  return result;
}

std::string InjectionBase::checkFileValidation(
  const std::string& file, const std::string& folder) {
  std::string file_ = file;
  if(base::StartsWith(file, "file://", base::CompareCase::SENSITIVE)) {
    // Starts with "file://" then remove this, but the file path is absolute
    file_.erase(file_.begin(), file_.begin()+7);
  } else if(!base::StartsWith(file, "/", base::CompareCase::SENSITIVE)) {
    // Just use file path which is based on app folder path
    // make it absolute file path : concatenate folder path and file path
    file_ = folder + "/" + file_;
  }

  // then the file_ is absolute file path
  base::FilePath appFolder = base::FilePath(folder);
  base::FilePath filePath = base::FilePath(file_);
  if(appFolder.IsParent(filePath) && base::PathExists(filePath))
    return file_;
  else
    return "";
}

void InjectionBase::setKeepAliveWebApp(bool keepAlive) {
  blink::WebFrame* frame = blink::WebLocalFrame::frameForCurrentContext();
  if (!frame)
    return;
  content::RenderView* view = content::RenderView::FromWebView(frame->view());
  if (!view)
    return;
  view->SetKeepAliveWebApp(keepAlive);
}

} // namespace extensions_v8
