// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_WAYLAND_SCREEN_H_
#define OZONE_WAYLAND_SCREEN_H_

#include <stdint.h>

#include "ui/gfx/geometry/rect.h"

struct wl_output;
struct wl_registry;

namespace ozonewayland {

class WaylandDisplay;

// WaylandScreen objects keep track of the current outputs (screens/monitors)
// that are available to the application.
class WaylandScreen {
 public:
  WaylandScreen(wl_registry* registry, uint32_t id);
  ~WaylandScreen();

  // Returns the active allocation of the screen.
  gfx::Rect Geometry() const { return rect_; }
  int32_t GetOutputTransform() const { return transform_; }
  int GetOutputTransformDegrees() const;

 private:
  // Callback functions that allows the display to initialize the screen's
  // position and available modes.
  static void OutputHandleGeometry(void* data,
                                   wl_output* output,
                                   int32_t x,
                                   int32_t y,
                                   int32_t physical_width,
                                   int32_t physical_height,
                                   int32_t subpixel,
                                   const char* make,
                                   const char* model,
                                   int32_t output_transform);

  static void OutputHandleMode(void* data,
                               wl_output* wl_output,
                               uint32_t flags,
                               int32_t width,
                               int32_t height,
                               int32_t refresh);

  static void OutputDone(void* data, struct wl_output* wl_output);
  static void OutputHandleScale(void* data,
                                struct wl_output* wl_output,
                                int32_t factor);

  // The Wayland output this object wraps
  wl_output* output_;

  // Rect and transform of active mode.
  gfx::Rect rect_;
  int32_t transform_;

  gfx::Rect pending_rect_;
  int32_t pending_transform_;

  int32_t scale_factor_;
  int32_t pending_scale_factor_;

  DISALLOW_COPY_AND_ASSIGN(WaylandScreen);
};

}  // namespace ozonewayland

#endif  // OZONE_WAYLAND_SCREEN_H_
