// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/webos_webview_renderer_state.h"

#include "content/public/browser/browser_thread.h"

namespace webos {

// static
WebOSWebViewRendererState* WebOSWebViewRendererState::GetInstance() {
  return base::Singleton<WebOSWebViewRendererState>::get();
}

WebOSWebViewRendererState::WebOSWebViewRendererState() {}

WebOSWebViewRendererState::~WebOSWebViewRendererState() {}

void WebOSWebViewRendererState::RegisterWebViewInfo(
    int render_process_id,
    int routing_id,
    const WebViewInfo& webview_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  RenderId render_id(render_process_id, routing_id);
  webview_info_map_[render_id] = webview_info;
}

void WebOSWebViewRendererState::UnRegisterWebViewInfo(int render_process_id,
                                                      int routing_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  RenderId render_id(render_process_id, routing_id);
  webview_info_map_.erase(render_id);
}

bool WebOSWebViewRendererState::GetInfo(int render_process_id,
                                        int routing_id,
                                        WebViewInfo* webview_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  RenderId render_id(render_process_id, routing_id);
  WebViewInfoMap::iterator iter = webview_info_map_.find(render_id);
  if (iter != webview_info_map_.end()) {
    *webview_info = iter->second;
    return true;
  }
  return false;
}

}  // namespace webos
