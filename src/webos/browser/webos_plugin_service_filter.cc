// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/browser/webos_plugin_service_filter.h"

#include "content/public/common/webplugininfo.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

namespace webos {

bool WebOSPluginServiceFilter::IsPluginAvailable(
    int render_process_id,
    int render_frame_id,
    const void* context,
    const GURL& url,
    const GURL& policy_url,
    content::WebPluginInfo* plugin) {
  if (plugin->type != content::WebPluginInfo::PLUGIN_TYPE_NPAPI)
    return true;

  if (render_frame_id == MSG_ROUTING_NONE)
    return true;

  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);

  if (web_contents) {
    std::string current_dir =
        plugin ? plugin->path.DirName().AsUTF8Unsafe() : std::string();
    std::vector<std::string> available_dirs = web_contents->GetAvailablePluginDirs();
    std::vector<std::string>::const_iterator it =
        std::find(available_dirs.begin(), available_dirs.end(), current_dir);
    if (it != available_dirs.end())
      return true;
  }

  return false;
}

bool WebOSPluginServiceFilter::CanLoadPlugin(int render_process_id,
                                             const base::FilePath& path) {
  return true;
}

} // namespace webos
