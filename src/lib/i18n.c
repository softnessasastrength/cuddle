#include "cuddle.h"
#include "config.h"

#ifdef CUDDLE_HAS_I18N
#include <libintl.h>
#include <locale.h>
#endif

void cuddle_i18n_init(void) {
#ifdef CUDDLE_HAS_I18N
  setlocale(LC_ALL, "");
  bindtextdomain(CUDDLE_GETTEXT_DOMAIN, CUDDLE_LOCALEDIR);
  textdomain(CUDDLE_GETTEXT_DOMAIN);
#else
  (void)0;
#endif
}
