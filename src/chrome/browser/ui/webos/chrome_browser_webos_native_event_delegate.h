// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef CHROME_BROWSER_UI_CHROME_BROWSER_WEBOS_NATIVE_EVENT_DELEGATE_H_
#define CHROME_BROWSER_UI_CHROME_BROWSER_WEBOS_NATIVE_EVENT_DELEGATE_H_

#include "base/macros.h"

#if defined(OS_WEBOS)
#include "webos/common/webos_native_event_delegate.h"

class BrowserView;

class ChromeBrowserWebOSNativeEventDelegate
    : public webos::WebOSNativeEventDelegate
{
 public:
  explicit ChromeBrowserWebOSNativeEventDelegate(BrowserView* view);
  // Overridden from WebOSNativeEventDelegate:
  void WindowHostClose() override;

 private:
  void OnClose();

  BrowserView* browser_view;
};  // class
#endif

#endif // CHROME_BROWSER_UI_CHROME_BROWSER_WEBOS_NATIVE_EVENT_DELEGATE_H_
