#include "cuddle.h"
#include "config.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *read_file(const char *path) {
  FILE *file = fopen(path, "r"); assert(file);
  assert(!fseek(file, 0, SEEK_END)); long size = ftell(file); assert(size >= 0);
  rewind(file); char *text = calloc((size_t)size + 1, 1); assert(text);
  assert(fread(text, 1, (size_t)size, file) == (size_t)size); assert(!fclose(file));
  return text;
}
int main(void) {
#ifndef CUDDLE_HAS_USER_CONFIG
  return 77;
#else
  char temp[] = "/tmp/cuddle-save-XXXXXX"; assert(mkdtemp(temp));
  char xdg[256], folder[300], path[320], moved[340], error[512];
  snprintf(xdg, sizeof(xdg), "%s/new-config", temp);
  snprintf(folder, sizeof(folder), "%s/cuddle", xdg);
  snprintf(path, sizeof(path), "%s/cuddle.conf", folder);
  snprintf(moved, sizeof(moved), "%s/original.conf", folder);
  assert(!setenv("XDG_CONFIG_HOME", xdg, 1));
  struct cuddle_preferences prefs = {"dim", "morning", 1, 2};
  assert(!cuddle_preferences_save(&prefs, error, sizeof(error)) && !error[0]);
  struct stat mode; assert(!stat(path, &mode)); assert((mode.st_mode & 0777) == 0600);
  struct cuddle_config cfg; assert(!cuddle_config_load(&cfg)); cuddle_config_apply(&cfg);
  assert(cuddle_preferences_get()->large_text && cuddle_preferences_get()->appearance == 2);
  assert(!strcmp(cuddle_preferences_get()->start_room, "morning"));
  FILE *file = fopen(path, "w"); assert(file);
  fputs("# keep this comment\nbrowser = /bin/echo\n custom_key = preserved\nappearance=dark\nlarge_text=true\nappearance=light\nstart_room=morning\ntheme=dim", file);
  assert(!fclose(file));
  prefs.appearance = 0; prefs.large_text = 0; strcpy(prefs.theme, "default"); strcpy(prefs.start_room, "hub");
  assert(!cuddle_preferences_save(&prefs, error, sizeof(error)));
  char *saved = read_file(path);
  assert(strstr(saved, "# keep this comment\nbrowser = /bin/echo\n custom_key = preserved\n"));
  assert(strstr(saved, "appearance = system\n") && !strstr(saved, "appearance=dark"));
  assert(!strstr(saved, "session") && !strstr(saved, "zones") && !strstr(saved, "answers="));
  strcpy(prefs.start_room, "invalid-room");
  assert(cuddle_preferences_save(&prefs, error, sizeof(error)) == -1 && error[0]);
  char *unchanged = read_file(path); assert(!strcmp(saved, unchanged)); free(unchanged);
  strcpy(prefs.start_room, "hub"); strcpy(prefs.theme, "../../escape");
  assert(cuddle_preferences_save(&prefs, error, sizeof(error)) == -1);
  strcpy(prefs.theme, "default");
  assert(!rename(path, moved)); assert(!symlink(moved, path));
  assert(cuddle_preferences_save(&prefs, error, sizeof(error)) == -1);
  assert(!lstat(path, &mode) && S_ISLNK(mode.st_mode));
  unchanged = read_file(moved); assert(!strcmp(saved, unchanged)); free(unchanged); free(saved);
  assert(!unlink(path)); assert(!unlink(moved));
  assert(!mkdir(path, 0700));
  assert(cuddle_preferences_save(&prefs, error, sizeof(error)) == -1);
  assert(!rmdir(path));
  /* Temporary files are removed after unsuccessful saves. */
  DIR *dir = opendir(folder); assert(dir); struct dirent *entry;
  while ((entry = readdir(dir))) assert(!strncmp(entry->d_name, ".", 1));
  closedir(dir);
  assert(!rmdir(folder)); assert(!rmdir(xdg)); assert(!rmdir(temp));
  puts("ok preference save: atomic settings-only update, unknown-key preservation, private mode, invalid/link/directory rejection");
  return 0;
#endif
}
