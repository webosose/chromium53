// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/webui/input_method_context_impl_wayland.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "ozone/platform/messages.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"
#include "ui/base/ime/composition_text.h"

#if defined(OS_WEBOS)
#include "ozone/ui/desktop_aura/desktop_window_tree_host_ozone.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "webos/common/webos_native_event_delegate.h"

#define WEBOS_PREEDIT_HLCOLOR 0xC6B0BA

namespace {

webos::WebOSNativeEventDelegate* GetNativeEventDelegate(unsigned handle) {
  views::DesktopWindowTreeHostOzone* host =
      views::DesktopWindowTreeHostOzone::GetHostForAcceleratedWidget(handle);
  if (host)
    return host->desktop_native_widget_aura()->GetNativeEventDelegate();
  else return NULL;
}
}
#endif

namespace ui {

InputMethodContextImplWayland::InputMethodContextImplWayland(
    gfx::AcceleratedWidget widget,
    LinuxInputMethodContextDelegate* delegate,
    OzoneGpuPlatformSupportHost* sender)
    : handled_(false),
      visible_(false),
      widget_(widget),
      delegate_(delegate),
      sender_(sender),
      focused_(false),
      weak_ptr_factory_(this) {
  CHECK(delegate_);
  sender_->RegisterHandler(this);
}

InputMethodContextImplWayland::~InputMethodContextImplWayland() {
  sender_->UnregisterHandler(this);
}

////////////////////////////////////////////////////////////////////////////////
// InputMethodContextImplWayland, ui::LinuxInputMethodContext implementation:
bool InputMethodContextImplWayland::DispatchKeyEvent(
    const KeyEvent& key_event) {
  return false;
}

void InputMethodContextImplWayland::Reset() {
  sender_->Send(new WaylandDisplay_ImeReset());
}

void InputMethodContextImplWayland::Focus() {
  focused_ = true;
}

void InputMethodContextImplWayland::Blur() {
  focused_ = false;
}

void InputMethodContextImplWayland::SetCursorLocation(const gfx::Rect&) {}

void InputMethodContextImplWayland::ChangeVKBVisibility(bool visible) {
  (visible) ? ShowInputPanel() : HideInputPanel();
}

void InputMethodContextImplWayland::OnTextInputTypeChanged(
    ui::TextInputType text_input_type, int text_input_flags) {
  if (text_input_type != ui::TEXT_INPUT_TYPE_NONE)
    sender_->Send(new WaylandDisplay_SetInputContentType(
        InputContentTypeFromTextInputType(text_input_type), text_input_flags));
}

InputContentType
InputMethodContextImplWayland::InputContentTypeFromTextInputType(
    ui::TextInputType text_input_type) {
  switch (text_input_type) {
    case ui::TEXT_INPUT_TYPE_NONE:
      return INPUT_CONTENT_TYPE_NONE;
    case ui::TEXT_INPUT_TYPE_TEXT:
      return INPUT_CONTENT_TYPE_TEXT;
    case ui::TEXT_INPUT_TYPE_PASSWORD:
      return INPUT_CONTENT_TYPE_PASSWORD;
    case ui::TEXT_INPUT_TYPE_SEARCH:
      return INPUT_CONTENT_TYPE_SEARCH;
    case ui::TEXT_INPUT_TYPE_EMAIL:
      return INPUT_CONTENT_TYPE_EMAIL;
    case ui::TEXT_INPUT_TYPE_NUMBER:
      return INPUT_CONTENT_TYPE_NUMBER;
    case ui::TEXT_INPUT_TYPE_TELEPHONE:
      return INPUT_CONTENT_TYPE_TELEPHONE;
    case ui::TEXT_INPUT_TYPE_URL:
      return INPUT_CONTENT_TYPE_URL;
    case ui::TEXT_INPUT_TYPE_DATE:
      return INPUT_CONTENT_TYPE_DATE;
    case ui::TEXT_INPUT_TYPE_DATE_TIME:
      return INPUT_CONTENT_TYPE_DATE_TIME;
    case ui::TEXT_INPUT_TYPE_DATE_TIME_LOCAL:
      return INPUT_CONTENT_TYPE_DATE_TIME_LOCAL;
    case ui::TEXT_INPUT_TYPE_MONTH:
      return INPUT_CONTENT_TYPE_MONTH;
    case ui::TEXT_INPUT_TYPE_TIME:
      return INPUT_CONTENT_TYPE_TIME;
    case ui::TEXT_INPUT_TYPE_WEEK:
      return INPUT_CONTENT_TYPE_WEEK;
    case ui::TEXT_INPUT_TYPE_TEXT_AREA:
      return INPUT_CONTENT_TYPE_TEXT_AREA;
    case ui::TEXT_INPUT_TYPE_CONTENT_EDITABLE:
      return INPUT_CONTENT_TYPE_CONTENT_EDITABLE;
    case ui::TEXT_INPUT_TYPE_DATE_TIME_FIELD:
      return INPUT_CONTENT_TYPE_DATE_TIME_FIELD;
    default:
      return INPUT_CONTENT_TYPE_TEXT;
  }
}

////////////////////////////////////////////////////////////////////////////////
// GpuPlatformSupportHost implementation:
void InputMethodContextImplWayland::OnChannelEstablished(
    int host_id, scoped_refptr<base::SingleThreadTaskRunner> send_runner,
        const base::Callback<void(IPC::Message*)>& send_callback) {
}

void InputMethodContextImplWayland::OnChannelDestroyed(int host_id) {
}

bool InputMethodContextImplWayland::OnMessageReceived(
    const IPC::Message& message) {
  handled_ = false;

  IPC_BEGIN_MESSAGE_MAP(InputMethodContextImplWayland, message)
  IPC_MESSAGE_HANDLER(WaylandInput_Commit, Commit)
  IPC_MESSAGE_HANDLER(WaylandInput_DeleteRange, DeleteRange)
  IPC_MESSAGE_HANDLER(WaylandInput_HideIme, HideIme)
  IPC_MESSAGE_HANDLER(WaylandInput_PreeditChanged, PreeditChanged)
  IPC_MESSAGE_HANDLER(WaylandInput_PreeditEnd, PreeditEnd)
  IPC_MESSAGE_HANDLER(WaylandInput_PreeditStart, PreeditStart)
#if defined(OS_WEBOS)
  IPC_MESSAGE_HANDLER(WaylandInput_InputPanelStateChanged,
                      OnInputPanelStateChanged)
  IPC_MESSAGE_HANDLER(WaylandInput_InputPanelRectChanged,
                      OnInputPanelRectChanged)
#endif
  IPC_END_MESSAGE_MAP()

  return handled_;
}

void InputMethodContextImplWayland::OnCommit(unsigned handle,
                                             const std::string& text) {
  base::string16 string_commited;
  if (base::IsStringUTF8(text))
    base::UTF8ToUTF16(text.c_str(), text.length(), &string_commited);
  else string_commited = base::ASCIIToUTF16(text);

  delegate_->OnCommit(string_commited);
}

void InputMethodContextImplWayland::OnPreeditChanged(
    unsigned handle, const std::string& text, const std::string& commit) {
  ui::CompositionText composition_text;
  if (base::IsStringUTF8(text)) {
    base::UTF8ToUTF16(text.c_str(), text.length(), &composition_text.text);
    composition_text.selection = gfx::Range(0, composition_text.text.length());
#if defined(OS_WEBOS)
    composition_text.underlines.push_back(
        ui::CompositionUnderline(0, composition_text.text.length(), 0, false,
                                 SkColorSetA(WEBOS_PREEDIT_HLCOLOR, 0xFF)));
#endif
  } else
    composition_text.text = base::ASCIIToUTF16(text);

  delegate_->OnPreeditChanged(composition_text);
}

void InputMethodContextImplWayland::OnDeleteRange(
  unsigned handle, int32_t index, uint32_t length) {
  delegate_->OnDeleteRange(index, length);
}

void InputMethodContextImplWayland::OnHideIme(unsigned handle,
                                              ui::ImeHiddenType hidden_type) {
  handled_ = (widget_ == handle);
  if (!handled_)
    return;

  delegate_->OnHideIme(hidden_type);
}

void InputMethodContextImplWayland::HideInputPanel() {
  sender_->Send(new WaylandDisplay_HideInputPanel());
  visible_ = false;
}

void InputMethodContextImplWayland::ShowInputPanel() {
  sender_->Send(new WaylandDisplay_ShowInputPanel(widget_));
  visible_ = true;
}

void InputMethodContextImplWayland::Commit(unsigned handle,
                                           const std::string& text) {
  handled_ = (widget_ == handle);
  if (!handled_)
    return;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&InputMethodContextImplWayland::OnCommit,
          weak_ptr_factory_.GetWeakPtr(), handle, text));
}

void InputMethodContextImplWayland::PreeditChanged(unsigned handle,
                                                   const std::string& text,
                                                   const std::string& commit) {
  handled_ = (widget_ == handle);
  if (!handled_)
    return;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&InputMethodContextImplWayland::OnPreeditChanged,
          weak_ptr_factory_.GetWeakPtr(), handle, text, commit));
}

void InputMethodContextImplWayland::DeleteRange(
  unsigned handle, int32_t index, uint32_t length) {
  handled_ = (widget_ == handle);
  if (!handled_)
    return;

  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
   base::Bind(&InputMethodContextImplWayland::OnDeleteRange,
     weak_ptr_factory_.GetWeakPtr(), handle, index, length));
}

void InputMethodContextImplWayland::HideIme(unsigned handle,
                                            ui::ImeHiddenType hidden_type) {
  if (hidden_type == ui::IME_CLOSE)
    HideInputPanel();
  handled_ = (widget_ == handle);
  if (!handled_)
    return;

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&InputMethodContextImplWayland::OnHideIme,
                 weak_ptr_factory_.GetWeakPtr(), handle, hidden_type));
}

void InputMethodContextImplWayland::PreeditEnd() {
}

void InputMethodContextImplWayland::PreeditStart() {
}

#if defined(OS_WEBOS)
void InputMethodContextImplWayland::SetSurroundingText(const std::string& text,
                                                       size_t cursor_position,
                                                       size_t anchor_position) {
  sender_->Send(new WaylandDisplay_SetSurroundingText(text, cursor_position,
                                                      anchor_position));
}

void InputMethodContextImplWayland::OnInputPanelStateChanged(
    unsigned handle,
    webos::InputPanelState state) {
  handled_ = (widget_ == handle);
  if (!handled_)
    return;

  webos::WebOSNativeEventDelegate* webos_native_event_delegate =
      GetNativeEventDelegate(handle);

  if (webos_native_event_delegate) {
    switch (state) {
      case webos::INPUT_PANEL_SHOWN:
        webos_native_event_delegate->InputPanelShown();
        break;
      case webos::INPUT_PANEL_HIDDEN:
        webos_native_event_delegate->InputPanelHidden();
        break;
      default:  // none
        break;
    }
  }
}

void InputMethodContextImplWayland::OnInputPanelRectChanged(unsigned handle,
                                                            int32_t x,
                                                            int32_t y,
                                                            uint32_t width,
                                                            uint32_t height) {
  handled_ = (widget_ == handle);
  if (!handled_)
    return;

  input_panel_rect_.SetRect(x, y, width, height);

  webos::WebOSNativeEventDelegate* webos_native_event_delegate =
      GetNativeEventDelegate(handle);

  if (webos_native_event_delegate)
    webos_native_event_delegate->InputPanelRectChanged();
}
#endif

}  // namespace ui
