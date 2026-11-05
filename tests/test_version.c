#include "cuddle.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

int main(void) {
  size_t n;
  const struct cuddle_protocol *p;

  if (strcmp(cuddle_version(), CUDDLE_VERSION) != 0) {
    return 1;
  }
  if (!cuddle_datadir() || !cuddle_datadir()[0]) {
    return 1;
  }
  n = cuddle_protocol_count();
  if (n == 0) {
    return 1;
  }
  p = cuddle_protocol_at(0);
  if (!p || !p->id || !p->flag) {
    return 1;
  }
  if (!cuddle_soft_signal_text()) {
    return 1;
  }
  printf("ok %s datadir=%s cli=%d tui=%d gui=%d\n", cuddle_version(),
         cuddle_datadir(), cuddle_has_cli(), cuddle_has_tui(), cuddle_has_gui());
  return 0;
}
