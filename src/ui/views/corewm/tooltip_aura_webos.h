// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef UI_VIEWS_COREWM_TOOLTIP_AURA_WEBOS_H_
#define UI_VIEWS_COREWM_TOOLTIP_AURA_WEBOS_H_

#include "base/macros.h"
#include "ui/views/corewm/tooltip.h"

namespace views {

namespace corewm {

class VIEWS_EXPORT TooltipAuraWebos : public views::corewm::Tooltip {
 public:
  static TooltipAuraWebos* Get();

  class Delegate {
   public:
    virtual void SetText(aura::Window* window,
                          const base::string16& tooltip_text,
                          const gfx::Point& location) = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual bool IsVisible() = 0;
  };

  void SetTooltipAuraWebosDelegate(Delegate* delegate) {
    delegate_ = delegate;
  }

 private:
  TooltipAuraWebos();
  ~TooltipAuraWebos();

  // views::corewm::Tooltip overrides:
  void SetText(aura::Window* window,
               const base::string16& tooltip_text,
               const gfx::Point& location) override;
  void Show() override;
  void Hide() override;
  bool IsVisible() override;
  int GetMaxWidth(const gfx::Point& location) const override;

  Delegate* delegate_;

  static TooltipAuraWebos* webos_tooltip_;

  DISALLOW_COPY_AND_ASSIGN(TooltipAuraWebos);
};

} // namespace corewm

} // namespace views

#endif  // UI_VIEWS_COREWM_TOOLTIP_AURA_WEBOS_H_
