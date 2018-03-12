// Copyright (c) 2016 LG Electronics, Inc.
// Copyright (c) 2017 LG Electronics, Inc.

#include "content/browser/webos/render_view_host_webos_impl.h"

#include "base/command_line.h"
#include "content/browser/renderer_host/render_view_host_delegate.h"
#include "content/common/page_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/memory_pressure_controller.h"
#include "content/public/common/web_preferences.h"

namespace content {

// RenderViewHostWebosImpl
// -------------------------------------------------------------------------

RenderViewHostWebosImpl::RenderViewHostWebosImpl(
    SiteInstance* instance,
    std::unique_ptr<RenderWidgetHostImpl> widget,
    RenderViewHostDelegate* delegate,
    int32_t routing_id,
    bool swapped_out,
    bool has_initialized_audio_host)
    : RenderViewHostImpl(instance,
                         std::move(widget),
                         delegate,
                         routing_id,
                         swapped_out,
                         has_initialized_audio_host) {}

RenderViewHostWebosImpl::~RenderViewHostWebosImpl() {}

void RenderViewHostWebosImpl::NotifyMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  RAW_PMLOG_DEBUG("[MemoryPressure] %s => Level: %d", __PRETTY_FUNCTION__,
                  memory_pressure_level);
  if (memory_pressure_level !=
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE) {
    // MemoryPressureListener do not handle MEMORY_PRESSURE_LEVEL_NONE.
    MemoryPressureController::SendPressureNotification(GetProcess(),
                                                       memory_pressure_level);
  }
}

bool RenderViewHostWebosImpl::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RenderViewHostWebosImpl, msg)
    IPC_MESSAGE_UNHANDLED(handled = RenderViewHostImpl::OnMessageReceived(msg))
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RenderViewHostWebosImpl::OnWebkitPreferencesChanged() {
  RenderViewHostImpl::OnWebkitPreferencesChanged();
}

}  // namespace content
