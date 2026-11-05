#!/usr/bin/env bash
# Rebuild existing dependencies only. No apt, sudo, or system installation.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$PROJECT_ROOT"
for tool in python3 meson ninja pkg-config cc c++; do
  if ! command -v "$tool" >/dev/null; then
    echo "Missing $tool. Run setup-crostini.sh from your Linux Terminal first." >&2
    exit 1
  fi
done
python3 tools/generate-rooms.py
RUN_ROOT="$PROJECT_ROOT/build-current"
mkdir -p "$RUN_ROOT"
exec > >(tee "$RUN_ROOT/rebuild.log") 2>&1
trap 'rc=$?; echo "Build stopped at line $LINENO. See $RUN_ROOT/rebuild.log" >&2; exit "$rc"' ERR
build_variant() {
  local name=$1 tui=$2 gui=$3
  local build="$RUN_ROOT/$name" prefix="$RUN_ROOT/$name-stage"
  local reconfigure=()
  if [[ -f "$build/build.ninja" ]]; then reconfigure=(--reconfigure); fi
  meson setup "${reconfigure[@]}" "$build" --prefix="$prefix" --libdir=lib \
    --buildtype=debugoptimized -Dcli=enabled -Dtui=enabled -Dgui=enabled \
    -Ddocs=enabled -Dtests=enabled -Dtui_toolkit="$tui" -Dgui_toolkit="$gui" \
    -Dplugins=disabled -Duser_config=enabled -Di18n=auto
  meson compile -C "$build"
  mkdir -p "$build/test-config"
  XDG_CONFIG_HOME="$build/test-config" meson test -C "$build" --print-errorlogs
  meson install -C "$build"
  python3 tests/smoke_frontends.py --prefix "$prefix" --tui "$tui" --gui "$gui"
}
build_variant default python html
build_variant native curses gtk
build_variant qt python qt
printf '\nBuilt and tested. Launch GTK: %q\n' "$RUN_ROOT/native-stage/bin/cuddle-gui"
printf 'Launch terminal hub: %q\n' "$RUN_ROOT/native-stage/bin/cuddle-tui"
printf 'List CLI rooms: %q --rooms\n' "$RUN_ROOT/native-stage/bin/cuddle"
