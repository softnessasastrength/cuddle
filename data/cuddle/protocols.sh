# lo-fi ports of Litmo containment protocols
# Source of truth: https://github.com/softnessasastrength/Litmo
# shellcheck shell=bash

# --- Soft Signal practice (docs/SOFT_SIGNAL.md, FIRST_RITUAL.md) ---

soft_signal_practice() {
  need_tty || return $?
  cat <<'EOF'

  === Soft Signal practice ===
  The only rule that actually matters: you can always stop.
  No explanation. Not to this program. Not to anyone. Not ever.

  Feel that once on purpose, before you ever need it for real.
  Practicing it is never required to continue anything else.

EOF
  choose 'Ready to fire a practice Soft Signal?
  1) Fire Soft Signal now
  2) Not yet — return
  q) Soft Signal anyway (that still counts)' 2
  case "$REPLY" in
    1|q)
      soft_signal
      printf '  Practice rep complete. The stop still works when it is real.\n'
      ;;
    2) printf '\nOkay. The exit stays lit whether you practice or not.\n' ;;
  esac
}

# --- The Map, Not The Mirror (docs/FIRST_RITUAL.md) ---

first_ritual() {
  need_tty || return $?
  cat <<'EOF'

  === The Map, Not The Mirror ===
  For younger-you: scared of ruining a good thing, convinced
  you are too much, no map for showing up safely.

  Four steps. Any order. Freely skippable. Completing them
  proves nothing about being fixed, healed, or ready.
  This is not consent, not diagnosis, not therapy.

EOF
  while :; do
    choose 'Which step?
  1) Name it — the “too much” story
  2) Locate it — today’s weather, not a test
  3) The armor — Soft Signal practice
  4) First reassurance
  5) Done for now
  q) Soft Signal' 5
    case "$REPLY" in
      1)
        choose 'Closest story your body tells (not a diagnosis):
  1) Delayed / cold reply
  2) After I asked for something
  3) After I shared a lot
  4) After conflict
  5) After I used an exit
  6) Morning leaving / being left
  7) Quiet room and I am sure something is wrong
  8) Good moment → panic about losing it
  9) Skip — I do not have to pick
  q) Soft Signal' 9
        [[ "$REPLY" == q ]] && { soft_signal; return 0; }
        if [[ "$REPLY" == 9 ]]; then
          printf '\nSkipping is allowed. Precision can wait forever.\n'
        else
          printf '\nNamed. That is enough. We are not solving it.\n'
        fi
        pause
        [[ "$REPLY" == q ]] && { soft_signal; return 0; }
        ;;
      2)
        printf '\nWe are not solving anything. Name the sky so it does not\nbecome someone else’s job to read your mood.\n'
        choose 'Energy right now?
  1) Crispy — awake, maybe too awake
  2) Toasty — warm, okay-ish
  3) Gremlin — low fuel, high need
  4) Flooded — language is thin
  5) Skip
  q) Soft Signal' 5
        [[ "$REPLY" == q ]] && { soft_signal; return 0; }
        choose 'Capacity for other humans?
  1) Green — some room
  2) Yellow — small doses
  3) Red — not now
  4) Skip
  q) Soft Signal' 4
        [[ "$REPLY" == q ]] && { soft_signal; return 0; }
        printf '\nWeather named. That is not a test you can fail.\n'
        pause
        [[ "$REPLY" == q ]] && { soft_signal; return 0; }
        ;;
      3) soft_signal_practice ;;
      4)
        cat <<'EOF'

  You do not have to earn these:

  · Needing connection does not make you too much.
  · Fear of abandonment is information, not a prophecy.
  · Soft Signal is not proof you ruined love.
  · You can be intense and still be worthy of staying-for.
  · “Too much” is often “not enough safety yet.”

  You do not owe anyone a finished version of yourself
  before you are allowed closeness — not a partner,
  not future-you, not this command.

EOF
        pause
        [[ "$REPLY" == q ]] && { soft_signal; return 0; }
        ;;
      5) exit_kind; return 0 ;;
      q) soft_signal; return 0 ;;
    esac
  done
}

# --- Morning Cuddle (docs/MORNING_CUDDLE_PROTOCOL.md) ---

morning_cuddle() {
  need_tty || return $?
  cat <<'EOF'

  === Morning Cuddle Protocol ===
  Because nothing says “I love you” like a formalized
  negotiation before coffee.

  High-risk, high-reward. One person is a functional human.
  The other is a half-dead gremlin who wants to be held
  and also needs to pee in about 8 minutes.

  This exists so 7:42am does not become:
  “am I needy / are they annoyed / is this ruining us.”

  Prior mornings are not consent. Soft Signal is sacred.

EOF
  local energy duration style
  choose 'Energy?
  1) Crispy — awake, wants a board meeting
  2) Toasty — melt mode
  3) Gremlin — barely conscious, maximum containment
  4) Exit Protocol — someone needs to get up NOW
  q) Soft Signal' 4
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  energy="$REPLY"
  if [[ "$energy" == 4 ]]; then
    printf '\nExit Protocol. No cuddle start. That is a complete morning.\nForehead-kiss energy optional: “I really liked that” can wait.\n'
    return 0
  fi
  choose 'Duration?
  1) Micro (2–5 min)
  2) Standard (8–15 min)
  3) Extended (20+ — fall-back-asleep hazard)
  q) Soft Signal' 3
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  duration="$REPLY"
  choose 'Style?
  1) Gentle Hold
  2) Full Burrito
  3) Back Scritches (service animal mode)
  4) Big Spoon Safety
  5) Starfish Adjacent (mostly existence confirmation)
  6) Jetpack Mode
  7) Koala Cling
  q) Soft Signal' 7
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  style="$REPLY"
  cat <<'EOF'

  Default morning zones (fail-closed where it matters):
    hair          yes
    stomach       negotiable
    legs          default yes (restless-legs can flip to no)
    face-to-face  no unless an explicit yes

  Soft Signal = instant release + “no worries, love you.”
  No TED talk.

EOF
  printf '  Sealed: energy %s · duration %s · style %s\n' "$energy" "$duration" "$style"
  while :; do
    choose 'During:
  1) Check-in — “You good, gremlin?” / “Still alive in there?”
  2) Gremlin pee banner (the 8-minute prophecy)
  3) Graceful exit + “I really liked that”
  q) Soft Signal' 3
    case "$REPLY" in
      1) printf '\nStill alive in there? You good, gremlin?\nA grunt counts as a full sentence.\n' ;;
      2) printf '\nGremlin pee window. Sacred duration clause: hot/pee exits are holy.\nLeaving the burrito to pee is not abandoning the relationship.\n' ;;
      3)
        cat <<'EOF'

  Graceful disengage.
  Exit ritual: forehead kiss + “I really liked that.”
  Positive reinforcement for a brain that otherwise audits joy.

  Joke ledger (not a score, not stored):
    +1 received morning affection without spiraling
    +1 did not feel guilty about wanting closeness

EOF
        return 0
        ;;
      q) soft_signal; return 0 ;;
    esac
  done
}

# --- Spooning Protocol (docs/SPOONING_PROTOCOL.md) ---

spooning() {
  need_tty || return $?
  cat <<'EOF'

  === Spooning Protocol ===
  Because apparently some of us cannot just fucking
  cuddle like a normal person.

  Negotiate, seal, hold, exit. Soft Signal is God Mode
  the entire time. A map for “am I allowed to need hold?”

EOF
  choose 'Role?
  1) Little / care-seeker
  2) Big
  3) Switch
  4) Parallel (near, not fused)
  5) Burrito
  6) Solo-adjacent
  q) Soft Signal' 6
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  choose 'Position (nervous-system job in parentheses)?
  1) Classic (baseline co-presence)
  2) Safety Spoon (hold + escape wrist)
  3) Safety Burrito (max containment + exit)
  4) Distance Spoon (connection without flood)
  5) Starfish Adjacent (lowest-demand closeness)
  6) Fortress of Solitude (solo-adjacent co-reg)
  7) Jetpack / Koala (cling with agency)
  8) Custom — name it yourself later
  q) Soft Signal' 8
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  choose 'Pressure / energy?
  1) Feather / barely there
  2) Gentle hold
  3) Deep pressure (only with a current yes)
  4) Reassurance needed — “you are allowed to need this”
  q) Soft Signal' 4
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  cat <<'EOF'

  Sealed for this moment only. Chest-adjacent shapes still
  need a current yes for back / shoulder / chest-over-clothes.

  Mid-spoon tools:
    Soft Signal  — God Mode
    Check-in     — “you are still wanted”
    Hot/pee exit — sacred, not rude

  You are allowed to need this.
  You are allowed to stop this.

EOF
  choose 'Now?
  1) Reassurance line
  2) End with aftercare energy
  q) Soft Signal' 2
  case "$REPLY" in
    1)
      printf '\nYou are allowed to need hold.\nNeeding it does not make you a burden.\nA yes is only for this spoon, not a permanent pass.\n'
      pause
      [[ "$REPLY" == q ]] && soft_signal
      ;;
    2) printf '\nLand the plane. Water, bathroom, blanket, or quiet.\nDistance after a hold is not a verdict.\n' ;;
    q) soft_signal ;;
  esac
}

# --- I'm Not Ready To Get Up Yet (docs/IM_NOT_READY_YET.md) ---

not_ready_yet() {
  need_tty || return $?
  cat <<'EOF'

  === I'm Not Ready To Get Up Yet ===
  Because the day is a court date and the bed is a sovereign nation.

  Not ready is information, not a character flaw.
  Snooze is negotiated, not stolen.

EOF
  choose 'Why not ready?
  1) Body still wants hold / dark / silence
  2) Anxiety about the day
  3) Leaving bed feels like abandoning myself
  4) Cold
  5) I do not know and that is enough
  q) Soft Signal' 5
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  choose 'Snooze unit?
  1) 2 minutes
  2) 5 minutes
  3) 10 minutes
  4) 15 minutes
  5) Open — stay until the body says go
  q) Soft Signal' 5
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  cat <<'EOF'

  Graceful exit scripts (pick one later, out loud or not):
    “Okay. I liked that. I’m getting up now.”
    “One more minute, then I’m moving.”
    “I need to start the day. Thank you for the hold.”

  Soft Signal still works mid-snooze — the day can start
  ugly if the body says so.

  Joke ledger (not stored):
    +1 asked for more without a full spiral
    +1 no self-hate for needing hold

EOF
}

# --- I'm Too Much / Fear of Abandonment (tooMuchCore.ts) ---

too_much() {
  need_tty || return $?
  cat <<'EOF'

  === I'm Too Much / Fear of Abandonment ===
  Because “too much” is a story the body tells when it is
  afraid of being left.

  DOOR SEALED · PANIC ROOM · SOFT SIGNAL LIT
  Not a diagnosis. Not a neediness score. Nothing stored.

EOF
  choose 'What fired the detector?
  1) Delayed / cold reply
  2) After I asked for something
  3) After I shared a lot
  4) After conflict
  5) After I used Soft Signal / an exit
  6) Quiet room (they seem fine)
  7) Urge to leave first
  8) Comparison — other people seem easier to love
  q) Soft Signal' 8
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  choose 'Intensity?
  1) Whisper — story starting
  2) Activated — body online
  3) High — hard to think
  4) Flooded — minimal words, body only
  q) Soft Signal' 4
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  if [[ "$REPLY" == 4 ]]; then
    step_walk \
      "Flood track. Soft Signal is free. No essays." \
      "Blanket. Weight. Floor. Pick one." \
      "Story can wait. Body first." \
      "You are not too much for needing a room." \
      "Stay, or leave with Soft Signal. Both allowed."
    return 0
  fi
  step_walk \
    "You are in the panic room. Soft Signal does not mean you failed." \
    "Orient: five things you can see, or feel under the blanket." \
    "Name the detector: “My brain says I am too much / they will leave.”" \
    "Detector ≠ court verdict. Stories can fire without being true." \
    "You do not have to send the perfect text or become smaller in this minute." \
    "Needing connection does not make you too much." \
    "Fear of abandonment is information, not a prophecy."
}

# --- Flood Protocol (docs/FLOOD_PROTOCOL.md) ---

flood_protocol() {
  need_tty || return $?
  cat <<'EOF'

  === Flood Protocol ===
  When language is gone. Six minimum steps.
  No partner contact. No essays. Soft Signal free.

EOF
  step_walk \
    "1. STOP. You do not have to talk. You do not have to explain." \
    "2. BODY. Sit, lie down, or put a hand on something solid." \
    "3. WATER. Drink if you can. Skip if you cannot. No grade." \
    "4. DELAY. No texts, no dumps, no processing essays right now." \
    "5. SOFT SIGNAL. You can leave this protocol too. That is success." \
    "6. LATER MAP. When words return, cuddle --hub is here. Not now."
  printf '\nLanding is the win. Intensity is not a crime.\n'
}

# --- Aftercare (docs/AFTERCARE_PROTOCOL.md) ---

aftercare() {
  need_tty || return $?
  cat <<'EOF'

  === Aftercare Protocol ===
  Land the plane. Soft Signal free.
  Do not make another person the only landing gear.

EOF
  choose 'After what?
  1) Touch / closeness
  2) Conflict / hard talk
  3) Flood / too-much spiral
  4) A good thing (joy can dysregulate too)
  5) Build spiral — you poured fear into a system
  q) Soft Signal' 5
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  case "$REPLY" in
    1)
      step_walk \
        "Soft Signal free — aftercare is not a trap." \
        "Water / bathroom / blanket without apology." \
        "One sentence of what felt good, or skip." \
        "Name capacity for more talk: now · later · not today." \
        "Exit without making distance a verdict."
      ;;
    2)
      step_walk \
        "Soft Signal free. No forced postmortem." \
        "Feet on floor. Longer exhale than inhale, three times if you can." \
        "One care sentence or silence — both valid." \
        "If still flooded: pause, not more essays." \
        "Book repair later if needed. Time-box it."
      ;;
    3)
      step_walk \
        "You already survived the peak. Soft Signal free." \
        "No processing essays for a bit." \
        "Water, food, cold face, or quiet — pick one." \
        "Delay-dump: no raw dump while flooded." \
        "When quieter, the hub is still here."
      ;;
    4)
      step_walk \
        "Soft Signal free even after good news." \
        "Let the good land without immediately auditing it." \
        "Notice the urge to sabotage or pre-abandon." \
        "Rest is allowed after joy." \
        "You can receive good without earning it."
      ;;
    5)
      step_walk \
        "Soft Signal free from the cathedral too." \
        "Close the laptop on purpose." \
        "Building was containment, not failure." \
        "One non-code need: water, walk, stretch, food." \
        "Bless the work without making it the only self."
      ;;
  esac
}

# --- Parallel Play (docs/PARALLEL_PLAY_SACRED.md) ---

parallel_play() {
  need_tty || return $?
  cat <<'EOF'

  === Parallel Play (sacred) ===
  Closeness is not a touch tax.
  Connection is not productivity.
  Soft Signal free. Non-touch unless separately sealed.

EOF
  choose 'Mode?
  1) Same-room silence (silence = co-regulation)
  2) Shared media (commentary optional)
  3) Side-by-side work (no productivity scoring)
  4) Ritual tea / water (the drink is the ceremony)
  5) Reading corner (pages are privacy)
  6) Parallel walk (free to split)
  q) Soft Signal' 6
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  cat <<'EOF'

  Entry ritual: name the mode out loud or in a look.
  Sacred rule: not talking / not touching is not failing.
  Exit ritual: “I liked being near you” — or nothing. Both fine.

  If someone wants touch, that is a new ask, not implied by this mode.

EOF
}

# --- Touch Language primer (docs/TOUCH_LANGUAGE.md) ---

touch_language() {
  need_tty || return $?
  cat <<'EOF'

  === Touch Language primer ===
  How touch tends to feel okay — pressure, speed, duration, zones.
  This is NEVER consent to touch.
  A map, a match, or a shared vibe never starts a session.
  Unset zones are off limits (fail closed).
  Hard limits win if they conflict with “welcomed” wording.

  Zone letters: W welcomed · A ask first · S soft limit · X off limits

  Twelve zones people forget to name:
    hands, arms, shoulders, upper back, neck, head/scalp,
    face, torso, lower back, outer hips, legs, feet.

EOF
  choose 'Pressure that usually feels okay (for you, today)?
  1) Feather-light
  2) Comfortably gentle
  3) Steady and grounding
  4) Skip — unset stays off limits
  q) Soft Signal' 4
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  choose 'Speed?
  1) Very slow
  2) Unhurried
  3) Moderate
  4) Skip
  q) Soft Signal' 4
  [[ "$REPLY" == q ]] && { soft_signal; return 0; }
  cat <<'EOF'

  Hold types people actually mean:
    side-by-side · hand holding · shoulder squeeze ·
    upper-back rest · side hug · no holds

  Soft Signal still ends contact.
  This primer is discarded when you quit — nothing is saved.

  If you need a yes, ask. If you get a maybe, treat it as no.

EOF
}
