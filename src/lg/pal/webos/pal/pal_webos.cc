// Copyright (c) 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lg/pal/webos/pal/pal_webos.h"

namespace pal {

PalWebos::PalWebos() {}

PalWebos::~PalWebos() {}

std::unique_ptr<Pal> g_instance;

Pal* Pal::GetInstance() {
  if (g_instance == nullptr)
    g_instance.reset(new PalWebos());

  return g_instance.get();
}

}  // namespace pal
