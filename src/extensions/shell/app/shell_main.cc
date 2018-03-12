// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "content/public/app/content_main.h"
#include "extensions/shell/app/shell_main_delegate.h"

#if defined(OS_WIN)
#include "content/public/app/sandbox_helper_win.h"
#include "sandbox/win/src/sandbox_types.h"
#endif

#if defined(OS_MACOSX)
#include "extensions/shell/app/shell_main_mac.h"
#endif

#if defined(OS_MACOSX)
int main(int argc, const char** argv) {
  // Do the delegate work in shell_main_mac to avoid having to export the
  // delegate types.
  return ::ContentMain(argc, argv);
}
#elif defined(OS_WIN)
int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int) {
  extensions::ShellMainDelegate delegate;
  content::ContentMainParams params(&delegate);

  sandbox::SandboxInterfaceInfo sandbox_info = {0};
  content::InitializeSandboxInfo(&sandbox_info);
  params.instance = instance;
  params.sandbox_info = &sandbox_info;

  return content::ContentMain(params);
}
#else  // non-Mac POSIX
int main(int argc, const char** argv) {
#if defined(OS_WEBOS)
  // During stress test some times XDG_RUNTIME_DIR environment variable is
  // not set. This leads to crash later at display::Display. So, just like
  // run script, do the fail safe check and set of XDG_RUNTIME_DIR here.
  char* xdg_dir = getenv("XDG_RUNTIME_DIR");
  if (!xdg_dir) {
    int return_val = setenv("XDG_RUNTIME_DIR", "/tmp/xdg", true);
    if (return_val) {
      printf("%s::%s >>> %d is XDG_RUNTIME_DIR is not set\n", __FILE__,
             __FUNCTION__, __LINE__);
      return -1;
    }
  }
#endif
  extensions::ShellMainDelegate delegate;
  content::ContentMainParams params(&delegate);

  params.argc = argc;
  params.argv = argv;

  return content::ContentMain(params);
}
#endif
