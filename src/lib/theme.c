#include "cuddle.h"

#include <stdio.h>
#include <string.h>

#define MAX_ROLES 16
#define FIELD 32

static struct cuddle_theme_role g_roles[MAX_ROLES];
static char g_store[MAX_ROLES][4][FIELD];
static int g_nroles;

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

int cuddle_theme_load(const char *name) {
  char path[768];
  char line[256];
  char role[FIELD], letter[FIELD], word[FIELD], attr[FIELD];
  FILE *fp;
  int n;

  g_nroles = 0;
  memset(g_roles, 0, sizeof(g_roles));
  memset(g_store, 0, sizeof(g_store));
  if (!name || !name[0] || strchr(name, '/') || strchr(name, '.')) {
    return -1;
  }
  if (snprintf(path, sizeof(path), "%s/themes/%s.theme", cuddle_datadir(), name) >=
      (int)sizeof(path)) {
    return -1;
  }
  fp = fopen(path, "r");
  if (!fp) {
    return -1;
  }
  while (fgets(line, sizeof(line), fp) && g_nroles < MAX_ROLES) {
    char *hash;
    trim(line);
    if (!line[0] || line[0] == '#') {
      continue;
    }
    hash = strchr(line, '#');
    if (hash) {
      *hash = '\0';
      trim(line);
    }
    attr[0] = '\0';
    n = sscanf(line, "%31s = %31s %31s %31s", role, letter, word, attr);
    if (n < 3) {
      n = sscanf(line, "%31s = %31s %31s", role, letter, word);
    }
    if (n < 3 || !letter[0] || !word[0]) {
      continue;
    }
    memcpy(g_store[g_nroles][0], role, FIELD);
    memcpy(g_store[g_nroles][1], letter, FIELD);
    memcpy(g_store[g_nroles][2], word, FIELD);
    memcpy(g_store[g_nroles][3], attr, FIELD);
    g_roles[g_nroles].role = g_store[g_nroles][0];
    g_roles[g_nroles].letter = g_store[g_nroles][1];
    g_roles[g_nroles].word = g_store[g_nroles][2];
    g_roles[g_nroles].attr = (n >= 4 && attr[0]) ? g_store[g_nroles][3] : NULL;
    g_nroles++;
  }
  fclose(fp);
  return 0;
}

const struct cuddle_theme_role *cuddle_theme_role(const char *role) {
  int i;

  if (!role) {
    return NULL;
  }
  for (i = 0; i < g_nroles; i++) {
    if (strcmp(g_roles[i].role, role) == 0) {
      return &g_roles[i];
    }
  }
  return NULL;
}
