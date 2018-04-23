# Copyright (c) 2016 LG Electronics, Inc. All rights reserved.
{
  # Lg variables definition which will be use inside all gyp files
  'variables': {
    'variables': {
      # Ozone platform wayland building
      'ozone_platform_wayland_external%': 0,

      # Whether we are building a WebOS build.
      'webos%': 0,

      # Whether the WebOS platform target is TV.
      'webos_tv%': 0,

      # Whether we're using fixes for both vanilla and webos browser
      'browser_common%': 1,

      # Set use_chromium_cbe to 1 with component set to static_library to make
      # one shared library libcbe.so.
      'use_chromium_cbe%': '0',

      # Path to the CBE shared resources
      'cbe_data%': '',

      # Use PmLog
      'use_pmlog%': 0,

      # LTTNG support
      'enable_lttng%': 0,

      # Use splash screen appliance
      'use_splash_screen%': 0,

      # LGE support NPAPI plugins
      'webos_plugin_support%': 1,

      # Use uMediaServer
      'use_umediaserver%': 0,

      # Widevine CDM support
      'enable_widevine_cdm%': 0,

      # LG SVP support
      'enable_lg_svp%': 0,

      # Should a hole for plug-ins be punched
      'punch_hole_for_plugins%': 0,

      # Use DirectMedia2
      'use_directmedia2%': 0,

      # Use WebOS Media Focus Extension
      'use_webos_media_focus_extension%': 0,

      # Use webOS graphics surface as backend for gpu memory buffer
      'use_webos_surface%': 0,

      # Enable VP9
      'enable_vp9%': 0,

      #External npapi plugin path
      'use_npapi_plugin_external_path%': 0,

      # Enable features for apollo
      'platform_apollo%': 0,
    },
    # Copy conditionally-set variables out one scope.
    'ozone_platform_wayland_external%': '<(ozone_platform_wayland_external)',
    'webos%': '<(webos)',
    'webos_tv%': '<(webos_tv)',
    'browser_common%': '<(browser_common)',
    'use_chromium_cbe%': '<(use_chromium_cbe)',
    'cbe_data%': '<(cbe_data)',
    'use_pmlog%': '<(use_pmlog)',
    'enable_lttng%': '<(enable_lttng)',
    'use_splash_screen%': '<(use_splash_screen)',
    'webos_plugin_support%': '<(webos_plugin_support)',
    'use_umediaserver%': '<(use_umediaserver)',
    'enable_widevine_cdm%': '<(enable_widevine_cdm)',
    'enable_lg_svp%': '<(enable_lg_svp)',
    'punch_hole_for_plugins%': '<(punch_hole_for_plugins)',
    'use_directmedia2%': '<(use_directmedia2)',
    'use_webos_media_focus_extension%': '<(use_webos_media_focus_extension)',
    'use_webos_surface%': '<(use_webos_surface)',
    'enable_vp9%': '<(enable_vp9)',
    'use_npapi_plugin_external_path%': '<(use_npapi_plugin_external_path)',
    'use_injections%': 0,
    'use_dynamic_injection_loading%': 0,
    'platform_apollo%': '<(platform_apollo)',

    'conditions': [
      ['webos!=0', {
        'posix_use_system_cross_toolchain%': 0,
        'use_allocator%': 'none',
        'use_cups%': 0,
        'use_gconf%': 0,
        'use_gio%': 0,
        'use_glib%': 1,
        'use_injections%': 1,
        'use_pmlog%': 1,
        'sysroot%': '<(sysroot)',
        'use_splash_screen%': 1,
        'punch_hole_for_plugins%': 1,
      }],
      # uMediaServer requires video hole support on TV.
      ['webos!=0 and use_umediaserver!=0', {
        'video_hole%': 1,
        'use_webos_media_focus_extension%': 1,
      }],
      ['webos==1 or agl==1', {
        'grit_defines': ['-D', 'webos'],
        'webos_locales': [
          'af', 'am', 'ar', 'as', 'az', 'bg', 'bn', 'bs', 'cs', 'da', 'de',
          'el', 'en-US', 'es-419', 'es', 'et', 'eu', 'fa', 'fi', 'fr-CA',
          'fr', 'ga', 'gu', 'ha', 'he', 'hi', 'hr', 'hu', 'id', 'it', 'ja',
          'kk', 'km', 'kn', 'ko', 'ku', 'lt', 'lv', 'mk', 'ml', 'mn', 'mr',
          'ms', 'nb', 'nl', 'or', 'pa', 'pl', 'ps', 'pt-BR', 'pt-PT', 'ro',
          'ru', 'sk', 'sl', 'sq', 'sr', 'sv', 'ta', 'te', 'th', 'tr', 'uk',
          'ur', 'uz', 'vi', 'zh-CN', 'zh-HK', 'zh-TW', 'zu',
        ],
      }],
    ],
  },
}
