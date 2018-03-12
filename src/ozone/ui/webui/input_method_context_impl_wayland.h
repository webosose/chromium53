// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_IMPL_WAYLAND_H_
#define OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_IMPL_WAYLAND_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "ozone/platform/input_content_type.h"
#include "ozone/platform/ozone_export_wayland.h"
#include "ui/base/ime/linux/linux_input_method_context.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/gpu_platform_support_host.h"

#if defined(OS_WEBOS)
#include "ui/gfx/geometry/rect.h"
#include "webos/common/webos_constants.h"
#endif

namespace ui {

class OzoneGpuPlatformSupportHost;

// An implementation of LinuxInputMethodContext for IME support on Ozone
// platform using Wayland.
class OZONE_WAYLAND_EXPORT InputMethodContextImplWayland
  : public LinuxInputMethodContext,
    public GpuPlatformSupportHost {
 public:
  InputMethodContextImplWayland(gfx::AcceleratedWidget widget,
                                ui::LinuxInputMethodContextDelegate* delegate,
                                OzoneGpuPlatformSupportHost* sender);
  ~InputMethodContextImplWayland() override;

  // overriden from ui::LinuxInputMethodContext
  bool DispatchKeyEvent(const ui::KeyEvent& key_event) override;
  void Reset() override;
  void Focus() override;
  void Blur() override;
  void SetCursorLocation(const gfx::Rect&) override;
  void ChangeVKBVisibility(bool visible) override;
  void OnTextInputTypeChanged(ui::TextInputType text_input_type,
                              int text_input_flags) override;
  bool IsInputPanelVisible() const override { return visible_; }
  bool IsInputMethodActive() const override { return focused_; }
#if defined(OS_WEBOS)
  gfx::Rect GetInputPanelRect() const override { return input_panel_rect_; }
  void SetSurroundingText(const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position) override;
#endif

 protected:
  bool OnMessageReceived(const IPC::Message&) override;
  InputContentType InputContentTypeFromTextInputType(
      TextInputType text_input_type);

 private:
  // GpuPlatformSupportHost
  void OnChannelEstablished(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& send_callback) override;
  void OnChannelDestroyed(int host_id) override;
  void OnPreeditChanged(unsigned handle,
                        const std::string& text,
                        const std::string& commit);
  void OnCommit(unsigned handle, const std::string& text);
  void OnDeleteRange(unsigned handle, int32_t index, uint32_t length);
  void OnHideIme(unsigned handle, ui::ImeHiddenType hidden_type);
  void ShowInputPanel();
  void HideInputPanel();
  void Commit(unsigned handle, const std::string& text);
  void PreeditChanged(unsigned handle,
                      const std::string& text,
                      const std::string& commit);
  void PreeditEnd();
  void PreeditStart();
  void DeleteRange(unsigned handle, int32_t index, uint32_t length);
  void HideIme(unsigned handle, ui::ImeHiddenType hidden_type);
#if defined(OS_WEBOS)
  void OnInputPanelStateChanged(unsigned handle, webos::InputPanelState state);
  void OnInputPanelRectChanged(unsigned handle,
                               int32_t x,
                               int32_t y,
                               uint32_t width,
                               uint32_t height);
  gfx::Rect input_panel_rect_;
#endif
  bool focused_;
  bool handled_;
  bool visible_;
  gfx::AcceleratedWidget widget_;
  // Must not be NULL.
  LinuxInputMethodContextDelegate* delegate_;
  OzoneGpuPlatformSupportHost* sender_;  // Not owned.
  // Support weak pointers for attach & detach callbacks.
  base::WeakPtrFactory<InputMethodContextImplWayland> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(InputMethodContextImplWayland);
};

}  // namespace ui

#endif  //  OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_IMPL_WAYLAND_H_
