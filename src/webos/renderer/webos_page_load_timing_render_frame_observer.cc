#include "webos/renderer/webos_page_load_timing_render_frame_observer.h"

#include "base/time/time.h"
#include "third_party/WebKit/public/web/WebPerformance.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "webos/common/webos_view_messages.h"
#include "content/public/renderer/render_frame.h"

namespace webos {

namespace {

base::TimeDelta ClampDelta(double event, double start) {
  if (event - start < 0)
    event = start;
  return base::Time::FromDoubleT(event) - base::Time::FromDoubleT(start);
}

} // namespace

WebOSPageLoadTimingRenderFrameObserver::WebOSPageLoadTimingRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

WebOSPageLoadTimingRenderFrameObserver::~WebOSPageLoadTimingRenderFrameObserver() {}

void WebOSPageLoadTimingRenderFrameObserver::DidChangePerformanceTiming() {
  // Check frame exists
  if(HasNoRenderFrame())
    return;

  if (PageLoadTimingIsFirstMeaningful()) {
    const blink::WebPerformance& perf =
        render_frame()->GetWebFrame()->performance();
    Send(new WebOSViewMsgHost_DidFirstMeaningfulPaint(
        routing_id(), perf.firstMeaningfulPaint()));
  }
}

void WebOSPageLoadTimingRenderFrameObserver::
    DidNonFirstMeaningPaintAfterLoad() {
  if (HasNoRenderFrame())
    return;

  Send(new WebOSViewMsgHost_DidNonFirstMeaningfulPaint(routing_id()));
}

void WebOSPageLoadTimingRenderFrameObserver::OnDestruct() {
  delete this;
}

bool WebOSPageLoadTimingRenderFrameObserver::HasNoRenderFrame() const {
  bool no_frame = !render_frame() || !render_frame()->GetWebFrame();
  DCHECK(!no_frame);
  return no_frame;
}

bool WebOSPageLoadTimingRenderFrameObserver::PageLoadTimingIsFirstMeaningful() {
  if (first_meaningful_paint_)
    return false;

  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->performance();

  if (perf.firstMeaningfulPaint() > 0.0) {
    first_meaningful_paint_ = ClampDelta(perf.firstMeaningfulPaint(), perf.navigationStart());
    return true;
  }
  return false;
}

} // namespace webos
