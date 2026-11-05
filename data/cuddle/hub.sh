# Containment hub — single door, not a buffet
# shellcheck shell=bash

hub() {
  need_tty || return $?
  banner
  while :; do
    choose '=== containment hub ===
  1) Consent-first guide (original)
  2) The Map — first ritual for younger-you
  3) Soft Signal practice
  4) Morning Cuddle
  5) Spooning Protocol
  6) I am not ready to get up yet
  7) I am too much / fear of abandonment
  8) Flood Protocol
  9) Aftercare
  10) Parallel play (closeness without a touch tax)
  11) Touch Language primer (never consent)
  q) Soft Signal / quit' 11
    case "$REPLY" in
      1) guide ;;
      2) first_ritual ;;
      3) soft_signal_practice ;;
      4) morning_cuddle ;;
      5) spooning ;;
      6) not_ready_yet ;;
      7) too_much ;;
      8) flood_protocol ;;
      9) aftercare ;;
      10) parallel_play ;;
      11) touch_language ;;
      q) soft_signal; return 0 ;;
    esac
  done
}
