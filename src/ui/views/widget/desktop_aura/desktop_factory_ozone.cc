// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include "ui/views/widget/desktop_aura/desktop_factory_ozone.h"

#include "base/logging.h"
#include "ui/ozone/platform_object.h"

namespace views {

// static
DesktopFactoryOzone* DesktopFactoryOzone::impl_ = nullptr;

DesktopFactoryOzone::DesktopFactoryOzone() {
  DCHECK(!impl_) << "There should only be a single DesktopFactoryOzone.";
  impl_ = this;
}

DesktopFactoryOzone::~DesktopFactoryOzone() {
  DCHECK_EQ(impl_, this);
  impl_ = nullptr;
}

DesktopFactoryOzone* DesktopFactoryOzone::GetInstance() {
  if (!impl_) {
    std::unique_ptr<DesktopFactoryOzone> factory =
        ui::PlatformObject<DesktopFactoryOzone>::Create();

    // TODO(tonikitoo): Currently need to leak this object.
    DesktopFactoryOzone* leaky = factory.release();
    DCHECK_EQ(impl_, leaky);
  }
  return impl_;
}

} // namespace views
