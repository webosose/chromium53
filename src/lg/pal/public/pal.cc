// Copyright (c) 2015-2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "lg/pal/public/pal.h"

// Redefine NOTIMPLEMENTED_POLICY to log once
#define NOTIMPLEMENTED_POLICY 5
#include "base/logging.h"

namespace pal {

Pal::Pal() {}

Pal::~Pal() {}

void Pal::PlatformShutdown() {}

}  // namespace pal
