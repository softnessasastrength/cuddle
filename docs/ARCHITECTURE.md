# Architecture

libcuddle is the ABI. Frontends are optional and only call the library.

| piece | path | role |
| --- | --- | --- |
| libcuddle | `src/lib` | version, paths, catalog, theme, config, plugins, i18n |
| CLI | `src/cli` | dispatcher + C flood |
| TUI | `src/tui` | python exec (`tui.py`) or ncurses |
| GUI | `src/gui` | local HTML, gtk4, or Qt6 |
| data | `data/cuddle` | `$datadir/cuddle` — bash engine, TUI script, HTML |

soversion is 0 (unstable). Plugins stay off until soversion 1 / project 1.0.

No network. No writes of protocol answers. Configuration loading is read-only.
GTK can explicitly save appearance/startup preferences through a separate settings-only API.
Touch Language is a map, never consent. Soft Signal (`q`) cannot be disabled.
