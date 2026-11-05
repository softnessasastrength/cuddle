#!/bin/sh
# Portable WSL / bare-metal / container build.
# Does not call apt, dnf, pacman, apk, xbps, or nix.
set -eu

root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$root"

# Optional user-local toolchain (meson, ninja, zig cc). Not required.
if [ -d "${HOME}/.local/cuddle-tools/bin" ]; then
  PATH="${HOME}/.local/cuddle-tools/bin:${PATH}"
  export PATH
  if [ -x "${HOME}/.local/cuddle-tools/bin/cc" ] && [ -z "${CC:-}" ]; then
    export CC="${HOME}/.local/cuddle-tools/bin/cc"
  fi
fi

BUILDDIR=${BUILDDIR:-builddir}
PREFIX=${PREFIX:-/usr}
LIBDIR=${LIBDIR:-lib}
DESTDIR=${DESTDIR:-"$root/stage"}
RUN_TESTS=1
SETUP_FLAGS="--prefix=${PREFIX} --libdir=${LIBDIR} --buildtype=release"
EXTRA_MESON=""

usage() {
  cat <<EOF
usage: ./build.sh [options] [-- meson-options...]

  --reconfigure   meson setup --reconfigure
  --no-test       skip meson test
  --prefix DIR    default: /usr
  --libdir DIR    default: lib   (Debian wants lib, not lib64)
  --builddir DIR  default: builddir
  --destdir DIR   default: ./stage
  -h, --help

Needs: meson, ninja, a C11 compiler, python3 (for -Dtui=auto).
Does not install packages. If tools are missing, install them yourself.
EOF
}

RECONFIGURE=0
while [ "$#" -gt 0 ]; do
  case "$1" in
    --reconfigure) RECONFIGURE=1 ;;
    --no-test) RUN_TESTS=0 ;;
    --prefix) PREFIX=$2; SETUP_FLAGS="--prefix=${PREFIX} --libdir=${LIBDIR} --buildtype=release"; shift ;;
    --libdir) LIBDIR=$2; SETUP_FLAGS="--prefix=${PREFIX} --libdir=${LIBDIR} --buildtype=release"; shift ;;
    --builddir) BUILDDIR=$2; shift ;;
    --destdir) DESTDIR=$2; shift ;;
    -h|--help) usage; exit 0 ;;
    --) shift; EXTRA_MESON="$*"; break ;;
    -D*) EXTRA_MESON="${EXTRA_MESON} $1" ;;
    *) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

need() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "build.sh: missing '$1' on PATH" >&2
    echo "install meson, ninja, and a C compiler in this environment." >&2
    exit 127
  fi
}

need meson
need ninja
if [ -z "${CC:-}" ]; then
  if command -v cc >/dev/null 2>&1; then
    :
  elif command -v gcc >/dev/null 2>&1; then
    export CC=gcc
  elif command -v clang >/dev/null 2>&1; then
    export CC=clang
  else
    echo "build.sh: no C compiler (cc, gcc, clang)" >&2
    exit 127
  fi
fi

case "$root" in
  /mnt/*)
    echo "build.sh: warning: building on $root (WSL /mnt is slow; /tmp or \$HOME is better)" >&2
    ;;
esac

if [ ! -f "$BUILDDIR/build.ninja" ] || [ "$RECONFIGURE" -eq 1 ]; then
  # shellcheck disable=SC2086
  meson setup "$BUILDDIR" $SETUP_FLAGS \
    -Dcli=enabled -Dtui=auto -Dgui=auto -Ddocs=enabled -Dtests=enabled \
    -Dplugins=disabled $EXTRA_MESON
fi

meson compile -C "$BUILDDIR"
if [ "$RUN_TESTS" -eq 1 ]; then
  meson test -C "$BUILDDIR" -v
fi

meson install -C "$BUILDDIR" --destdir "$DESTDIR"

bin="$DESTDIR$PREFIX/bin/cuddle"
libdir=$(dirname "$(find "$DESTDIR$PREFIX" -name 'libcuddle.so*' 2>/dev/null | head -n 1)")
if [ -n "$libdir" ]; then
  export LD_LIBRARY_PATH="${libdir}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
export CUDDLE_SHARE="$DESTDIR$PREFIX/share/cuddle"
export CUDDLE_BINDIR="$DESTDIR$PREFIX/bin"

if [ -x "$bin" ]; then
  "$bin" --version
fi

echo "staged: $DESTDIR$PREFIX"
echo "run:    CUDDLE_SHARE=$CUDDLE_SHARE CUDDLE_BINDIR=$CUDDLE_BINDIR LD_LIBRARY_PATH=$LD_LIBRARY_PATH $bin"
