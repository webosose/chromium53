/*
 * Copyright 2013 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrCoordTransform_DEFINED
#define GrCoordTransform_DEFINED

#include "GrProcessor.h"
#include "SkMatrix.h"
#include "GrTexture.h"
#include "GrTypes.h"
#include "GrShaderVar.h"

/**
 * Coordinates available to GrProcessor subclasses for requesting transformations. Transformed
 * coordinates are made available in the the portion of fragment shader emitted by the effect.
 *
 * The precision of the shader var that interpolates the transformed coordinates can be specified.
 */
enum GrCoordSet {
    /**
     * The user-space coordinates that map to the fragment being rendered. This is the space in
     * which SkShader operates. It is usually the space in which geometry passed to SkCanvas is
     * specified (before the view matrix is applied). However, some draw calls take explicit local
     * coords that map onto the geometry (e.g. drawVertices, drawBitmapRectToRect).
     */
    kLocal_GrCoordSet,

    /**
     * The device space position of the fragment being shaded.
     */
    kDevice_GrCoordSet,
};

/**
 * A class representing a linear transformation from one of the built-in coordinate sets (local or
 * position). GrProcessors just define these transformations, and the framework does the rest of the
 * work to make the transformed coordinates available in their fragment shader.
 */
class GrCoordTransform : SkNoncopyable {
public:
    GrCoordTransform() : fSourceCoords(kLocal_GrCoordSet) { SkDEBUGCODE(fInProcessor = false); }

    /**
     * Create a transformation that maps [0, 1] to a proxy's boundaries. The proxy origin also
     * implies whether a y-reversal should be performed.
     */
    GrCoordTransform(GrCoordSet sourceCoords, const GrTexture* texture) {

        SkASSERT(texture);
        SkDEBUGCODE(fInProcessor = false);
        this->reset(sourceCoords, texture);
    }

    /**
     * Create a transformation from a matrix. The proxy origin also implies whether a y-reversal
     * should be performed.
     */
    GrCoordTransform(GrCoordSet sourceCoords, const SkMatrix& m,
                     const GrTexture* texture) {
        SkDEBUGCODE(fInProcessor = false);
        SkASSERT(texture);
        this->reset(sourceCoords, m, texture);
    }

    /**
     * Create a transformation that applies the matrix to a coord set.
     */
    GrCoordTransform(GrCoordSet sourceCoords, const SkMatrix& m) {
        SkDEBUGCODE(fInProcessor = false);
        this->reset(sourceCoords, m);
    }

    void reset(GrCoordSet sourceCoords, const GrTexture* texture) {
        SkASSERT(!fInProcessor);
        SkASSERT(texture);
        this->reset(sourceCoords, MakeDivByTextureWHMatrix(texture), texture);
    }

    void reset(GrCoordSet, const SkMatrix&, const GrTexture*);
    void reset(GrCoordSet sourceCoords, const SkMatrix& m);

    GrCoordTransform& operator= (const GrCoordTransform& that) {
        SkASSERT(!fInProcessor);
        fSourceCoords = that.fSourceCoords;
        fMatrix = that.fMatrix;
        fReverseY = that.fReverseY;
        return *this;
    }

    /**
     * Access the matrix for editing. Note, this must be done before adding the transform to an
     * effect, since effects are immutable.
     */
    SkMatrix* accessMatrix() {
        SkASSERT(!fInProcessor);
        return &fMatrix;
    }

    bool operator==(const GrCoordTransform& that) const {
        return fSourceCoords == that.fSourceCoords &&
               fMatrix.cheapEqualTo(that.fMatrix) &&
               fReverseY == that.fReverseY;
    }

    bool operator!=(const GrCoordTransform& that) const { return !(*this == that); }

    GrCoordSet sourceCoords() const { return fSourceCoords; }
    const SkMatrix& getMatrix() const { return fMatrix; }
    bool reverseY() const { return fReverseY; }

    /** Useful for effects that want to insert a texture matrix that is implied by the texture
        dimensions */
    static inline SkMatrix MakeDivByTextureWHMatrix(const GrTexture* texture) {
        SkASSERT(texture);
        SkMatrix mat;
        (void)mat.setIDiv(texture->width(), texture->height());
        return mat;
    }

private:
    GrCoordSet              fSourceCoords;
    SkMatrix                fMatrix;
    bool                    fReverseY;
    typedef SkNoncopyable INHERITED;

#ifdef SK_DEBUG
public:
    void setInProcessor() const { fInProcessor = true; }
private:
    mutable bool fInProcessor;
#endif
};

#endif
