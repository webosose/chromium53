// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ProgrammaticScrollAnimator_h
#define ProgrammaticScrollAnimator_h

#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/paint/PaintLayerScrollableArea.h"
#include "platform/geometry/FloatPoint.h"
#include "platform/heap/Handle.h"
#include "platform/scroll/ScrollAnimatorCompositorCoordinator.h"
#include "wtf/Allocator.h"
#include "wtf/Noncopyable.h"
#include <memory>

namespace blink {

class CompositorAnimationTimeline;
class CompositorScrollOffsetAnimationCurve;
class FrameView;
class LocalFrame;
class PaintLayerScrollableArea;
class ScrollableArea;
class Settings;

// Animator for fixed-destination scrolls, such as those triggered by
// CSSOM View scroll APIs.
class ProgrammaticScrollAnimator : public ScrollAnimatorCompositorCoordinator {
    WTF_MAKE_NONCOPYABLE(ProgrammaticScrollAnimator);
public:
    static ProgrammaticScrollAnimator* create(ScrollableArea* scrollableArea)
    {
        return new ProgrammaticScrollAnimator(scrollableArea);
    }

    virtual ~ProgrammaticScrollAnimator();

    void scrollToOffsetWithoutAnimation(const FloatPoint&);
    void animateToOffset(FloatPoint);

#if defined(OS_WEBOS)
    bool isWebOSNativeScrollEnabled();
#endif

    // ScrollAnimatorCompositorCoordinator implementation.
    void resetAnimationState() override;
    void cancelAnimation() override;
    void takeOverCompositorAnimation() override { };
    ScrollableArea* getScrollableArea() const override { return m_scrollableArea; }
    void tickAnimation(double monotonicTime) override;
    void updateCompositorAnimations() override;
    void notifyCompositorAnimationFinished(int groupId) override;
    void notifyCompositorAnimationAborted(int groupId) override { };
    void layerForCompositedScrollingDidChange(CompositorAnimationTimeline*) override;

    DECLARE_TRACE();

private:
    explicit ProgrammaticScrollAnimator(ScrollableArea*);

    void notifyPositionChanged(const DoublePoint&);

    Member<ScrollableArea> m_scrollableArea;
    std::unique_ptr<CompositorScrollOffsetAnimationCurve> m_animationCurve;
    FloatPoint m_targetOffset;
    double m_startTime;
};

} // namespace blink

#endif // ProgrammaticScrollAnimator_h
