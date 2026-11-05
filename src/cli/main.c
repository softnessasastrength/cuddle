#include "cuddle.h"
#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int cuddle_cli_flood(void);
int cuddle_cli_room(const char *id);
void cuddle_cli_rooms(void);

static void usage(void) {
  fputs(
      "cuddle — a small reminder that presence is a language.\n"
      "\n"
      "Usage:\n"
      "  cuddle              short invitation\n"
      "  cuddle --soft       longer note\n"
      "  cuddle --guide      consent-first guide\n"
      "  cuddle --hub        bash containment hub\n"
      "  cuddle --tui        TUI hub (if built)\n"
      "  cuddle --tui flood  TUI flood screen\n"
      "  cuddle --tui touch  Touch Language map (never consent)\n"
      "  cuddle --gui        Soft Signal Stop GUI (if built)\n"
      "  cuddle --map        first ritual\n"
      "  cuddle --stop       Soft Signal practice (CLI)\n"
      "  cuddle --morning    morning cuddle\n"
      "  cuddle --spoon      spooning protocol\n"
      "  cuddle --hold       not ready to get up yet\n"
      "  cuddle --too-much   abandonment panic room\n"
      "  cuddle --flood      flood protocol (C, no python)\n"
      "  cuddle --aftercare  land the plane\n"
      "  cuddle --parallel   closeness without a touch tax\n"
      "  cuddle --touch      touch language primer (CLI)\n"
      "  cuddle --help       this text\n"
      "  cuddle --rooms      list shared Litmo rooms\n"
      "  cuddle --room ID    open a shared room\n"
      "  cuddle --conflict   conflict practice\n"
      "  cuddle --interest   want or should?\n"
      "  cuddle --repair     soft return\n"
      "  cuddle --version    print version\n"
      "\n"
      "q is always Soft Signal. Nothing is stored or sent.\n"
      "Touch Language is a map, never consent.\n",
      stdout);
}

static int exec_bindir(const char *name, char *const argv[]) {
  char path[768];
  const char *bindir = cuddle_bindir();

  if (snprintf(path, sizeof(path), "%s/%s", bindir, name) >= (int)sizeof(path)) {
    fprintf(stderr, "cuddle: bindir path too long\n");
    return 1;
  }
  execv(path, argv);
  fprintf(stderr, "cuddle: exec %s: %s\n", path, strerror(errno));
  return 127;
}

static int exec_engine(int argc, char **argv) {
  char engine[768];
  char **fwd;
  int i;

  if (cuddle_data_file(engine, sizeof(engine), "engine.sh") != 0) {
    fprintf(stderr, "cuddle: datadir path too long\n");
    return 1;
  }
  if (access(engine, R_OK) != 0) {
    fprintf(stderr, "cuddle: missing engine at %s\n", engine);
    return 1;
  }
  setenv("CUDDLE_SHARE", cuddle_datadir(), 1);
  fwd = calloc((size_t)argc + 2, sizeof(*fwd));
  if (!fwd) {
    return 1;
  }
  fwd[0] = "bash";
  fwd[1] = engine;
  for (i = 1; i < argc; i++) {
    fwd[i + 1] = argv[i];
  }
  execvp("bash", fwd);
  fprintf(stderr, "cuddle: exec bash: %s\n", strerror(errno));
  free(fwd);
  return 127;
}

int main(int argc, char **argv) {
  struct cuddle_config cfg;

  cuddle_i18n_init();
  cuddle_config_load(&cfg);
  cuddle_config_apply(&cfg);
  if (!cuddle_theme_role("header")) {
    cuddle_theme_load("default");
  }
  if (cuddle_plugins_enabled()) {
    cuddle_plugins_scan();
  }

  if (argc < 2) {
    const char *start = cuddle_preferences_get()->start_room;
    if (strcmp(start, "hub")) return cuddle_cli_room(start);
    return exec_engine(argc, argv);
  }
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    usage();
    return 0;
  }
  if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
    printf("cuddle %s\n", cuddle_version());
    return 0;
  }
  if (strcmp(argv[1], "--rooms") == 0) { cuddle_cli_rooms(); return 0; }
  if (strcmp(argv[1], "--room") == 0) {
    if (argc != 3) { fputs("Usage: cuddle --room ID\n", stderr); return 2; }
    return cuddle_cli_room(argv[2]);
  }
  if (strcmp(argv[1], "--conflict") == 0) return cuddle_cli_room("conflict");
  if (strcmp(argv[1], "--interest") == 0) return cuddle_cli_room("interest");
  if (strcmp(argv[1], "--repair") == 0) return cuddle_cli_room("repair");
  if (strcmp(argv[1], "--flood") == 0) {
    return cuddle_cli_flood();
  }
  if (strcmp(argv[1], "--tui") == 0) {
    char *tui_argv[4];
    const char *screen = argv[2];

    if (!cuddle_has_frontend(CUDDLE_FRONTEND_TUI)) {
      fprintf(stderr, "cuddle: TUI was not built\n");
      return 1;
    }
    if (!screen || screen[0] == '-') {
      screen = cuddle_preferences_get()->start_room;
    }
    tui_argv[0] = "cuddle-tui";
    tui_argv[1] = (char *)screen;
    tui_argv[2] = NULL;
    setenv("CUDDLE_SHARE", cuddle_datadir(), 1);
    setenv("CUDDLE_BIN", argv[0], 1);
    return exec_bindir("cuddle-tui", tui_argv);
  }
  if (strcmp(argv[1], "--gui") == 0 || strcmp(argv[1], "--stop-gui") == 0) {
    char *gui_argv[2];

    if (!cuddle_has_frontend(CUDDLE_FRONTEND_GUI)) {
      fprintf(stderr, "cuddle: GUI was not built\n");
      return 1;
    }
    gui_argv[0] = "cuddle-gui";
    gui_argv[1] = NULL;
    setenv("CUDDLE_SHARE", cuddle_datadir(), 1);
    return exec_bindir("cuddle-gui", gui_argv);
  }
  return exec_engine(argc, argv);
}
