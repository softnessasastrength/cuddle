test_version = executable(
  'test_version',
  files('test_version.c'),
  dependencies: libcuddle_dep,
  install: false,
  build_rpath: meson.current_build_dir() / '../src/lib',
)

test_theme = executable(
  'test_theme',
  files('test_theme.c'),
  dependencies: libcuddle_dep,
  install: false,
  build_rpath: meson.current_build_dir() / '../src/lib',
)

test_config = executable(
  'test_config',
  files('test_config.c'),
  dependencies: libcuddle_dep,
  install: false,
  build_rpath: meson.current_build_dir() / '../src/lib',
)

test('version-and-catalog', test_version)
test(
  'theme-letter-and-word',
  test_theme,
  env: {'CUDDLE_SHARE': meson.project_source_root() / 'data' / 'cuddle'},
)
test('config-missing-is-defaults', test_config)

if build_cli
  test('cli-version', cuddle_exe, args: ['--version'])
  test('cli-help', cuddle_exe, args: ['--help'])
  test(
    'cli-help-lists-flags',
    find_program('sh'),
    args: [meson.project_source_root() / 'tests' / 'check_help.sh', cuddle_exe.full_path()],
  )
  test('cli-flood-needs-tty', cuddle_exe, args: ['--flood'], should_fail: true)
endif

if build_tui
  test('tui-version', cuddle_tui_exe, args: ['--version'])
  test('tui-help', cuddle_tui_exe, args: ['--help'])
  test('tui-needs-tty', cuddle_tui_exe, should_fail: true)
endif

if build_gui
  test('gui-version', cuddle_gui_exe, args: ['--version'])
  test('gui-help', cuddle_gui_exe, args: ['--help'])
endif

rooms_test = executable('test_rooms', 'test_rooms.c', dependencies: libcuddle_dep, install: false)
test('rooms-state-and-stop', rooms_test)
if python3.found()
  test('room-catalog-generated-tables', python3,
    args: [meson.project_source_root() / 'tools/generate-rooms.py', '--check'])
endif

if build_gui and gui_toolkit == 'qt'
  test('qt-widget-navigation', cuddle_gui_exe, args: ['--self-test'], env: {'QT_QPA_PLATFORM': 'offscreen'})
elif build_gui and gui_toolkit == 'gtk'
  test('gtk-widget-navigation', cuddle_gui_exe, args: ['--self-test'])
endif

preferences_test = executable('test_preferences', 'test_preferences.c', dependencies: libcuddle_dep, install: false)
test('shared-preferences', preferences_test)

preferences_save_test = executable('test_preferences_save', 'test_preferences_save.c', dependencies: libcuddle_dep, install: false)
test('explicit-preference-saving', preferences_save_test)

if python3.found()
  test('ppa-source-validation-offline', python3, args: [meson.project_source_root() / 'tests/test_ppa.py'])
endif
