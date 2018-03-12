# Copyright 2014 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    # For using select file dialog webui implementation please apply
    # ozone/patches/0008-Add-file-picker-support-using-WebUI.patch
    'use_select_file_dialog_webui_impl%': 0,
  },

  'targets': [
    {
      'target_name': 'webui',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/ui/accessibility/accessibility.gyp:ax_gen',
        '<(DEPTH)/ui/base/ui_base.gyp:ui_base',
        '<(DEPTH)/build/linux/system.gyp:pangocairo',
        '<(DEPTH)/ui/gfx/ipc/gfx_ipc.gyp:gfx_ipc',
        '<(DEPTH)/ipc/ipc.gyp:ipc',
      ],
      'sources': [
        'ozone_webui.h',
        'ozone_webui.cc',
        'input_method_context_impl_wayland.h',
        'input_method_context_impl_wayland.cc',
      ],
      'conditions': [
        ['<(use_select_file_dialog_webui_impl)==1', {
          'defines': [
            'USE_SELECT_FILE_DIALOG_WEBUI_IMPL',
          ],
          'sources': [
            'file_picker_web_dialog.h',
            'select_file_dialog_impl_webui.h',
            'select_file_dialog_impl_webui.cc',
          ],
        }],
      ],
    }
  ]
}
