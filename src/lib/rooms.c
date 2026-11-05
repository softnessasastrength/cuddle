/* Litmo room adaptation, MIT. See docs/LITMO-PORT.md for provenance. */
#include "cuddle.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "rooms-data.inc"

static const char *k_zones[] = {"head / scalp", "face", "neck", "shoulders", "arms", "hands",
  "upper back", "torso", "lower back", "outer hips", "legs", "feet"};
static const char *k_status[] = {"X off limits", "A ask first", "W welcomed", "S soft limit"};

size_t cuddle_room_count(void) { return sizeof(k_rooms) / sizeof(k_rooms[0]); }
const struct cuddle_room *cuddle_room_at(size_t i) {
  return i < cuddle_room_count() ? &k_rooms[i] : NULL;
}
const struct cuddle_room *cuddle_room_find(const char *id) {
  if (!id) return NULL;
  for (size_t i = 0; i < cuddle_room_count(); i++)
    if (strcmp(id, k_rooms[i].id) == 0) return &k_rooms[i];
  return NULL;
}
void cuddle_session_stop(struct cuddle_session *s) {
  if (!s) return;
  memset(s, 0, sizeof(*s));
  s->room_index = SIZE_MAX;
  s->status = CUDDLE_SESSION_STOPPED;
}
int cuddle_session_begin(struct cuddle_session *s, const char *id) {
  if (!s) return -1;
  cuddle_session_stop(s);
  const struct cuddle_room *r = cuddle_room_find(id);
  if (!r) return -1;
  for (size_t i = 0; i < CUDDLE_ROOM_HISTORY; i++) s->selections[i] = SIZE_MAX;
  s->room_index = (size_t)(r - k_rooms);
  s->status = CUDDLE_SESSION_ACTIVE;
  return 0;
}
const struct cuddle_room *cuddle_session_room(const struct cuddle_session *s) {
  if (!s || s->status != CUDDLE_SESSION_ACTIVE) return NULL;
  return cuddle_room_at(s->room_index);
}
const struct cuddle_room_step *cuddle_session_step(const struct cuddle_session *s) {
  const struct cuddle_room *r = cuddle_session_room(s);
  return r && s->step_index < r->step_count ? &r->steps[s->step_index] : NULL;
}
int cuddle_session_choose(struct cuddle_session *s, size_t choice) {
  const struct cuddle_room *r = cuddle_session_room(s);
  const struct cuddle_room_step *step = cuddle_session_step(s);
  if (!r || !step) return -1;
  if ((step->choice_count && choice >= step->choice_count) || (!step->choice_count && choice)) return -1;
  int target = (int)s->step_index + 1;
  const struct cuddle_room_choice *c = step->choice_count ? &step->choices[choice] : NULL;
  if (c) target = c->target;
  if (target == -2) { cuddle_session_stop(s); return 0; }
  if (target == -1 || (size_t)target >= r->step_count) {
    /* Clearing state also prevents back from reviving a finished session. */
    cuddle_session_stop(s);
    s->status = CUDDLE_SESSION_FINISHED;
    return 0;
  }
  if (c && c->navigation) {
    for (size_t i = 0; i < s->history_count; i++) {
      if (s->history[i] == (size_t)target) {
        while (s->history_count > i) (void)cuddle_session_back(s);
        return 0;
      }
    }
  }
  if (s->history_count >= CUDDLE_ROOM_HISTORY) return -1;
  if (c && step->record && !c->navigation) s->selections[s->step_index] = choice;
  if (c && c->signal) {
    s->answered |= c->signal;
    s->signal_bits &= ~c->signal;
    s->unknown &= ~c->signal;
    if (c->value == 1) s->signal_bits |= c->signal;
    if (c->value == 2) s->unknown |= c->signal;
  }
  s->history[s->history_count++] = s->step_index;
  s->step_index = (size_t)target;
  return 0;
}
int cuddle_session_back(struct cuddle_session *s) {
  const struct cuddle_room *r = cuddle_session_room(s);
  if (!r || !s->history_count) return -1;
  s->selections[s->step_index] = SIZE_MAX;
  s->step_index = s->history[--s->history_count];
  s->selections[s->step_index] = SIZE_MAX;
  if (r->kind == CUDDLE_ROOM_INTEREST && s->step_index < 6) {
    unsigned keep = (1u << s->step_index) - 1u;
    s->signal_bits &= keep; s->answered &= keep; s->unknown &= keep;
  }
  return 0;
}
int cuddle_session_summary(const struct cuddle_session *s, char *buffer, size_t size) {
  if (!buffer || !size) return -1;
  buffer[0] = '\0';
  const struct cuddle_room *r = cuddle_session_room(s);
  if (!r) return -1;
  size_t used = 0;
  for (size_t i = 0; i < r->step_count; i++) {
    const struct cuddle_room_step *step = &r->steps[i];
    size_t choice = s->selections[i];
    if (!step->record || choice >= step->choice_count || step->choices[choice].navigation) continue;
    int written = snprintf(buffer + used, size - used, "%s: %s\n", step->title, step->choices[choice].label);
    if (written < 0 || (size_t)written >= size - used) { buffer[0] = '\0'; return -1; }
    used += (size_t)written;
  }
  return 0;
}
int cuddle_session_cycle_zone(struct cuddle_session *s, size_t zone) {
  const struct cuddle_room *r = cuddle_session_room(s);
  if (!r || r->kind != CUDDLE_ROOM_TOUCH || zone >= CUDDLE_TOUCH_ZONES) return -1;
  s->zones[zone] = (s->zones[zone] + 1) % 4;
  return 0;
}
const char *cuddle_touch_zone(size_t zone) { return zone < CUDDLE_TOUCH_ZONES ? k_zones[zone] : NULL; }
const char *cuddle_touch_status(unsigned status) { return status < 4 ? k_status[status] : k_status[0]; }
struct cuddle_interest_reading cuddle_interest_read(unsigned bits, unsigned answered, unsigned unknown) {
  if ((answered & 63) != 63 || unknown || (bits & 16))
    return (struct cuddle_interest_reading){"Unknown / knowing can wait", "Pause or Soft Signal. Uncertainty is not permission. Interest is not consent."};
  unsigned pull = bits & 3, pressure = bits & (4 | 8 | 32);
  if ((!pull && pressure) || (pull && pressure && (bits & 32) && !(bits & 1)))
    return (struct cuddle_interest_reading){"Performing interest suspected", "Should or fear may be doing the talking. Slow down. A soft no or pause is available. Interest is not consent."};
  if (!pull)
    return (struct cuddle_interest_reading){"No clear want signal", "No or not now is information, not cruelty. Interest is not consent."};
  if (!pressure)
    return (struct cuddle_interest_reading){"Want signals present", "Body or mind pull without the marked pressure signals. Still not consent: a specific ask and another person's freely given agreement are separate."};
  return (struct cuddle_interest_reading){"Mixed parts", "Want and pressure are both present. Decide later if you want. Ambiguity is allowed. Interest is not consent."};
}
