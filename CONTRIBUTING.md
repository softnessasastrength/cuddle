# Contributing to Cuddle

Build locally with `bash rebuild-cuddle.sh`. The CI matrix covers Python/HTML,
ncurses/GTK4 and Python/Qt6. GTK widget tests require a display or Xvfb; Qt widget
tests use the offscreen platform. Terminal integration tests use PTYs.

Keep the app offline. Room answers and touch-map labels stay ephemeral. Explicit
preference saving accepts appearance/startup settings only. Soft Signal must
remain available everywhere and cannot require an explanation or saved answer.
Touch Language and discussion choices are never another person's consent.

Edit room content in `data/cuddle/rooms.json`, regenerate with
`python3 tools/generate-rooms.py`, and keep room behavior in the shared engine.
Run the relevant tests plus the complete native build for changes to that engine.
Do not enable plugins while the ABI is unstable.

Do not commit keys, credentials, local configuration, backups, build directories,
release artifacts or personal audit/handoff reports. A PPA source archive is
immutable once frozen; source changes after a release need a new upstream version.
Do not regenerate an already-published `.orig.tar.xz` with different contents.

Keep package uploads and signing separate from ordinary builds. No upload,
publishing, signing or dependency installation runs automatically in the app.

Local-only agent handoffs and audit reports are intentionally ignored. Keep new
contributor-facing guidance in this document and technical documentation instead.
The initial GitHub import includes post-PPA upload-tool compatibility fixes;
there is no matching release tag until a new reviewed release is cut.
