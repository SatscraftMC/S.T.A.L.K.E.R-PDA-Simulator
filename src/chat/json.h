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

/* The tags array of a NIP-01 event: one internal walk over the raw array
   that json_get("tags") yields as a substring.

   json_tags_rebuild() re-emits every string pair in received order as the
   canonical tags_json - the verifier's input to the serialization, so an
   extra or reordered tag round-trips byte-identically or the mandatory id
   recheck would fail on honest messages. Elements that are not a
   well-formed string pair are skipped (no part of them is re-emitted).
   A missing tags key rebuilds to "[]".

   json_tag() returns the first occurrence's value, unescaped - what the
   carriers' chan filter, peer naming and the display label read ("first
   occurrence wins"). */
bool json_tags_rebuild(const char * obj, char * out, size_t outn);
bool json_tag(const char * obj, const char * name, char * out, size_t outn);

/* Escape a string into a JSON string body (no quotes added). Returns the
   number of bytes written, excluding the terminator. Bytes >=0x80 pass
   through raw (valid UTF-8). */
size_t json_escape(const char * in, char * out, size_t outn);

#endif
