# Copyright (c) 2017 LG Electronics, Inc.

{
  'variables': {
    'repack_locales_webos': 'tools/build/webos_repack_locales.py',
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/webos',
  },
  'inputs': [
    '<(repack_locales_webos)',
    '<!@pymod_do_main(webos_repack_locales -i -p <(OS) -g <(grit_out_dir) -s <(SHARED_INTERMEDIATE_DIR) -x <(SHARED_INTERMEDIATE_DIR) <(pak_locales))'
  ],
  'outputs': [
    '<!@pymod_do_main(webos_repack_locales -o -p <(OS) -g <(grit_out_dir) -s <(grit_out_dir) -x <(SHARED_INTERMEDIATE_DIR) <(pak_locales))'
  ],
  'action': [
    'python',
    '<(repack_locales_webos)',
    '-p', '<(OS)',
    '-g', '<(grit_out_dir)',
    '-s', '<(SHARED_INTERMEDIATE_DIR)',
    '-x', '<(SHARED_INTERMEDIATE_DIR)/.',
    '<@(pak_locales)',
  ],
}
