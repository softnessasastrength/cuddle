#include "cuddle.h"

#include <stdio.h>
#include <string.h>

static int check_role(const char *name) {
  const struct cuddle_theme_role *r = cuddle_theme_role(name);
  if (!r || !r->letter || !r->word) {
    fprintf(stderr, "missing role %s\n", name);
    return 0;
  }
  if (!r->letter[0] || !r->word[0]) {
    fprintf(stderr, "role %s needs letter and word\n", name);
    return 0;
  }
  return 1;
}

int main(void) {
  if (cuddle_theme_load("default") != 0) {
    fprintf(stderr, "theme load failed (CUDDLE_SHARE=%s)\n", cuddle_datadir());
    return 1;
  }
  if (!check_role("header") || !check_role("signal") || !check_role("flood") ||
      !check_role("touch_w") || !check_role("touch_a") || !check_role("touch_s") ||
      !check_role("touch_x")) {
    return 1;
  }
  puts("ok theme");
  return 0;
}
