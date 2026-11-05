#!/bin/bash
# Build unsigned .debs from this tree.
# Copies onto a Linux fs (/tmp). Never pack from /mnt/c.
# Does not dput. Does not sign. Does not ask for a GPG passphrase.
set -eu
set -o pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$root"

VER=1.4.0
DEBVER=1.4.0-1
SRCNAME=cuddle
MAINT="Softness as Strength <thatcomputergeek021800@gmail.com>"
HOMEPAGE=https://softnessasastrength.com/
# Use a unique workspace; never delete a pre-existing source/build directory.
BUILD_PARENT=$(mktemp -d "${TMPDIR:-/tmp}/cuddle-debs.XXXXXX")
WORK="$BUILD_PARENT/cuddle-${VER}"
OUT="$root/packaging/out"
DEBS="$BUILD_PARENT/debs"
ARCH=$(dpkg --print-architecture)
MULTIARCH=$(dpkg-architecture -qDEB_HOST_MULTIARCH)

if [ -d "${HOME}/.local/cuddle-tools/bin" ]; then
  PATH="${HOME}/.local/cuddle-tools/bin:${PATH}"
  export PATH
fi

# Prefer system gcc for packages. Zig stays a fallback.
if [ -x /usr/bin/gcc ]; then
  CC=/usr/bin/gcc
else
  CC=${CC:-cc}
fi
export CC

case "$root" in
  /mnt/*)
    echo "dpkg-build-unsigned.sh: refusing to pack from $root (WSL /mnt is mode 777)" >&2
    echo "run this from /tmp or \$HOME." >&2
    exit 2
    ;;
esac

need() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "missing '$1' on PATH" >&2
    exit 127
  fi
}

if [ -d "$root/packaging/debian" ]; then
  OVERLAY="$root/packaging/debian"
elif [ -d "$root/debian" ]; then
  OVERLAY="$root/debian"
else
  echo "missing Debian overlay (packaging/debian or debian)" >&2
  exit 1
fi

umask 022

need meson
need ninja
need dpkg-deb
need tar
need xz

mkdir -p "$WORK" "$OUT" "$DEBS"

echo "copying source -> $WORK"
tar -C "$root" \
  --exclude='./builddir' \
  --exclude='./build-*' \
  --exclude='./.deps' \
  --exclude='./stage' \
  --exclude='./packaging/out' \
  --exclude='./packaging/debian' \
  --exclude='./.git' \
  --exclude='./*.deb' \
  --exclude='./cuddle_*.deb' \
  -cf - . | tar -C "$WORK" -xf -

# orig tarball: upstream without debian/
echo "writing $OUT/${SRCNAME}_${VER}.orig.tar.xz"
tar -C "$BUILD_PARENT" --exclude="${SRCNAME}-${VER}/packaging" --exclude="${SRCNAME}-${VER}/debian" \
  -c "${SRCNAME}-${VER}" \
  | xz -9 > "$OUT/${SRCNAME}_${VER}.orig.tar.xz"
cp -a "$OUT/${SRCNAME}_${VER}.orig.tar.xz" "$BUILD_PARENT/"

mkdir -p "$WORK/debian"
cp -a "$OVERLAY/." "$WORK/debian/"
chmod +x "$WORK/debian/rules"

have_dh=0
have_gtk=0
have_curses=0
if command -v dh >/dev/null 2>&1; then
  have_dh=1
fi
if command -v pkg-config >/dev/null 2>&1; then
  if pkg-config --exists gtk4; then have_gtk=1; fi
  if pkg-config --exists ncursesw || pkg-config --exists ncurses; then
    have_curses=1
  fi
fi

if [ "$have_dh" -eq 1 ] && [ "$have_gtk" -eq 1 ] && [ "$have_curses" -eq 1 ]; then
  echo "official path: dpkg-buildpackage (curses + gtk, unsigned)"
  cd "$WORK"
  dpkg-buildpackage -us -uc -b --no-sign
  cd "$BUILD_PARENT"
  mv -f libcuddle0_*.deb libcuddle-dev_*.deb \
    ${SRCNAME}_*.deb ${SRCNAME}-data_*.deb ${SRCNAME}-tui_*.deb ${SRCNAME}-gui_*.deb \
    libcuddle0-dbgsym_*.ddeb cuddle-dbgsym_*.ddeb cuddle-tui-dbgsym_*.ddeb cuddle-gui-dbgsym_*.ddeb \
    ${SRCNAME}_*.buildinfo ${SRCNAME}_*.changes "$DEBS"/ 2>/dev/null || true
  cp -a "$DEBS"/. "$OUT"/
  echo "debs: $OUT"
  exit 0
fi

echo "debhelper/gtk/ncurses not all present."
echo "falling back to Meson html/python split debs (unsigned)."
echo "debian/rules still wants: -Dtui_toolkit=curses -Dgui_toolkit=gtk"

cd "$WORK"
BUILD="$WORK/pkgbuild"
DEST="$WORK/destdir"
rm -rf "$BUILD" "$DEST"
meson setup "$BUILD" \
  --prefix=/usr --libdir=lib --buildtype=release \
  -Dpkgconfigdir="lib/$MULTIARCH/pkgconfig" \
  -Dcli=enabled -Dtui=auto -Dgui=auto \
  -Dtui_toolkit=python -Dgui_toolkit=html \
  -Dplugins=disabled -Ddocs=enabled -Dtests=enabled -Di18n=auto
meson compile -C "$BUILD"
meson test -C "$BUILD" -v
meson install -C "$BUILD" --destdir "$DEST"

# gzip man pages for policy
if [ -d "$DEST/usr/share/man" ]; then
  find "$DEST/usr/share/man" -type f -name '*.1' -exec gzip -9n {} +
fi

# strip ELF
if command -v strip >/dev/null 2>&1; then
  find "$DEST/usr/bin" "$DEST/usr/lib" -type f -exec file {} \; \
    | awk -F: '/ELF/ {print $1}' \
    | while IFS= read -r f; do
        strip --strip-unneeded "$f" 2>/dev/null || true
      done
fi

shlib_deps() {
  if command -v dpkg-shlibdeps >/dev/null 2>&1; then
    (
      cd "$WORK"
      export LD_LIBRARY_PATH="$DEST/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
      # shellcheck disable=SC2086
      dpkg-shlibdeps -O $1 2>/dev/null | sed -n 's/^shlibs:Depends=//p' || true
    )
  fi
}

docdir() {
  pkg=$1
  d="$2/usr/share/doc/$pkg"
  mkdir -p "$d"
  cp "$OVERLAY/copyright" "$d/copyright"
  gzip -9n -c "$OVERLAY/changelog" > "$d/changelog.Debian.gz"
}

control() {
  pkg=$1
  ver=$2
  arch=$3
  section=$4
  depends=$5
  rec=$6
  desc=$7
  long=$8
  printf 'Package: %s\n' "$pkg"
  printf 'Version: %s\n' "$ver"
  printf 'Section: %s\n' "$section"
  printf 'Priority: optional\n'
  printf 'Architecture: %s\n' "$arch"
  printf 'Maintainer: %s\n' "$MAINT"
  printf 'Homepage: %s\n' "$HOMEPAGE"
  if [ -n "$depends" ]; then
    printf 'Depends: %s\n' "$depends"
  fi
  if [ -n "$rec" ]; then
    printf 'Recommends: %s\n' "$rec"
  fi
  printf 'Description: %s\n' "$desc"
  printf '%s\n' "$long" | sed 's/^/ /'
}

md5() {
  (
    cd "$1"
    find usr -type f -print0 | sort -z | xargs -0 md5sum
  ) > "$1/DEBIAN/md5sums" 2>/dev/null || true
}

pack() {
  pkg=$1
  src=$2
  # Policy: dirs 0755, shared libs 0644. umask 002 otherwise leaves 0775.
  find "$src" -type d -exec chmod 0755 {} +
  find "$src" -type f -name 'libcuddle.so*' -exec chmod 0644 {} +
  fakeroot=
  if command -v fakeroot >/dev/null 2>&1; then
    fakeroot=fakeroot
  fi
  $fakeroot dpkg-deb --root-owner-group --build "$src" "$DEBS/${pkg}_${DEBVER}_$(awk -F': ' '/^Architecture:/{print $2; exit}' "$src/DEBIAN/control").deb"
}

# --- libcuddle0 ---
P=$WORK/pkg/libcuddle0
mkdir -p "$P/DEBIAN" "$P/usr/lib"
cp -a "$DEST/usr/lib/libcuddle.so.1.4.0" "$P/usr/lib/"
cp -a "$DEST/usr/lib/libcuddle.so.0" "$P/usr/lib/"
docdir libcuddle0 "$P"
printf 'libcuddle 0 libcuddle0 (>= 1.4.0)\n' > "$P/DEBIAN/shlibs"
printf 'activate-noawait ldconfig\n' > "$P/DEBIAN/triggers"
chmod 0644 "$P/usr/lib/libcuddle.so.1.4.0"
dep=$(shlib_deps "$DEST/usr/lib/libcuddle.so.1.4.0")
[ -n "$dep" ] || dep="libc6 (>= 2.34)"
control libcuddle0 "$DEBVER" "$ARCH" libs "$dep" "" \
  "cuddle core library" \
  "libcuddle holds version, datadir paths, the protocol catalog, themes,
and optional read-only XDG config. soversion 0: ABI is not frozen.
Nothing is stored or sent." > "$P/DEBIAN/control"
md5 "$P"
pack libcuddle0 "$P"

# --- libcuddle-dev ---
P=$WORK/pkg/libcuddle-dev
mkdir -p "$P/DEBIAN" "$P/usr/lib/$MULTIARCH/pkgconfig" "$P/usr/include"
cp -a "$DEST/usr/lib/libcuddle.so" "$P/usr/lib/"
cp -a "$DEST/usr/lib/$MULTIARCH/pkgconfig/cuddle.pc" "$P/usr/lib/$MULTIARCH/pkgconfig/"
cp -a "$DEST/usr/include/cuddle.h" "$DEST/usr/include/cuddle-plugin.h" "$P/usr/include/"
docdir libcuddle-dev "$P"
control libcuddle-dev "$DEBVER" "$ARCH" libdevel \
  "libcuddle0 (= $DEBVER)" "" \
  "development files for libcuddle" \
  "Headers and pkg-config for libcuddle. Plugin ABI is documented but
the loader stays disabled until soversion 1." > "$P/DEBIAN/control"
md5 "$P"
pack libcuddle-dev "$P"

# --- cuddle-data ---
P=$WORK/pkg/cuddle-data
mkdir -p "$P/DEBIAN" "$P/usr/share"
cp -a "$DEST/usr/share/cuddle" "$P/usr/share/"
docdir cuddle-data "$P"
control cuddle-data "$DEBVER" all misc \
  "" "python3" \
  "arch-independent data for cuddle" \
  "Bash engine, themes, TUI script, and the local Soft Signal HTML page
installed under /usr/share/cuddle." > "$P/DEBIAN/control"
md5 "$P"
pack cuddle-data "$P"

# --- cuddle ---
P=$WORK/pkg/cuddle
mkdir -p "$P/DEBIAN" "$P/usr/bin" "$P/usr/share/man/man1" "$P/usr/share/doc/cuddle"
cp -a "$DEST/usr/bin/cuddle" "$P/usr/bin/"
cp -a "$DEST/usr/share/man/man1/cuddle.1.gz" "$P/usr/share/man/man1/"
if [ -f "$DEST/usr/share/doc/cuddle/cuddle.conf.example" ]; then
  cp -a "$DEST/usr/share/doc/cuddle/cuddle.conf.example" "$P/usr/share/doc/cuddle/"
fi
docdir cuddle "$P"
dep=$(shlib_deps "$DEST/usr/bin/cuddle")
[ -n "$dep" ] || dep="libc6 (>= 2.34)"
dep="$dep, libcuddle0 (= $DEBVER), cuddle-data (>= $DEBVER)"
control cuddle "$DEBVER" "$ARCH" misc "$dep" "cuddle-tui, cuddle-gui" \
  "consent-first local companion (CLI)" \
  "Local, file-less companion. Soft Signal (q) needs no explanation.
The CLI dispatches to the bash engine and a C flood protocol.
Not therapy. Not a product. Nothing is stored or sent." > "$P/DEBIAN/control"
md5 "$P"
pack cuddle "$P"

# --- cuddle-tui ---
P=$WORK/pkg/cuddle-tui
mkdir -p "$P/DEBIAN" "$P/usr/bin" "$P/usr/share/man/man1"
cp -a "$DEST/usr/bin/cuddle-tui" "$P/usr/bin/"
cp -a "$DEST/usr/share/man/man1/cuddle-tui.1.gz" "$P/usr/share/man/man1/"
docdir cuddle-tui "$P"
dep=$(shlib_deps "$DEST/usr/bin/cuddle-tui")
[ -n "$dep" ] || dep="libc6 (>= 2.34)"
dep="$dep, libcuddle0 (= $DEBVER), cuddle-data (>= $DEBVER), python3"
control cuddle-tui "$DEBVER" "$ARCH" misc "$dep" "" \
  "terminal frontend for cuddle" \
  "Python curses hub (this unsigned fallback). q is Soft Signal.
Touch Language is a map, never consent." > "$P/DEBIAN/control"
md5 "$P"
pack cuddle-tui "$P"

# --- cuddle-gui ---
P=$WORK/pkg/cuddle-gui
mkdir -p "$P/DEBIAN" "$P/usr/bin" "$P/usr/share/man/man1"
cp -a "$DEST/usr/bin/cuddle-gui" "$P/usr/bin/"
cp -a "$DEST/usr/share/man/man1/cuddle-gui.1.gz" "$P/usr/share/man/man1/"
docdir cuddle-gui "$P"
dep=$(shlib_deps "$DEST/usr/bin/cuddle-gui")
[ -n "$dep" ] || dep="libc6 (>= 2.34)"
dep="$dep, libcuddle0 (= $DEBVER), cuddle-data (>= $DEBVER)"
control cuddle-gui "$DEBVER" "$ARCH" misc "$dep" "xdg-utils" \
  "local Soft Signal GUI for cuddle" \
  "Opens the local HTML Soft Signal page (this unsigned fallback).
Local only. No network." > "$P/DEBIAN/control"
md5 "$P"
pack cuddle-gui "$P"

cp -a "$DEBS"/. "$OUT"/
echo
echo "unsigned debs:"
ls -l "$OUT"/*.deb "$OUT"/*.orig.tar.xz 2>/dev/null || ls -l "$DEBS"
echo
echo "install:  sudo dpkg -i $OUT/libcuddle0_${DEBVER}_${ARCH}.deb $OUT/cuddle-data_${DEBVER}_all.deb $OUT/cuddle_${DEBVER}_${ARCH}.deb"
echo "optional: sudo dpkg -i $OUT/cuddle-tui_${DEBVER}_${ARCH}.deb $OUT/cuddle-gui_${DEBVER}_${ARCH}.deb $OUT/libcuddle-dev_${DEBVER}_${ARCH}.deb"
echo "sign later is a separate step. do not dput from here."
