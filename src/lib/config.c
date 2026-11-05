#include "cuddle.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct cuddle_preferences g_preferences = {"default", "hub", 0, 0};
const struct cuddle_preferences *cuddle_preferences_get(void) { return &g_preferences; }

static void trim(char *s) {
  char *e;
  while (*s == ' ' || *s == '\t') {
    memmove(s, s + 1, strlen(s));
  }
  e = s + strlen(s);
  while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' || e[-1] == '\r')) {
    *--e = '\0';
  }
}

int cuddle_config_path(char *buf, size_t n) {
  const char *xdg = getenv("XDG_CONFIG_HOME");
  const char *home = getenv("HOME");

  if (xdg && xdg[0]) {
    return snprintf(buf, n, "%s/cuddle/cuddle.conf", xdg) >= (int)n ? -1 : 0;
  }
  if (home && home[0]) {
    return snprintf(buf, n, "%s/.config/cuddle/cuddle.conf", home) >= (int)n ? -1 : 0;
  }
  return -1;
}

static void set_field(char *dst, size_t n, const char *src) {
  size_t len = strlen(src);
  if (len >= n) {
    len = n - 1;
  }
  memcpy(dst, src, len);
  dst[len] = '\0';
}

int cuddle_config_load(struct cuddle_config *out) {
  char path[768];
  char line[1024];
  FILE *fp;

  if (!out) {
    return -1;
  }
  memset(out, 0, sizeof(*out));

#ifndef CUDDLE_HAS_USER_CONFIG
  return 0;
#else
  if (cuddle_config_path(path, sizeof(path)) != 0) {
    return 0;
  }
  fp = fopen(path, "r");
  if (!fp) {
    return 0; /* missing file = defaults */
  }
  while (fgets(line, sizeof(line), fp)) {
    char *eq;
    char *key;
    char *val;
    trim(line);
    if (!line[0] || line[0] == '#') {
      continue;
    }
    eq = strchr(line, '=');
    if (!eq) {
      continue;
    }
    *eq = '\0';
    key = line;
    val = eq + 1;
    trim(key);
    trim(val);
    if (strcmp(key, "datadir_override") == 0) {
      set_field(out->datadir_override, sizeof(out->datadir_override), val);
    } else if (strcmp(key, "browser") == 0) {
      set_field(out->browser, sizeof(out->browser), val);
    } else if (strcmp(key, "tui_screen") == 0) {
      set_field(out->tui_screen, sizeof(out->tui_screen), val);
    } else if (strcmp(key, "theme") == 0) {
      set_field(out->theme, sizeof(out->theme), val);
    } else if (strcmp(key, "start_room") == 0) {
      if (!strcmp(val, "hub") || cuddle_room_find(val)) set_field(out->start_room, sizeof(out->start_room), val);
    } else if (strcmp(key, "large_text") == 0) {
      out->large_text = !strcmp(val, "true") || !strcmp(val, "yes") || !strcmp(val, "1");
    } else if (strcmp(key, "appearance") == 0) {
      out->appearance = !strcmp(val, "dark") ? 2 : (!strcmp(val, "light") ? 1 : 0);
    }
  }
  fclose(fp);
  return 0;
#endif
}

void cuddle_config_apply(const struct cuddle_config *cfg) {
  if (!cfg) {
    return;
  }
  if (cfg->datadir_override[0]) {
    setenv("CUDDLE_SHARE", cfg->datadir_override, 1);
  }
  if (cfg->browser[0]) {
    setenv("BROWSER", cfg->browser, 1);
  }
  if (cfg->tui_screen[0]) {
    setenv("CUDDLE_TUI_SCREEN", cfg->tui_screen, 1);
  }
  memset(&g_preferences, 0, sizeof(g_preferences));
  set_field(g_preferences.theme, sizeof(g_preferences.theme), cfg->theme[0] ? cfg->theme : "default");
  const char *start = cfg->start_room[0] ? cfg->start_room : (cfg->tui_screen[0] ? cfg->tui_screen : "hub");
  set_field(g_preferences.start_room, sizeof(g_preferences.start_room),
            !strcmp(start, "hub") || cuddle_room_find(start) ? start : "hub");
  g_preferences.large_text = cfg->large_text != 0;
  g_preferences.appearance = cfg->appearance >= 0 && cfg->appearance <= 2 ? cfg->appearance : 0;
  (void)cuddle_theme_load(g_preferences.theme);
}
