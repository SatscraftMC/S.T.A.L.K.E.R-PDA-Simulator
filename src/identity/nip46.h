#ifndef PDA_NIP46_H
#define PDA_NIP46_H

#include <stdbool.h>
#include <stddef.h>

/* The NIP-46 remote signer (the bunker) - build-order.md #4.
 *
 * The device's nsec never has to live here: the key stays in Amber/nsec.app
 * and every signing call identity.c makes is answered over slice C's relay
 * path (nstr/nstr.h - this module's whole outside world; no socket, no
 * framing, no transport of its own).
 *
 * Two pairing flows:
 *   flow 1  bunker://<signer-pub>?relay=...&secret=...   pasted on the User
 *           page through the [PAIR] door (what Amber/nsec.app display)
 *   flow 2  nostrconnect://<client-pub>?relay=...&secret=...   generated
 *           here, shown as a QR on [CODE], the phone scans it
 *
 * State is one of five lowercase tokens - off, waiting, paired, unreachable,
 * error - with a reason string; the state row and every test assertion read
 * them through nip46_state(). Precedence when several hold: error > waiting
 * > paired/unreachable.
 *
 * Files, both 0600 next to prefs, both owned by this file alone:
 *   <config-dir>/identity.bunker          the session (the token's secret,
 *                                          the disposable client private key,
 *                                          the cached user-pubkey)
 *   <config-dir>/identity.bunker.pending  a flow-2 arm, session-scoped: a
 *                                          boot deletes a leftover one
 *
 * No secret and no config path is ever written to a log. */

/* both token shapes, and the config relay they carry, are capped at this -
   over it the arm refuses with "token too long" */
#define NIP46_TOKEN_MAX 512

typedef enum {
    NIP46_OFF = 0,          /* no session, no arm - the local key signs */
    NIP46_WAITING,          /* a flow-2 code is showing, the phone has not come */
    NIP46_PAIRED,           /* a session that parses; "ready (not tried)" until
                               an operation speaks, then "connected" */
    NIP46_UNREACHABLE,      /* the last operation failed - its reason */
    NIP46_ERROR             /* the session file is there but does not parse */
} nip46_state_t;

void        nip46_init(void);
                /* boot: run the NIP-44 vector gate, drop a leftover pending
                   file, read identity.bunker once into memory, print the
                   boot state line. No dial - house style. Idempotent. */

bool        nip46_parse(const char * bunker_uri, char * why, size_t whyn);
                /* flow 1, validate only: writes nothing (refuse-before-write,
                   like the wallet's connect door). why = one honest line. */

bool        nip46_show_code(char * token_out, size_t outn, char * why, size_t whyn);
                /* flow 2, arm (mint) - idempotent while armed: it rebuilds and
                   returns the same token from the stored client key + secret,
                   never re-minting mid-wait. */

bool        nip46_paired(void);      /* identity.bunker present AND it parses */
bool        nip46_has_session(void); /* identity.bunker exists, junk or not */
bool        nip46_waiting(void);     /* the flow-2 pending file exists */

void        nip46_note_refusal(const char * reason);
                /* a refusal decided OUTSIDE this module (other_ui.c's door
                   conflicts): stores the reason on the current state and
                   prints the same reason-only state: line the internal ones
                   use. The caller owns the text (it passes a pinned literal)
                   and prints the [nip46] refused: line itself. */

unsigned    nip46_version(void);     /* bumped on commit, unpair, cancel and the
                                        first successful fill of an empty
                                        user_pub - other_ui_tick's identity-
                                        change signal */

const char *nip46_user_pub(void);    /* the cached user-pubkey, 64 hex or "" */

/* the enum's five lowercase words - the row's assembly and the log's grep
   surface, so the UI never spells them a second time */
const char * nip46_state_name(nip46_state_t st);

nip46_state_t nip46_state(char * why, size_t whyn);
                /* the one accessor the state row reads; why = the row's
                   reason text (the same string the state: log line carries) */

bool        nip46_pair(const char * bunker_uri, char * why, size_t whyn);
                /* flow 1: validate, save (0600), then connect +
                   get_public_key inside the bounded pump. True once the
                   session file is written - reach truth is the state row's,
                   not the return's. False = a refusal, nothing written. */

void        nip46_unpair(void);      /* delete identity.bunker (whatever its
                                        state) and any pending file; the local
                                        identity returns */
void        nip46_cancel(void);      /* flow-2 cancel: the pending file and the
                                        held handle only - never the session */

bool        nip46_ensure_pub(char * why, size_t whyn);
                /* the bounded ensure pump behind identity_prepare: filled ->
                   true; empty -> connect + get_public_key on a per-op
                   connection, failure sets why */

bool        nip46_sign(const char * unsigned_json, const char * expect_id,
                       char * sig129, char * why, size_t whyn);
                /* connect (or reuse) -> sign_event -> the four acceptance
                   checks, in their pinned order, all owned here. expect_id
                   comes from identity.c, computed over chat's serialization.
                   Any failure returns false and sets why - and while paired
                   that means the message is withheld, never sent unsigned. */

void        nip46_poll(void);        /* budget-0 pass: drives the flow-2 wait
                                        (and only that), called from the Other
                                        panel's 500 ms tick */

#endif
