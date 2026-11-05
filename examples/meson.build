install_data(
  'cuddle.conf.example',
  install_dir: get_option('datadir') / 'doc' / 'cuddle',
)

if get_option('plugin_examples')
  shared_module(
    'hello',
    files('plugins/hello.c'),
    name_prefix: '',
    include_directories: [inc, cfg_inc],
    install: false,
  )
endif
