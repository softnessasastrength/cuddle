# Packaging Cuddle

Meson is distro-neutral. Package-manager commands and distro package names stay
in packaging recipes and setup helpers, not in `meson.build`.

## Native local packages

The current Debian overlay is `debian/`. Its rules select ncurses and GTK4,
with plugins disabled. To build and verify packages without installing them:

```bash
bash build-debs.sh
```

The helper copies source into a unique workspace, runs `dpkg-buildpackage`,
extracts the six runtime/development packages, and tests their staged runtime.
Outputs, debug symbols and checksums go under `releases/`; `releases/latest`
points at the last successful local build. These paths are ignored by Git.

| Package | Contents |
| --- | --- |
| `libcuddle0` | Shared library, unstable ABI soversion 0 |
| `libcuddle-dev` | Public headers, development symlink and pkg-config metadata |
| `cuddle` | CLI and shared-room/Bash dispatch |
| `cuddle-tui` | ncurses hub |
| `cuddle-gui` | GTK4 room hub and desktop integration |
| `cuddle-data` | Bash/Python/HTML data, room catalog and themes |

The Qt6 and Python/HTML variants can be built separately with
`rebuild-cuddle.sh`. They are not additional binary packages in the native split.

## Installation staging

```bash
meson setup builddir --prefix=/usr --libdir=lib --buildtype=release \
  -Dtui=enabled -Dtui_toolkit=curses -Dgui=enabled -Dgui_toolkit=gtk \
  -Dplugins=disabled
meson compile -C builddir
meson test -C builddir --print-errorlogs
meson install -C builddir --destdir "$PWD/stage"
```

Runtime dependencies belong in `debian/control` and generated shlibs metadata.
Libraries currently use `/usr/lib`; packages do not claim multiarch co-installation.
The pkg-config file is installed in the host's multiarch discovery directory.

## Launchpad

See [LAUNCHPAD.md](LAUNCHPAD.md). Root Debian packaging uses `unstable`; PPA
preparation creates a separate overlay targeting the requested Ubuntu series.
Launchpad receives signed source-only artifacts and builds Ubuntu binaries itself.
A frozen/published upstream archive cannot be recreated with different contents.
Use a new upstream version for the next source release.

The older `tools/dpkg-build-unsigned.sh` is retained for compatibility; the root
`build-debs.sh` is the preferred current native-package entry point.
