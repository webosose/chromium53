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

#ifndef WEBOS_COMMON_WEBOS_RESOURCE_DELEGATE_H_
#define WEBOS_COMMON_WEBOS_RESOURCE_DELEGATE_H_

#include "ui/base/resource/resource_bundle.h"

namespace webos {

class WebosResourceDelegate : public ui::ResourceBundle::Delegate {
 public:
  static void InitializeResourceBundle();

  WebosResourceDelegate() {}
  ~WebosResourceDelegate() override {}

  // ui:ResourceBundle::Delegate implementation:
  base::FilePath GetPathForResourcePack(const base::FilePath& pack_path,
                                        ui::ScaleFactor scale_factor) override;
  base::FilePath GetPathForLocalePack(const base::FilePath& pack_path,
                                      const std::string& locale) override;
  gfx::Image GetImageNamed(int resource_id) override;
  gfx::Image GetNativeImageNamed(int resource_id) override;
  base::RefCountedStaticMemory* LoadDataResourceBytes(
      int resource_id,
      ui::ScaleFactor scale_factor) override;
  bool GetRawDataResource(int resource_id,
                          ui::ScaleFactor scale_factor,
                          base::StringPiece* value) override;
  bool GetLocalizedString(int message_id, base::string16* value) override;
  bool LoadBrowserResources() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebosResourceDelegate);
};

}  // namespace

#endif  //  WEBOS_COMMON_WEBOS_RESOURCE_DELEGATE_H_