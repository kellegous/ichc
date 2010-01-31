{
  'variables': {
    'javascript_engine': 'none',
  },
  'targets': [
    # ICHC
    {
      'target_name': 'ichc',
      'type': 'executable',
      'dependencies': [
        'base/base.gyp:base',
        'third_party/zlib/zlib.gyp:zlib',
      ],
      'sources': [
        'ichc.cc',
        'chrome/common/zip.cc',
        'chrome/common/zip.h',
        'net/base/file_stream.h',
        'net/base/file_stream_posix.cc',
        'net/base/file_stream_win.cc',
        'net/base/net_errors.h',
        'net/base/net_error_list.h',
        'net/base/completion_callback.h',
      ],
      'include_dirs': [
        '.',
      ],
      'conditions': [
        ['OS=="win"', {
          'sources/': [ ['exclude', '_(mac|linux|posix)\\.cc$'] ],
        }],
        ['OS=="linux"', {
          'sources/': [ ['exclude', '_(mac|win)\\.cc$'] ],
        }],
        ['OS=="mac"', {
          'sources/': [ ['exclude', '_(linux|win)\\.cc$'] ],
        }],
      ]
    },
    # ICHC
  ],
}
