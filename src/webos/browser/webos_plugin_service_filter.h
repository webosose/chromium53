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

#ifndef WEBOS_BROWSER_WEBOS_PLUGIN_SERVICE_FILTER_H_
#define WEBOS_BROWSER_WEBOS_PLUGIN_SERVICE_FILTER_H_

#include "content/public/browser/plugin_service_filter.h"

namespace webos {

class WebOSPluginServiceFilter : public content::PluginServiceFilter {
 public:
  WebOSPluginServiceFilter() {}
  ~WebOSPluginServiceFilter() {}

  virtual bool IsPluginAvailable(int render_process_id,
                                 int render_frame_id,
                                 const void* context,
                                 const GURL& url,
                                 const GURL& policy_url,
                                 content::WebPluginInfo* plugin);

  virtual bool CanLoadPlugin(int render_process_id,
                             const base::FilePath& path);
};

} // namespace webos

#endif  // WEBOS_BROWSER_WEBOS_PLUGIN_SERVICE_FILTER_H_
