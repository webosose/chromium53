# Copyright (c) 2016 LG Electronics, Inc. All rights reserved.
{
  'target_defaults': {
    # Lg defines
    'conditions': [
      ['ozone_platform_wayland_external==1', {
        'defines': ['OZONE_PLATFORM_WAYLAND_EXTERNAL=1'],
      }],
      ['icu_use_data_file_flag==1', {
        'defines': ['ICU_USE_DATA_FILE_FLAG=1'],
      }],
      ['webos!=0', {
        'defines': ['OS_WEBOS=1'],
        'ldflags!': [
          '-Wl,-z,now',
          '-Wl,-z,relro',
          '-Wl,--fatal-warnings',
        ],
      }],
      ['webos_tv!=0', {
        'defines': [
          'PLATFORM_WEBOS_TV=1',
        ],
      }],
      ['use_chromium_cbe!=0', {
        'defines': [
          'COMPONENT_BUILD',
          'USE_CBE',
        ],
      }],
      ['use_xkbcommon==1', {
        'defines': ['USE_XKBCOMMON'],
      }],
      ['use_injections==1', {
        'defines': ['USE_INJECTIONS=1'],
      }],
      ['use_dynamic_injection_loading!=0', {
        'defines': ['USE_DYNAMIC_INJECTION_LOADING=1'],
      }],
      ['use_pmlog==1', {
        'defines': ['USE_PMLOG=1'],
      }],
      ['browser_common==1', {
        'defines': ['BROWSER_COMMON=1'],
      }],
      ['enable_lttng==1', {
        'defines': ['ENABLE_LTTNG=1'],
      }],
      ['use_splash_screen==1', {
        'defines': ['USE_SPLASH_SCREEN=1'],
      }],
      ['webos_plugin_support==1', {
        'defines': ['WEBOS_PLUGIN_SUPPORT=1'],
      }],
      ['use_umediaserver==1', {
        'defines': ['USE_UMEDIASERVER=1'],
      }],
      ['enable_widevine_cdm==1', {
        'defines': ['WIDEVINE_CDM_AVAILABLE'],
      }],
      ['enable_lg_svp==1', {
        'defines': ['ENABLE_LG_SVP=1', 'USE_SVP=1'],
      }],
      ['punch_hole_for_plugins==1', {
        'defines': ['PUNCH_HOLE_FOR_PLUGINS=1'],
      }],
      ['use_npapi_plugin_external_path==1', {
        'defines': ['USE_NPAPI_PLUGIN_EXTERNAL_PATH=1'],
      }],
      ['use_gst_media==1', {
        'defines': ['USE_GST_MEDIA=1'],
      }],
      ['use_webos_media_focus_extension==1', {
        'defines': ['USE_WEBOS_MEDIA_FOCUS_EXTENSION=1'],
      }],
      ['use_webos_surface==1', {
        'defines': ['USE_WEBOS_SURFACE=1'],
      }],
      ['enable_vp9==1', {
        'defines': ['ENABLE_VP9=1'],
      }],
      ['platform_apollo==1', {
        'defines': ['PLATFORM_APOLLO=1', 'USE_BROADCOM=1'],
      }],
      ['platform_apollo==1', {
        'defines': [
          'UMS_INTERNAL_API_VERSION=2',
        ],
      }],
    ],
  },
}
