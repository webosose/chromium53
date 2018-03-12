# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/components/accessibility',
    'grit_resource_ids': 'accessibility_resource_ids',
  },
  'targets': [
    {
      'target_name': 'accessibility_strings',
      'type': 'none',
      'actions': [
        {
          'action_name': 'generate_accessibility_strings',
          'variables': {
            'grit_grd_file': 'accessibility_strings.grd',
          },
          'includes': [ '../../build/grit_action.gypi' ],
        },
      ],
      'includes': [ '../../build/grit_target.gypi' ],
    },
  ]
}
