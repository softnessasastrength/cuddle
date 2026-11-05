#!/usr/bin/env bash
# cuddle — softness as a command; lo-fi Litmo containment
set -euo pipefail

resolve_share() {
  local here
  if [[ -n "${CUDDLE_SHARE:-}" && -f "$CUDDLE_SHARE/lib.sh" ]]; then
    return 0
  fi
  here="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
  if [[ -f "$here/../share/cuddle/lib.sh" ]]; then
    CUDDLE_SHARE="$(CDPATH= cd -- "$here/../share/cuddle" && pwd)"
  elif [[ -f /usr/share/cuddle/lib.sh ]]; then
    CUDDLE_SHARE=/usr/share/cuddle
  else
    printf 'cuddle: cannot find protocol files in /usr/share/cuddle\n' >&2
    exit 1
  fi
}

resolve_share
# shellcheck source=/usr/share/cuddle/lib.sh
source "$CUDDLE_SHARE/lib.sh"
# shellcheck source=/usr/share/cuddle/guide.sh
source "$CUDDLE_SHARE/guide.sh"
# shellcheck source=/usr/share/cuddle/protocols.sh
source "$CUDDLE_SHARE/protocols.sh"
# shellcheck source=/usr/share/cuddle/hub.sh
source "$CUDDLE_SHARE/hub.sh"

run_tui() {
  local screen="${1:-hub}"
  need_tty || return $?
  if ! command -v python3 >/dev/null 2>&1; then
    printf 'cuddle --tui needs python3. Opening the bash hub instead.\n' >&2
    hub
    return
  fi
  CUDDLE_BIN="${CUDDLE_BIN:-$0}" python3 "$CUDDLE_SHARE/tui.py" "$screen"
}

open_gui() {
  local f="$CUDDLE_SHARE/gui/stop.html"
  if [[ ! -f "$f" ]]; then
    printf 'cuddle: missing Soft Signal page at %s\n' "$f" >&2
    return 1
  fi
  if command -v wslview >/dev/null 2>&1; then
    wslview "$f" >/dev/null 2>&1 &
  elif command -v explorer.exe >/dev/null 2>&1; then
    explorer.exe "$(wslpath -w "$f")" >/dev/null 2>&1 &
  elif command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$f" >/dev/null 2>&1 &
  elif command -v open >/dev/null 2>&1; then
    open "$f" >/dev/null 2>&1 &
  else
    printf 'Open this file in a browser (local, no network):\n  %s\n' "$f"
    return 0
  fi
  printf 'Opened Soft Signal in a browser. Local file. Nothing is sent.\n'
}

usage() {
  cat <<EOF
cuddle — a small reminder that presence is a language.
         lo-fi port of Litmo containment protocols.

Usage:
  cuddle              short invitation
  cuddle --soft       longer note
  cuddle --guide      original consent-first guide
  cuddle --hub        bash containment hub
  cuddle --tui        TUI hub (flood, touch map, Soft Signal)
  cuddle --tui flood  TUI flood screen (huge type)
  cuddle --tui touch  TUI Touch Language map (never consent)
  cuddle --gui        local HTML Soft Signal Stop button
  cuddle --map        first ritual (the map, not the mirror)
  cuddle --stop       Soft Signal practice (CLI)
  cuddle --morning    morning cuddle protocol
  cuddle --spoon      spooning protocol
  cuddle --hold       I am not ready to get up yet
  cuddle --too-much   abandonment / too-much panic room
  cuddle --flood      when language is gone (CLI)
  cuddle --aftercare  land the plane
  cuddle --parallel   closeness without a touch tax
  cuddle --touch      touch language primer (CLI)
  cuddle --help       this text
  cuddle --version    print version

q is always Soft Signal: stop now, no explanation, no penalty.
Nothing here phones home, writes files, or remembers answers.
Not therapy. Armor, not homework.
Consent stays with you.
EOF
}

short() {
  cat <<EOF

  come closer if you want to.
  stay if you want to.
  leave if you need to.

  $SOFTNESS

  (cuddle --tui opens the cathedral. cuddle --gui is Stop.)

EOF
}

long() {
  cat <<EOF

  Touch is not a transaction — it is a language.

  This command does not assume you want contact.
  It does not keep a ledger of who said yes.
  It only leaves the door unlocked.

  cuddle --tui is a real terminal cathedral: hub, flood,
  Touch Language map, Soft Signal chrome. --gui opens a
  local Stop button. --hub keeps the bash menus.

  q always works. Completing a protocol proves nothing.
  This is a starting point, not a product.

  $SOFTNESS

  https://softnessasastrength.com/
  https://github.com/softnessasastrength/Litmo

EOF
}

case "${1:-}" in
  -h|--help) usage ;;
  --soft|-s) long ;;
  --guide|-g) guide ;;
  --hub|--cathedral) hub ;;
  --tui) run_tui "${2:-hub}" ;;
  --gui|--stop-gui) open_gui ;;
  --map|--ritual|--first-ritual) first_ritual ;;
  --stop|--soft-signal) soft_signal_practice ;;
  --morning|--morning-cuddle) morning_cuddle ;;
  --spoon|--spooning) spooning ;;
  --hold|--not-ready) not_ready_yet ;;
  --too-much|--toomuch) too_much ;;
  --flood) flood_protocol ;;
  --aftercare) aftercare ;;
  --parallel|--parallel-play) parallel_play ;;
  --touch|--touch-language) touch_language ;;
  --version|-V) printf 'cuddle %s\n' "$VERSION" ;;
  "") short ;;
  *) usage; exit 1 ;;
esac
