// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_NPAPI_WEBPLUGIN_ACCELERATED_SURFACE_WEBOS_H_
#define CONTENT_CHILD_NPAPI_WEBPLUGIN_ACCELERATED_SURFACE_WEBOS_H_

#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

// Interface class for interacting with an accelerated plugin surface.
class WebPluginAcceleratedSurface {
 public:
  virtual ~WebPluginAcceleratedSurface() {}

  // Sets the size of the surface.
  virtual void SetSize(const gfx::Size& size) = 0;

  virtual void Suspend() = 0;

  virtual void Resume() = 0;
};

}  // namespace content

#endif  // CONTENT_CHILD_NPAPI_WEBPLUGIN_ACCELERATED_SURFACE_WEBOS_H_
