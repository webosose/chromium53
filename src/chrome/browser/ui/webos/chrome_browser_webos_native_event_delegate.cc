// Copyright (c) 2017 LG Electronics, Inc.

#include "chrome/browser/ui/webos/chrome_browser_webos_native_event_delegate.h"

#if defined(OS_WEBOS)
#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/browser_list.h"

ChromeBrowserWebOSNativeEventDelegate::ChromeBrowserWebOSNativeEventDelegate(
    BrowserView* view) : browser_view(view) {
}

void ChromeBrowserWebOSNativeEventDelegate::OnClose() {
  chrome::CloseWindow(browser_view->browser());
}

// Overridden from WebOSNativeEventDelegate:
void ChromeBrowserWebOSNativeEventDelegate::WindowHostClose() {
  base::MessageLoopForUI::current()->task_runner().get()->PostTask(
      FROM_HERE, base::Bind(&ChromeBrowserWebOSNativeEventDelegate::OnClose,
                            base::Unretained(this)));
}
#endif

