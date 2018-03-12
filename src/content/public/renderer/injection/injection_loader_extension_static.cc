// Copyright (c) 2014 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/injection/injection_loader_extension.h"
#include "injection/palmsystem/palmsystem_injection.h"

#include "v8/include/v8.h"

#include <string>

namespace {

  const char kPalmSystemInjection[] = "palmsystem";

} // namespace

namespace extensions_v8 {

v8::Extension* InjectionLoaderExtension::Get(
    const std::string& name) {
  if (name == kPalmSystemInjection)
    return extensions_v8::PalmSystemInjectionExtension::Get();
  return NULL;
}

} // namespace extensions_v8
