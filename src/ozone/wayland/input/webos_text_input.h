// Copyright (c) 2014-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef OZONE_WAYLAND_TEXT_INPUT_H_
#define OZONE_WAYLAND_TEXT_INPUT_H_

#include "ozone/platform/input_content_type.h"
#include "ozone/wayland/display.h"

struct text_model;

namespace base {
template <typename Type>
struct DefaultSingletonTraits;
}

namespace ozonewayland {

class WaylandWindow;
class WaylandSeat;

class WaylandTextInput {
 public:
  static WaylandTextInput* GetInstance();

  void ResetIme();
  void ShowInputPanel(wl_seat* input_seat, unsigned windowhandle);
  void HideInputPanel(wl_seat* input_seat);
  void SetActiveWindow(WaylandWindow* window);
  void SetHiddenState();
  void SetInputContentType(ui::InputContentType content_type,
                           int text_input_flags);
  void SetSurroundingText(const std::string& text,
                          size_t cursor_position,
                          size_t anchor_position);

  void OnWindowAboutToDestroy(unsigned windowhandle);

  static void OnCommitString(void* data,
                             struct text_model* text_input,
                             uint32_t serial,
                             const char* text);

  static void OnPreeditString(void* data,
                             struct text_model* text_input,
                             uint32_t serial,
                             const char* text,
                             const char* commit);

  static void OnDeleteSurroundingText(void* data,
                             struct text_model* text_input,
                             uint32_t serial,
                             int32_t index,
                             uint32_t length);

  static void OnCursorPosition(void* data,
                             struct text_model* text_input,
                             uint32_t serial,
                             int32_t index,
                             int32_t anchor);

  static void OnPreeditStyling(void* data,
                             struct text_model* text_input,
                             uint32_t serial,
                             uint32_t index,
                             uint32_t length,
                             uint32_t style);

  static void OnPreeditCursor(void* data,
                            struct text_model* text_input,
                            uint32_t serial,
                            int32_t index);

  static uint32_t KeyNumberFromKeySymCode(uint32_t key_sym);

  static void OnModifiersMap(void* data,
                            struct text_model* text_input,
                            struct wl_array* map);

  static void OnKeysym(void* data,
                       struct text_model* text_input,
                       uint32_t serial,
                       uint32_t time,
                       uint32_t key,
                       uint32_t state,
                       uint32_t modifiers);

  static void OnEnter(void* data,
                      struct text_model* text_input,
                      struct wl_surface* surface);

  static void OnLeave(void* data,
                      struct text_model* text_input);

  static void OnInputPanelState(void* data,
                      struct text_model* text_input,
                      uint32_t state);

  static void OnTextModelInputPanelRect(void *data,
                      struct text_model *text_input,
                      int32_t x,
                      int32_t y,
                      uint32_t width,
                      uint32_t height);

private:
  enum InputPanelState {
    InputPanelUnknownState = 0xffffffff,
    InputPanelHidden = 0,
    InputPanelShown = 1,
    InputPanelShowing = 2
  };

  friend struct base::DefaultSingletonTraits<WaylandTextInput>;

  WaylandTextInput();
  ~WaylandTextInput();

  gfx::Rect input_panel_rect_;
  struct text_model* text_model_;
  bool is_visible_;
  InputPanelState state_;
  ui::ImeHiddenType hidden_type_;
  ui::InputContentType input_content_type_;
  int text_input_flags_;
  WaylandWindow* active_window_;
  WaylandWindow* last_active_window_;

  DISALLOW_COPY_AND_ASSIGN(WaylandTextInput);
};

} // namespace ozonewayland

#endif  // OZONE_WAYLAND_TEXT_INPUT_H_
