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

#ifndef CONTENT_PUBLIC_RENDERER_INJECTION_INJECTION_BASE_H_
#define CONTENT_PUBLIC_RENDERER_INJECTION_INJECTION_BASE_H_

#include "v8/include/v8.h"

namespace extensions_v8 {

class CONTENT_EXPORT InjectionBase {
 public:
  InjectionBase();

 protected:
  static void NotImplemented(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  static void SendCommand(
      const std::string& function);
  static void SendCommand(
      const std::string& function,
      const std::vector<std::string>& arguments);
  static std::string CallFunction(
      const std::string& function);
  static std::string CallFunction(
      const std::string& function,
      const std::vector<std::string>& arguments);
  static std::string checkFileValidation(
      const std::string& file,
      const std::string& folder);
  static void setKeepAliveWebApp(bool keepAlive);
};

} // namespace extensions_v8

#endif  // CONTENT_PUBLIC_RENDERER_INJECTION_INJECTION_BASE_H_
