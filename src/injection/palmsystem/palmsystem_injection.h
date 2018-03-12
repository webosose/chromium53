// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#ifndef CHROME_INJECTION_PALMSYSTEM_INJECTION_H_
#define CHROME_INJECTION_PALMSYSTEM_INJECTION_H_

#include "v8/include/v8.h"
#include "base/compiler_specific.h"
#include "content/public/renderer/injection/injection_data_manager.h"

#if defined(COMPONENT_BUILD) || defined(USE_DYNAMIC_INJECTION_LOADING)

#if defined(PALMSYSTEM_IMPLEMENTATION)
#define PALMSYSTEM_EXPORT __attribute__((visibility("default")))
#else
#define PALMSYSTEM_EXPORT
#endif // defined(PALMSYSTEM_IMPLEMENTATION)

#else
 #define PALMSYSTEM_EXPORT
#endif // defined(COMPONENT_BUILD) || defined(USE_DYNAMIC_INJECTION_LOADING)

#if defined(USE_DYNAMIC_INJECTION_LOADING)

extern "C" {
namespace extensions_v8 {

PALMSYSTEM_EXPORT v8::Extension* createInjection();

} // namespace extensions_v8
} // extern "C"

#else // defined(USE_DYNAMIC_INJECTION_LOADING)

namespace extensions_v8 {

class PALMSYSTEM_EXPORT PalmSystemInjectionExtension {
 public:
  static v8::Extension* Get();
};

} // namespace extensions_v8

#endif // defined(USE_DYNAMIC_INJECTION_LOADING)
#endif // CHROME_INJECTION_PALMSYSTEM_INJECTION_H_
