# Cuddle

Offline Litmo rooms for ordinary human weather. Cuddle offers a CLI, terminal
interfaces, and native GTK4 and Qt6 room hubs, backed by a shared C library.

**Soft Signal (`q` / Esc) stops a room without requiring a reason.** Room answers
and touch labels stay in memory and clear when the room ends. Only appearance
and startup preferences can be explicitly saved. Touch Language is a map,
never consent. Not therapy. No network calls, scoring, or haptics in the app.

## Rooms

Flood Protocol · Touch Language · Conflict Practice · Want or Should? · Morning
Cuddle · Spooning · I’m Too Much · Parallel Play · Aftercare · Soft Return.

Morning and Spooning include selectable discussion details and a temporary
summary. A selection in the app never records another person's agreement.

## Build on Linux

Dependencies include a C11 compiler, Meson, Ninja and pkg-config. Native
frontends additionally use GTK4, ncurses and optionally Qt6.

For a Debian-based development environment such as Chromebook Crostini:

```bash
bash setup-crostini.sh
```

This helper installs development dependencies through apt, then builds and tests
Python/HTML, ncurses/GTK4 and Python/Qt6 variants into project-local directories.
It does not change ChromeOS settings.

Once dependencies are installed, rebuild without sudo:

```bash
bash rebuild-cuddle.sh
./build-current/native-stage/bin/cuddle-gui
./build-current/native-stage/bin/cuddle-tui
./build-current/native-stage/bin/cuddle --rooms
./build-current/native-stage/bin/cuddle --room conflict
```

For a native Meson build without system installation:

```bash
meson setup builddir --prefix="$PWD/stage" --libdir=lib \
  -Dtui=enabled -Dtui_toolkit=curses \
  -Dgui=enabled -Dgui_toolkit=gtk -Dplugins=disabled
meson compile -C builddir
meson test -C builddir --print-errorlogs
meson install -C builddir
./stage/bin/cuddle-gui
```

Toolkit options: `tui_toolkit=python|curses`, `gui_toolkit=html|gtk|qt`.
The HTML variant is a simple local STOP page. The Python TUI calls libcuddle
through ctypes; GTK4, Qt6, ncurses and CLI rooms use the same native engine.

## Desktop and preferences

```bash
bash install-desktop.sh
```

This registers user-local **Cuddle** and **Cuddle Terminal** launchers pointing
at the current native build.

In GTK, open **Preferences** or press **Ctrl+,** to preview and explicitly save
appearance, larger text, protocol-label theme and starting room. Leaving without
saving cancels the preview. Room answers are never included in that file.

The optional command-line helper edits the same preferences:

```bash
bash preferences-cuddle.sh --appearance dark --large-text true --start-room hub
```

## Packaging

```bash
bash build-debs.sh
```

Six unsigned native GTK4/ncurses Debian packages and optional debug symbols are
written to `releases/`; extracted package runtime checks run before a successful
release is linked at `releases/latest`. No packages are installed by this helper.

The Ubuntu PPA target is **26.04 LTS (resolute)**. Source-only packaging, signing
and explicit upload are separate operations. See [Launchpad instructions](docs/LAUNCHPAD.md)
and [PPA build status](https://launchpad.net/~xtoxicwastx/+archive/ubuntu/cuddle/+packages).
Do not add an Ubuntu PPA to Debian Crostini; use the local build there.

## Source map

| Path | Purpose |
| --- | --- |
| `src/lib/` | libcuddle, room state, themes and preferences |
| `src/cli/` | CLI and Bash dispatch |
| `src/tui/` | Python launcher and native ncurses frontend |
| `src/gui/` | GTK4, Qt6 and local HTML launcher |
| `data/cuddle/rooms.json` | Shared room content source |
| `data/cuddle/` | Python interface, Bash protocols and local assets |
| `debian/` | Native GTK4/ncurses packaging |
| `tests/` | Engine, preferences, terminal and upload validation |

After changing `rooms.json`, run `python3 tools/generate-rooms.py`; the checked-in
C tables are validated by the test suite. Plugins remain disabled and library
ABI soversion 0 is unstable.

See [architecture](docs/ARCHITECTURE.md), [shared preferences](docs/SHARED-PREFERENCES.md),
[Litmo provenance](docs/LITMO-PORT.md) and [contributing](CONTRIBUTING.md).

## License

MIT. Litmo-derived code is included with the owner's permission to use it under
MIT; external dependencies retain their own licenses. See [LICENSE](LICENSE).
