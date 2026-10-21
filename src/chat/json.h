#ifndef PDA_CHAT_JSON_H
#define PDA_CHAT_JSON_H

#include <stdbool.h>
#include <stddef.h>

/* Minimal JSON helpers for the chat transports. No allocation, no dependency:
   the transports only ever need to pull a few scalar fields out of a flat
   object and to build one. */

/* Copy the value of a top-level key out of a JSON object.
   Strings are unescaped, numbers/booleans/null are copied as text, and
   nested objects/arrays are copied as raw text. Returns false when the key
   is not present. */
bool json_get(const char * obj, const char * key, char * out, size_t outn);

/* Same, but the value must be a number. */
bool json_get_int(const char * obj, const char * key, long * out);

/* Escape a string into a JSON string body (no quotes added). Returns the
   number of bytes written, excluding the terminator. */
size_t json_escape(const char * in, char * out, size_t outn);

#endif
