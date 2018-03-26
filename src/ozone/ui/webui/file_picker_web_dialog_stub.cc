// Copyright 2018 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/webui/file_picker_web_dialog.h"

namespace ui {

// static
void FilePickerWebDialog::ShowDialog(SelectFileDialog::Type type,
                                     gfx::NativeWindow owning_window,
                                     content::WebContents* contents,
                                     SelectFileDialog::Listener* listener) {
// For using select file dialog webui implementation remove this stub and
// apply ozone/patches/0008-Add-file-picker-support-using-WebUI.patch
  NOTIMPLEMENTED();
}

}  // namespace ui
