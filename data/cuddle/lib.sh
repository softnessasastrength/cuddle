# cuddle shared helpers — lo-fi Litmo containment
# shellcheck shell=bash

SOFTNESS="Softness is not the opposite of strength. It is the highest form of it."
VERSION="1.4.0"
LITMO_URL="https://github.com/softnessasastrength/Litmo"

need_tty() {
  if [[ ! -t 0 || ! -t 1 ]]; then
    printf 'cuddle needs an interactive terminal for this command. Try cuddle --help.\n' >&2
    return 2
  fi
}

banner() {
  cat <<EOF

  cuddle ${VERSION} — lo-fi Litmo containment
  Local only. Nothing is stored. Nothing is sent.
  q is Soft Signal: stop now, no explanation, no penalty.
  Not therapy. Not a product. Armor, not homework.
  ${LITMO_URL}

EOF
}

pause() {
  printf '\nPress Enter to continue (q = Soft Signal). '
  local _line
  IFS= read -r _line || return 0
  case "$_line" in
    q|Q) REPLY=q; return 0 ;;
  esac
  REPLY=""
}

# choose PROMPT MAX
# Sets REPLY to 1..MAX or q. EOF is Soft Signal.
choose() {
  local prompt="$1" max="$2" answer
  while :; do
    printf '\n%b\n> ' "$prompt"
    IFS= read -r answer || { REPLY=q; return 0; }
    case "$answer" in
      q|Q) REPLY=q; return 0 ;;
    esac
    if [[ "$answer" =~ ^[0-9]+$ ]] && (( answer >= 1 && answer <= max )); then
      REPLY="$answer"
      return 0
    fi
    printf 'Choose 1-%s, or q for Soft Signal.\n' "$max"
  done
}

soft_signal() {
  cat <<'EOF'

  ── Soft Signal ──
  Ended immediately.
  No explanation. No penalty. Not a failure.

  Stopping safely is success.
  This is not emergency services.
  If you are in immediate danger, contact local
  emergency services or a trusted person nearby.

EOF
}

step_walk() {
  # Print each argument as a step; Enter continues; q Soft Signals.
  local line
  for line in "$@"; do
    printf '\n  %s\n' "$line"
    pause
    if [[ "${REPLY:-}" == q ]]; then
      soft_signal
      return 0
    fi
  done
  REPLY=""
}

exit_kind() {
  cat <<'EOF'

  Take care of yourself.
  A yes is only for this moment.
  You can come back later, or not.

EOF
}
