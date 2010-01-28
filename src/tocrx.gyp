{
  'targets': [
    # TOCRX
    {
      'target_name': 'tocrx',
      'type': 'executable',
      'dependencies': [
        'base/base.gyp:base',
      ],
      'sources': [
        'tocrx.cc',
      ],
      'include_dirs': [
        '.',
      ],
      'conditions': [
      ]
    },
    # TOCRX
  ],
}
