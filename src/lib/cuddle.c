#include "cuddle.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char k_soft_signal[] =
    "Ended immediately.\n"
    "No explanation. No penalty. Not a failure.\n"
    "Stopping safely is success.\n";

static const struct cuddle_protocol k_protocols[] = {
    {"guide", "Consent-first guide", "--guide"},
    {"hub", "Containment hub", "--hub"},
    {"tui", "TUI hub", "--tui"},
    {"gui", "Soft Signal GUI", "--gui"},
    {"map", "First ritual", "--map"},
    {"stop", "Soft Signal practice", "--stop"},
    {"morning", "Morning Cuddle", "--morning"},
    {"spoon", "Spooning Protocol", "--spoon"},
    {"hold", "Not ready to get up", "--hold"},
    {"too-much", "I'm too much", "--too-much"},
    {"flood", "Flood Protocol", "--flood"},
    {"aftercare", "Aftercare", "--aftercare"},
    {"parallel", "Parallel play", "--parallel"},
    {"touch", "Touch Language", "--touch"},
};

const char *cuddle_version(void) {
  return CUDDLE_VERSION;
}

const char *cuddle_prefix(void) {
  const char *e = getenv("CUDDLE_PREFIX");
  return (e && e[0]) ? e : CUDDLE_PREFIX;
}

const char *cuddle_bindir(void) {
  const char *e = getenv("CUDDLE_BINDIR");
  return (e && e[0]) ? e : CUDDLE_BINDIR;
}

const char *cuddle_datadir(void) {
  const char *e = getenv("CUDDLE_SHARE");
  return (e && e[0]) ? e : CUDDLE_DATADIR;
}

int cuddle_has_cli(void) {
#ifdef CUDDLE_HAS_CLI
  return 1;
#else
  return 0;
#endif
}

int cuddle_has_tui(void) {
#ifdef CUDDLE_HAS_TUI
  return 1;
#else
  return 0;
#endif
}

int cuddle_has_gui(void) {
#ifdef CUDDLE_HAS_GUI
  return 1;
#else
  return 0;
#endif
}

int cuddle_has_frontend(enum cuddle_frontend frontend) {
  switch (frontend) {
  case CUDDLE_FRONTEND_CLI:
    return cuddle_has_cli();
  case CUDDLE_FRONTEND_TUI:
    return cuddle_has_tui();
  case CUDDLE_FRONTEND_GUI:
    return cuddle_has_gui();
  default:
    return 0;
  }
}

const char *cuddle_soft_signal_text(void) {
  return k_soft_signal;
}

int cuddle_data_file(char *buf, size_t n, const char *relative) {
  int w;

  if (!buf || n == 0 || !relative) {
    return -1;
  }
  w = snprintf(buf, n, "%s/%s", cuddle_datadir(), relative);
  if (w < 0 || (size_t)w >= n) {
    return -1;
  }
  return 0;
}

size_t cuddle_protocol_count(void) {
  return sizeof(k_protocols) / sizeof(k_protocols[0]);
}

const struct cuddle_protocol *cuddle_protocol_at(size_t index) {
  if (index >= cuddle_protocol_count()) {
    return NULL;
  }
  return &k_protocols[index];
}
