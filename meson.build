# cuddle — portable Meson build
#
# Packager contract. This file contains no package-manager commands
# and no /etc/os-release branches.
#
#   meson setup builddir --prefix=/usr --buildtype=release
#   meson compile -C builddir
#   meson test    -C builddir
#   meson install -C builddir --destdir "$DESTDIR"
#
# Feature flags: -Dcli= -Dtui= -Dgui= -Ddocs= -Dtests=
#                -Dplugins= (default disabled) -Duser_config= -Di18n=
# Path policy:   --prefix --bindir --libdir --datadir --mandir
# Runtime deps belong in packaging metadata, not here (bash, python3, xdg-utils).
#
# See docs/PACKAGING.md, docs/WSL.md, docs/ARCHITECTURE.md, docs/EXPANSION.md.

project(
  'cuddle',
  'c',
  version: '1.4.0',
  license: 'MIT',
  meson_version: '>=0.61.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'buildtype=debugoptimized',
  ],
)

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language: 'c')

# FHS-relative under whatever prefix the packager set.
#   bindir  -> $prefix/bin
#   libdir  -> $prefix/lib | lib64 | $triplet  (packager policy)
#   datadir -> $prefix/share/cuddle
#   mandir  -> $prefix/share/man
prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
libdir = prefix / get_option('libdir')
datadir = prefix / get_option('datadir')
cuddle_datadir = datadir / 'cuddle'

want_cli = get_option('cli')
want_tui = get_option('tui')
tui_toolkit = get_option('tui_toolkit')
want_gui = get_option('gui')
gui_toolkit = get_option('gui_toolkit')
want_docs = get_option('docs')
want_tests = get_option('tests')
want_plugins = get_option('plugins')
want_user_config = get_option('user_config')
want_i18n = get_option('i18n')

python3 = find_program('python3', required: false)
msgfmt = find_program('msgfmt', required: want_i18n)
cc = meson.get_compiler('c')
dl_dep = cc.find_library('dl', required: want_plugins)
has_libintl = cc.has_header('libintl.h')
curses_dep = dependency('', required: false)

# Toolkit names only as pkg-config modules. Packagers map those to
# distro package names (see docs/PACKAGING.md).
build_cli = not want_cli.disabled()
if tui_toolkit == 'curses'
  curses_dep = dependency('ncursesw', required: false)
  if not curses_dep.found()
    curses_dep = dependency('ncurses', required: want_tui.enabled())
  endif
  build_tui = not want_tui.disabled() and curses_dep.found()
  if want_tui.enabled() and not curses_dep.found()
    error('tui=enabled with tui_toolkit=curses requires ncursesw or ncurses via pkg-config')
  endif
else
  build_tui = python3.found() and (want_tui.enabled() or want_tui.auto())
  if want_tui.enabled() and not python3.found()
    error('tui=enabled with tui_toolkit=python requires python3 in PATH at configure time')
  endif
endif
gtk_dep = dependency('', required: false)
qt_dep = dependency('', required: false)
if gui_toolkit == 'gtk'
  gtk_dep = dependency('gtk4', required: want_gui.enabled(), version: '>=4.6')
  build_gui = not want_gui.disabled() and gtk_dep.found()
elif gui_toolkit == 'qt'
  add_languages('cpp', native: false)
  qt_dep = dependency('qt6', modules: ['Core', 'Widgets'], required: want_gui.enabled())
  build_gui = not want_gui.disabled() and qt_dep.found()
  if want_gui.enabled() and not qt_dep.found()
    error('gui=enabled with gui_toolkit=qt requires Qt6 Core+Widgets via pkg-config')
  endif
else
  build_gui = not want_gui.disabled()
endif
build_docs = not want_docs.disabled()
build_tests = not want_tests.disabled()
build_plugins = not want_plugins.disabled() and dl_dep.found()
build_user_config = not want_user_config.disabled()
build_i18n = has_libintl and msgfmt.found() and (want_i18n.enabled() or want_i18n.auto())

if want_plugins.enabled() and not dl_dep.found()
  error('plugins=enabled requires libdl')
endif
if want_i18n.enabled() and (not msgfmt.found() or not has_libintl)
  error('i18n=enabled requires msgfmt and libintl.h')
endif

fs = import('fs')
if get_option('bash_engine') and not fs.exists('data/cuddle/engine.sh')
  error('data/cuddle/engine.sh missing (required when bash_engine=true)')
endif

conf = configuration_data()
conf.set_quoted('CUDDLE_VERSION', meson.project_version())
conf.set_quoted('CUDDLE_PREFIX', prefix)
conf.set_quoted('CUDDLE_BINDIR', bindir)
conf.set_quoted('CUDDLE_DATADIR', cuddle_datadir)
if python3.found()
  conf.set_quoted('CUDDLE_PYTHON', python3.full_path())
else
  conf.set_quoted('CUDDLE_PYTHON', 'python3')
endif
if build_cli
  conf.set('CUDDLE_HAS_CLI', 1)
endif
if build_tui
  conf.set('CUDDLE_HAS_TUI', 1)
endif
if build_gui
  conf.set('CUDDLE_HAS_GUI', 1)
endif
if build_plugins
  conf.set('CUDDLE_HAS_PLUGINS', 1)
endif
if build_user_config
  conf.set('CUDDLE_HAS_USER_CONFIG', 1)
endif
if build_i18n
  conf.set('CUDDLE_HAS_I18N', 1)
endif
conf.set_quoted('CUDDLE_LOCALEDIR', prefix / get_option('localedir'))
conf.set_quoted('CUDDLE_GETTEXT_DOMAIN', meson.project_name())

configure_file(
  input: 'src/lib/config.h.in',
  output: 'config.h',
  configuration: conf,
)

inc = include_directories('src/lib')
cfg_inc = include_directories('.')

subdir('src')
subdir('data')
subdir('examples')
if build_docs
  subdir('docs')
endif
if build_i18n
  subdir('po')
endif
if build_tests
  subdir('tests')
endif

summary(
  {
    'prefix': prefix,
    'bindir': bindir,
    'libdir': libdir,
    'datadir': cuddle_datadir,
    'cli': build_cli,
    'tui': build_tui,
    'tui_toolkit': tui_toolkit,
    'gui': build_gui,
    'gui_toolkit': gui_toolkit,
    'docs': build_docs,
    'tests': build_tests,
    'bash_engine': get_option('bash_engine'),
    'plugins': build_plugins,
    'user_config': build_user_config,
    'i18n': build_i18n,
  },
  section: 'cuddle',
)
