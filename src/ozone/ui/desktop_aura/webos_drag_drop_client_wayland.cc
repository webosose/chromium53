// # Copyright (c) 2015 LG Electronics, Inc.

#include "ozone/ui/desktop_aura/webos_drag_drop_client_wayland.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "ozone/ui/desktop_aura/drag_drop_tracker.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/path.h"
#include "ui/wm/public/drag_drop_delegate.h"

namespace views {

namespace {
// The duration of the drag cancel animation in millisecond.
const int kCancelAnimationDuration = 250;

// The frame rate of the drag cancel animation in hertz.
const int kCancelAnimationFrameRate = 60;

gfx::Rect AdjustDragImageBoundsForScaleAndOffset(
    const gfx::Rect& drag_image_bounds,
    int vertical_offset,
    float scale,
    gfx::Vector2d* drag_image_offset) {
  gfx::Point final_origin = drag_image_bounds.origin();
  gfx::SizeF final_size = gfx::SizeF(drag_image_bounds.size());
  final_size.Scale(scale);
  drag_image_offset->set_x(drag_image_offset->x() * scale);
  drag_image_offset->set_y(drag_image_offset->y() * scale);
  float total_x_offset = drag_image_offset->x();
  float total_y_offset = drag_image_offset->y() - vertical_offset;
  final_origin.Offset(-total_x_offset, -total_y_offset);
  return gfx::ToEnclosingRect(
      gfx::RectF(gfx::PointF(final_origin), final_size));
}

}

class DragDropTrackerDelegate : public aura::WindowDelegate {
 public:
  explicit DragDropTrackerDelegate(WebosDragDropClientWayland* controller)
      : drag_drop_controller_(controller) {}
  virtual ~DragDropTrackerDelegate() {}

  // Overridden from WindowDelegate:
  gfx::Size GetMinimumSize() const override {
    return gfx::Size();
  }

  gfx::Size GetMaximumSize() const override {
    return gfx::Size();
  }

  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override {}
  gfx::NativeCursor GetCursor(const gfx::Point& point) override {
    return gfx::kNullCursor;
  }
  int GetNonClientComponent(const gfx::Point& point) const override {
    return HTCAPTION;
  }
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child, const gfx::Point& location) override {
    return true;
  }
  bool CanFocus() override { return true; }
  void OnCaptureLost() override {
    if (drag_drop_controller_->IsDragDropInProgress())
      drag_drop_controller_->DragCancel();
  }
  void OnPaint(const ui::PaintContext& context) override {}
  void OnDeviceScaleFactorChanged(float device_scale_factor) override {}
  void OnWindowDestroying(aura::Window* window) override {}
  void OnWindowDestroyed(aura::Window* window) override {}
  void OnWindowTargetVisibilityChanged(bool visible) override {}
  bool HasHitTestMask() const override {
    return true;
  }
  void GetHitTestMask(gfx::Path* mask) const override {
    DCHECK(mask->isEmpty());
  }

 private:
  WebosDragDropClientWayland* drag_drop_controller_;

  DISALLOW_COPY_AND_ASSIGN(DragDropTrackerDelegate);
};

////////////////////////////////////////////////////////////////////////////////
// WebosDragDropClientWayland, public:

WebosDragDropClientWayland::WebosDragDropClientWayland(aura::Window* window,
                                                       ui::PlatformWindow*)
    : drag_data_(NULL),
      drag_operation_(0),
      root_window_(window),
      drag_window_(NULL),
      drag_source_window_(NULL),
      should_block_during_drag_drop_(true),
      drag_drop_window_delegate_(new DragDropTrackerDelegate(this)),
      current_drag_event_source_(ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE),
      weak_factory_(this) {
  if (root_window_)
    root_window_->AddPreTargetHandler(this);
}

WebosDragDropClientWayland::~WebosDragDropClientWayland() {
  if (root_window_)
    root_window_->RemovePreTargetHandler(this);

  Cleanup();

  if (cancel_animation_)
    cancel_animation_->End();

  if (drag_image_)
    drag_image_.reset();
}

int WebosDragDropClientWayland::StartDragAndDrop(
    const ui::OSExchangeData& data,
    aura::Window* root_window,
    aura::Window* source_window,
    const gfx::Point& root_location,
    int operation,
    ui::DragDropTypes::DragEventSource source) {
  if (IsDragDropInProgress())
    return 0;

  const ui::OSExchangeData::Provider* provider = &data.provider();
  if (source == ui::DragDropTypes::DRAG_EVENT_SOURCE_TOUCH &&
      provider->GetDragImage().size().IsEmpty())
    return 0;

  current_drag_event_source_ = source;
  DragDropTracker* tracker =
      new DragDropTracker(root_window, drag_drop_window_delegate_.get());
  tracker->TakeCapture();
  drag_drop_tracker_.reset(tracker);

  drag_source_window_ = source_window;
  if (drag_source_window_)
    drag_source_window_->AddObserver(this);

  drag_data_ = &data;
  drag_operation_ = operation;

  gfx::Point start_location = root_location;
  provider->GetDragImage();
  provider->GetDragImageOffset();

  drag_image_final_bounds_for_cancel_animation_ = gfx::Rect(
      start_location - provider->GetDragImageOffset(),
      provider->GetDragImage().size());

  drag_image_.reset(new DragImageView(source_window->GetRootWindow(), source));
  drag_image_->SetImage(provider->GetDragImage());
  drag_image_offset_ = provider->GetDragImageOffset();

  gfx::Rect drag_image_bounds(start_location, drag_image_->GetPreferredSize());

  drag_image_->SetBoundsInScreen(drag_image_bounds);
  drag_image_->SetWidgetVisible(true);

  drag_window_ = NULL;

  if (cancel_animation_)
    cancel_animation_->End();

  if (should_block_during_drag_drop_) {
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    base::MessageLoopForUI* loop = base::MessageLoopForUI::current();
    base::MessageLoop::ScopedNestableTaskAllower allow_nested(loop);
    run_loop.Run();
  }

  if (!cancel_animation_.get() || !cancel_animation_->is_animating()) {
    if (drag_source_window_)
      drag_source_window_->RemoveObserver(this);
    drag_source_window_ = NULL;
  }

  return drag_operation_;
}

void WebosDragDropClientWayland::DragUpdate(
    aura::Window* target, const ui::LocatedEvent& event) {
  aura::client::DragDropDelegate* delegate = NULL;

  if (target != drag_window_) {
    if (drag_window_) {
      if ((delegate = aura::client::GetDragDropDelegate(drag_window_)))
        delegate->OnDragExited();
      if (drag_window_ != drag_source_window_)
        drag_window_->RemoveObserver(this);
    }
    drag_window_ = target;

    if (drag_window_ != drag_source_window_)
      drag_window_->AddObserver(this);

    if ((delegate = aura::client::GetDragDropDelegate(drag_window_))) {
      ui::DropTargetEvent e(*drag_data_,
                            event.location(),
                            event.root_location(),
                            drag_operation_);
      e.set_flags(event.flags());
      delegate->OnDragEntered(e);
    }
  } else {
    if ((delegate = aura::client::GetDragDropDelegate(drag_window_))) {
      ui::DropTargetEvent e(*drag_data_,
                            event.location(),
                            event.root_location(),
                            drag_operation_);
      e.set_flags(event.flags());
      delegate->OnDragUpdated(e);

      aura::client::CursorClient* cursor_client =
          aura::client::GetCursorClient(root_window_);
      if (cursor_client)
        cursor_client->SetCursor(ui::kCursorGrabbing);
    }
  }

  DCHECK(drag_image_.get());

  if (drag_image_->visible()) {
    gfx::Point root_location_in_screen = event.root_location();
    aura::client::ScreenPositionClient* screen_position_client =
        aura::client::GetScreenPositionClient(root_window_);
    if (screen_position_client) {
      screen_position_client->ConvertPointToScreen(
          target->GetRootWindow(), &root_location_in_screen);
    }
    drag_image_->SetScreenPosition(
        root_location_in_screen - drag_image_offset_);
  }
}

void WebosDragDropClientWayland::Drop(
    aura::Window* target, const ui::LocatedEvent& event) {
  aura::client::CursorClient* cursor_client =
      aura::client::GetCursorClient(root_window_);
  if (cursor_client)
    cursor_client->SetCursor(ui::kCursorPointer);

  aura::client::DragDropDelegate* delegate = NULL;

  if (target != drag_window_)
    DragUpdate(target, event);

  DCHECK(target == drag_window_);

  if ((delegate = aura::client::GetDragDropDelegate(target))) {
    ui::DropTargetEvent e(
        *drag_data_, event.location(), event.root_location(), drag_operation_);
    e.set_flags(event.flags());
    drag_operation_ = delegate->OnPerformDrop(e);
    if (drag_operation_ == 0)
      StartCanceledAnimation(kCancelAnimationDuration);
    else
      drag_image_.reset();
  } else {
    drag_image_.reset();
  }

  Cleanup();

  if (should_block_during_drag_drop_)
    quit_closure_.Run();
}

void WebosDragDropClientWayland::DragCancel() {
  DoDragCancel(kCancelAnimationDuration);
}

bool WebosDragDropClientWayland::IsDragDropInProgress() {
  return !!drag_drop_tracker_.get();
}

void WebosDragDropClientWayland::OnKeyEvent(ui::KeyEvent* event) {
  if (IsDragDropInProgress() && event->key_code() == ui::VKEY_ESCAPE) {
    DragCancel();
    event->StopPropagation();
  }
}

void WebosDragDropClientWayland::OnMouseEvent(ui::MouseEvent* event) {
  if (!IsDragDropInProgress())
    return;

  // If current drag session was not started by mouse, dont process this mouse
  // event, but consume it so it does not interfere with current drag session.
  if (current_drag_event_source_ !=
      ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE) {
    event->StopPropagation();
    return;
  }

  aura::Window* translated_target = drag_drop_tracker_->GetTarget(*event);
  if (!translated_target) {
    DragCancel();
    event->StopPropagation();
    return;
  }

  std::unique_ptr<ui::LocatedEvent> translated_event(
      drag_drop_tracker_->ConvertEvent(translated_target, *event));

  switch (translated_event->type()) {
    case ui::ET_MOUSE_DRAGGED:
      DragUpdate(translated_target, *translated_event.get());
      break;
    case ui::ET_MOUSE_RELEASED:
      Drop(translated_target, *translated_event.get());
      break;
    default:
      // We could also reach here because RootWindow may sometimes generate a
      // bunch of fake mouse events
      // (aura::RootWindow::PostMouseMoveEventAfterWindowChange).
      break;
  }
  //event->StopPropagation();
}

void WebosDragDropClientWayland::OnWindowDestroyed(aura::Window* window) {
  if (drag_window_ == window)
    drag_window_ = NULL;
  if (drag_source_window_ == window)
    drag_source_window_ = NULL;
}

void WebosDragDropClientWayland::OnDragEnter(
    gfx::AcceleratedWidget windowhandle, float x, float y,
    const std::vector<std::string>& mime_types, uint32_t serial) {
  VLOG(1) <<  __FUNCTION__ << " windowhandle=" << windowhandle
           << " x=" << x << " y=" << y << " serial=" << serial;
}

void WebosDragDropClientWayland::OnDragDataReceived(int pipefd) {
  VLOG(1) <<  __FUNCTION__ << " pipefd=" << pipefd;
}

void WebosDragDropClientWayland::OnDragLeave() {
  VLOG(1) <<  __FUNCTION__;
}

void WebosDragDropClientWayland::OnDragMotion(
    float x, float y, uint32_t time) {
  VLOG(2) << __FUNCTION__ << " x=" << x << " y=" << y << " time=" << time;
}

void WebosDragDropClientWayland::OnDragDrop() {
  VLOG(1) <<  __FUNCTION__;
}

////////////////////////////////////////////////////////////////////////////////
// WebosDragDropClientWayland, protected:

gfx::LinearAnimation* WebosDragDropClientWayland::CreateCancelAnimation(
    int duration,
    int frame_rate,
    gfx::AnimationDelegate* delegate) {
  return new gfx::LinearAnimation(duration, frame_rate, delegate);
}

////////////////////////////////////////////////////////////////////////////////
// WebosDragDropClientWayland, private:

void WebosDragDropClientWayland::AnimationEnded(
    const gfx::Animation* animation) {
  cancel_animation_.reset();

  // By the time we finish animation, another drag/drop session may have
  // started. We do not want to destroy the drag image in that case.
  if (!IsDragDropInProgress())
    drag_image_.reset();
}

void WebosDragDropClientWayland::DoDragCancel(
    int drag_cancel_animation_duration_ms) {
  aura::client::CursorClient* cursor_client =
      aura::client::GetCursorClient(root_window_);
  if (cursor_client)
    cursor_client->SetCursor(ui::kCursorPointer);

  aura::client::DragDropDelegate* delegate = drag_window_?
      aura::client::GetDragDropDelegate(drag_window_) : NULL;
  if (delegate)
    delegate->OnDragExited();

  Cleanup();

  drag_operation_ = 0;
  StartCanceledAnimation(drag_cancel_animation_duration_ms);
  if (should_block_during_drag_drop_)
    quit_closure_.Run();
}

void WebosDragDropClientWayland::AnimationProgressed(
    const gfx::Animation* animation) {
  gfx::Rect current_bounds = animation->CurrentValueBetween(
      drag_image_initial_bounds_for_cancel_animation_,
      drag_image_final_bounds_for_cancel_animation_);
  drag_image_->SetBoundsInScreen(current_bounds);
}

void WebosDragDropClientWayland::AnimationCanceled(
    const gfx::Animation* animation) {
  AnimationEnded(animation);
}

void WebosDragDropClientWayland::StartCanceledAnimation(
    int animation_duration_ms) {
  DCHECK(drag_image_.get());

  drag_image_initial_bounds_for_cancel_animation_ =
      drag_image_->GetBoundsInScreen();
  cancel_animation_.reset(CreateCancelAnimation(animation_duration_ms,
                                                kCancelAnimationFrameRate,
                                                this));
  cancel_animation_->Start();
}

void WebosDragDropClientWayland::Cleanup()  {
  if (drag_window_)
    drag_window_->RemoveObserver(this);
  drag_window_ = NULL;
  drag_data_ = NULL;

  std::unique_ptr<views::DragDropTracker> holder = std::move(drag_drop_tracker_);
}

}  // namespace views
