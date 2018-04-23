# Copyright (c) 2016 LG Electronics, Inc.

{
  'targets': [
    {
      'target_name': 'chromium',
      'type': 'none',
      'dependencies': [
        '<(DEPTH)/chrome/chrome.gyp:chrome',
      ],
      'conditions': [
        ['webos==1', {
          'dependencies': [
            '<(DEPTH)/lg/browser/install.gyp:install_webos',
          ],
          'dependencies!': [
            '<(DEPTH)/chrome/chrome.gyp:chrome',
          ],
        }],
        ['use_x11==1', {
          'dependencies': [
            '<(DEPTH)/lg/browser/install.gyp:install_x86',
          ],
        }],
      ],
    },
  ],
  'variables': {
    'chromium_code': 1,
    'allocator_target': '../../base/allocator/allocator.gyp:allocator',
    'protoc_out_dir': '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
    'cbe_extra_dependencies': [],

    # Add list of extra components to add to libcbe.so
    'chromium_child_dependencies': [
      '<(DEPTH)/content/content.gyp:content_gpu',
      '<(DEPTH)/content/content.gyp:content_plugin',
      '<(DEPTH)/content/content.gyp:content_ppapi_plugin',
      '<(DEPTH)/third_party/WebKit/public/blink_devtools.gyp:blink_devtools_frontend_resources',
    ],
    'conditions': [
      ['webos==1 or agl==1', {
        'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/chrome',
        'chromium_browser_dependencies': [
          '<(DEPTH)/chrome/chrome.gyp:browser',
          '<(DEPTH)/chrome/chrome.gyp:common',
          '<(DEPTH)/sync/sync.gyp:sync',
          '<@(cbe_extra_dependencies)',
        ],
        'chromium_child_dependencies': [
          '<(DEPTH)/chrome/chrome.gyp:child',
          '<(DEPTH)/chrome/chrome.gyp:common',
          '<(DEPTH)/chrome/chrome.gyp:gpu',
          '<(DEPTH)/chrome/chrome.gyp:plugin',
          '<(DEPTH)/chrome/chrome.gyp:renderer',
          '<(DEPTH)/chrome/chrome.gyp:utility',
          '<(DEPTH)/sync/sync.gyp:sync',
        ],
        'conditions': [
          ['webos==1' , {
            'cbe_extra_dependencies': [
              '<(DEPTH)/webos/webos_impl.gyp:webosport',
            ],
          }],
          ['enable_plugins==1 and disable_nacl==0', {
            'chromium_child_dependencies': [
              '<(DEPTH)/components/nacl/renderer/plugin/plugin.gyp:nacl_trusted_plugin',
            ],
          }],
          ['enable_basic_printing==1 or enable_print_preview==1', {
            'chromium_browser_dependencies': [
              '<(DEPTH)/printing/printing.gyp:printing',
            ],
          }],
          ['enable_print_preview==1', {
            'chromium_browser_dependencies': [
              '<(DEPTH)/chrome/chrome.gyp:service',
            ],
          }],
        ],
      }],
    ],
  },
  'conditions': [
    ['use_chromium_cbe==1', {
      'targets': [
        {
          'target_name': 'cbe',
          'type': 'shared_library',
          'dependencies': [
            '<@(chromium_browser_dependencies)',
            '<@(chromium_child_dependencies)',
            '<(DEPTH)/content/content.gyp:content_app_both',
            '<(DEPTH)/extensions/shell/app_shell.gyp:app_shell_lib',
            '<(DEPTH)/pdf/pdf.gyp:pdf',
          ],
          'conditions': [
            ['webos==1', {
              'sources': [
                '../../chrome/app/chrome_dll_resource.h',
                '../../chrome/app/chrome_main.cc',
                '../../chrome/app/chrome_main_delegate.cc',
                '../../chrome/app/chrome_main_delegate.h',
              ],
              'dependencies': [
                '<(DEPTH)/chrome/chrome.gyp:installer_util',
                '<(DEPTH)/build/linux/system.gyp:PmLogLib',
              ],
            }],
            ['ozone_platform_eglfs==1', {
              'defines': [
                'OZONE_PLATFORM_EGLFS',
              ],
            }],
          ],
        },
      ],
    }],
  ],
}
