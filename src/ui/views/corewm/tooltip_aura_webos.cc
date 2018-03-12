// Copyright (c) 2016 LG Electronics, Inc.

#include "ui/views/corewm/tooltip_aura_webos.h"

#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace views {

namespace corewm {

namespace {
const int kTooltipMaxWidthPixels = 400;
}

// static
TooltipAuraWebos* TooltipAuraWebos::webos_tooltip_ = NULL;

// static
TooltipAuraWebos* TooltipAuraWebos::Get() {
  if (!webos_tooltip_)
    webos_tooltip_ = new TooltipAuraWebos();

  return webos_tooltip_;
}

TooltipAuraWebos::TooltipAuraWebos()
    : delegate_(NULL) {
}

TooltipAuraWebos::~TooltipAuraWebos() {
}

////////////////////////////////////////////////////////////////////////////////
// WebosTooltip, views::corewm::Tooltip overrides:
////////////////////////////////////////////////////////////////////////////////
void TooltipAuraWebos::SetText(aura::Window* window,
                          const base::string16& tooltip_text,
                          const gfx::Point& location) {
  if (delegate_)
    delegate_->SetText(window, tooltip_text, location);
}

void TooltipAuraWebos::Show() {
  if (delegate_)
    delegate_->Show();
}

void TooltipAuraWebos::Hide() {
  if (delegate_)
    delegate_->Hide();
}

bool TooltipAuraWebos::IsVisible() {
  return delegate_ && delegate_->IsVisible();
}

int TooltipAuraWebos::GetMaxWidth(const gfx::Point& location) const {
  display::Screen* screen = display::Screen::GetScreen();
  gfx::Rect display_bounds(screen->GetDisplayNearestPoint(location).bounds());
  return std::min(kTooltipMaxWidthPixels, (display_bounds.width() + 1) / 2);
}

} // namespace corewm

} // namespace views
