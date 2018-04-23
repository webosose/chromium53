# Copyright 2015 LG Electronics. All right reserved.
# Copyright 2017 LG Electronics. All right reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
  {
    'target_name': 'install_webos',
    'type': 'none',
    'variables': {
      'install': '<(PRODUCT_DIR)/install',
      'install_app_shell': '<(PRODUCT_DIR)/install_app_shell',
      'repack': '<(SHARED_INTERMEDIATE_DIR)/repack',
      'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/chrome',
    },
    'dependencies': [
      '<(DEPTH)/chrome/chrome.gyp:chrome',
    ],
    'actions' : [
      {
        'action_name': 'copy appinfo',
        'inputs': ['common/appinfo.json.in'],
        'outputs': ['<(install)/appinfo.json'],
        'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
      },
      {
        'action_name': 'copy_kill_webbrowser',
        'inputs':  ['common/kill_webbrowser.in'],
        'outputs': ['<(install)/kill_webbrowser'],
        'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
      },
      {
        'action_name': 'copy_app_shell_run_appshell',
        'inputs':  ['app_shell/run_appshell.in'],
        'outputs': ['<(install_app_shell)/run_appshell'],
        'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
      },
      {
        'action_name': 'copy app_shell EXE to install',
        'inputs': ['<(PRODUCT_DIR)/app_shell'],
        'outputs': ['<(install_app_shell)/app_shell'],
        'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
      },
      {
        'action_name': 'copy app_shell resource pak files',
        'inputs': ['<(PRODUCT_DIR)/extensions_shell_and_test.pak', '<(PRODUCT_DIR)/content_shell.pak', '<(PRODUCT_DIR)/content_resources.pak', '<(PRODUCT_DIR)/shell_resources.pak', '<(PRODUCT_DIR)/natives_blob.bin', ],
        'outputs': ['<(install_app_shell)/extensions_shell_and_test.pak', '<(install_app_shell)/content_shell.pak', '<(install_app_shell)/content_resources.pak', '<(install_app_shell)/shell_resources.pak', '<(install_app_shell)/natives_blob.bin'],
        'action': ['cp', '<@(_inputs)', '<(install_app_shell)'],
      },
    ],
    'copies': [
      {
        'destination': '<(install)',
        'files': [ 'common/etc', 'common/resources' ],
      },
      {
        'destination': '<(PRODUCT_DIR)',
        'files': [ 'common/services' ],
      },
    ],
    'conditions': [
      ['platform_apollo==1', {
        'actions' : [
          {
            'action_name': 'create_assets_dir',
            'variables' : {
              'output_dir' : [
                '<(install)/assets',
              ]
            },
            'inputs' : [],
            'outputs': [''],
            'action': ['mkdir', '-p', '<@(output_dir)'],
          },
          {
            'action_name': 'copy_webbrowser_icon',
            'inputs':  ['common/assets/hd1080/webbrowser_icon.png'],
            'outputs': ['<(install)/assets/webbrowser_icon.png'],
            'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
          },
          {
            'action_name': 'copy_webbrowser_splash',
            'inputs':  ['common/assets/hd1080/webbrowser_splash.png'],
            'outputs': ['<(install)/assets/webbrowser_splash.png'],
            'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
          },
        ],
      }, {
        'copies': [
          {
            'destination': '<(install)',
            'files': [ 'common/assets' ],
          },
        ],
      }],
      ['webos==1 or agl==1', {
        'actions' : [
          {
            'action_name': 'copy_chrome_run_webbrowser',
            'inputs':  ['chrome/run_webbrowser.in'],
            'outputs': ['<(install)/run_webbrowser'],
            'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
          },
          {
            'action_name': 'copy chrome EXE to install',
            'inputs': ['<(PRODUCT_DIR)/chrome'],
            'outputs': ['<(install)/chrome'],
            'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
          },
          {
            'action_name': 'copy chrome resource pak files',
            'inputs': ['<(repack)/resources.pak', '<(repack)/chrome_100_percent.pak', '<(repack)/chrome_200_percent.pak'],
            'outputs': ['<(install)/resources.pak', '<(install)/chrome_100_percent.pak', '<(install)/chrome_200_percent.pak'],
            'action': ['cp', '<@(_inputs)', '<(install)'],
          },
        ],
        'copies': [
          {
            'destination': '<(install)/locales',
            'files': [
              '<!@pymod_do_main(repack_locales -o -p <(OS) -g <(grit_out_dir) -s <(SHARED_INTERMEDIATE_DIR) -x <(SHARED_INTERMEDIATE_DIR) <(locales))'
            ],
          },
          {
            'destination': '<(install)/pseudo_locales',
            'files': [
              '<!@pymod_do_main(repack_locales -o -p <(OS) -g <(grit_out_dir) -s <(SHARED_INTERMEDIATE_DIR) -x <(SHARED_INTERMEDIATE_DIR) <(pseudo_locales))'
            ],
          },
        ],
      }],
    ],
  }],
}
