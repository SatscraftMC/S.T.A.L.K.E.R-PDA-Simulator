#ifndef PDA_IDENTITY_H
#define PDA_IDENTITY_H

#include <stdbool.h>
#include <stddef.h>

/* The device's nostr identity - the only door to the key.
 *
 * The key lives in identity.nsec, next to the prefs file (same directory, so
 * PDA_PREFS isolates tests and two instances on one box together). The path
 * string and the raw secret bytes exist only inside identity.c: everything
 * else goes through the accessors here.
 *
 * Failure convention: the string accessors return "" and never NULL when
 * there is no usable key (identity_pub_hex()[0] == '\0' is the one condition
 * callers test for an "not set" state). Chat keeps working with no key -
 * messages go out unsigned, with a SYSTEM line saying why.
 *
 * No key material in any log, ever: the npub may be logged, the nsec never. */

bool        identity_init(void);        /* load the key file, or generate + save */

const char * identity_pub_hex(void);    /* 64 hex; "" only if init failed */
const char * identity_npub(void);       /* bech32, for display; "" if init failed */
const char * identity_nsec(void);       /* view-only, for the User page reveal;
                                           "" if init failed - never NULL */

bool        identity_import_nsec(const char * nsec_or_hex);  /* validate, then save */
bool        identity_regenerate(void);  /* new keypair, save; old pub is gone */

/* The crypto the event format needs: identity holds keys, hashes and
   signatures only - the *serialization* is chat's business (chat.c). */
bool        identity_event_id(const char * serialization, char * hex65);
            /* SHA-256 -> 64 hex; false only on bad arguments - a real failure
               here means chat_event_build fails and nothing is sent */
bool        identity_sign_id(const char * id_hex, char * sig_hex);       /* BIP-340 */
bool        identity_verify_id(const char * pub_hex, const char * id_hex,
                               const char * sig_hex);
bool        identity_npub_from_pub_hex(const char * pub_hex,   /* any device's key: */
                                       char * out, size_t outn); /* the Contacts tab
                                                                   shows npubs */

#endif
