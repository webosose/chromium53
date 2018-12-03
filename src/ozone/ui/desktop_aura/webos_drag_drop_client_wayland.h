// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#ifndef OZONE_UI_DESKTOP_AURA_WEBOS_DRAG_DROP_CLIENT_WAYLAND_H_
#define OZONE_UI_DESKTOP_AURA_WEBOS_DRAG_DROP_CLIENT_WAYLAND_H_

#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ozone/ui/desktop_aura/drag_image_view.h"
#include "ui/aura/window_observer.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/event.h"
#include "ui/events/event_handler.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/views_export.h"
#include "ui/wm/public/drag_drop_client.h"

namespace gfx {
class LinearAnimation;
class Point;
}

namespace ui {
class OSExchangeData;
class PlatformWindow;
}

namespace views {
class DragDropTracker;
class DragDropTrackerDelegate;

class VIEWS_EXPORT WebosDragDropClientWayland
    : public aura::client::DragDropClient,
      public ui::EventHandler,
      public gfx::AnimationDelegate,
      public aura::WindowObserver {
 public:
  WebosDragDropClientWayland(aura::Window* window,
                             ui::PlatformWindow* platform_window);
  virtual ~WebosDragDropClientWayland();

  // Overridden from aura::client::DragDropClient:
  int StartDragAndDrop(
      const ui::OSExchangeData& data,
      aura::Window* root_window,
      aura::Window* source_window,
      const gfx::Point& root_location,
      int operation,
      ui::DragDropTypes::DragEventSource source) override;
  void DragUpdate(aura::Window* target,
                  const ui::LocatedEvent& event) override;
  void Drop(aura::Window* target, const ui::LocatedEvent& event) override;
  void DragCancel() override;
  bool IsDragDropInProgress() override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnTouchEvent(ui::TouchEvent* event) override { }
  void OnGestureEvent(ui::GestureEvent* event) override { }

  // Overridden from aura::WindowObserver.
  void OnWindowDestroyed(aura::Window* window) override;

  // Events received via IPC from the WaylandDataSource in the GPU process.
  void OnDragEnter(gfx::AcceleratedWidget windowhandle,
                   float x,
                   float y,
                   const std::vector<std::string>& mime_types,
                   uint32_t serial);
  void OnDragDataReceived(int pipefd);
  void OnDragLeave();
  void OnDragMotion(float x, float y, uint32_t time);
  void OnDragDrop();

 protected:
  // Helper method to create a LinearAnimation object that will run the drag
  // cancel animation. Caller take ownership of the returned object. Protected
  // for testing.
  gfx::LinearAnimation* CreateCancelAnimation(int duration, int frame_rate,
                                              gfx::AnimationDelegate* delegate);

  // Actual implementation of |DragCancel()|. protected for testing.
  void DoDragCancel(int drag_cancel_animation_duration_ms);

 private:
  // Overridden from gfx::AnimationDelegate:
  void AnimationEnded(const gfx::Animation* animation) override;
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationCanceled(const gfx::Animation* animation) override;

  // Helper method to start drag widget flying back animation.
  void StartCanceledAnimation(int animation_duration_ms);

  void Cleanup();

  std::unique_ptr<DragImageView> drag_image_;
  gfx::Vector2d drag_image_offset_;
  const ui::OSExchangeData* drag_data_;
  int drag_operation_;

  aura::Window* root_window_;

  // Window that is currently under the drag cursor.
  aura::Window* drag_window_;

  // Starting and final bounds for the drag image for the drag cancel animation.
  gfx::Rect drag_image_initial_bounds_for_cancel_animation_;
  gfx::Rect drag_image_final_bounds_for_cancel_animation_;

  std::unique_ptr<gfx::LinearAnimation> cancel_animation_;

  // The location of |held_move_event_| is in |window_|'s coordinate.
  std::unique_ptr<ui::LocatedEvent> held_move_event_;

  // Window that started the drag.
  aura::Window* drag_source_window_;

  // Indicates whether the caller should be blocked on a drag/drop session.
  // Only be used for tests.
  bool should_block_during_drag_drop_;

  std::unique_ptr<views::DragDropTracker> drag_drop_tracker_;
  std::unique_ptr<DragDropTrackerDelegate> drag_drop_window_delegate_;

  ui::DragDropTypes::DragEventSource current_drag_event_source_;

  // Closure for quitting nested message loop.
  base::Closure quit_closure_;

  base::WeakPtrFactory<WebosDragDropClientWayland> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebosDragDropClientWayland);
};

}  // namespace views

#endif  // OZONE_UI_DESKTOP_AURA_WEBOS_DRAG_DROP_CLIENT_WAYLAND_H_
