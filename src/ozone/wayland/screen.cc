// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/screen.h"

#include <wayland-client.h>

#include "ozone/wayland/display.h"

namespace ozonewayland {

WaylandScreen::WaylandScreen(wl_registry* registry, uint32_t id)
    : output_(NULL), rect_(0, 0, 0, 0), transform_(-1) {
  static const wl_output_listener kOutputListener = {
      WaylandScreen::OutputHandleGeometry, WaylandScreen::OutputHandleMode,
      WaylandScreen::OutputDone,
  };

  output_ = static_cast<wl_output*>(
      wl_registry_bind(registry, id, &wl_output_interface, 2));
  wl_output_add_listener(output_, &kOutputListener, this);
  DCHECK(output_);
}

WaylandScreen::~WaylandScreen() {
  wl_output_destroy(output_);
}

int WaylandScreen::GetOutputTransformDegrees() const {
  int rotation = 0;
  switch (transform_) {
    case WL_OUTPUT_TRANSFORM_90:
      rotation = 90;
      break;
    case WL_OUTPUT_TRANSFORM_180:
      rotation = 180;
      break;
    case WL_OUTPUT_TRANSFORM_270:
      rotation = 270;
      break;
    default:
      rotation = 0;
      break;
  }
  return rotation;
}

// static
void WaylandScreen::OutputHandleGeometry(void *data,
                                         wl_output *output,
                                         int32_t x,
                                         int32_t y,
                                         int32_t physical_width,
                                         int32_t physical_height,
                                         int32_t subpixel,
                                         const char* make,
                                         const char* model,
                                         int32_t output_transform) {
  WaylandScreen* screen = static_cast<WaylandScreen*>(data);
  screen->pending_rect_.set_origin(
      gfx::Point(x, y));  // We don't really support other than (0,0) origin
  screen->pending_transform_ = output_transform;
}

// static
void WaylandScreen::OutputHandleMode(void* data,
                                     wl_output* wl_output,
                                     uint32_t flags,
                                     int32_t width,
                                     int32_t height,
                                     int32_t refresh) {
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    WaylandScreen* screen = static_cast<WaylandScreen*>(data);
    screen->pending_rect_.set_size(gfx::Size(width, height));
  }
}

// static
void WaylandScreen::OutputDone(void* data, struct wl_output* wl_output) {
  WaylandScreen* screen = static_cast<WaylandScreen*>(data);
  if (screen->rect_ != screen->pending_rect_ ||
      screen->pending_transform_ != screen->transform_) {
    screen->rect_ = screen->pending_rect_;
    screen->transform_ = screen->pending_transform_;

    unsigned width = screen->rect_.width();
    unsigned height = screen->rect_.height();
    int rotation = screen->GetOutputTransformDegrees();

    // In case of OzoneWaylandScreen::LookAheadOutputGeometry we reach this
    // point and WaylandDisplay instance is still null. That is intentional
    // because we need to ensure we have fetched geometry before continuing
    // ozone
    // initialization.
    if (WaylandDisplay::GetInstance())
      WaylandDisplay::GetInstance()->OutputScreenChanged(width, height,
                                                         rotation);
  }
}

}  // namespace ozonewayland
