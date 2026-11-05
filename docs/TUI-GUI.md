# TUI / GUI constraints

- Soft Signal (`q` / Esc) needs no reason and cannot be disabled.
- No network. Local HTML only for the default GUI.
- Touch Language is a map, never consent. Unset zones are off limits.
- Completing a screen is not a score. Nothing is stored.
- Combo toolkits: `tui_toolkit=python|curses`, `gui_toolkit=html|gtk|qt`.
  Dependencies via pkg-config, never `apt-get` in Meson.
