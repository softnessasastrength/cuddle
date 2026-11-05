#include "cuddle.h"
#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int roomy;
static void text(WINDOW *pad, int *row, int width, const char *s) {
  const char *p = s;
  while (*p && *row < 240) {
    const char *end = strchr(p, '\n');
    size_t n = end ? (size_t)(end - p) : strlen(p);
    if (n > (size_t)width) {
      n = (size_t)width;
      while (n > 0 && p[n] != ' ') n--;
      if (!n) n = (size_t)width;
    }
    mvwaddnstr(pad, (*row)++, 0, p, (int)n);
    p += n;
    if (*p == '\n' || *p == ' ') p++;
  }
  (*row) += roomy ? 2 : 1;
}

int main(int argc, char **argv) {
  if (argc > 1 && !strcmp(argv[1], "--version")) { printf("cuddle-tui %s\n", cuddle_version()); return 0; }
  if (argc > 1 && !strcmp(argv[1], "--help")) { puts("Usage: cuddle-tui [hub|signal|ROOM_ID]\nRooms: flood touch conflict interest morning spoon too-much parallel aftercare repair"); return 0; }
  setlocale(LC_ALL, "");
  struct cuddle_config cfg;
  cuddle_config_load(&cfg); cuddle_config_apply(&cfg);
  const char *start = argc > 1 ? argv[1] : (cuddle_preferences_get()->start_room);
  int hub = !strcmp(start, "hub"), selected = 0, choice = 0, scroll = 0;
  struct cuddle_session session;
  cuddle_session_stop(&session);
  if (argc > 2 || (!hub && strcmp(start, "signal") && cuddle_session_begin(&session, start))) {
    fputs("cuddle-tui: unknown room; use cuddle --rooms\n", stderr); return 2;
  }
  if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) { fputs("cuddle-tui needs an interactive terminal.\n", stderr); return 2; }
  if (!initscr()) return 1;
  cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
  const struct cuddle_preferences *preferences = cuddle_preferences_get();
  roomy = preferences->large_text;
  if (has_colors() && preferences->appearance) {
    start_color();
    init_pair(1, preferences->appearance == 2 ? COLOR_WHITE : COLOR_BLACK,
                 preferences->appearance == 2 ? COLOR_BLACK : COLOR_WHITE);
    bkgd(COLOR_PAIR(1));
  }
  int running = 1;
  while (running) {
    erase();
    if (LINES < 10 || COLS < 32) {
      mvaddnstr(0, 0, "Resize terminal. q stops.", COLS - 1); refresh();
      int key = getch();
      if (key == 'q' || key == 'Q' || key == 27) { cuddle_session_stop(&session); running = 0; }
      continue;
    }
    attron(A_REVERSE); mvaddnstr(0, 0, " Cuddle · local only · nothing saved ", COLS - 1); attroff(A_REVERSE);
    int width = COLS - 4, row = 0, focus = -1, count = 0;
    WINDOW *pad = newpad(256, COLS);
    if (!pad) break;
    if (has_colors() && preferences->appearance) wbkgd(pad, COLOR_PAIR(1));
    if (hub) {
      text(pad, &row, width, "What would help right now? Touch Language is never consent.");
      count = (int)cuddle_room_count();
      for (int i = 0; i < count; i++) {
        const struct cuddle_room *r = cuddle_room_at((size_t)i);
        if (i == selected) { wattron(pad, A_REVERSE); focus = row; }
        mvwprintw(pad, row++, 0, "%s", r->title);
        wattroff(pad, A_REVERSE);
      }
      row++; text(pad, &row, width, cuddle_room_at((size_t)selected)->summary);
    } else if (session.status != CUDDLE_SESSION_ACTIVE) {
      text(pad, &row, width, session.status == CUDDLE_SESSION_FINISHED ?
        "You can land here. Completing a room proves nothing. Nothing is saved." : cuddle_soft_signal_text());
      text(pad, &row, width, "Enter returns to rooms. q / Esc leaves.");
    } else {
      const struct cuddle_room *r = cuddle_session_room(&session);
      text(pad, &row, width, r->title);
      if (r->kind == CUDDLE_ROOM_TOUCH) {
        text(pad, &row, width, "A map, never consent. Every new visit starts off limits.");
        count = CUDDLE_TOUCH_ZONES;
        for (int i = 0; i < count; i++) {
          if (i == choice) { wattron(pad, A_REVERSE); focus = row; }
          mvwprintw(pad, row++, 0, "%s: %s", cuddle_touch_zone((size_t)i), cuddle_touch_status(session.zones[i]));
          wattroff(pad, A_REVERSE);
        }
      } else {
        const struct cuddle_room_step *s = cuddle_session_step(&session);
        text(pad, &row, width, s->title); text(pad, &row, width, s->body);
        if ((!strcmp(r->id, "morning") || !strcmp(r->id, "spoon")) && session.step_index + 1 == r->step_count) {
          char summary[4096];
          if (!cuddle_session_summary(&session, summary, sizeof(summary))) text(pad, &row, width, summary);
        }
        if (r->kind == CUDDLE_ROOM_INTEREST && session.step_index == 6) {
          struct cuddle_interest_reading read = cuddle_interest_read(session.signal_bits, session.answered, session.unknown);
          text(pad, &row, width, read.label); text(pad, &row, width, read.advice);
        }
        count = (int)(s->choice_count ? s->choice_count : 1);
        for (int i = 0; i < count; i++) {
          if (i == choice) wattron(pad, A_REVERSE);
          mvwprintw(pad, row++, 0, "%d) %s", i + 1, s->choice_count ? s->choices[i].label : "Next / skip");
          wattroff(pad, A_REVERSE);
        }
      }
    }
    int viewport = LINES - 4;
    if (focus >= 0 && (focus < scroll || focus >= scroll + viewport)) scroll = focus > viewport - 1 ? focus - viewport + 1 : 0;
    if (scroll > row - viewport) scroll = row > viewport ? row - viewport : 0;
    if (scroll < 0) scroll = 0;
    mvaddnstr(LINES - 1, 0, "up/down · enter choose · b back · PgDn scroll · q STOP", COLS - 1);
    wnoutrefresh(stdscr);
    pnoutrefresh(pad, scroll, 0, 2, 2, LINES - 3, COLS - 2); doupdate(); delwin(pad);
    int key = getch();
    if (key == 'q' || key == 'Q' || key == 27) {
      if (!hub && session.status != CUDDLE_SESSION_ACTIVE) running = 0;
      else { cuddle_session_stop(&session); hub = 0; }
      choice = 0; scroll = 0; continue;
    }
    if (key == KEY_NPAGE) { scroll += viewport / 2; continue; }
    if (key == KEY_PPAGE) { scroll -= viewport / 2; continue; }
    if (!hub && session.status != CUDDLE_SESSION_ACTIVE) {
      if (key == '\n' || key == KEY_ENTER || key == ' ') { hub = 1; scroll = 0; }
      continue;
    }
    int *cursor = hub ? &selected : &choice;
    if (key == KEY_UP || key == 'k') *cursor = (*cursor + count - 1) % count;
    else if (key == KEY_DOWN || key == 'j') *cursor = (*cursor + 1) % count;
    else if (key == 'b' && !hub) { (void)cuddle_session_back(&session); choice = 0; scroll = 0; }
    else if (key == '\n' || key == KEY_ENTER || key == ' ' || (!hub && key >= '1' && key <= '6')) {
      if (hub) { (void)cuddle_session_begin(&session, cuddle_room_at((size_t)selected)->id); hub = 0; }
      else if (cuddle_session_room(&session)->kind == CUDDLE_ROOM_TOUCH) (void)cuddle_session_cycle_zone(&session, (size_t)choice);
      else (void)cuddle_session_choose(&session, key >= '1' && key <= '6' ? (size_t)(key - '1') : (size_t)choice);
      choice = 0; scroll = 0;
    }
  }
  cuddle_session_stop(&session); endwin(); return 0;
}
