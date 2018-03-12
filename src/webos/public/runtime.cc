// Copyright (c) 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/public/runtime.h"

#include "webos/public/runtime_delegates.h"

namespace webos {

Runtime* Runtime::GetInstance() {
  return base::Singleton<Runtime>::get();
}

Runtime::Runtime()
    : luna_service_delegate_(NULL),
      cookie_store_util_delegate_(NULL),
      platform_delegate_(NULL),
      is_mrcu_paired_(false),
      is_network_connected_(true),
      is_foreground_app_enyo_(false),
      is_main_getter_created_(false) {}

Runtime::~Runtime() {}

void Runtime::InitializeLunaService(
    LunaServiceDelegate* luna_service_delegate) {
  luna_service_delegate_ = luna_service_delegate;
}

void Runtime::InitializeCookieStoreUtil(
    CookieStoreUtilDelegate* cookie_store_util_delegate) {
  cookie_store_util_delegate_ = cookie_store_util_delegate;
}

LSHandle* Runtime::GetLSHandle() {
  if (luna_service_delegate_)
    return luna_service_delegate_->GetLSHandle();
  return NULL;
}

void Runtime::FlushStoreCookie(PowerOffState power_off_state,
                               std::string timestamp) {
  if (cookie_store_util_delegate_)
    cookie_store_util_delegate_->FlushStoreCookie(power_off_state, timestamp);
}

void Runtime::OnCursorVisibilityChanged(bool visible) {
  if (platform_delegate_)
    platform_delegate_->OnCursorVisibilityChanged(visible);
}

void Runtime::SetNetworkConnected(bool is_connected) {
  if (is_network_connected_ == is_connected)
    return;

  is_network_connected_ = is_connected;

  if (platform_delegate_)
    platform_delegate_->OnNetworkStateChanged(is_connected);
}

void Runtime::SetLocale(std::string locale) {
  if (current_locale_ == locale)
    return;

  current_locale_ = locale;

  if (platform_delegate_)
    platform_delegate_->OnLocaleInfoChanged(locale);
}

}  // namespace webos
