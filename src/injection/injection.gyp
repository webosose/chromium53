# Copyright (c) 2014 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'dependencies': [
      '../v8/src/v8.gyp:v8',
      '../third_party/WebKit/public/blink_headers.gyp:blink_headers',
    ],
    'include_dirs': [
      '..',
    ],
  },
  'targets': [
  {
    'target_name': 'palmsystem_inj',
    'link_settings': {
      'libraries': [
        '-lluna-service2',
      ],
    },
    'dependencies': [
      '../build/linux/system.gyp:glib',
    ],
    'defines': [
      'PALMSYSTEM_IMPLEMENTATION',
    ],
    'sources': [
      'palmsystem/palmsystem_injection.cc',
      'palmsystem/palmsystem_injection.h',
      'palmsystem/luna_service_mgr.cc',
      'palmsystem/luna_service_mgr.h',
      'palmsystem/pmtracer.h',
      'palmsystem/pmtrace_bundle_provider.h',
    ],
    'conditions': [
      ['enable_lttng==1', {
          'dependencies': [
            '../build/linux/system.gyp:lttng-ust',
          ],
          'include_dirs': [
            'palmsystem',
          ],
          'sources': [
            'palmsystem/pmtrace_bundle_provider.c',
          ],
      }],
      ['use_dynamic_injection_loading!=0', {
        'type': 'shared_library',
      }, { #'use_dynamic_injection_loading!=1'
        'type': 'static_library',
      }],
    ],
  }],
}
