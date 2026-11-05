#include "cuddle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
  struct cuddle_config cfg;
  cuddle_i18n_init();
  cuddle_config_load(&cfg);
  cuddle_config_apply(&cfg);
  if (argc > 1 && (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-V"))) {
    printf("cuddle-gui %s\n", cuddle_version());
    return 0;
  }
  if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) {
    puts("Usage: cuddle-gui");
    return 0;
  }
  if (argc > 1) {
    fprintf(stderr, "cuddle-gui: unexpected argument\n");
    return 2;
  }
  char page[768];
  const char *browser;

  if (cuddle_data_file(page, sizeof(page), "gui/stop.html") != 0) {
    fprintf(stderr, "cuddle-gui: datadir path too long\n");
    return 1;
  }
  if (access(page, R_OK) != 0) {
    fprintf(stderr, "cuddle-gui: missing %s\n", page);
    return 1;
  }
  browser = getenv("BROWSER");
  if (browser && browser[0]) {
    execlp(browser, browser, page, (char *)NULL);
  }
  execlp("xdg-open", "xdg-open", page, (char *)NULL);
  printf("Open this file in a browser (local, no network):\n  %s\n", page);
  return 0;
}
