# Copyright (c) 2017 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'include_dirs': [
    '..',
  ],
  'defines': [
    'PAL_IMPLEMENTATION',
  ],
  # sources for platform implementation
  'sources': [
    'pal_webos.cc',
    'pal_webos.h',
  ],
}
