// Copyright 2018 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/webui/file_picker_web_dialog.h"

#if defined(PLATFORM_APOLLO)
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/simple_message_box.h"
#endif

namespace ui {

// static
void FilePickerWebDialog::ShowDialog(SelectFileDialog::Type type,
                                     gfx::NativeWindow owning_window,
                                     content::WebContents* contents,
                                     SelectFileDialog::Listener* listener) {
// For using select file dialog webui implementation remove this stub and
// apply ozone/patches/0008-Add-file-picker-support-using-WebUI.patch
#if defined(PLATFORM_APOLLO)
  chrome::ShowWarningMessageBox(NULL, base::ASCIIToUTF16("WARNING"),
      base::ASCIIToUTF16("Download/Upload isn't supported"));
#else
  NOTIMPLEMENTED();
#endif
}

}  // namespace ui
