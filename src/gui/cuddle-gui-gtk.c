#include "cuddle.h"
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>

static gboolean g_self_test;

struct ui {
  GtkWidget *win, *stack, *content, *scroll, *hub_heading, *last_room;
  GtkWidget *pref_appearance, *pref_large, *pref_theme, *pref_start, *pref_message, *pref_save;
  gboolean updating, system_dark;
  struct cuddle_preferences saved_preferences;
  struct cuddle_session session;
};
static void render(struct ui *u);
static void clear_content(struct ui *u) {
  GtkWidget *child;
  while (u->content && (child = gtk_widget_get_first_child(u->content)))
    gtk_box_remove(GTK_BOX(u->content), child);
}
static void show_preferences(GtkButton *b, struct ui *u);
static void apply_appearance(struct ui *u, const struct cuddle_preferences *preferences);
static void scroll_top(struct ui *u) {
  GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(u->scroll));
  gtk_adjustment_set_value(adjustment, gtk_adjustment_get_lower(adjustment));
}
static void discard_preview(struct ui *u) {
  if (!g_strcmp0(gtk_stack_get_visible_child_name(GTK_STACK(u->stack)), "preferences"))
    apply_appearance(u, &u->saved_preferences);
}
static GtkWidget *label(GtkWidget *box, const char *text, const char *style) {
  GtkWidget *w = gtk_label_new(text);
  gtk_label_set_wrap(GTK_LABEL(w), TRUE);
  gtk_label_set_xalign(GTK_LABEL(w), 0);
  gtk_label_set_wrap_mode(GTK_LABEL(w), PANGO_WRAP_WORD_CHAR);
  gtk_label_set_max_width_chars(GTK_LABEL(w), 64);
  gtk_label_set_selectable(GTK_LABEL(w), TRUE);
  if (style) gtk_widget_add_css_class(w, style);
  gtk_box_append(GTK_BOX(box), w);
  return w;
}
static GtkWidget *button(GtkWidget *box, const char *text, GCallback cb, struct ui *u) {
  GtkWidget *w = gtk_button_new_with_label(text);
  gtk_widget_set_halign(w, GTK_ALIGN_FILL);
  GtkWidget *text_label = gtk_button_get_child(GTK_BUTTON(w));
  if (GTK_IS_LABEL(text_label)) {
    gtk_label_set_wrap(GTK_LABEL(text_label), TRUE);
    gtk_label_set_wrap_mode(GTK_LABEL(text_label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_max_width_chars(GTK_LABEL(text_label), 54);
    gtk_label_set_xalign(GTK_LABEL(text_label), 0);
  }
  g_signal_connect(w, "clicked", cb, u);
  gtk_box_append(GTK_BOX(box), w);
  return w;
}
static void home(GtkButton *b, struct ui *u) {
  (void)b;
  discard_preview(u);
  cuddle_session_stop(&u->session);
  clear_content(u);
  gtk_stack_set_visible_child_name(GTK_STACK(u->stack), "hub");
  if (u->last_room) gtk_widget_grab_focus(u->last_room);
  else { gtk_widget_grab_focus(u->hub_heading); scroll_top(u); }
}
static void stop(GtkButton *b, struct ui *u) {
  (void)b;
  discard_preview(u);
  cuddle_session_stop(&u->session);
  render(u);
}
static void choose(GtkButton *b, struct ui *u) {
  size_t index = GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(b), "choice"));
  (void)cuddle_session_choose(&u->session, index);
  render(u);
}
static void back(GtkButton *b, struct ui *u) {
  (void)b;
  (void)cuddle_session_back(&u->session);
  render(u);
}
static void zone(GtkButton *b, struct ui *u) {
  size_t index = GPOINTER_TO_SIZE(g_object_get_data(G_OBJECT(b), "zone"));
  (void)cuddle_session_cycle_zone(&u->session, index);
  char text[160];
  snprintf(text, sizeof(text), "%s — %s", cuddle_touch_zone(index), cuddle_touch_status(u->session.zones[index]));
  gtk_button_set_label(b, text);
}
static void open_room(GtkButton *b, struct ui *u) {
  u->last_room = GTK_WIDGET(b);
  const char *id = g_object_get_data(G_OBJECT(b), "room");
  (void)cuddle_session_begin(&u->session, id);
  render(u);
}
static void apply_appearance(struct ui *u, const struct cuddle_preferences *p) {
  if (p->large_text) gtk_widget_add_css_class(u->win, "quiet");
  else gtk_widget_remove_css_class(u->win, "quiet");
  gboolean dark = p->appearance == 0 ? u->system_dark : p->appearance == 2;
  g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", dark, NULL);
}
static struct cuddle_preferences preference_draft(struct ui *u) {
  struct cuddle_preferences draft = {0};
  draft.appearance = (int)gtk_drop_down_get_selected(GTK_DROP_DOWN(u->pref_appearance));
  draft.large_text = gtk_check_button_get_active(GTK_CHECK_BUTTON(u->pref_large));
  snprintf(draft.theme, sizeof(draft.theme), "%s", gtk_drop_down_get_selected(GTK_DROP_DOWN(u->pref_theme)) == 1 ? "dim" : "default");
  guint start = gtk_drop_down_get_selected(GTK_DROP_DOWN(u->pref_start));
  const struct cuddle_room *room = start ? cuddle_room_at(start - 1) : NULL;
  snprintf(draft.start_room, sizeof(draft.start_room), "%s", room ? room->id : "hub");
  return draft;
}
static void draft_changed(struct ui *u) {
  if (u->updating) return;
  struct cuddle_preferences draft = preference_draft(u);
  gboolean dirty = draft.appearance != u->saved_preferences.appearance ||
    draft.large_text != u->saved_preferences.large_text || strcmp(draft.theme, u->saved_preferences.theme) ||
    strcmp(draft.start_room, u->saved_preferences.start_room);
  apply_appearance(u, &draft);
  gtk_widget_set_sensitive(u->pref_save, dirty);
  gtk_widget_remove_css_class(u->pref_message, "error");
  gtk_label_set_text(GTK_LABEL(u->pref_message), dirty ? "Previewing changes. Save to use them next time and in other interfaces." : "Your saved preferences are shown here.");
}
static void selection_changed(GObject *object, GParamSpec *spec, gpointer data) {
  (void)object; (void)spec; draft_changed(data);
}
static void text_changed(GtkCheckButton *button, gpointer data) { (void)button; draft_changed(data); }
static void set_preference_controls(struct ui *u, const struct cuddle_preferences *p) {
  u->updating = TRUE;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(u->pref_appearance), (guint)p->appearance);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(u->pref_large), p->large_text);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(u->pref_theme), !strcmp(p->theme, "dim") ? 1 : 0);
  guint index = 0;
  for (size_t i = 0; i < cuddle_room_count(); i++) if (!strcmp(p->start_room, cuddle_room_at(i)->id)) index = (guint)i + 1;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(u->pref_start), index);
  u->updating = FALSE;
  draft_changed(u);
}
static void save_preferences(GtkButton *button, struct ui *u) {
  (void)button;
  struct cuddle_preferences draft = preference_draft(u);
  char error[512];
  if (cuddle_preferences_save(&draft, error, sizeof(error))) {
    gtk_widget_add_css_class(u->pref_message, "error");
    gtk_label_set_text(GTK_LABEL(u->pref_message), error);
  } else {
    struct cuddle_config config;
    cuddle_config_load(&config); cuddle_config_apply(&config);
    u->saved_preferences = *cuddle_preferences_get();
    apply_appearance(u, &u->saved_preferences);
    gtk_widget_remove_css_class(u->pref_message, "error");
    gtk_label_set_text(GTK_LABEL(u->pref_message), "Saved. Appearance is applied now; the starting room applies next time you open Cuddle.");
    gtk_widget_set_sensitive(u->pref_save, FALSE);
  }
  gtk_widget_grab_focus(u->pref_message);
#if GTK_CHECK_VERSION(4, 14, 0)
  gtk_accessible_announce(GTK_ACCESSIBLE(u->win), gtk_label_get_text(GTK_LABEL(u->pref_message)), GTK_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_MEDIUM);
#endif
}
static void defaults(GtkButton *button, struct ui *u) {
  (void)button;
  const struct cuddle_preferences initial = {"default", "hub", 0, 0};
  set_preference_controls(u, &initial);
}
static void show_preferences(GtkButton *button, struct ui *u) {
  (void)button;
  if (!g_strcmp0(gtk_stack_get_visible_child_name(GTK_STACK(u->stack)), "preferences")) {
    gtk_widget_grab_focus(u->pref_appearance);
    return;
  }
  cuddle_session_stop(&u->session);
  clear_content(u);
  set_preference_controls(u, &u->saved_preferences);
  gtk_stack_set_visible_child_name(GTK_STACK(u->stack), "preferences");
  gtk_widget_grab_focus(u->pref_appearance);
  scroll_top(u);
}
static GtkWidget *preference_select(GtkWidget *page, const char *name, const char *const *options, struct ui *u) {
  GtkWidget *name_label = label(page, name, "heading");
  GtkWidget *control = gtk_drop_down_new_from_strings(options);
  gtk_widget_set_hexpand(control, TRUE);
  gtk_accessible_update_relation(GTK_ACCESSIBLE(control), GTK_ACCESSIBLE_RELATION_LABELLED_BY, name_label, NULL, -1);
  gtk_box_append(GTK_BOX(page), control);
  g_signal_connect(control, "notify::selected", G_CALLBACK(selection_changed), u);
  return control;
}
static void build_preferences(struct ui *u) {
  u->updating = TRUE;
  GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_stack_add_named(GTK_STACK(u->stack), page, "preferences");
  label(page, "Preferences", "title-1");
  label(page, "Only display and startup settings are saved. Room answers and touch maps stay temporary. Opening this screen ends the current room.", NULL);
  const char *appearance[] = {"System appearance", "Light", "Dark", NULL};
  const char *themes[] = {"Default protocol labels", "Dim protocol labels", NULL};
  u->pref_appearance = preference_select(page, "Appearance", appearance, u);
  u->pref_large = gtk_check_button_new_with_label("Larger text and more breathing room");
  for (GtkWidget *child = gtk_widget_get_first_child(u->pref_large); child; child = gtk_widget_get_next_sibling(child)) {
    if (GTK_IS_LABEL(child)) {
      gtk_label_set_wrap(GTK_LABEL(child), TRUE);
      gtk_label_set_max_width_chars(GTK_LABEL(child), 30);
    }
  }
  gtk_box_append(GTK_BOX(page), u->pref_large);
  g_signal_connect(u->pref_large, "toggled", G_CALLBACK(text_changed), u);
  u->pref_theme = preference_select(page, "Protocol label theme", themes, u);
  const char **starts = g_new0(const char *, cuddle_room_count() + 2);
  starts[0] = "Rooms hub";
  for (size_t i = 0; i < cuddle_room_count(); i++) starts[i + 1] = cuddle_room_at(i)->title;
  u->pref_start = preference_select(page, "Starting room", starts, u);
  g_free(starts);
  label(page, "Appearance changes are previewed here. Back to rooms discards changes you have not saved.", "dim-label");
  u->pref_message = label(page, "", NULL);
  gtk_widget_set_focusable(u->pref_message, TRUE);

  u->pref_save = button(page, "Save preferences", G_CALLBACK(save_preferences), u);
  gtk_widget_add_css_class(u->pref_save, "suggested-action");
  button(page, "Preview defaults", G_CALLBACK(defaults), u);
  button(page, "Back to rooms", G_CALLBACK(home), u);
  u->updating = FALSE;
}
static gboolean key(GtkEventControllerKey *c, guint value, guint code, GdkModifierType mods, gpointer data) {
  (void)c; (void)code;
  struct ui *u = data;
  if ((mods & GDK_CONTROL_MASK) && value == GDK_KEY_comma) { show_preferences(NULL, u); return TRUE; }
  if ((mods & GDK_ALT_MASK) && value == GDK_KEY_Home) { home(NULL, u); return TRUE; }
  if ((mods & GDK_ALT_MASK) && value == GDK_KEY_Left && cuddle_session_room(&u->session)) { back(NULL, u); return TRUE; }
  if (value == GDK_KEY_Page_Down || value == GDK_KEY_Page_Up) {
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(u->win));
    if (focus && (GTK_IS_DROP_DOWN(focus) || gtk_widget_get_ancestor(focus, GTK_TYPE_DROP_DOWN))) return FALSE;
    GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(u->scroll));
    double direction = value == GDK_KEY_Page_Down ? 1 : -1;
    gtk_adjustment_set_value(adjustment, gtk_adjustment_get_value(adjustment) + direction * gtk_adjustment_get_page_size(adjustment) * 0.85);
    return TRUE;
  }
  if (value == GDK_KEY_Escape || value == GDK_KEY_q || value == GDK_KEY_Q) {
    stop(NULL, data); /* Stopped sessions never restart on another stop key. */
    return TRUE;
  }
  return FALSE;
}
static void render(struct ui *u) {
  clear_content(u);
  gtk_stack_set_visible_child_name(GTK_STACK(u->stack), "room");
  const struct cuddle_room *r = cuddle_session_room(&u->session);
  if (!r) {
    label(u->content, u->session.status == CUDDLE_SESSION_FINISHED ? "You can land here." : "Soft Signal", "title-1");
    label(u->content, u->session.status == CUDDLE_SESSION_FINISHED ?
      "Room answers are never saved. Completing a room proves nothing." : cuddle_soft_signal_text(), NULL);
    GtkWidget *b = button(u->content, "Return to rooms", G_CALLBACK(home), u);
    gtk_widget_grab_focus(b);
    scroll_top(u);
    return;
  }
  GtkWidget *heading = label(u->content, r->title, "title-1");
  gtk_widget_set_focusable(heading, TRUE);
  label(u->content, r->summary, "dim-label");
  if (r->kind == CUDDLE_ROOM_TOUCH) {
    label(u->content, "These labels never grant permission. Each new visit starts off limits.", NULL);
    for (size_t i = 0; i < CUDDLE_TOUCH_ZONES; i++) {
      char text[160];
      snprintf(text, sizeof(text), "%s — %s", cuddle_touch_zone(i), cuddle_touch_status(u->session.zones[i]));
      GtkWidget *b = button(u->content, text, G_CALLBACK(zone), u);
      g_object_set_data(G_OBJECT(b), "zone", GSIZE_TO_POINTER(i));
    }
  } else {
    const struct cuddle_room_step *s = cuddle_session_step(&u->session);
    label(u->content, s->title, "title-2");
    label(u->content, s->body, NULL);
    if ((!strcmp(r->id, "morning") || !strcmp(r->id, "spoon")) && u->session.step_index + 1 == r->step_count) {
      char summary[4096];
      if (!cuddle_session_summary(&u->session, summary, sizeof(summary))) label(u->content, summary, NULL);
    }
    if (r->kind == CUDDLE_ROOM_INTEREST && u->session.step_index == 6) {
      struct cuddle_interest_reading reading = cuddle_interest_read(u->session.signal_bits, u->session.answered, u->session.unknown);
      label(u->content, reading.label, "title-2");
      label(u->content, reading.advice, NULL);
    }
    for (size_t i = 0; i < (s->choice_count ? s->choice_count : 1); i++) {
      GtkWidget *b = button(u->content, s->choice_count ? s->choices[i].label : "Next / skip", G_CALLBACK(choose), u);
      g_object_set_data(G_OBJECT(b), "choice", GSIZE_TO_POINTER(i));
    }
    if (u->session.history_count) {
      GtkWidget *previous = button(u->content, "Back", G_CALLBACK(back), u);
      gtk_widget_set_tooltip_text(previous, "Previous step (Alt+Left)");
    }
  }
  gtk_widget_grab_focus(heading);
  scroll_top(u);
}
static void activate(GtkApplication *app, gpointer unused) {
  (void)unused;
  GtkWindow *existing = gtk_application_get_active_window(app);
  if (existing) { gtk_window_present(existing); return; }
  struct ui *u = g_new0(struct ui, 1);
  cuddle_session_stop(&u->session);
  u->win = gtk_application_window_new(app);
  g_object_set_data_full(G_OBJECT(u->win), "cuddle-ui", u, g_free);
  gtk_window_set_title(GTK_WINDOW(u->win), "Cuddle — rooms for ordinary human weather");
  gtk_window_set_default_size(GTK_WINDOW(u->win), 720, 800);
  gtk_widget_set_size_request(u->win, 360, 480);
  gtk_widget_add_css_class(u->win, "cuddle-window");
  g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &u->system_dark, NULL);
  u->saved_preferences = *cuddle_preferences_get();
  GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_widget_set_margin_top(root, 16); gtk_widget_set_margin_bottom(root, 16);
  gtk_widget_set_margin_start(root, 16); gtk_widget_set_margin_end(root, 16);
  gtk_window_set_child(GTK_WINDOW(u->win), root);
  label(root, "Cuddle · offline rooms", "title-2");
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(root), toolbar);
  GtkWidget *rooms_button = button(toolbar, "Rooms", G_CALLBACK(home), u);
  gtk_widget_set_tooltip_text(rooms_button, "Return to rooms (Alt+Home)");
  GtkWidget *preferences_button = button(toolbar, "Preferences", G_CALLBACK(show_preferences), u);
  gtk_widget_set_tooltip_text(preferences_button, "Ends the current room; opens appearance and startup settings (Ctrl+,)");
  GtkWidget *stop_button = button(toolbar, "STOP", G_CALLBACK(stop), u);
  gtk_widget_add_css_class(stop_button, "destructive-action");
  gtk_widget_set_tooltip_text(stop_button, "Soft Signal — stop immediately (q / Esc). No explanation needed.");
  gtk_widget_set_hexpand(stop_button, TRUE);
  const struct cuddle_preferences *preferences = cuddle_preferences_get();
  apply_appearance(u, preferences);
  GtkWidget *scroll = gtk_scrolled_window_new();
  u->scroll = scroll;
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_append(GTK_BOX(root), scroll);
  u->stack = gtk_stack_new();
  gtk_stack_set_vhomogeneous(GTK_STACK(u->stack), FALSE);
  gtk_stack_set_hhomogeneous(GTK_STACK(u->stack), FALSE);
  gtk_stack_set_transition_type(GTK_STACK(u->stack), GTK_STACK_TRANSITION_TYPE_NONE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), u->stack);
  GtkWidget *hub = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_stack_add_named(GTK_STACK(u->stack), hub, "hub");
  u->hub_heading = label(hub, "What would help right now?", "title-1");
  gtk_widget_set_focusable(u->hub_heading, TRUE);
  label(hub, "Personal containment, with the joke intact. Not therapy. No scores or stored room answers. You can leave any room.", NULL);
  for (size_t i = 0; i < cuddle_room_count(); i++) {
    const struct cuddle_room *r = cuddle_room_at(i);
    char text[512]; snprintf(text, sizeof(text), "%s\n%s", r->title, r->summary);
    GtkWidget *b = button(hub, text, G_CALLBACK(open_room), u);
    gtk_widget_add_css_class(b, "room-card");
    g_object_set_data(G_OBJECT(b), "room", (gpointer)r->id);
  }
  u->content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
  gtk_stack_add_named(GTK_STACK(u->stack), u->content, "room");
  build_preferences(u);
  char footer[256];
  snprintf(footer, sizeof(footer), "Cuddle %s · q / Esc stops · Room answers stay temporary", cuddle_version());
  label(root, footer, "dim-label");
  GtkEventController *keys = gtk_event_controller_key_new();
  gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
  g_signal_connect(keys, "key-pressed", G_CALLBACK(key), u);
  gtk_widget_add_controller(u->win, keys);
  GtkCssProvider *css = gtk_css_provider_new();
#if GTK_CHECK_VERSION(4, 12, 0)
  gtk_css_provider_load_from_string(css, ".cuddle-window { font-size: 17px; } .cuddle-window.quiet { font-size: 22px; } .cuddle-window button { padding: 12px; min-height: 24px; } .cuddle-window .room-card { padding: 16px; }");
#else
  gtk_css_provider_load_from_data(css, ".cuddle-window { font-size: 17px; } .cuddle-window.quiet { font-size: 22px; } .cuddle-window button { padding: 12px; min-height: 24px; } .cuddle-window .room-card { padding: 16px; }", -1);
#endif
  gtk_style_context_add_provider_for_display(gtk_widget_get_display(u->win), GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref(css);
  if (!g_self_test && strcmp(preferences->start_room, "hub")) {
    if (!cuddle_session_begin(&u->session, preferences->start_room)) render(u);
  }
  if (!g_self_test) {
    gtk_window_present(GTK_WINDOW(u->win));
    if (!strcmp(preferences->start_room, "hub")) { gtk_widget_grab_focus(u->hub_heading); scroll_top(u); }
  }
}
static GtkWidget *find_button(GtkWidget *root, const char *text) {
  if (GTK_IS_BUTTON(root) && !g_strcmp0(gtk_button_get_label(GTK_BUTTON(root)), text)) return root;
  for (GtkWidget *child = gtk_widget_get_first_child(root); child; child = gtk_widget_get_next_sibling(child)) {
    GtkWidget *found = find_button(child, text);
    if (found) return found;
  }
  return NULL;
}
static gboolean click_test(struct ui *u, const char *text) {
  GtkWidget *b = find_button(u->win, text);
  if (!b) { fprintf(stderr, "GTK test: missing button %s\n", text); return FALSE; }
  g_signal_emit_by_name(b, "clicked");
  return TRUE;
}
static int smoke(GtkApplication *app) {
  if (!gtk_init_check()) { puts("GTK widget tests need a display; skipped."); return 77; }
  GError *error = NULL;
  if (!g_application_register(G_APPLICATION(app), NULL, &error)) {
    fprintf(stderr, "GTK test registration: %s\n", error->message); g_clear_error(&error); return 1;
  }
  activate(app, NULL);
  GtkWindow *win = GTK_WINDOW(gtk_application_get_windows(app)->data);
  struct ui *u = g_object_get_data(G_OBJECT(win), "cuddle-ui");
  if (!click_test(u, "Conflict Practice\nA solo rehearsal. No winning, grading, or messages sent.") ||
      !click_test(u, "Repair script") || u->session.step_index != 4) return 1;
  stop(NULL, u); stop(NULL, u);
  if (u->session.status != CUDDLE_SESSION_STOPPED) return 1;
  home(NULL, u);
  if (!click_test(u, "Touch Language\nA map, never consent. Every zone starts off limits.") ||
      !click_test(u, "head / scalp — X off limits") || u->session.zones[0] != 1) return 1;
  stop(NULL, u); home(NULL, u);
  if (!click_test(u, "Touch Language\nA map, never consent. Every zone starts off limits.") || u->session.zones[0]) return 1;
  home(NULL, u);
  if (!click_test(u, "Want or Should?\nSort want, obligation, fear, and uncertainty. Interest is not consent.") || !click_test(u, "Not sure")) return 1;
  for (int i = 0; i < 5; i++) if (!click_test(u, "No")) return 1;
  if (!u->session.unknown || u->session.step_index != 6) return 1;
  if (!click_test(u, "Soft Signal") || u->session.answered || u->session.signal_bits || u->session.unknown) return 1;
  home(NULL, u);
  if (!click_test(u, "Morning Cuddle\nA small negotiation before coffee. Exit is always available.") ||
      !click_test(u, "Crispy — awake / board meeting") || !click_test(u, "Micro (2–5 minutes)") ||
      !click_test(u, "More styles") || !click_test(u, "Jetpack Mode") ||
      !click_test(u, "Company only / no touch") || !click_test(u, "You good, gremlin?")) return 1;
  char summary[4096];
  if (cuddle_session_summary(&u->session, summary, sizeof(summary)) || !strstr(summary, "Jetpack Mode")) return 1;
  stop(NULL, u);
  if (!click_test(u, "Preferences")) return 1;
  if (u->session.status != CUDDLE_SESSION_STOPPED) return 1;
  gtk_drop_down_set_selected(GTK_DROP_DOWN(u->pref_appearance), 2);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(u->pref_large), TRUE);
  if (!gtk_widget_has_css_class(u->win, "quiet")) return 1;
  if (!click_test(u, "Back to rooms") || gtk_widget_has_css_class(u->win, "quiet")) return 1;
  char *path = g_build_filename(g_getenv("XDG_CONFIG_HOME"), "cuddle", "cuddle.conf", NULL);
  if (g_file_test(path, G_FILE_TEST_EXISTS)) { g_free(path); return 1; }
  if (!click_test(u, "Preferences")) { g_free(path); return 1; }
  gtk_drop_down_set_selected(GTK_DROP_DOWN(u->pref_appearance), 2);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(u->pref_large), TRUE);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(u->pref_start), 5); /* morning */
  if (!click_test(u, "Save preferences") || !g_file_test(path, G_FILE_TEST_EXISTS)) { g_free(path); return 1; }
  struct cuddle_config saved;
  cuddle_config_load(&saved);
  if (saved.appearance != 2 || !saved.large_text || strcmp(saved.start_room, "morning")) { g_free(path); return 1; }
  if (!click_test(u, "Preview defaults") || gtk_widget_has_css_class(u->win, "quiet")) { g_free(path); return 1; }
  if (!key(NULL, GDK_KEY_Escape, 0, 0, u) || !gtk_widget_has_css_class(u->win, "quiet") || u->session.status != CUDDLE_SESSION_STOPPED) { g_free(path); return 1; }
  g_free(path);
  gtk_window_destroy(win);
  puts("ok GTK navigation, Morning summary, preference preview/save/cancel, and global stop");
  return 0;
}
int main(int argc, char **argv) {
  if (argc > 1 && !strcmp(argv[1], "--version")) { printf("cuddle-gui %s\n", cuddle_version()); return 0; }
  if (argc > 1 && !strcmp(argv[1], "--help")) { puts("Usage: cuddle-gui\nShared Litmo rooms. q / Esc stops. Preferences saves only display and startup settings."); return 0; }
  g_self_test = argc > 1 && !strcmp(argv[1], "--self-test");
  char *test_directory = NULL;
  if (g_self_test) {
    test_directory = g_dir_make_tmp("cuddle-gtk-test-XXXXXX", NULL);
    if (!test_directory) return 1;
    g_setenv("XDG_CONFIG_HOME", test_directory, TRUE);
  }
  struct cuddle_config cfg;
  cuddle_i18n_init(); cuddle_config_load(&cfg); cuddle_config_apply(&cfg);
  GtkApplication *app = gtk_application_new("org.softnessasstrength.Cuddle", g_self_test ? G_APPLICATION_NON_UNIQUE : (GApplicationFlags)0);
  if (g_self_test) {
    int result = smoke(app);
    g_object_unref(app);
    char *path = g_build_filename(test_directory, "cuddle", "cuddle.conf", NULL);
    char *folder = g_build_filename(test_directory, "cuddle", NULL);
    g_remove(path); g_rmdir(folder); g_rmdir(test_directory);
    g_free(path); g_free(folder); g_free(test_directory);
    return result;
  }
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  return status;
}
