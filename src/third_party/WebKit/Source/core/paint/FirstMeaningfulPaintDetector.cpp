// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/FirstMeaningfulPaintDetector.h"

#include "core/css/FontFaceSet.h"
#include "core/fetch/ResourceFetcher.h"
#include "core/frame/Settings.h"
#include "core/paint/PaintTiming.h"

#if defined(OS_WEBOS)
#include "core/frame/Settings.h"
#include "core/loader/DocumentLoader.h"
#endif

namespace blink {

namespace {

// Web fonts that laid out more than this number of characters block First
// Meaningful Paint.
const int kBlankCharactersThreshold = 200;

// FirstMeaningfulPaintDetector stops observing layouts and reports First
// Meaningful Paint when this duration passed from last network activity.
const double kSecondsWithoutNetworkActivityThreshold = 0.5;

// If this value is going to small, the splash will be dismissed earlier.
// That means page tend to be rendered incrementally.
// Otherwise, if it's big, splash will be dimissed after all render job ended.
// Initially, I set a experimental value (33ms) from render job & raster timing.
const double kSecondsNotifyFMPDetectThreshold = 0.033;

} // namespace

FirstMeaningfulPaintDetector& FirstMeaningfulPaintDetector::from(Document& document)
{
    return PaintTiming::from(document).firstMeaningfulPaintDetector();
}

FirstMeaningfulPaintDetector::FirstMeaningfulPaintDetector(PaintTiming* paintTiming)
    : m_paintTiming(paintTiming)
    , m_networkStableTimer(this, &FirstMeaningfulPaintDetector::networkStableTimerFired)
    , m_notifyFMPDetectTimer(this, &FirstMeaningfulPaintDetector::notifyFMPDetectTimerFired)
{
}

Document* FirstMeaningfulPaintDetector::document()
{
    return m_paintTiming->document();
}

// Computes "layout significance" (http://goo.gl/rytlPL) of a layout operation.
// Significance of a layout is the number of layout objects newly added to the
// layout tree, weighted by page height (before and after the layout).
// A paint after the most significance layout during page load is reported as
// First Meaningful Paint.
void FirstMeaningfulPaintDetector::markNextPaintAsMeaningfulIfNeeded(
    const LayoutObjectCounter& counter, int contentsHeightBeforeLayout,
    int contentsHeightAfterLayout, int visibleHeight)
{
    if (m_state == Reported)
        return;

    unsigned delta = counter.count() - m_prevLayoutObjectCount;
    m_prevLayoutObjectCount = counter.count();

    if (visibleHeight == 0)
        return;

    double ratioBefore = std::max(1.0, static_cast<double>(contentsHeightBeforeLayout) / visibleHeight);
    double ratioAfter = std::max(1.0, static_cast<double>(contentsHeightAfterLayout) / visibleHeight);
    double significance = delta / ((ratioBefore + ratioAfter) / 2);

    // If the page has many blank characters, the significance value is
    // accumulated until the text become visible.
    int approximateBlankCharacterCount = FontFaceSet::approximateBlankCharacterCount(*document());
    if (approximateBlankCharacterCount > kBlankCharactersThreshold) {
        m_accumulatedSignificanceWhileHavingBlankText += significance;
    } else {
        significance += m_accumulatedSignificanceWhileHavingBlankText;
        m_accumulatedSignificanceWhileHavingBlankText = 0;
        if (significance > m_maxSignificanceSoFar) {
            PMLOG_DEBUG("FMPDetector", "FMPDetector::markNextPaintAsMeaningfulIfNeeded(%p) detects NextPaintIsMeaningful as significance(%f)", this, significance);
            m_state = NextPaintIsMeaningful;
            m_maxSignificanceSoFar = significance;
        }
    }
}

void FirstMeaningfulPaintDetector::notifyPaint()
{
    if (m_state != NextPaintIsMeaningful)
        return;

    // Skip document background-only paints.
    if (m_paintTiming->firstPaint() == 0.0)
        return;

    m_provisionalFirstMeaningfulPaint = monotonicallyIncreasingTime();
    m_state = NextPaintIsNotMeaningful;
    PMLOG_DEBUG("FMPDetector", "FMPDetector::notifyPaint(%p) timestamps provisionalFirstMeaningfulPaint(%f) after firstPaint", this, m_provisionalFirstMeaningfulPaint);

#if defined(OS_WEBOS)
    // set candidate FMP first
    m_paintTiming->setFirstMeaningfulPaintCandidate(m_provisionalFirstMeaningfulPaint);

    if (document()->settings()->notifyFMPDirectly()) {
        m_state = Reported;
        m_paintTiming->setFirstMeaningfulPaint(m_provisionalFirstMeaningfulPaint);
        PMLOG_DEBUG("FMPDetector", "FMPDetector::notifyPaint(%p) report FMP without timer", this);
        return;
    }
#endif

    m_notifyFMPDetectTimer.startOneShot(kSecondsNotifyFMPDetectThreshold, BLINK_FROM_HERE);
}

void FirstMeaningfulPaintDetector::checkNetworkStable()
{
    DCHECK(document());
    if (m_state == Reported || document()->fetcher()->hasPendingRequest())
        return;

#if defined(OS_WEBOS)
    Settings* settings = document()->settings();
    if (settings) {
        if (settings->networkStableTimeout() < 0)
            settings->setNetworkStableTimeout(kSecondsWithoutNetworkActivityThreshold);
        m_networkStableTimer.startOneShot(settings->networkStableTimeout(), BLINK_FROM_HERE);
    } else
#endif
        m_networkStableTimer.startOneShot(kSecondsWithoutNetworkActivityThreshold, BLINK_FROM_HERE);
}

#if defined(OS_WEBOS)
void FirstMeaningfulPaintDetector::stopNetworkStableTimer()
{
    PMLOG_DEBUG("FMPDetector", "FMPDetector::stopNetworkStableTimer(%p)", this);
    m_networkStableTimer.stop();
}
#endif

void FirstMeaningfulPaintDetector::networkStableTimerFired(Timer<FirstMeaningfulPaintDetector>*)
{
    if (m_state == Reported || !document() || document()->fetcher()->hasPendingRequest())
        return;

    if (m_provisionalFirstMeaningfulPaint) {
        m_paintTiming->setFirstMeaningfulPaint(m_provisionalFirstMeaningfulPaint);
        m_state = Reported;
        PMLOG_DEBUG("FMPDetector", "FMPDetector::networkStableTimerFired(%p) marks FMP", this);
    }
#if defined(OS_WEBOS)
    else {
        // notify paint when First Meaningful Paint was not detected until loading resources
        notifyNonFirstMeaningfulPaint();
    }
#endif
}

void FirstMeaningfulPaintDetector::resetStateToMarkNextPaintForContainer()
{
    m_state = NextPaintIsNotMeaningful;
    m_maxSignificanceSoFar = 0.0;
    m_provisionalFirstMeaningfulPaint = 0.0;
    m_accumulatedSignificanceWhileHavingBlankText = 0.0;
    m_prevLayoutObjectCount = 0;
}

void FirstMeaningfulPaintDetector::notifyFMPDetectTimerFired(Timer<FirstMeaningfulPaintDetector>*)
{
    DCHECK(document());

    if (m_state != Reported && m_provisionalFirstMeaningfulPaint) {
        m_paintTiming->setFirstMeaningfulPaint(m_provisionalFirstMeaningfulPaint);
        m_state = Reported;
        PMLOG_DEBUG("FMPDetector", "FMPDetector::nofifyFMPDetectTimerFired(%p) marks FMP", this);
    }
}

#if defined(OS_WEBOS)
void FirstMeaningfulPaintDetector::notifyNonFirstMeaningfulPaint()
{
    DCHECK(document());

    if (document() && document()->loader()) {
        document()->loader()->commitNonFirstMeaningfulPaintAfterLoad();
        m_state = Reported;
        PMLOG_DEBUG("FMPDetector", "FMPDetector::notifyNonFirstMeaningfulPaint(%p)", this);
    }
}
#endif

DEFINE_TRACE(FirstMeaningfulPaintDetector)
{
    visitor->trace(m_paintTiming);
}

} // namespace blink
