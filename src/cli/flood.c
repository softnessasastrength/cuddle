/* The same Flood flow is used by every native room frontend. */
int cuddle_cli_room(const char *id);
int cuddle_cli_flood(void) { return cuddle_cli_room("flood"); }
