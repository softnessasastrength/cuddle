#include "cuddle.h"
#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
#ifndef CUDDLE_HAS_USER_CONFIG
  return 77;
#else
  char temp[] = "/tmp/cuddle-prefs-XXXXXX";
  assert(mkdtemp(temp));
  assert(!setenv("XDG_CONFIG_HOME", temp, 1));
  char dir[256], path[300];
  snprintf(dir, sizeof(dir), "%s/cuddle", temp);
  assert(!mkdir(dir, 0700));
  snprintf(path, sizeof(path), "%s/cuddle.conf", dir);
  FILE *file = fopen(path, "w"); assert(file);
  fputs("appearance=dark\nlarge_text=true\nstart_room=morning\ntheme=dim\n", file);
  assert(!fclose(file));
  struct cuddle_config cfg;
  assert(!cuddle_config_load(&cfg)); cuddle_config_apply(&cfg);
  const struct cuddle_preferences *prefs = cuddle_preferences_get();
  assert(prefs->appearance == 2 && prefs->large_text == 1);
  assert(!strcmp(prefs->start_room, "morning") && !strcmp(prefs->theme, "dim"));
  file = fopen(path, "w"); assert(file);
  fputs("appearance=not-a-theme\nlarge_text=maybe\nstart_room=not-a-room\n", file);
  assert(!fclose(file));
  assert(!cuddle_config_load(&cfg)); cuddle_config_apply(&cfg);
  assert(prefs->appearance == 0 && !prefs->large_text && !strcmp(prefs->start_room, "hub"));
  assert(!unlink(path));
  assert(!cuddle_config_load(&cfg)); cuddle_config_apply(&cfg);
  assert(prefs->appearance == 0 && !prefs->large_text && !strcmp(prefs->theme, "default"));
  assert(access(path, F_OK) != 0); /* Library never created a config. */
  assert(!rmdir(dir)); assert(!rmdir(temp));
  puts("ok shared preferences: valid, invalid, missing, no implicit writes");
  return 0;
#endif
}
