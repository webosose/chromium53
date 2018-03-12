# Copyright (c) 2017 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  # Only platform implementation
  'conditions': [
    ['webos==1', {
      'includes': [
        'webos/pal/pal.gypi',
      ],
    }],
  ],
}
