#include "cuddle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void cuddle_cli_rooms(void) {
  for (size_t i = 0; i < cuddle_room_count(); i++) {
    const struct cuddle_room *r = cuddle_room_at(i);
    printf("%-12s %s\n             %s\n", r->id, r->title, r->summary);
  }
}

int cuddle_cli_room(const char *id) {
  struct cuddle_session session;
  if (cuddle_session_begin(&session, id)) {
    fprintf(stderr, "cuddle: unknown room '%s'; use --rooms\n", id ? id : "");
    return 2;
  }
  if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
    fprintf(stderr, "cuddle --room needs an interactive terminal.\n");
    return 2;
  }
  char line[128];
  while (session.status == CUDDLE_SESSION_ACTIVE) {
    const struct cuddle_room *r = cuddle_session_room(&session);
    const struct cuddle_room_step *step = cuddle_session_step(&session);
    const struct cuddle_preferences *prefs = cuddle_preferences_get();
    const char *color = getenv("NO_COLOR") || !prefs->appearance ? "" : (prefs->appearance == 2 ? "\033[97;40m" : "\033[30;47m");
    printf("\n%s%s%s — %s\n", color, r->title, color[0] ? "\033[0m" : "", r->summary);
    if (prefs->large_text) putchar('\n');
    if (r->kind == CUDDLE_ROOM_TOUCH) {
      for (size_t i = 0; i < CUDDLE_TOUCH_ZONES; i++)
        printf("%2zu) %-15s %s\n", i + 1, cuddle_touch_zone(i), cuddle_touch_status(session.zones[i]));
      puts("Enter a zone number to cycle its map label. This is never permission.");
    } else {
      printf("\n%s\n%s\n", step->title, step->body);
      if ((!strcmp(r->id, "morning") || !strcmp(r->id, "spoon")) && session.step_index + 1 == r->step_count) {
        char summary[4096];
        if (!cuddle_session_summary(&session, summary, sizeof(summary))) puts(summary);
      }
      if (r->kind == CUDDLE_ROOM_INTEREST && session.step_index == 6) {
        struct cuddle_interest_reading reading = cuddle_interest_read(session.signal_bits, session.answered, session.unknown);
        printf("\n%s\n%s\n", reading.label, reading.advice);
      }
      for (size_t i = 0; i < step->choice_count; i++) printf("%zu) %s\n", i + 1, step->choices[i].label);
      if (!step->choice_count) puts("Enter next.");
    }
    fputs("q / Esc stop immediately · b back · nothing saved\n> ", stdout);
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin) || line[0] == 'q' || line[0] == 'Q' || line[0] == 27) {
      cuddle_session_stop(&session);
      break;
    }
    if (line[0] == 'b') { (void)cuddle_session_back(&session); continue; }
    char *end;
    unsigned long n = strtoul(line, &end, 10);
    if (*end != '\n' && *end != '\0') continue;
    if (r->kind == CUDDLE_ROOM_TOUCH) {
      if (n > 0 && n <= CUDDLE_TOUCH_ZONES) (void)cuddle_session_cycle_zone(&session, n - 1);
    } else if (!step->choice_count && line[0] == '\n') {
      (void)cuddle_session_choose(&session, 0);
    } else if (n > 0 && n <= step->choice_count) {
      (void)cuddle_session_choose(&session, n - 1);
    }
  }
  if (session.status == CUDDLE_SESSION_STOPPED) fputs(cuddle_soft_signal_text(), stdout);
  else puts("You can leave here. Nothing is saved. Completing a room proves nothing.");
  return 0;
}
