# Expansion

- Plugins: `-Dplugins=enabled` dlopens `$datadir/cuddle/plugins` and
  `$XDG_DATA_HOME/cuddle/plugins`. Off until soversion 1.
- Themes: `themes/<name>.theme`. `letter` and `word` required so color
  is never the only cue.
- Config: optional XDG `cuddle.conf`. Loading is read-only; missing = defaults.
  GTK explicitly saves appearance/startup preferences only. Room state is never written.
- i18n: `-Di18n=auto` via gettext. `po/LINGUAS` stays empty until a
  human writes catalogs.
- ABI: freeze `cuddle.h` at project 1.0. Additions OK. Removals bump
  soversion.
