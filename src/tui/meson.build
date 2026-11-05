cuddle_rpath = '$ORIGIN/../' + get_option('libdir')

if tui_toolkit == 'curses'
  cuddle_tui_exe = executable(
    'cuddle-tui',
    files('cuddle-tui-curses.c'),
    dependencies: [libcuddle_dep, curses_dep],
    install: true,
    install_rpath: cuddle_rpath,
  )
else
  cuddle_tui_exe = executable(
    'cuddle-tui',
    files('cuddle-tui.c'),
    dependencies: libcuddle_dep,
    install: true,
    install_rpath: cuddle_rpath,
  )
endif
