{
  'includes': [ 'common.gypi' ],
  'targets': [
    {
      'target_name': 'bale',
      'type': 'executable',
      'sources': [
        './deps/nodeuv-fs/fs.h', 
        './deps/debug/debug.h',
        './deps/json/json.h',
        './deps/json/deps/json11/json11.cpp',
        './bale.cc',
      ],
      'include_dirs': [
        './deps/nodeuv-fs',
        './deps/json/deps/json11',
        './deps/json',
        './deps/debug',
      ],
      'dependencies': [
        './deps/nodeuv-fs/deps/libuv/uv.gyp:libuv',
        './deps/nodeuv-fs/build.gyp:nodeuv-fs',
      ],
    },
  ],
}
