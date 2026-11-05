#include "cuddle.h"
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
  char script[768];
  struct cuddle_config cfg;
  cuddle_i18n_init();
  cuddle_config_load(&cfg);
  cuddle_config_apply(&cfg);
  const char *screen = cuddle_preferences_get()->start_room;

  if (argc > 1 && (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-V"))) {
    printf("cuddle-tui %s\n", cuddle_version());
    return 0;
  }
  if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
    puts("Usage: cuddle-tui [hub|signal|ROOM_ID]\nUse cuddle --rooms to list room IDs.");
    return 0;
  }

  if (argc > 1 && argv[1] && argv[1][0] != '-') {
    screen = argv[1];
  }
  if (argc > 2 || (strcmp(screen, "hub") && strcmp(screen, "signal") && !cuddle_room_find(screen))) {
    fprintf(stderr, "cuddle-tui: unknown room; use cuddle --rooms\n");
    return 2;
  }
  if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
    fprintf(stderr, "cuddle-tui needs an interactive terminal.\n");
    return 2;
  }
  if (cuddle_data_file(script, sizeof(script), "tui.py") != 0) {
    fprintf(stderr, "cuddle-tui: datadir path too long\n");
    return 1;
  }
  if (access(script, R_OK) != 0) {
    fprintf(stderr, "cuddle-tui: missing %s\n", script);
    return 1;
  }
  setenv("CUDDLE_SHARE", cuddle_datadir(), 1);
  execlp(CUDDLE_PYTHON, CUDDLE_PYTHON, script, screen, (char *)NULL);
  fprintf(stderr, "cuddle-tui: exec %s: %s\n", CUDDLE_PYTHON, strerror(errno));
  return 127;
}
