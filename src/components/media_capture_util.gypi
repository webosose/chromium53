# Copyright 2017 LG Electronics, Inc. All rights reserved.

{
  'targets': [
    {
      'target_name': 'media_capture_util',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../content/content.gyp:content_browser',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'media_capture_util/devices_dispatcher.cc',
        'media_capture_util/devices_dispatcher.h',
      ],
    },
  ],
}
