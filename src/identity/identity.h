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
 * callers test for a "not set" state) - and also when identity_remote() is
 * true but the remote signer has not given us its key yet. Chat keeps
 * working with no key - messages go out unsigned, with a SYSTEM line saying
 * why - except while remote, where the message is withheld instead: a paired
 * device never puts an unsigned line on the wire.
 *
 * While identity_remote() is true the bunker's user key *is* the device
 * identity: identity_pub_hex()/identity_npub() serve it, and identity_nsec()
 * returns "" because the local nsec is not the active key.
 *
 * No key material in any log, ever: the npub may be logged, the nsec never. */

bool        identity_init(void);        /* load the key file, or generate + save */

const char * identity_pub_hex(void);    /* 64 hex; "" if init failed, or if the
                                           remote session has no known key yet */
const char * identity_npub(void);       /* bech32, for display; "" in the same two
                                           cases */
const char * identity_nsec(void);       /* view-only, for the User page reveal;
                                           "" if init failed or while the identity
                                           is remote - never NULL */

/* true iff a remote signer session exists and it parses - a junk identity.bunker
   is error, not remote: the local identity keeps working while the state row
   shows the honest reason, and [UNPAIR] deletes even a junk file */
bool        identity_remote(void);

bool        identity_import_nsec(const char * nsec_or_hex);  /* validate, then save */
bool        identity_regenerate(void);  /* new keypair, save; old pub is gone */

/* The crypto the event format needs: identity holds keys, hashes and
   signatures only - the *serialization* is chat's business (chat.c). */
bool        identity_event_id(const char * serialization, char * hex65);
            /* SHA-256 -> 64 hex; false only on bad arguments - a real failure
               here means chat_event_build fails and nothing is sent */
bool        identity_verify_id(const char * pub_hex, const char * id_hex,
                               const char * sig_hex);

/* The key gate chat calls first: while the identity is remote, make sure a
   usable user-pubkey exists (the bounded ensure pump - what turns a saved but
   never-reached session into a reachable one on a send). Local backend:
   always true, no cost. On false the one-shot last-failure reason is set. */
bool        identity_prepare(void);

/* Serialize -> hash -> sign -> id + sig. The dispatcher hashes inside (one hash
   total), so id_hex is chat's only id source on the signed path. The local
   backend signs with the device key and ignores unsigned_json; the remote
   backend hands the unsigned event to the signer and runs its four acceptance
   checks. Returns false with identity_last_failure() set. */
bool        identity_sign_event(const char * serialization,   /* our id source */
                                const char * unsigned_json,    /* the NIP-46 param */
                                char * id_hex /* 65 */, char * sig_hex /* 129 */);

/* The one-shot reason the last identity_prepare()/identity_sign_event() failed:
   set on failure, cleared by either succeeding, so it can never go stale.
   chat_send reads it once for the withheld SYSTEM line; an empty reason keeps
   today's exact wording (a local buffer failure is not the signer's fault). */
const char * identity_last_failure(void);

bool        identity_npub_from_pub_hex(const char * pub_hex,   /* any device's key: */
                                       char * out, size_t outn); /* the Contacts tab
                                                                   shows npubs */

#endif
