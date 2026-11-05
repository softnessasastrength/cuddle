#include "cuddle.h"
#include "config.h"

#ifdef CUDDLE_HAS_PLUGINS
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_PLUGINS 32

struct loaded_plugin {
  void *handle;
  const struct cuddle_plugin_v1 *info;
};

static struct loaded_plugin g_loaded[MAX_PLUGINS];
static int g_nloaded;

static int load_one(const char *path) {
  void *h;
  const struct cuddle_plugin_v1 *(*get)(void);
  const struct cuddle_plugin_v1 *info;

  if (g_nloaded >= MAX_PLUGINS) {
    return -1;
  }
  h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!h) {
    return -1;
  }
  get = (const struct cuddle_plugin_v1 *(*)(void))dlsym(h, "cuddle_plugin_get_v1");
  if (!get) {
    dlclose(h);
    return -1;
  }
  info = get();
  if (!info || info->abi != CUDDLE_PLUGIN_ABI) {
    dlclose(h);
    return -1;
  }
  if (info->init && info->init() != 0) {
    dlclose(h);
    return -1;
  }
  g_loaded[g_nloaded].handle = h;
  g_loaded[g_nloaded].info = info;
  g_nloaded++;
  return 0;
}

static int ends_with_so(const char *name) {
  size_t n = strlen(name);
  return n > 3 && strcmp(name + n - 3, ".so") == 0;
}

static void scan_dir(const char *dir) {
  DIR *d;
  struct dirent *ent;
  char path[768];
  struct stat st;

  if (!dir || !dir[0]) {
    return;
  }
  d = opendir(dir);
  if (!d) {
    return;
  }
  while ((ent = readdir(d)) != NULL) {
    if (ent->d_name[0] == '.' || !ends_with_so(ent->d_name)) {
      continue;
    }
    if (snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name) >= (int)sizeof(path)) {
      continue;
    }
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
      continue;
    }
    (void)load_one(path);
  }
  closedir(d);
}
#endif

int cuddle_plugins_enabled(void) {
#ifdef CUDDLE_HAS_PLUGINS
  return 1;
#else
  return 0;
#endif
}

int cuddle_plugins_scan(void) {
#ifdef CUDDLE_HAS_PLUGINS
  char dir[768];
  const char *xdg;

  if (cuddle_data_file(dir, sizeof(dir), "plugins") == 0) {
    scan_dir(dir);
  }
  xdg = getenv("XDG_DATA_HOME");
  if (xdg && xdg[0]) {
    if (snprintf(dir, sizeof(dir), "%s/cuddle/plugins", xdg) < (int)sizeof(dir)) {
      scan_dir(dir);
    }
  } else {
    const char *home = getenv("HOME");
    if (home && home[0] &&
        snprintf(dir, sizeof(dir), "%s/.local/share/cuddle/plugins", home) <
            (int)sizeof(dir)) {
      scan_dir(dir);
    }
  }
  return g_nloaded;
#else
  return 0;
#endif
}

void cuddle_plugins_unload(void) {
#ifdef CUDDLE_HAS_PLUGINS
  int i;

  for (i = g_nloaded - 1; i >= 0; i--) {
    if (g_loaded[i].info && g_loaded[i].info->fini) {
      g_loaded[i].info->fini();
    }
    if (g_loaded[i].handle) {
      dlclose(g_loaded[i].handle);
    }
    g_loaded[i].handle = NULL;
    g_loaded[i].info = NULL;
  }
  g_nloaded = 0;
#endif
}
