if get_option('bash_engine')
  install_subdir(
    'cuddle',
    install_dir: get_option('datadir'),
    exclude_files: ['meson.build'],
  )
endif

if build_gui
  install_data(
    'desktop/org.softnessasstrength.Cuddle.desktop',
    install_dir: get_option('datadir') / 'applications',
  )
  install_data(
    'metainfo/org.softnessasstrength.Cuddle.metainfo.xml',
    install_dir: get_option('datadir') / 'metainfo',
  )
  install_data(
    'icons/hicolor/scalable/apps/org.softnessasstrength.Cuddle.svg',
    install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
  )
endif

if build_cli
  install_data(
    'completions/cuddle',
    install_dir: get_option('datadir') / 'bash-completion' / 'completions',
  )
endif
