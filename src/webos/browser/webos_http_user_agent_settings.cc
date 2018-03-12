// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/browser/webos_http_user_agent_settings.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace webos {

WebOSHttpUserAgentSettings::WebOSHttpUserAgentSettings() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

WebOSHttpUserAgentSettings::~WebOSHttpUserAgentSettings() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

std::string WebOSHttpUserAgentSettings::GetAcceptLanguage() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // Note: Please take a look at WebOSNetworkDelegate::OnBeforeURLRequest if you
  // want to override accept-language header
  return std::string();
}

std::string WebOSHttpUserAgentSettings::GetUserAgent() const {
  return std::string();
}

}  // namespace webos
