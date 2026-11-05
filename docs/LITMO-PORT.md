# Litmo rooms in Cuddle

This port adapts the owner's Litmo flow content and logic into Cuddle's MIT-licensed source, under the owner's explicit permission to reuse their code under MIT (2026-11-04). It does not change the upstream Litmo repository's license. External libraries retain their licenses. Exact upstream commit and blob references are in `litmo-source.json`.

Source repository: https://github.com/softnessasastrength/Litmo
Source revision: f4d00b2761d75d5d55d4124416d29befca556d8d

## What moved

- Flood: the six minimum steps from `app/lib/floodProtocolCore.ts`.
- Conflict: solo rehearsal, body check, I-statement template, repair script and exit/pause/reschedule moves from `conflictSimCore.ts`.
- Want or Should: six signal questions and conservative interpretation from `interestReCore.ts`. Unanswered or uncertain signals remain unknown; no confidence score is shown. The reading is never consent or a diagnosis.
- Morning: capacity/exit first, time-window suggestions, styles and check-in language from `morningCuddleCore.ts`. Boundaries default off limits, and no affection or gratitude is required to leave.
- Parallel Play: same-room silence, shared media, side-by-side work, tea/water, walk and reading corner, with entry and exit suggestions from `parallelPlayCore.ts`.
- Aftercare: branches for closeness, conflict, flood, good things and build spirals from `aftercareCore.ts`.
- Existing Cuddle themes, Spooning, Too Much and Soft Return are available in the same hub.

These are offline adaptations, not complete replicas of Litmo's React screens or all their negotiation models. The app does not record a real partner's agreement. History, timestamps, ledgers, scoring, notifications, haptics, Supabase and network services were not ported. No clinical outcomes are claimed.

## One engine

`data/cuddle/rooms.json` is the content source. `python3 tools/generate-rooms.py` validates it and generates checked-in `src/lib/rooms-data.inc`. `--check` checks for stale tables. Rebuild scripts regenerate it; direct Meson users should regenerate after editing the JSON.

`src/lib/rooms.c` owns sessions, branch targets, actual navigation history, touch labels, interest interpretation and stopping. Ending, stopping, opening a room, or returning home clears state. Back cannot revive an ended session; backing up during interest questions clears later answers.

The GTK4 and Qt6 hubs, native ncurses TUI, and `cuddle --room ID` use the C API. The Python TUI calls exactly that API through `room_bridge.py` using ctypes; it contains no separate protocol catalog or interpretation algorithm. With a custom installation layout, `CUDDLE_LIB` can point it at the matching `libcuddle.so.0`.

The optional HTML frontend remains a simple local STOP page. Existing Bash guide/protocol flags remain available; use `--room morning`, `--room parallel`, etc. for the new shared versions. `--flood`, `--conflict`, `--interest` and `--repair` use the shared engine directly.

## Rebuild from Crostini Terminal

```bash
bash ~/cuddle/rebuild-cuddle.sh
~/cuddle/build-current/native-stage/bin/cuddle-gui
~/cuddle/build-current/native-stage/bin/cuddle-tui
~/cuddle/build-current/native-stage/bin/cuddle --rooms
~/cuddle/build-current/native-stage/bin/cuddle --room conflict
```

This builds Python/HTML, ncurses/GTK and Python/Qt separately, runs tests, stages each under `build-current/`, and performs no apt or sudo operations. `setup-crostini.sh` remains available for first-time dependency installation.

The suite covers branch validity, stop from every step, irreversible ending, touch reset, uncertainty and pressure interpretation, invalid choices, back-history, CLI dispatch and actual terminal key paths through PTYs. Qt widget tests use its offscreen platform. GTK widget tests return skip status 77 when no display is accessible. Help/version checks do not initialize a display. Screen appearance and desktop integration still warrant a human visual check.

Original artifact backups remain in `~/cuddle-original-20261104/`; the immediate pre-port source snapshot is in `backups/pre-litmo-port-20261104-213114.tar.gz`.

## Subsequent expansion

Morning now has capacity/exit choices, duration, all seven style options across
pages, boundary discussion and check-in choices. Spooning has all seven roles,
all fourteen position names across pages, pressure, energy, duration, boundary,
check-in and optional reassurance choices. Wording preserves a free exit and
does not prescribe restraint. Choices are only a discussion map, not another
person's agreement. Paging does not exhaust navigation history, and going back
clears choices from the abandoned path.

`cuddle_session_summary` formats this visit's chosen labels from the C session;
all frontends use it. Stopping or finishing discards both selections and summary.
See SHARED-PREFERENCES.md for common startup settings.
