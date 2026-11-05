libcuddle_src = files('cuddle.c', 'plugin.c', 'theme.c', 'config.c', 'i18n.c', 'rooms.c', 'preferences.c')

libcuddle_deps = []
if build_plugins
  libcuddle_deps += dl_dep
endif

libcuddle = library(
  'cuddle',
  libcuddle_src,
  include_directories: [inc, cfg_inc],
  dependencies: libcuddle_deps,
  install: true,
  version: meson.project_version(),
  soversion: '0',
)

libcuddle_dep = declare_dependency(
  link_with: libcuddle,
  include_directories: [inc, cfg_inc],
)

install_headers('cuddle.h', 'cuddle-plugin.h')

pkg = import('pkgconfig')
pkgconfigdir = get_option('pkgconfigdir')
if pkgconfigdir == ''
  pkgconfigdir = get_option('libdir') / 'pkgconfig'
endif
pkg.generate(
  libcuddle,
  name: 'cuddle',
  filebase: 'cuddle',
  description: 'cuddle core library (paths, version, protocol catalog)',
  install_dir: pkgconfigdir,
)
