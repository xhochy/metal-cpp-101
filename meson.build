project('metal-with-cpp', 'cpp',
  default_options: ['cpp_std=c++23'])

executable('metal-list-devices', 'src/list_devices.cpp',
  include_directories: ['metal-cpp'],
  link_args: ['-framework', 'Metal', '-framework', 'Foundation'])
