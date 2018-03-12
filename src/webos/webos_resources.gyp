# Copyright 2016 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'variables': {
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/webos',
    'grit_resource_ids': 'webos_resource_ids',
  },
  'targets': [
    {
      'target_name': 'webos_resources_pack',
      'type': 'none',
      'dependencies': [
        'webos_resources.gyp:network_error_resources',
        'webos_resources.gyp:webos_inspector_resources',
        '../ui/resources/ui_resources.gyp:ui_resources',
        '../components/accessibility/accessibility_strings.gyp:accessibility_strings',
      ],
      'actions': [
        {
          'action_name': 'repack_webos_resources',
          'variables': {
            'pak_inputs': [
              '<(SHARED_INTERMEDIATE_DIR)/blink/devtools_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/blink/public/resources/blink_image_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/blink/public/resources/blink_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/app/resources/content_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/content_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/webos/webos_inspector_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/ui_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/webui_resources.pak',
            ],
            'pak_output': '<(SHARED_INTERMEDIATE_DIR)/webos/repack/webos_resources.pak',
          },
          'includes': [ '../build/repack_action.gypi' ],
        },
        {
          'action_name': 'repack_webos_locales',
          'variables': {
            'pak_locales': '<(webos_locales)',
          },
          'includes': [ 'webos_repack_locales.gypi' ],
        },
      ],
    },
    {
      'target_name': 'webos_inspector_resources',
      'type': 'none',
      'actions': [
        {
          'action_name': 'generate_webos_inspector_resources',
          'variables': {
            'grit_grd_file': 'browser/resources/webos_resources.grd',
          },
          'includes': [ '../build/grit_action.gypi' ],
        },
      ],
      'includes': [ '../build/grit_target.gypi' ],
    },
    {
      'target_name': 'network_error_resources',
      'type': 'none',
      'actions': [
        {
          'action_name': 'generate_network_error_resources',
          'variables': {
            'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/webos/network_error_resources',
            'grit_grd_file': 'app/resources/network_error_resources.grd',
          },
          'includes': [ '../build/grit_action.gypi' ],
        },
      ],
      'includes': [ '../build/grit_target.gypi' ],
    },
  ]
}
