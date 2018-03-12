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

#ifndef WEBOS_COMMON_WEBOS_CONTENT_CLIENT_H_
#define WEBOS_COMMON_WEBOS_CONTENT_CLIENT_H_

#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_piece.h"
#include "content/public/common/content_client.h"

namespace webos {

class WebOSContentClient : public content::ContentClient {
 public:
  virtual ~WebOSContentClient();

  //content::ContentClient implementation
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(
      int resource_id) const override;
  gfx::Image& GetNativeImageNamed(int resource_id) const override;
  std::string GetUserAgent() const override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;
};

} // namespace webos

#endif /* WEBOS_COMMON_WEBOS_CONTENT_CLIENT_H_ */
