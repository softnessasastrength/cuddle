/* SPDX-License-Identifier: MIT
 * Explicit appearance/startup preference saving. No room/session data is accepted.
 */
#include "cuddle.h"
#include "config.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int cuddle_config_path(char *buffer, size_t size);

static int fail(char *error, size_t size, const char *message) {
  if (error && size) snprintf(error, size, "%s", message);
  return -1;
}
#ifdef CUDDLE_HAS_USER_CONFIG
static int directories(char *path) {
  for (char *p = path + 1; ; p++) {
    if (*p == '/' || !*p) {
      char saved = *p; *p = '\0';
      struct stat st;
      int result = mkdir(path, 0700);
      int bad = result && errno != EEXIST;
      if (!bad && (stat(path, &st) || !S_ISDIR(st.st_mode))) { errno = ENOTDIR; bad = 1; }
      *p = saved;
      if (bad) return -1;
      if (!saved) return 0;
    }
  }
}
static int managed_key(const char *line) {
  while (*line == ' ' || *line == '\t') line++;
  if (*line == '#') return 0;
  const char *end = strchr(line, '=');
  if (!end) return 0;
  while (end > line && (end[-1] == ' ' || end[-1] == '\t')) end--;
  const char *keys[] = {"appearance", "large_text", "theme", "start_room"};
  for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
    if (strlen(keys[i]) == (size_t)(end - line) && !strncmp(line, keys[i], (size_t)(end - line))) return 1;
  return 0;
}
#endif
int cuddle_preferences_save(const struct cuddle_preferences *p, char *error, size_t size) {
  if (error && size) error[0] = '\0';
  if (!p || !memchr(p->theme, '\0', sizeof(p->theme)) || !memchr(p->start_room, '\0', sizeof(p->start_room)) ||
      (strcmp(p->theme, "default") && strcmp(p->theme, "dim")) ||
      (strcmp(p->start_room, "hub") && !cuddle_room_find(p->start_room)) ||
      p->appearance < 0 || p->appearance > 2 || p->large_text < 0 || p->large_text > 1)
    return fail(error, size, "Choose a valid appearance, text size, theme, and starting room.");
#ifndef CUDDLE_HAS_USER_CONFIG
  return fail(error, size, "Preference saving is disabled in this build.");
#else
  char path[768], directory[768], temporary[800];
  if (cuddle_config_path(path, sizeof(path))) return fail(error, size, "Cannot locate the user configuration directory.");
  if (path[0] != '/') return fail(error, size, "The configuration directory must be an absolute Linux path.");
  struct stat st;
  if (!lstat(path, &st)) {
    if (!S_ISREG(st.st_mode)) return fail(error, size, "The configuration file is a link or directory. Edit it directly to preserve that layout.");
  } else if (errno != ENOENT) return fail(error, size, strerror(errno));
  snprintf(directory, sizeof(directory), "%s", path);
  char *slash = strrchr(directory, '/');
  if (!slash) return fail(error, size, "Cannot locate the configuration directory.");
  *slash = '\0';
  if (directories(directory)) return fail(error, size, strerror(errno));
  FILE *input = fopen(path, "r");
  if (!input && errno != ENOENT) return fail(error, size, strerror(errno));
  snprintf(temporary, sizeof(temporary), "%s/.cuddle.conf.XXXXXX", directory);
  int fd = mkstemp(temporary);
  if (fd < 0) { if (input) fclose(input); return fail(error, size, strerror(errno)); }
  FILE *output = fdopen(fd, "w");
  if (!output) {
    int saved = errno; close(fd); unlink(temporary); if (input) fclose(input);
    return fail(error, size, strerror(saved));
  }
  int bad = 0, saved_errno = 0, last = '\n';
  char *line = NULL; size_t capacity = 0, total = 0; ssize_t length;
  if (input) {
    while ((length = getline(&line, &capacity, input)) >= 0) {
      total += (size_t)length;
      if (total > 1024 * 1024) { bad = 1; saved_errno = EFBIG; break; }
      if (!managed_key(line)) {
        if (fwrite(line, 1, (size_t)length, output) != (size_t)length) { bad = 1; break; }
        if (length) last = line[length - 1];
      }
    }
    if (ferror(input)) bad = 1;
    if (fclose(input)) bad = 1;
  } else if (fputs("# Cuddle display and startup preferences. Room answers are never saved.\n", output) < 0) bad = 1;
  free(line);
  if (last != '\n' && fputc('\n', output) == EOF) bad = 1;
  const char *appearances[] = {"system", "light", "dark"};
  if (fprintf(output, "appearance = %s\nlarge_text = %s\ntheme = %s\nstart_room = %s\n",
              appearances[p->appearance], p->large_text ? "true" : "false", p->theme, p->start_room) < 0) bad = 1;
  if (fflush(output) || fsync(fd)) bad = 1;
  if (!saved_errno) saved_errno = errno;
  if (fclose(output)) { bad = 1; saved_errno = errno; }
  if (!bad && rename(temporary, path)) { bad = 1; saved_errno = errno; }
  if (bad) {
    unlink(temporary);
    return fail(error, size, saved_errno ? strerror(saved_errno) : "Could not save preferences. The previous file was kept.");
  }
  return 0;
#endif
}
