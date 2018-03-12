// Copyright (c) 2017 LG Electronics, Inc.

#include "chrome/browser/ui/webos/chrome_browser_webos_native_event_delegate.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/ui/browser_commands.h"

// static
ChromeBrowserWebOSNativeEventDelegate*
    ChromeBrowserWebOSNativeEventDelegate::delegate_ = NULL;

// static
ChromeBrowserWebOSNativeEventDelegate*
ChromeBrowserWebOSNativeEventDelegate::Get() {
  return delegate_;
}

// static
void ChromeBrowserWebOSNativeEventDelegate::Initialize(Browser* browser) {
  if (!delegate_) {
    delegate_ = new ChromeBrowserWebOSNativeEventDelegate();
    delegate_->SetBrowser(browser);
  }
}

ChromeBrowserWebOSNativeEventDelegate::
    ~ChromeBrowserWebOSNativeEventDelegate() {
  delegate_ = NULL;
}

#if defined(USE_OZONE)
void ChromeBrowserWebOSNativeEventDelegate::OnClose() {
  chrome::CloseWindow(browser_);
}

// Overridden from WebOSNativeEventDelegate:
void ChromeBrowserWebOSNativeEventDelegate::WindowHostClose() {
  base::MessageLoopForUI::current()->task_runner().get()->PostTask(
      FROM_HERE, base::Bind(&ChromeBrowserWebOSNativeEventDelegate::OnClose,
                            base::Unretained(this)));
}
#endif

////////////////////////////////////////////////////////////////////////////
// Private APIs, constructor : ChromeBrowserWebOSNativeEventDelegate
ChromeBrowserWebOSNativeEventDelegate::ChromeBrowserWebOSNativeEventDelegate() {
  delegate_ = this;
}

