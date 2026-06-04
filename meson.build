project('metal-with-cpp', 'cpp',
  default_options: ['cpp_std=c++23'])

# Compile the Metal shader (.metal → .air → .metallib)
xcrun = find_program('xcrun')

metal_air = custom_target('add_air',
  input   : 'src/add.metal',
  output  : 'add.air',
  command : [xcrun, '-sdk', 'macosx', 'metal', '-c', '@INPUT@', '-o', '@OUTPUT@'],
)

metal_lib = custom_target('default_metallib',
  input   : metal_air,
  output  : 'default.metallib',
  command : [xcrun, '-sdk', 'macosx', 'metallib', '@INPUT@', '-o', '@OUTPUT@'],
)

metallib_path = meson.current_build_dir() / 'default.metallib'

# Compile the executables and embed the metallib into them
executable('metal-list-devices', 'src/list_devices.cpp',
  include_directories: ['metal-cpp'],
  link_args: ['-framework', 'Metal', '-framework', 'Foundation'])

executable('metal-addition', 'src/basic_addition.cpp',
  include_directories: ['metal-cpp'],
  link_args: ['-framework', 'Metal', '-framework', 'Foundation', '-Wl,-sectcreate,__TEXT,__metallib,' + metallib_path],
  link_depends: [metal_lib])

