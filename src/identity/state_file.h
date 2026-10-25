#ifndef PDA_STATE_FILE_H
#define PDA_STATE_FILE_H

#include <stdbool.h>
#include <stddef.h>

/* The one writer for state files that must not be lost: the identity key and
   contacts.conf (prefs keeps its own simple writer - it is settings, and
   prefs_save truncates first).

   state_write() is atomic on POSIX: the contents go into <path>.tmp with
   mode 0600, are fsync'd, then renamed over the target. A mid-write failure
   (ENOSPC/EIO) leaves the old file intact, and the caller only changes its
   in-memory copy after the rename succeeded - disk and memory never diverge.

   state_ensure_dir() makes the directory the file lives in (the mkdir
   pattern of prefs.c:108-115), because a fresh box may not have one yet. */

bool state_ensure_dir(const char * path);
bool state_write(const char * path, const char * data, size_t len);

#endif
