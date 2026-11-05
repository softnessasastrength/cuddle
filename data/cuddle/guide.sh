# original cuddle 1.1.0 consent guide
# shellcheck shell=bash

need_flow() {
  choose 'What would help most right now?
  1) Quiet company
  2) A hug or other touch
  3) Reassurance / being heard
  4) Practical help
  5) I am not sure
  q) Soft Signal' 5
  [[ "$REPLY" == q ]] && { soft_signal; return; }
  case "$REPLY" in
    1) printf '\nYou might ask: “Could we sit together for a bit? No pressure to talk.”\nName a duration or an easy exit if that helps.\n' ;;
    2) printf '\nTouch needs a specific, freely given yes. Try: “Would a hug feel okay right now?”\nIf the answer is anything but an enthusiastic yes, treat it as no and stay kind.\n' ;;
    3) printf '\nYou might ask: “Do you have capacity to listen for ten minutes? I do not need you to fix it.”\n' ;;
    4) printf '\nTry one concrete request: “Could you bring me water / help me make a plan / check in tomorrow?”\n' ;;
    5) printf '\nNot knowing is enough information. Start smaller: water, a blanket, a walk, silence, or asking for company.\n' ;;
  esac
  pause
  [[ "$REPLY" == q ]] && soft_signal
}

ask_flow() {
  printf '\nA good ask makes refusal safe and the request concrete.\n'
  choose 'What kind of request are you considering?
  1) Touch or a hug
  2) Time together
  3) Emotional support
  4) Something else
  q) Soft Signal' 4
  [[ "$REPLY" == q ]] && { soft_signal; return; }
  case "$REPLY" in
    1) printf '\n“Would you be open to a hug? It is completely okay to say no.”\nOffer choices when useful: brief/long, seated/standing, no touch but nearby.\n' ;;
    2) printf '\n“Would you like to spend some low-key time together this week? If not, no worries.”\nBe clear about the shape: a call, a walk, parallel quiet, or a set time.\n' ;;
    3) printf '\n“Do you have the bandwidth to hear something tender? You can say no or choose another time.”\n' ;;
    4) printf '\nUse this shape: “Would you be willing to [specific thing]? It is okay if not; I can handle a no.”\n' ;;
  esac
  printf '\nA yes is for this moment, not a permanent pass. Check in again if anything changes.\n'
  pause
  [[ "$REPLY" == q ]] && soft_signal
}

boundary_flow() {
  choose 'What boundary do you need?
  1) Less or no touch
  2) More time / slower pace
  3) A clearer agreement
  4) To stop an interaction now
  q) Soft Signal' 4
  [[ "$REPLY" == q ]] && { soft_signal; return; }
  case "$REPLY" in
    1) printf '\n“I care about you, and I do not want touch right now. Please give me space.”\nYou do not owe a justification.\n' ;;
    2) printf '\n“I want to slow down. I need time to check in with myself before deciding.”\n' ;;
    3) printf '\n“I am comfortable with [what is okay], but not [what is not]. Can we agree to check first?”\n' ;;
    4) printf '\n“Stop.” “Not now.” “I need to leave.” All are complete sentences.\nPrioritize getting to a safer place or contacting someone you trust if you need to.\n' ;;
  esac
  pause
  [[ "$REPLY" == q ]] && soft_signal
}

uncertainty_flow() {
  printf '\nUncertainty is not consent, and it does not make you difficult.\n\n'
  printf 'You can pause: “I am not sure. I need a minute.”\n'
  printf 'You can choose a lower-stakes option: sit nearby, talk, or make a plan for later.\n'
  printf 'You can opt out entirely. A caring person can handle a no, a pause, or a change of mind.\n'
  pause
  [[ "$REPLY" == q ]] && soft_signal
}

opt_out_flow() {
  printf '\nYou can leave this guide, decline contact, or change your mind at any point.\n'
  printf 'No explanation is required. Take the next small step that feels safe.\n'
  printf 'If you are in immediate danger, contact local emergency services or a trusted person nearby.\n'
  pause
  [[ "$REPLY" == q ]] && soft_signal
}

guide() {
  need_tty || return $?
  while :; do
    printf '\n=== cuddle: consent-first guide ===\n'
    printf 'This is private: no network, no files, no saved answers.\n'
    choose 'Where would you like to start?
  1) Identify what I need
  2) Ask for affection or support
  3) Set or name a boundary
  4) I feel uncertain
  5) Opt out / leave
  q) Soft Signal / quit' 5
    case "$REPLY" in
      1) need_flow ;;
      2) ask_flow ;;
      3) boundary_flow ;;
      4) uncertainty_flow ;;
      5) opt_out_flow ;;
      q) soft_signal; return 0 ;;
    esac
  done
}
