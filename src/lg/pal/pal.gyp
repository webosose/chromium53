# Copyright (c) 2017 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'pal',
      'type': '<(component)',
      'include_dirs': [
        '../..',
      ],
      'dependencies': [
        '../../base/base.gyp:base',
      ],
      'export_dependent_settings': [
        '../../base/base.gyp:base',
      ],
      'includes': [
        'pal.gypi',
      ],
      'sources': [
        'public/pal.cc',
        'public/pal.h',
      ],
    },
  ],
}
