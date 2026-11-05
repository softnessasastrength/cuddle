#include "cuddle.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
  struct cuddle_config cfg;
  const char *old_home = getenv("HOME");
  const char *old_xdg = getenv("XDG_CONFIG_HOME");

  setenv("HOME", "/tmp/cuddle-no-such-home", 1);
  unsetenv("XDG_CONFIG_HOME");
  if (cuddle_config_load(&cfg) != 0) {
    return 1;
  }
  if (cfg.datadir_override[0] || cfg.browser[0] || cfg.tui_screen[0]) {
    fprintf(stderr, "missing config should be defaults\n");
    return 1;
  }
  if (old_home) {
    setenv("HOME", old_home, 1);
  }
  if (old_xdg) {
    setenv("XDG_CONFIG_HOME", old_xdg, 1);
  }
  puts("ok config");
  return 0;
}
