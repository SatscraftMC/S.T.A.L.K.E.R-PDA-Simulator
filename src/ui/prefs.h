#ifndef PDA_PREFS_H
#define PDA_PREFS_H

#include <stdbool.h>

/* A tiny persistent key/value store for the settings the user can change on
   the device (the Other tab). Plain "key = value" lines in a file, so it can
   be edited by hand too.

   The file is $PDA_PREFS, or $XDG_CONFIG_HOME/pda/prefs.conf, or
   ~/.config/pda/prefs.conf. */

#define PREFS_MAX 32

void prefs_init(void);
void prefs_load(void);
void prefs_save(void);

const char * prefs_str(const char * key, const char * fallback);
bool prefs_bool(const char * key, bool fallback);
void prefs_set_str(const char * key, const char * value);
void prefs_set_bool(const char * key, bool value);

const char * prefs_path(void);

#endif
