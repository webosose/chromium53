// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef WEBOS_PEPPER_HOST_FACTORY_H
#define WEBOS_PEPPER_HOST_FACTORY_H

#include "base/macros.h"
#include "ppapi/host/host_factory.h"

namespace content {
class BrowserPpapiHost;
}  // namespace content

namespace webos {

class WebOSPepperHostFactory : public ppapi::host::HostFactory {
 public:
  // Non-owning pointer to the filter must outlive this class.
  explicit WebOSPepperHostFactory(content::BrowserPpapiHost* host);
  ~WebOSPepperHostFactory() override;

  std::unique_ptr<ppapi::host::ResourceHost> CreateResourceHost(
      ppapi::host::PpapiHost* host,
      PP_Resource resource,
      PP_Instance instance,
      const IPC::Message& message) override;

 private:
  // Non-owning pointer.
  content::BrowserPpapiHost* host_;

  DISALLOW_COPY_AND_ASSIGN(WebOSPepperHostFactory);
};

}  // namespace webos

#endif  // WEBOS_PEPPER_HOST_FACTORY_H
