// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#ifndef LG_PAL_WEBOS_PAL_PAL_WEBOS_H_
#define LG_PAL_WEBOS_PAL_PAL_WEBOS_H_

#include <memory>
#include <mutex>
#include "lg/pal/public/pal.h"

namespace pal {

class PalWebos : public Pal {
 public:
  PalWebos();
  ~PalWebos() override;
};

}  // namespace pal

#endif  // LG_PAL_WEBOS_PAL_PAL_WEBOS_H_
