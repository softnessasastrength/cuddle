#ifndef CUDDLE_H
#define CUDDLE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * libcuddle public ABI.
 *
 * soversion is 0 (unstable). At project 1.0, freeze the symbols in this
 * header. Additions remain OK. Removals or changed signatures bump soversion.
 * Tags: vMAJOR.MINOR.PATCH. Library soversion is independent of project semver.
 */

#define CUDDLE_VERSION_MAJOR 1
#define CUDDLE_VERSION_MINOR 4
#define CUDDLE_VERSION_PATCH 0
#define CUDDLE_ABI_SOVERSION 0
#define CUDDLE_PLUGIN_ABI 1

enum cuddle_frontend {
  CUDDLE_FRONTEND_CLI = 1,
  CUDDLE_FRONTEND_TUI = 2,
  CUDDLE_FRONTEND_GUI = 3
};

struct cuddle_protocol {
  const char *id;
  const char *title;
  const char *flag;
};

const char *cuddle_version(void);
const char *cuddle_prefix(void);
const char *cuddle_bindir(void);
const char *cuddle_datadir(void);

int cuddle_has_cli(void);
int cuddle_has_tui(void);
int cuddle_has_gui(void);
int cuddle_has_frontend(enum cuddle_frontend frontend);

/* Soft Signal copy. Same text for every frontend. */
const char *cuddle_soft_signal_text(void);

/*
 * Write datadir/relative into buf (NUL-terminated).
 * Returns 0 on success, -1 if arguments are invalid or the result
 * would not fit.
 */
int cuddle_data_file(char *buf, size_t n, const char *relative);

size_t cuddle_protocol_count(void);
const struct cuddle_protocol *cuddle_protocol_at(size_t index);

/* --- plugins (loader compiled only with -Dplugins=enabled) --- */

struct cuddle_plugin_v1 {
  unsigned abi; /* must equal CUDDLE_PLUGIN_ABI; newer major is refused */
  const char *id;
  const char *title;
  const char *protocol_id;
  const char *protocol_flag;
  int (*init)(void);
  void (*fini)(void);
};

int cuddle_plugins_enabled(void);
int cuddle_plugins_scan(void);
void cuddle_plugins_unload(void);

/* --- themes (local files only; letter + word required) --- */

struct cuddle_theme_role {
  const char *role;
  const char *letter;
  const char *word;
  const char *attr;
};

int cuddle_theme_load(const char *name);
const struct cuddle_theme_role *cuddle_theme_role(const char *role);

/* --- XDG config loading is read-only; explicit preference saving is separate. --- */

struct cuddle_config {
  char datadir_override[512];
  char browser[256];
  char tui_screen[64];
  char theme[64];
  char start_room[64];
  int large_text;
  int appearance; /* 0 system, 1 light, 2 dark */
};
struct cuddle_preferences {
  char theme[64];
  char start_room[64];
  int large_text;
  int appearance;
};
const struct cuddle_preferences *cuddle_preferences_get(void);
/* Explicit opt-in save: appearance/theme/start room only; never accepts a session. */
int cuddle_preferences_save(const struct cuddle_preferences *preferences, char *error, size_t size);

int cuddle_config_load(struct cuddle_config *out);
void cuddle_config_apply(const struct cuddle_config *cfg);

void cuddle_i18n_init(void);

/* --- Litmo rooms: immutable content, ephemeral state, no I/O --- */
enum cuddle_room_kind { CUDDLE_ROOM_STEPS, CUDDLE_ROOM_TOUCH, CUDDLE_ROOM_INTEREST };
enum cuddle_session_status { CUDDLE_SESSION_IDLE, CUDDLE_SESSION_ACTIVE,
                             CUDDLE_SESSION_FINISHED, CUDDLE_SESSION_STOPPED };
struct cuddle_room_choice {
  const char *label;
  int target; /* -1 finishes, -2 stops, otherwise a step index. */
  unsigned signal;
  unsigned value; /* interest: 0 no, 1 yes, 2 unknown */
  int navigation; /* Paging choice, never part of a discussion summary. */
};
struct cuddle_room_step {
  const char *title;
  const char *body;
  const struct cuddle_room_choice *choices;
  size_t choice_count;
  int record; /* Include selected labels in this visit's discussion summary. */
};
struct cuddle_room {
  const char *id;
  const char *title;
  const char *summary;
  enum cuddle_room_kind kind;
  const struct cuddle_room_step *steps;
  size_t step_count;
};
#define CUDDLE_TOUCH_ZONES 12
#define CUDDLE_ROOM_HISTORY 32
struct cuddle_session {
  size_t room_index;
  size_t step_index;
  enum cuddle_session_status status;
  unsigned signal_bits;
  unsigned answered;
  unsigned unknown;
  unsigned zones[CUDDLE_TOUCH_ZONES]; /* 0 off limits, 1 ask, 2 welcomed, 3 soft limit */
  size_t history[CUDDLE_ROOM_HISTORY];
  size_t history_count;
  size_t selections[CUDDLE_ROOM_HISTORY];
};
struct cuddle_interest_reading { const char *label; const char *advice; };
size_t cuddle_room_count(void);
const struct cuddle_room *cuddle_room_at(size_t index);
const struct cuddle_room *cuddle_room_find(const char *id);
/* begin clears every previous answer. Invalid IDs leave a stopped session. */
int cuddle_session_begin(struct cuddle_session *session, const char *id);
const struct cuddle_room *cuddle_session_room(const struct cuddle_session *session);
const struct cuddle_room_step *cuddle_session_step(const struct cuddle_session *session);
/* A choice is zero based; choice 0 advances an ordinary step. */
int cuddle_session_choose(struct cuddle_session *session, size_t choice);
int cuddle_session_back(struct cuddle_session *session);
/* Formats this visit's choices only; never writes a file or grants consent. */
int cuddle_session_summary(const struct cuddle_session *session, char *buffer, size_t size);
void cuddle_session_stop(struct cuddle_session *session);
int cuddle_session_cycle_zone(struct cuddle_session *session, size_t zone);
const char *cuddle_touch_zone(size_t zone);
const char *cuddle_touch_status(unsigned status);
struct cuddle_interest_reading cuddle_interest_read(unsigned signal_bits, unsigned answered,
                                                    unsigned unknown);

#ifdef __cplusplus
}
#endif

#endif /* CUDDLE_H */
