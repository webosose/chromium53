// Copyright 2014 LG Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/input/webos_text_input.h"

#include <string>
#include <curses.h>
#include <linux/input.h>

#include "base/memory/singleton.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/input/keyboard.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ozone/wayland/window.h"
#include "ui/events/keycodes/keyboard_code_conversion.h"
#include "ui/events/keycodes/xkb_keysym.h"
#include "ui/base/ime/text_input_flags.h"
#include "wayland-text-client-protocol.h"

namespace ozonewayland {

const uint32_t kIMEModifierFlagShft = 1;
const uint32_t kIMEModifierFlagCtrl = 2;
const uint32_t kIMEModifierFlagAlt = 4;
const uint32_t kIMEModifierAllFlags = 7;

uint32_t GetModifierKey(uint32_t key_sym) {
  switch (key_sym) {
    case kIMEModifierFlagShft:
      return ui::EF_SHIFT_DOWN;
    case kIMEModifierFlagCtrl:
      return ui::EF_CONTROL_DOWN;
    case kIMEModifierFlagAlt:
      return ui::EF_ALT_DOWN;
    default:
      return ui::EF_NONE;
  }
}

uint32_t ContentHintFromInputContentType(ui::InputContentType content_type,
                                         int input_flags) {
  uint32_t wl_hint = (TEXT_MODEL_CONTENT_HINT_AUTO_COMPLETION |
                      TEXT_MODEL_CONTENT_HINT_AUTO_CAPITALIZATION);
  if (content_type == ui::INPUT_CONTENT_TYPE_PASSWORD)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_PASSWORD;

  // hint from flags
  // TODO TEXT_INPUT_FLAG_SPELLCHECK_ON remains.
  //      The wayland-text-client doesn't offer the spellcheck yet.
  if (input_flags & ui::TEXT_INPUT_FLAG_SENSITIVE_ON)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_SENSITIVE_DATA;
  if (input_flags & ui::TEXT_INPUT_FLAG_AUTOCOMPLETE_ON)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_AUTO_COMPLETION;
  if (input_flags & ui::TEXT_INPUT_FLAG_AUTOCORRECT_ON)
    wl_hint |= TEXT_MODEL_CONTENT_HINT_AUTO_CORRECTION;

  return wl_hint;
}

uint32_t ContentPurposeFromInputContentType(ui::InputContentType content_type) {
  switch (content_type) {
    case ui::INPUT_CONTENT_TYPE_PASSWORD:
      return TEXT_MODEL_CONTENT_PURPOSE_PASSWORD;
    case ui::INPUT_CONTENT_TYPE_EMAIL:
      return TEXT_MODEL_CONTENT_PURPOSE_EMAIL;
    case ui::INPUT_CONTENT_TYPE_NUMBER:
      return TEXT_MODEL_CONTENT_PURPOSE_NUMBER;
    case ui::INPUT_CONTENT_TYPE_TELEPHONE:
      return TEXT_MODEL_CONTENT_PURPOSE_PHONE;
    case ui::INPUT_CONTENT_TYPE_URL:
      return TEXT_MODEL_CONTENT_PURPOSE_URL;
    case ui::INPUT_CONTENT_TYPE_DATE:
      return TEXT_MODEL_CONTENT_PURPOSE_DATE;
    case ui::INPUT_CONTENT_TYPE_DATE_TIME:
    case ui::INPUT_CONTENT_TYPE_DATE_TIME_LOCAL:
      return TEXT_MODEL_CONTENT_PURPOSE_DATETIME;
    case ui::INPUT_CONTENT_TYPE_TIME:
      return TEXT_MODEL_CONTENT_PURPOSE_TIME;
    default:
      return TEXT_MODEL_CONTENT_PURPOSE_NORMAL;
  }
}

const struct text_model_listener text_model_listener_ = {
  WaylandTextInput::OnCommitString,
  WaylandTextInput::OnPreeditString,
  WaylandTextInput::OnDeleteSurroundingText,
  WaylandTextInput::OnCursorPosition,
  WaylandTextInput::OnPreeditStyling,
  WaylandTextInput::OnPreeditCursor,
  WaylandTextInput::OnModifiersMap,
  WaylandTextInput::OnKeysym,
  WaylandTextInput::OnEnter,
  WaylandTextInput::OnLeave,
  WaylandTextInput::OnInputPanelState,
  WaylandTextInput::OnTextModelInputPanelRect
};

static uint32_t serial = 0;

WaylandTextInput::WaylandTextInput()
    : input_panel_rect_(0, 0, 0, 0),
      text_model_(NULL),
      is_visible_(false),
      state_(InputPanelUnknownState),
      hidden_type_(ui::IME_ACTIVE),
      input_content_type_(ui::INPUT_CONTENT_TYPE_NONE),
      text_input_flags_(0),
      active_window_(NULL),
      last_active_window_(NULL) {}

WaylandTextInput::~WaylandTextInput() {
  if (text_model_)
    text_model_destroy(text_model_);
}

WaylandTextInput* WaylandTextInput::GetInstance() {
  return base::Singleton<WaylandTextInput>::get();
}

void WaylandTextInput::ResetIme() {
  if (!text_model_) {
    if (WaylandDisplay::GetInstance()->GetTextModelFactory()) {
      text_model_ = text_model_factory_create_text_model(
        WaylandDisplay::GetInstance()->GetTextModelFactory());
      text_model_add_listener(text_model_, &text_model_listener_, this);
    }
  } else {
    text_model_reset(text_model_, serial);
  }
}

void WaylandTextInput::ShowInputPanel(wl_seat* input_seat,
                                      unsigned windowhandle) {
  if (is_visible_) {
    if (state_ == InputPanelHidden)
      HideInputPanel(input_seat);
    else {
      // VKB may be invisible in some cases.
      // So it should be reactivated
      if (last_active_window_ &&
          last_active_window_->Handle() == windowhandle) {
        text_model_activate(
            text_model_, serial, input_seat,
            last_active_window_->ShellSurface()->GetWLSurface());
        text_model_set_content_type(
            text_model_, ContentHintFromInputContentType(input_content_type_,
                                                         text_input_flags_),
            ContentPurposeFromInputContentType(input_content_type_));
      }
      return;
    }
  }

  if (!text_model_) {
    if (WaylandDisplay::GetInstance()->GetTextModelFactory()) {
      text_model_ = text_model_factory_create_text_model(
        WaylandDisplay::GetInstance()->GetTextModelFactory());
      text_model_add_listener(text_model_, &text_model_listener_, this);
    }
  }

  if (text_model_ && active_window_ &&
      active_window_->Handle() == windowhandle) {
    text_model_activate(text_model_,
                        serial,
                        input_seat,
                        active_window_->ShellSurface()->GetWLSurface());
    text_model_set_content_type(
        text_model_,
        ContentHintFromInputContentType(input_content_type_, text_input_flags_),
        ContentPurposeFromInputContentType(input_content_type_));

    is_visible_ = true;
  }
}

void WaylandTextInput::HideInputPanel(wl_seat* input_seat) {
  if (!is_visible_)
    return;

  if (text_model_) {
    SetHiddenState();
    text_model_reset(text_model_, serial);
    text_model_deactivate(text_model_, input_seat);
    text_model_destroy(text_model_);
    text_model_ = NULL;
    is_visible_ = false;
  }
}

void WaylandTextInput::SetActiveWindow(WaylandWindow* window) {
  active_window_ = window;
  if (active_window_)
    last_active_window_ = active_window_;
}

void WaylandTextInput::SetHiddenState() {
  input_panel_rect_.SetRect(0, 0, 0, 0);
  if (last_active_window_) {
    WaylandDisplay::GetInstance()->InputPanelRectChanged(
        last_active_window_->Handle(), 0, 0, 0, 0);
    WaylandDisplay::GetInstance()->InputPanelStateChanged(
        last_active_window_->Handle(),
        webos::InputPanelState::INPUT_PANEL_HIDDEN);
  }
}

void WaylandTextInput::SetInputContentType(ui::InputContentType content_type,
                                           int text_input_flags) {
    input_content_type_ = content_type;
    text_input_flags_ = text_input_flags;
}

void WaylandTextInput::SetSurroundingText(const std::string& text,
                                          size_t cursor_position,
                                          size_t anchor_position) {
  if (text_model_)
    text_model_set_surrounding_text(text_model_, text.c_str(), cursor_position,
                                    anchor_position);
}

void WaylandTextInput::OnWindowAboutToDestroy(unsigned windowhandle) {
  if (active_window_ && active_window_->Handle() == windowhandle)
    active_window_ = NULL;

  if (last_active_window_ && last_active_window_->Handle() == windowhandle)
    last_active_window_ = NULL;
}

void WaylandTextInput::OnCommitString(void* data,
                                      struct text_model* text_input,
                                      uint32_t serial,
                                      const char* text) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  if (instance->last_active_window_)
    dispatcher->Commit(instance->last_active_window_->Handle(), std::string(text));
}

void WaylandTextInput::OnPreeditString(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       const char* text,
                                       const char* commit) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  if (instance->last_active_window_)
    dispatcher->PreeditChanged(instance->last_active_window_->Handle(),
        std::string(text), std::string(commit));
}

void WaylandTextInput::OnDeleteSurroundingText(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       int32_t index,
                                       uint32_t length) {
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  if (instance->last_active_window_)
    dispatcher->DeleteRange(instance->last_active_window_->Handle(), index, length);
}

void WaylandTextInput::OnCursorPosition(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       int32_t index,
                                       int32_t anchor) {
}

void WaylandTextInput::OnPreeditStyling(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       uint32_t index,
                                       uint32_t length,
                                       uint32_t style) {
}

void WaylandTextInput::OnPreeditCursor(void* data,
                                       struct text_model* text_input,
                                       uint32_t serial,
                                       int32_t index) {
}

void WaylandTextInput::OnModifiersMap(void* data,
                                      struct text_model* text_input,
                                      struct wl_array* map) {
}

uint32_t WaylandTextInput::KeyNumberFromKeySymCode(uint32_t key_sym,
                                                   uint32_t modifiers) {
  switch (key_sym) {
    case XKB_KEY_Escape: return KEY_ESC;
    case XKB_KEY_F1: return KEY_F1;
    case XKB_KEY_F2: return KEY_F2;
    case XKB_KEY_F3: return KEY_F3;
    case XKB_KEY_F4: return KEY_F4;
    case XKB_KEY_F5: return KEY_F5;
    case XKB_KEY_F6: return KEY_F6;
    case XKB_KEY_F7: return KEY_F7;
    case XKB_KEY_F8: return KEY_F8;
    case XKB_KEY_F9: return KEY_F9;
    case XKB_KEY_F10: return KEY_F10;
    case XKB_KEY_F11: return KEY_F11;
    case XKB_KEY_F12: return KEY_F12;
    case XKB_KEY_BackSpace: return KEY_BACKSPACE;
    case XKB_KEY_Tab: return KEY_TAB;
    case XKB_KEY_Caps_Lock: return KEY_CAPSLOCK;
    case XKB_KEY_ISO_Enter:
    case XKB_KEY_Return: return KEY_ENTER;
    case XKB_KEY_Shift_L: return KEY_LEFTSHIFT;
    case XKB_KEY_Control_L: return KEY_LEFTCTRL;
    case XKB_KEY_Alt_L: return KEY_LEFTALT;
    case XKB_KEY_Scroll_Lock: return KEY_SCROLLLOCK;
    case XKB_KEY_Insert: return KEY_INSERT;
    case XKB_KEY_Delete: return KEY_DELETE;
    case XKB_KEY_Home: return KEY_HOME;
    case XKB_KEY_End: return KEY_END;
    case XKB_KEY_Prior: return KEY_PAGEUP;
    case XKB_KEY_Next: return KEY_PAGEDOWN;
    case XKB_KEY_Left: return KEY_LEFT;
    case XKB_KEY_Up: return KEY_UP;
    case XKB_KEY_Right: return KEY_RIGHT;
    case XKB_KEY_Down: return KEY_DOWN;
    case XKB_KEY_Num_Lock: return KEY_NUMLOCK;
    case XKB_KEY_KP_Enter: return KEY_KPENTER;
    case XKB_KEY_XF86Back: return KEY_PREVIOUS;
    case 0x2f:   return KEY_KPSLASH;
    case 0x2d:   return KEY_KPMINUS;
    case 0x2a:   return KEY_KPASTERISK;
    case 0x37:   return KEY_KP7;
    case 0x38:   return KEY_KP8;
    case 0x39:   return KEY_KP9;
    case 0x34:   return KEY_KP4;
    case 0x35:   return KEY_KP5;
    case 0x36:   return KEY_KP6;
    case 0x31:   return KEY_KP1;
    case 0x32:   return KEY_KP2;
    case 0x33:   return KEY_KP3;
    case 0x30:   return KEY_KP0;
    case 0x2e:   return KEY_KPDOT;
    case 0x2b:   return KEY_KPPLUS;
    case 0x43:
    case 0x63:
      return modifiers & kIMEModifierFlagCtrl ? KEY_C : KEY_UNKNOWN;
    case 0x56:
    case 0x76:
      return modifiers & kIMEModifierFlagCtrl ? KEY_V : KEY_UNKNOWN;
    case 0x58:
    case 0x78:
      return modifiers & kIMEModifierFlagCtrl ? KEY_X : KEY_UNKNOWN;
    case 0x1200011:  return KEY_RED;
    case 0x1200012:  return KEY_GREEN;
    case 0x1200013:  return KEY_YELLOW;
    case 0x1200014:  return KEY_BLUE;
    default:
      return KEY_UNKNOWN;
  }
}

void WaylandTextInput::OnKeysym(void* data,
                                struct text_model* text_input,
                                uint32_t serial,
                                uint32_t time,
                                uint32_t key,
                                uint32_t state,
                                uint32_t modifiers) {
  uint32_t key_code = KeyNumberFromKeySymCode(key, modifiers);
  if (key_code == KEY_UNKNOWN)
    return;

  // Copied from WaylandKeyboard::OnKeyNotify().
  ui::EventType type = ui::ET_KEY_PRESSED;
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  dispatcher->SetSerial(serial);
  if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    type = ui::ET_KEY_RELEASED;
  const uint32_t device_id = wl_proxy_get_id(
      reinterpret_cast<wl_proxy*>(text_input));

  uint32_t flag = kIMEModifierFlagAlt;
  while (flag) {
    dispatcher->TextInputModifier(state, GetModifierKey(flag & modifiers));
    flag = flag >> 1;
  }

  WaylandTextInput* wl_text_input = static_cast<WaylandTextInput*>(data);
  dispatcher->KeyNotify(type, key_code, ui::SOURCE_TYPE_VKB, device_id);

  bool hide_ime = false;
  if (key_code == KEY_PREVIOUS || key_code == KEY_UP || key_code == KEY_DOWN)
    if (wl_text_input->state_ == InputPanelHidden)
      hide_ime = true;

  if (state == WL_KEYBOARD_KEY_STATE_RELEASED &&
      (key_code == KEY_ENTER || key_code == KEY_KPENTER) &&
      (wl_text_input->input_content_type_ !=
       ui::InputContentType::INPUT_CONTENT_TYPE_TEXT_AREA))
    hide_ime = true;

  if (key_code == KEY_TAB)
    hide_ime = true;

  if (hide_ime && wl_text_input->last_active_window_)
    dispatcher->HideIme(wl_text_input->last_active_window_->Handle(),
                        ui::IME_CLOSE);
}

void WaylandTextInput::OnEnter(void* data,
                               struct text_model* text_input,
                               struct wl_surface* surface) {
  WaylandTextInput* webos_text_input = static_cast<WaylandTextInput*>(data);
  webos_text_input->hidden_type_ = ui::IME_ACTIVE;
}

void WaylandTextInput::OnLeave(void* data,
                               struct text_model* text_input) {
  static_cast<WaylandTextInput*>(data)->hidden_type_ = ui::IME_RESTORE;
}

void WaylandTextInput::OnInputPanelState(void* data,
                               struct text_model* text_input,
                               uint32_t state) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();
  instance->state_ = static_cast<InputPanelState>(state);

  switch (state) {
    case InputPanelShown:
      if (instance->last_active_window_)
        dispatcher->InputPanelStateChanged(
            instance->last_active_window_->Handle(),
            webos::InputPanelState::INPUT_PANEL_SHOWN);
      break;
    case InputPanelHidden:
      instance->SetHiddenState();
      break;
    default:
      break;
  }

  if (instance->is_visible_ && instance->last_active_window_ && state == InputPanelHidden) {
    dispatcher->HideIme(instance->last_active_window_->Handle(),
                        instance->hidden_type_);
  }
}

void WaylandTextInput::OnTextModelInputPanelRect(void *data,
                      struct text_model *text_model,
                      int32_t x,
                      int32_t y,
                      uint32_t width,
                      uint32_t height) {
  WaylandTextInput* instance = static_cast<WaylandTextInput*>(data);
  WaylandDisplay* dispatcher = WaylandDisplay::GetInstance();

  gfx::Rect oldRect(instance->input_panel_rect_);
  instance->input_panel_rect_.SetRect(x, y, width, height);

  if (instance->last_active_window_ && instance->input_panel_rect_ != oldRect)
    dispatcher->InputPanelRectChanged(instance->last_active_window_->Handle(),
                                      x, y, width, height);
}

}  // namespace ozonewayland
