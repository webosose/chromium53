// Copyright 2016-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/app/webos_main.h"

#include "content/public/app/content_main.h"
#include "webos/app/webos_content_main_delegate.h"
#include "webos/public/runtime.h"

namespace webos {

WebOSMain::WebOSMain(WebOSMainDelegate* delegate) : delegate_(delegate) {}

int WebOSMain::Run(int argc, const char** argv) {
  WebOSContentMainDelegate webos_main_delegate;
  webos_main_delegate.SetBrowserStartupCallback(
      base::Bind(&WebOSMainDelegate::AboutToCreateContentBrowserClient,
                 base::Unretained(delegate_)));
  content::ContentMainParams params(&webos_main_delegate);

  params.argc = argc;
  params.argv = argv;

  return content::ContentMain(params);
}

}  // namespace webos
