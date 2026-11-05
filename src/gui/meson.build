cuddle_rpath = '$ORIGIN/../' + get_option('libdir')

if gui_toolkit == 'gtk'
  cuddle_gui_exe = executable(
    'cuddle-gui',
    files('cuddle-gui-gtk.c'),
    dependencies: [libcuddle_dep, gtk_dep],
    install: true,
    install_rpath: cuddle_rpath,
  )
elif gui_toolkit == 'qt'
  cuddle_gui_exe = executable(
    'cuddle-gui',
    files('cuddle-gui-qt.cpp'),
    dependencies: [libcuddle_dep, qt_dep],
    install: true,
    install_rpath: cuddle_rpath,
  )
else
  cuddle_gui_exe = executable(
    'cuddle-gui',
    files('cuddle-gui.c'),
    dependencies: libcuddle_dep,
    install: true,
    install_rpath: cuddle_rpath,
  )
endif
