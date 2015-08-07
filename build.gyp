{
  'includes': [ 'common.gypi' ],
  'targets': [
    {
      'target_name': 'bale',
      'type': 'executable',
      'sources': [
        './deps/docopt/docopt.h',
        './deps/docopt/docopt.cpp',
        './deps/nodeuv-fs/fs.h',
        './deps/nodeuv-path/path.h',
        './deps/nodeuv-path/src/path.cc',
        './deps/debug/debug.h',
        './deps/json11/json11.hpp',
        './deps/json11/json11.cpp',
        './bale.cc',
      ],
      'include_dirs': [
        './deps/docopt',
        './deps/json11',
        './deps/nodeuv-fs',
        './deps/nodeuv-path',
        './deps/debug',
      ],
      'dependencies': [
        './deps/nodeuv-fs/deps/libuv/uv.gyp:libuv',
        './deps/nodeuv-fs/build.gyp:nodeuv-fs',
      ],
    },
  ],
}
