// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/solid_color_layer.h"

#include "cc/layers/solid_color_layer_impl.h"

namespace cc {

std::unique_ptr<LayerImpl> SolidColorLayer::CreateLayerImpl(
    LayerTreeImpl* tree_impl) {
  return SolidColorLayerImpl::Create(tree_impl, id());
}

scoped_refptr<SolidColorLayer> SolidColorLayer::Create() {
  return make_scoped_refptr(new SolidColorLayer());
}

SolidColorLayer::SolidColorLayer() : force_draw_transparent_color_(false) {}

SolidColorLayer::~SolidColorLayer() {}

void SolidColorLayer::SetBackgroundColor(SkColor color) {
  SetContentsOpaque(SkColorGetA(color) == 255);
  Layer::SetBackgroundColor(color);
}

void SolidColorLayer::PushPropertiesTo(LayerImpl* layer) {
  Layer::PushPropertiesTo(layer);

  SolidColorLayerImpl* layer_impl = static_cast<SolidColorLayerImpl*>(layer);
  layer_impl->SetForceDrawTransparentColor(force_draw_transparent_color_);
}

void SolidColorLayer::SetForceDrawTransparentColor(bool force_draw) {
  if (force_draw_transparent_color_ == force_draw)
    return;

  force_draw_transparent_color_ = force_draw;
  SetNeedsCommit();
}

}  // namespace cc
