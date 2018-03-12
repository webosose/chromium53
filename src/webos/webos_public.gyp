# Copyright (c) 2016 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'runtime',
      'type': '<(component)',
      'dependencies': [
        '../base/base.gyp:base',
      ],
      'sources': [
        'public/runtime.cc',
        'public/runtime.h',
        'public/runtime_delegates.h',
      ],
      'include_dirs': [
        '../..',
      ],
    },
  ],
}

