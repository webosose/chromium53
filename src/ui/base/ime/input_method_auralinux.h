// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_INPUT_METHOD_AURALINUX_H_
#define UI_BASE_IME_INPUT_METHOD_AURALINUX_H_

#include <memory>

#include "base/macros.h"
#include "ui/base/ime/composition_text.h"
#include "ui/base/ime/input_method_base.h"
#include "ui/base/ime/linux/linux_input_method_context.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

// A ui::InputMethod implementation for Aura on Linux platforms. The
// implementation details are separated to ui::LinuxInputMethodContext
// interface.
class UI_BASE_IME_EXPORT InputMethodAuraLinux
    : public InputMethodBase,
      public LinuxInputMethodContextDelegate {
 public:
  explicit InputMethodAuraLinux(
      gfx::AcceleratedWidget widget,
      internal::InputMethodDelegate* delegate);
  ~InputMethodAuraLinux() override;

  virtual std::unique_ptr<LinuxInputMethodContext> CreateInputMethodContext(
      gfx::AcceleratedWidget widget,
      bool is_simple);

  LinuxInputMethodContext* GetContext(bool is_simple) const;
  LinuxInputMethodContext* GetContextForTesting(bool is_simple);

  // Overriden from InputMethod.
  void OnFocus() override;
  void DetachTextInputClient(TextInputClient* client) override;
  bool OnUntranslatedIMEMessage(const base::NativeEvent& event,
                                NativeEventResult* result) override;
  void DispatchKeyEvent(ui::KeyEvent* event) override;
  void OnTextInputTypeChanged(const TextInputClient* client) override;
  void OnCaretBoundsChanged(const TextInputClient* client) override;
  void CancelComposition(const TextInputClient* client) override;
  void OnInputLocaleChanged() override;
  std::string GetInputLocale() override;
  bool IsCandidatePopupOpen() const override;
  void ShowImeIfNeeded() override;
  void HideIme(ui::ImeHiddenType hidden_type) override;

  // Overriden from ui::LinuxInputMethodContextDelegate
  void OnCommit(const base::string16& text) override;
  void OnPreeditChanged(const CompositionText& composition_text) override;
  void OnPreeditEnd() override;
  void OnPreeditStart() override{};
  void OnDeleteRange(int32_t index, uint32_t length) override;
  void OnHideIme(ui::ImeHiddenType hidden_type) override;
  void SetImeEnabled(bool enable) override;
  void SetImeSupported(bool enable) override;
  gfx::Rect GetInputPanelRect() const override;
  gfx::Rect GetCaretBounds() const override;
  bool IsVisible() const override;
  bool IsInputMethodActive() const override;

 protected:
  // Overridden from InputMethodBase.
  void OnWillChangeFocusedClient(TextInputClient* focused_before,
                                 TextInputClient* focused) override;
  void OnDidChangeFocusedClient(TextInputClient* focused_before,
                                TextInputClient* focused) override;

 private:
  bool HasInputMethodResult();
  bool NeedInsertChar() const;
  ui::EventDispatchDetails SendFakeProcessKeyEvent(ui::KeyEvent* event) const;
  void ConfirmCompositionText();
  void UpdateContextFocusState();
  void ResetContext();
  bool CanShowIme();

  // Processes the key event after the event is processed by the system IME or
  // the extension.
  void ProcessKeyEventDone(ui::KeyEvent* event, bool filtered, bool is_handled);

  // Callback function for IMEEngineHandlerInterface::ProcessKeyEvent().
  // It recovers the context when the event is being passed to the extension and
  // call ProcessKeyEventDone() for the following processing. This is necessary
  // as this method is async. The environment may be changed by other generated
  // key events by the time the callback is run.
  void ProcessKeyEventByEngineDone(ui::KeyEvent* event,
                                   bool filtered,
                                   bool composition_changed,
                                   ui::CompositionText* composition,
                                   base::string16* result_text,
                                   bool is_handled);

  std::unique_ptr<LinuxInputMethodContext> context_;
  std::unique_ptr<LinuxInputMethodContext> context_simple_;

  base::string16 result_text_;

  ui::CompositionText composition_;

  // The current text input type used to indicates if |context_| and
  // |context_simple_| are focused or not.
  TextInputType text_input_type_;

  // Indicates if currently in sync mode when handling a key event.
  // This is used in OnXXX callbacks from GTK IM module.
  bool is_sync_mode_;

  // Indicates if the composition text is changed or deleted.
  bool composition_changed_;

  // If it's true then all input method result received before the next key
  // event will be discarded.
  bool suppress_next_result_;
  bool is_ime_enabled_;
  bool is_ime_supported_;

  // Used for making callbacks.
  base::WeakPtrFactory<InputMethodAuraLinux> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodAuraLinux);
};

}  // namespace ui

#endif  // UI_BASE_IME_INPUT_METHOD_AURALINUX_H_
