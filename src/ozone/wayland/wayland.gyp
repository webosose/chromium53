# Copyright 2013 The Chromium Authors. All rights reserved.
# Copyright 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'variables': {
      'enable_drm_support%': 0,
      'use_data_device_manager%': 0,
    },
    'enable_drm_support%': '<(enable_drm_support)',
    'use_data_device_manager%': '<(use_data_device_manager)',
  },

  'targets': [
    {
      'target_name': 'wayland_toolkit',
      'type': 'static_library',
      'variables': {
        'WAYLAND_VERSION': '1.4.0',
        'wayland_packages': [
          'wayland-client >= <(WAYLAND_VERSION)',
          'wayland-cursor >= <(WAYLAND_VERSION)',
          'wayland-egl',
        ],
      },
      'defines': [
        'OZONE_WAYLAND_IMPLEMENTATION',
      ],
      'cflags': [
        '<!@(<(pkg-config) --cflags <(wayland_packages))',
      ],
      'direct_dependent_settings': {
        'cflags': [
          '<!@(<(pkg-config) --cflags <(wayland_packages))',
        ],
      },
      'link_settings': {
        'ldflags': [
          '<!@(<(pkg-config) --libs-only-L --libs-only-other <(wayland_packages))',
        ],
        'libraries': [
          '<!@(<(pkg-config) --libs-only-l <(wayland_packages))',
        ],
      },
      'dependencies': [
        '../../base/base.gyp:base',
        '../../skia/skia.gyp:skia',
        '../../ui/gfx/ipc/gfx_ipc.gyp:gfx_ipc',
        '../../ui/ozone/ozone.gyp:ozone_base',
      ],
      'include_dirs': [
        '../..',
        '<(DEPTH)/third_party/khronos',
      ],
      'conditions': [
        ['webos!=0', {
          'variables': {
            'wayland_packages': [
              'wayland-webos-client',
            ],
          },
          'dependencies': [
            '../../ui/platform_window/webos/platform_window_webos.gyp:platform_window_webos_ipc',
          ],
          'sources': [
            'input/webos_text_input.cc',
            'input/webos_text_input.h',
            'group/wayland_webos_surface_group.cc',
            'group/wayland_webos_surface_group.h',
            'group/webos_surface_group.cc',
            'group/webos_surface_group.h',
            'group/webos_surface_group_layer.cc',
            'group/webos_surface_group_layer.h',
            'group/webos_surface_group_compositor.cc',
            'group/webos_surface_group_compositor.h',
            'protocol/wayland-webos-extension-protocol.c',
            'protocol/wayland-webos-extension-client-protocol.h',
            'shell/webos_shell_surface.cc',
            'shell/webos_shell_surface.h',
          ],
          'sources!': [
            'input/text_input.cc',
            'input/text_input.h',
          ],
        }],
        ['<(enable_drm_support)==1', {
          'defines': [
          'ENABLE_DRM_SUPPORT',
          ],
          'sources': [
            'egl/wayland_pixmap.cc',
            'egl/wayland_pixmap.h',
            'protocol/wayland-drm-protocol.cc',
            'protocol/wayland-drm-protocol.h',
          ],
          'cflags': [
            '<!@(<(pkg-config) --cflags gbm)',
          ],
          'link_settings': {
            'ldflags': [
              '<!@(<(pkg-config) --libs-only-L --libs-only-other gbm)',
            ],
            'libraries': [
              '<!@(<(pkg-config) --libs-only-l gbm)',
            ],
          }
        }],
        ['<(use_data_device_manager)==1', {
          'defines': ['USE_DATA_DEVICE_MANAGER',],
          'sources': [
            'data_device.cc',
            'data_device.h',
          ],
        }],
      ],
      'sources': [
        'data_offer.cc',
        'data_offer.h',
        'display.cc',
        'display.h',
        'display_poll_thread.cc',
        'display_poll_thread.h',
        'ozone_wayland_screen.cc',
        'ozone_wayland_screen.h',
        'screen.cc',
        'screen.h',
        'seat.cc',
        'seat.h',
        'window.cc',
        'window.h',
        'egl/egl_window.cc',
        'egl/egl_window.h',
        'egl/surface_ozone_wayland.cc',
        'egl/surface_ozone_wayland.h',
        'input/cursor.cc',
        'input/cursor.h',
        'input/keyboard.cc',
        'input/keyboard.h',
        'input/pointer.cc',
        'input/pointer.h',
        'input/text_input.h',
        'input/text_input.cc',
        'input/touchscreen.cc',
        'input/touchscreen.h',
        'protocol/text-protocol.c',
        'protocol/text-client-protocol.h',
        'protocol/ivi-application-protocol.c',
        'protocol/ivi-application-client-protocol.h',
        'protocol/xdg-shell-protocol.c',
        'protocol/xdg-shell-client-protocol.h',
        'shell/shell.cc',
        'shell/shell.h',
        'shell/shell_surface.h',
        'shell/shell_surface.cc',
        'shell/wl_shell_surface.cc',
        'shell/wl_shell_surface.h',
        'shell/xdg_shell_surface.cc',
        'shell/xdg_shell_surface.h',
        'shell/ivi_shell_surface.cc',
        'shell/ivi_shell_surface.h',
      ],
    },
  ]
}
