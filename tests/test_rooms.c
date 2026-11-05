#include "cuddle.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  struct cuddle_session s;
  assert(cuddle_room_count() == 10);
  assert(!cuddle_room_find(NULL));
  assert(!cuddle_room_at(cuddle_room_count()));
  assert(cuddle_session_begin(&s, "not-a-room") == -1);
  assert(s.status == CUDDLE_SESSION_STOPPED);
  assert(cuddle_session_choose(&s, 0) == -1);
  assert(cuddle_session_back(&s) == -1);
  assert(!cuddle_session_begin(&s, "touch"));
  for (size_t i = 0; i < CUDDLE_TOUCH_ZONES; i++) assert(s.zones[i] == 0);
  for (unsigned i = 1; i <= 4; i++) {
    assert(!cuddle_session_cycle_zone(&s, 0)); assert(s.zones[0] == i % 4);
  }
  assert(cuddle_session_cycle_zone(&s, CUDDLE_TOUCH_ZONES) == -1);
  cuddle_session_cycle_zone(&s, 0);
  cuddle_session_stop(&s);
  assert(s.zones[0] == 0 && !s.signal_bits && !s.answered && !s.history_count);
  assert(!cuddle_session_room(&s));
  assert(cuddle_session_cycle_zone(&s, 0) == -1);
  assert(!cuddle_session_begin(&s, "interest"));
  struct cuddle_session unchanged = s;
  assert(cuddle_session_choose(&s, 99) == -1);
  assert(!memcmp(&s, &unchanged, sizeof(s)));
  assert(!cuddle_session_choose(&s, 0)); /* body yes */
  assert(s.signal_bits == 1 && s.answered == 1);
  assert(!cuddle_session_choose(&s, 2)); /* mind unknown */
  assert(s.unknown == 2);
  assert(!cuddle_session_back(&s));
  assert(s.answered == 1 && !s.unknown);
  assert(!cuddle_session_back(&s));
  assert(!s.answered && !s.signal_bits);
  for (int i = 0; i < 6; i++) assert(!cuddle_session_choose(&s, 1)); /* all explicitly no */
  assert(s.step_index == 6 && s.answered == 63);
  assert(!strcmp(cuddle_interest_read(s.signal_bits, s.answered, s.unknown).label, "No clear want signal"));
  assert(!strcmp(cuddle_interest_read(3, 63, 0).label, "Want signals present"));
  assert(!strcmp(cuddle_interest_read(0, 0, 0).label, "Unknown / knowing can wait"));
  assert(!strcmp(cuddle_interest_read(3, 63, 1).label, "Unknown / knowing can wait"));
  assert(!strcmp(cuddle_interest_read(16, 63, 0).label, "Unknown / knowing can wait"));
  assert(!strcmp(cuddle_interest_read(4, 63, 0).label, "Performing interest suspected"));
  assert(!strcmp(cuddle_interest_read(5, 63, 0).label, "Mixed parts"));
  assert(!cuddle_session_choose(&s, 3)); /* Soft Signal */
  assert(s.status == CUDDLE_SESSION_STOPPED && !s.answered);
  assert(cuddle_session_back(&s) == -1);
  assert(!cuddle_session_begin(&s, "conflict"));
  assert(!cuddle_session_choose(&s, 1)); assert(s.step_index == 4); /* repair branch */
  assert(!cuddle_session_choose(&s, 0)); assert(s.step_index == 3);
  assert(!cuddle_session_back(&s)); assert(s.step_index == 4); /* actual history, not index-1 */
  assert(!cuddle_session_choose(&s, 1)); assert(s.status == CUDDLE_SESSION_STOPPED);
  assert(!cuddle_session_begin(&s, "morning"));
  assert(!cuddle_session_choose(&s, 3)); assert(s.status == CUDDLE_SESSION_STOPPED); /* immediate exit */
  /* All 3^6 yes/no/unknown paths must reach a conservative reading. */
  for (unsigned path = 0; path < 729; path++) {
    assert(!cuddle_session_begin(&s, "interest"));
    unsigned remaining = path;
    for (unsigned question = 0; question < 6; question++) {
      assert(!cuddle_session_choose(&s, remaining % 3));
      remaining /= 3;
    }
    assert(s.step_index == 6 && s.answered == 63);
    struct cuddle_interest_reading read = cuddle_interest_read(s.signal_bits, s.answered, s.unknown);
    if (s.unknown || (s.signal_bits & 16)) assert(!strcmp(read.label, "Unknown / knowing can wait"));
    assert(read.label && read.advice);
    cuddle_session_stop(&s);
    assert(!s.signal_bits && !s.answered && !s.unknown);
  }
  /* Rich negotiation choices are an ephemeral discussion map. */
  assert(!cuddle_session_begin(&s, "spoon"));
  for (unsigned i = 0; i < 100; i++) {
    assert(!cuddle_session_choose(&s, 5)); /* More roles */
    assert(!cuddle_session_choose(&s, 2)); /* Back to roles */
    assert(s.step_index == 0 && s.history_count == 0);
  }
  assert(!cuddle_session_choose(&s, 5)); /* more roles */
  assert(!cuddle_session_choose(&s, 1)); /* Solo Practice -> positions */
  assert(!cuddle_session_choose(&s, 5)); /* more positions */
  assert(!cuddle_session_choose(&s, 5)); /* final positions page */
  assert(!cuddle_session_choose(&s, 0)); /* Lap Nest -> pressure */
  assert(!cuddle_session_choose(&s, 1)); /* Gentle */
  assert(!cuddle_session_choose(&s, 0)); /* Cozy silence */
  assert(!cuddle_session_choose(&s, 4)); /* Until hot/bathroom/done */
  assert(!cuddle_session_choose(&s, 0)); /* All touch off limits */
  assert(!cuddle_session_choose(&s, 1)); /* Need space */
  assert(!cuddle_session_choose(&s, 1)); /* Skip reassurance */
  assert(s.step_index == 11);
  char summary[4096];
  assert(!cuddle_session_summary(&s, summary, sizeof(summary)));
  assert(strstr(summary, "Solo Practice") && strstr(summary, "Lap Nest") && strstr(summary, "Gentle"));
  assert(!strstr(summary, "More positions") && !strstr(summary, "More roles"));
  char tiny[2] = "x";
  assert(cuddle_session_summary(&s, tiny, sizeof(tiny)) == -1 && !tiny[0]);
  assert(!cuddle_session_back(&s)); /* clear reassurance choice */
  assert(!cuddle_session_summary(&s, summary, sizeof(summary)) && !strstr(summary, "Skip reassurance"));
  cuddle_session_stop(&s);
  assert(cuddle_session_summary(&s, summary, sizeof(summary)) == -1 && !summary[0]);
  /* Every step choice is valid; stop at every step is irreversible. */
  for (size_t i = 0; i < cuddle_room_count(); i++) {
    const struct cuddle_room *r = cuddle_room_at(i);
    for (size_t j = 0; j < r->step_count; j++) {
      const struct cuddle_room_step *step = &r->steps[j];
      for (size_t k = 0; k < (step->choice_count ? step->choice_count : 1); k++) {
        assert(!cuddle_session_begin(&s, r->id)); s.step_index = j;
        assert(!cuddle_session_choose(&s, k));
      }
      assert(!cuddle_session_begin(&s, r->id)); s.step_index = j;
      cuddle_session_stop(&s);
      assert(cuddle_session_choose(&s, 0) == -1 && cuddle_session_back(&s) == -1);
    }
  }
  assert(!cuddle_session_begin(&s, "flood"));
  for (size_t i = 0; i < cuddle_room_find("flood")->step_count; i++) assert(!cuddle_session_choose(&s, 0));
  assert(s.status == CUDDLE_SESSION_FINISHED);
  assert(cuddle_session_back(&s) == -1 && !s.history_count);
  puts("ok shared rooms: choices, history, fail-closed defaults, unknown signals, stop and reset");
  return 0;
}
