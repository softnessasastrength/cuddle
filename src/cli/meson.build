cuddle_rpath = '$ORIGIN/../' + get_option('libdir')

cuddle_exe = executable(
  'cuddle',
  files('main.c', 'flood.c', 'room.c'),
  dependencies: libcuddle_dep,
  install: true,
  install_rpath: cuddle_rpath,
)
