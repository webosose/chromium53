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

#ifndef WEBOS_APP_WEBOS_MAIN_H_
#define WEBOS_APP_WEBOS_MAIN_H_

#include "webos/common/webos_export.h"

namespace webos {

class WEBOS_EXPORT WebOSMainDelegate {
 public:
  virtual void AboutToCreateContentBrowserClient() = 0;
};

class WEBOS_EXPORT WebOSMain {
 public:
  explicit WebOSMain(WebOSMainDelegate* delegate);
  int Run(int argc, const char** argv);

 private:
  WebOSMainDelegate* delegate_;
};

}  // namespace webos

#endif  // WEBOS_APP_WEBOS_MAIN_H_
