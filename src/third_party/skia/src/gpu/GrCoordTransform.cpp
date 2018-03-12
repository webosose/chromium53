/*
 * Copyright 2014 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "GrCoordTransform.h"
#include "GrCaps.h"
#include "GrContext.h"
#include "GrGpu.h"

void GrCoordTransform::reset(GrCoordSet sourceCoords, const SkMatrix& m, const GrTexture* texture) {
    SkASSERT(texture);
    SkASSERT(!fInProcessor);

    fSourceCoords = sourceCoords;
    fMatrix = m;
    fReverseY = kBottomLeft_GrSurfaceOrigin == texture->origin();
}

void GrCoordTransform::reset(GrCoordSet sourceCoords,
                             const SkMatrix& m) {
    SkASSERT(!fInProcessor);
    fSourceCoords = sourceCoords;
    fMatrix = m;
    fReverseY = false;
}
