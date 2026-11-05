# Shared preferences

All frontends read `$XDG_CONFIG_HOME/cuddle/cuddle.conf`, or `~/.config/cuddle/cuddle.conf`. Loading settings never writes the file. GTK's **Preferences** screen has an explicit **Save preferences** action that writes only appearance, text size, protocol theme, and starting room. Preview changes are discarded when you leave without saving. Qt's per-window appearance toggle remains temporary; it reads the same saved file at startup. Room answers are never saved.

Open GTK Preferences with the toolbar button or **Ctrl+,**. Opening it ends the current room. **Back to rooms** or Soft Signal discards an unsaved preview. **Preview defaults** does not save until you choose Save. Saved appearance applies immediately; the starting room applies on the next launch.

The command-line helper is also available:

```bash
bash ~/cuddle/preferences-cuddle.sh --appearance dark --large-text true --start-room morning
```

Options:

- `--appearance system|light|dark`: GUI palette and native terminal foreground/background. System keeps the toolkit/terminal default.
- `--large-text true|false`: larger text in GTK/Qt; roomier paragraphs in terminal interfaces. The app does not change the terminal emulator's font size.
- `--theme default|dim`: libcuddle protocol-label theme loaded from the local themes directory. This is separate from appearance.
- `--start-room hub|ROOM_ID`: initial GUI/TUI room. CLI `cuddle --tui` uses it too; plain `cuddle` opens a configured non-hub room, otherwise keeps its short invitation. Explicit room arguments override the preference.

With no options, the helper shows the current file and usage. Omitted settings and unrelated keys such as browser and datadir_override are preserved. Existing files are backed up under `~/cuddle/backups/`. New configuration files use mode 0600. No protocol choices or touch labels are written.

Example file:

```ini
appearance = dark
large_text = true
start_room = morning
theme = dim
```

Restart an already-open app to apply startup preferences. Unknown rooms or appearance names fall back to hub/system. Missing files use default theme, hub, and normal text. Older `tui_screen` values remain supported as a fallback when `start_room` is absent.


The GTK save action preserves unrelated keys/comments and uses a temporary file plus atomic replacement with mode 0600. Errors leave the existing file in place and appear in the Preferences screen. Symlinked configuration files are left intact and can be edited directly instead. No room state is passed to the saving API.
