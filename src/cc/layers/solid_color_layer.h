// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#ifndef CC_LAYERS_SOLID_COLOR_LAYER_H_
#define CC_LAYERS_SOLID_COLOR_LAYER_H_

#include "base/macros.h"
#include "cc/base/cc_export.h"
#include "cc/layers/layer.h"

namespace cc {

// A Layer that renders a solid color. The color is specified by using
// SetBackgroundColor() on the base class.
class CC_EXPORT SolidColorLayer : public Layer {
 public:
  static scoped_refptr<SolidColorLayer> Create();

  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;

  void SetBackgroundColor(SkColor color) override;
  void PushPropertiesTo(LayerImpl* layer) override;

  // Sets whether the quads with transparent color will be drawn
  // or ignore as meaninless.
  // Defaults to false.
  void SetForceDrawTransparentColor(bool force_draw);

 protected:
  SolidColorLayer();

 private:
  ~SolidColorLayer() override;

  bool force_draw_transparent_color_;

  DISALLOW_COPY_AND_ASSIGN(SolidColorLayer);
};

}  // namespace cc
#endif  // CC_LAYERS_SOLID_COLOR_LAYER_H_
