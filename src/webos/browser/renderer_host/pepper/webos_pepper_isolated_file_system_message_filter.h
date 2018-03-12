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

#ifndef WEBOS_PEPPER_ISOLATED_FILE_SYSTEM_MESSAGE_FILTER_H
#define WEBOS_PEPPER_ISOLATED_FILE_SYSTEM_MESSAGE_FILTER_H

#include <set>
#include <string>

#include "base/files/file_path.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/private/ppb_isolated_file_system_private.h"
#include "ppapi/host/resource_host.h"
#include "ppapi/host/resource_message_filter.h"

namespace content {
class BrowserPpapiHost;
}

namespace ppapi {
namespace host {
struct HostMessageContext;
}  // namespace host
}  // namespace ppapi

namespace webos {

class WebOSPepperIsolatedFileSystemMessageFilter
    : public ppapi::host::ResourceMessageFilter {
 public:
  static WebOSPepperIsolatedFileSystemMessageFilter* Create(
      PP_Instance instance,
      content::BrowserPpapiHost* host);

  // ppapi::host::ResourceMessageFilter implementation.
  scoped_refptr<base::TaskRunner> OverrideTaskRunnerForMessage(
      const IPC::Message& msg) override;
  int32_t OnResourceMessageReceived(
      const IPC::Message& msg,
      ppapi::host::HostMessageContext* context) override;

 private:
  WebOSPepperIsolatedFileSystemMessageFilter(
      int render_process_id,
      ppapi::host::PpapiHost* ppapi_host_);

  ~WebOSPepperIsolatedFileSystemMessageFilter() override;

  int32_t OnOpenFileSystem(ppapi::host::HostMessageContext* context,
                           PP_IsolatedFileSystemType_Private type);
  int32_t OpenPluginPrivateFileSystem(ppapi::host::HostMessageContext* context);

  const int render_process_id_;

  ppapi::host::PpapiHost* ppapi_host_;

  DISALLOW_COPY_AND_ASSIGN(WebOSPepperIsolatedFileSystemMessageFilter);
};

}  // namespace webos

#endif  // WEBOS_PEPPER_ISOLATED_FILE_SYSTEM_MESSAGE_FILTER_H
