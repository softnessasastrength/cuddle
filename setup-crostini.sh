#!/usr/bin/env bash
# Run from Crostini's Terminal app: bash ~/cuddle/setup-crostini.sh
# Installs Debian development dependencies; stages Cuddle only inside this project.
set -Eeuo pipefail

PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$PROJECT_ROOT"
if (( EUID == 0 )); then
  echo 'Run this as your normal Linux user, without sudo. It requests sudo only for apt.' >&2
  exit 2
fi
if [[ ! -f meson.build || ! -f src/lib/cuddle.c ]]; then
  echo "Cuddle source not found beside this script: $PROJECT_ROOT" >&2
  exit 2
fi
STAMP=$(date +%Y%m%d-%H%M%S)-$$
RUN_ROOT="$PROJECT_ROOT/build-crostini-$STAMP"
BACKUP_ROOT="$PROJECT_ROOT/backups"
mkdir -p "$RUN_ROOT" "$BACKUP_ROOT"
exec > >(tee -a "$RUN_ROOT/setup.log") 2>&1
trap 'rc=$?; printf "\nSetup stopped (exit %s) at line %s. Log: %s/setup.log\n" "$rc" "$LINENO" "$RUN_ROOT" >&2; exit "$rc"' ERR

printf 'Project: %s\nBuilds/log: %s\n' "$PROJECT_ROOT" "$RUN_ROOT"
printf '\nPreserving source before dependency setup...\n'
tar --exclude='./backups' --exclude='./build-*' --exclude='./builddir' \
    --exclude='./stage' --exclude='./.deps' --exclude='./.git' \
    -czf "$BACKUP_ROOT/cuddle-source-$STAMP.tar.gz" .
sha256sum "$BACKUP_ROOT/cuddle-source-$STAMP.tar.gz" > "$BACKUP_ROOT/cuddle-source-$STAMP.tar.gz.sha256"

printf '\nChecking Debian package-server DNS...\n'
if ! getent ahosts deb.debian.org; then
  echo 'Cannot resolve deb.debian.org in this terminal. Check Crostini connectivity, then rerun.' >&2
  exit 1
fi
printf '\nInstalling Linux development dependencies...\n'
sudo apt-get update
sudo apt-get install -y build-essential meson ninja-build pkgconf debhelper \
  libgtk-4-dev libncurses-dev qt6-base-dev gettext python3 xdg-utils

python3 tools/generate-rooms.py

build_variant() {
  local name=$1 tui=$2 gui=$3
  local build="$RUN_ROOT/$name" prefix="$RUN_ROOT/$name-stage"
  printf '\nBuilding %s (TUI=%s, GUI=%s)...\n' "$name" "$tui" "$gui"
  meson setup "$build" --prefix="$prefix" --libdir=lib --buildtype=debugoptimized \
    -Dcli=enabled -Dtui=enabled -Dgui=enabled -Ddocs=enabled -Dtests=enabled \
    -Dtui_toolkit="$tui" -Dgui_toolkit="$gui" -Dplugins=disabled \
    -Duser_config=enabled -Di18n=auto
  meson compile -C "$build"
  # Isolate tests from personal configuration; do not overwrite HOME.
  mkdir -p "$build/test-config"
  XDG_CONFIG_HOME="$build/test-config" meson test -C "$build" --print-errorlogs
  meson install -C "$build"
  "$prefix/bin/cuddle" --version
  "$prefix/bin/cuddle-tui" --version
  "$prefix/bin/cuddle-gui" --version
  if [[ "$tui" == python ]]; then
    for screen in hub flood touch signal; do
      python3 "$prefix/share/cuddle/tui.py" "$screen" --dump > "$build/$screen.txt"
    done
  fi
  if [[ -f "$PROJECT_ROOT/tests/smoke_frontends.py" ]]; then
    python3 "$PROJECT_ROOT/tests/smoke_frontends.py" --prefix "$prefix" --tui "$tui" --gui "$gui"
  fi
}

# Separate build directories are necessary: each toolkit produces the same executable names.
build_variant default python html
build_variant native curses gtk
build_variant qt python qt

cat > "$RUN_ROOT/RESULTS.txt" <<RESULTS
Cuddle library and all frontend variants built; configured tests passed.
Default (Python TUI / HTML GUI): $RUN_ROOT/default-stage/bin
Native (ncurses TUI / GTK4 GUI): $RUN_ROOT/native-stage/bin
Qt6 GUI (plus Python TUI): $RUN_ROOT/qt-stage/bin
Log: $RUN_ROOT/setup.log
Source backup: $BACKUP_ROOT/cuddle-source-$STAMP.tar.gz

Launch the GTK4 GUI:
  "$RUN_ROOT/native-stage/bin/cuddle-gui"
Launch the ncurses TUI:
  "$RUN_ROOT/native-stage/bin/cuddle-tui"
Launch the CLI:
  "$RUN_ROOT/native-stage/bin/cuddle" --help

Interactive GUI appearance and desktop integration still require visual checking.
No Cuddle files were installed into /usr. No ChromeOS settings were changed.
RESULTS
printf '\nSUCCESS\n'
cat "$RUN_ROOT/RESULTS.txt"
