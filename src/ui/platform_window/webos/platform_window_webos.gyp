# Copyright 2017 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [{
    'target_name': 'platform_window_webos',
    'type': '<(component)',
    'dependencies': [
      '../../../base/base.gyp:base',
    ],
    'sources': [
      'window_group_configuration.cc',
      'window_group_configuration.h',
    ],
  },
  {
    'target_name': 'platform_window_webos_ipc',
    'type': '<(component)',
    'dependencies': [
      '<(DEPTH)/base/base.gyp:base',
      '<(DEPTH)/ipc/ipc.gyp:ipc',
      'platform_window_webos',
    ],
    'defines': [
      'PLATFORM_WINDOW_WEBOS_IPC_IMPLEMENTATION',
    ],
    'sources': [
      'ipc/window_group_configuration_param_traits.cc',
      'ipc/window_group_configuration_param_traits.h',
      'ipc/window_group_configuration_param_traits_macros.h',
    ],
  }],
}