#ifndef WEBOS_PAGE_LOAD_TIMING_RENDER_FRAME_OBSERVER_H_
#define WEBOS_PAGE_LOAD_TIMING_RENDER_FRAME_OBSERVER_H_

#include "base/optional.h"
#include "base/time/time.h"
#include "content/public/renderer/render_frame_observer.h"

namespace webos {

// WebOsPageLoadTimingRenderFrameObserver observes RenderFrame notifications, and
// sends page load timing information to the browser process over IPC.
class WebOSPageLoadTimingRenderFrameObserver : public content::RenderFrameObserver {
  public:
    explicit WebOSPageLoadTimingRenderFrameObserver(content::RenderFrame* render_frame);
    ~WebOSPageLoadTimingRenderFrameObserver() override;

    // RenderFrameObserver implementation
    void DidChangePerformanceTiming() override;
    void ResetStateToMarkNextPaintForContainer() override {
      first_meaningful_paint_ = base::nullopt;
    }
    void DidNonFirstMeaningPaintAfterLoad() override;

    void OnDestruct() override;
  private:
    bool HasNoRenderFrame() const;
    bool PageLoadTimingIsFirstMeaningful();
    base::Optional<base::TimeDelta> first_meaningful_paint_;
};

} // namespace webos

#endif // WEBOS_PAGE_LOAD_TIMING_RENDER_FRAME_OBSERVER_H_
