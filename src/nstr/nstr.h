#ifndef PDA_NSTR_H
#define PDA_NSTR_H

#include <stdbool.h>
#include <stddef.h>

/* The nstr relay client: one WebSocket (+ TLS on wss://) connection to one
   nostr relay, spoken NIP-01. The module is deliberately dumb - it moves
   complete messages in and out and knows nothing about nostr beyond that:
   no subscriptions, no reconnect policy, no room filter (the callers own
   those: the chat carrier, the NIP-47 wallet, the future remote signer).

   No thread, no malloc in the message path: everything is stepped from the
   LVGL tick, exactly like the carriers. A connection handle is a fixed
   slot; the carrier holds one at a time, a wallet operation its own.

   The wire: one complete WebSocket text message is exactly one NIP-01
   JSON array (["EVENT", ...], ["REQ", ...] out; ["EVENT", ...], ["OK", ...],
   ["EOSE", ...], ["NOTICE", ...], ["CLOSED", ...] in). Nothing is line
   split: newlines inside a message are JSON whitespace, not delimiters. */

typedef struct nstr nstr_t;              /* opaque; fixed slots, no malloc in the API */

typedef enum {
    NSTR_IDLE,         /* parsed and resolved, not yet stepped */
    NSTR_CONNECTING,   /* TCP / TLS / WebSocket handshake in progress */
    NSTR_UP,           /* the pipe is open: messages go in and out */
    NSTR_FAILED        /* terminal: see nstr_reason() */
} nstr_state_t;

/* Open an attempt: parse "wss://host[:port][/path]" (TLS, default port 443)
   or "ws://..." (default port 80), resolve the host and load the trust
   bundle this attempt will need. NULL only when all NSTR_CONN_MAX slots
   are taken - no existing handle is ever evicted.

   The resolve is the one blocking call of an attempt; the attempt deadline
   (20 s) starts when this returns. A bad URL, an unresolvable host or an
   unreadable PDA_NSTR_CA come back as a handle already in NSTR_FAILED
   with that reason - close it, no step needed to see the verdict. */
nstr_t * nstr_open(const char * url);

/* One bounded non-blocking step. budget_ms = 0 does one pass with a poll
   timeout of 0 (never waits inside the tick); budget_ms > 0 loops
   non-blocking passes until the budget is spent or progress stalls - what a
   wallet pump asks for. Returns true while the connection is still live
   (CONNECTING or UP - keep stepping it), false when terminal (FAILED or
   the peer closed) - then the caller closes. */
bool nstr_step(nstr_t * c, int budget_ms);

/* Queue one NIP-01 client message (a JSON array, <= 4095 bytes). True =
   queued: the module owns the writes from here. False = not queued (not
   UP, too big, or the outbound queue is full - logged "[nstr] send
   refused" under PDA_DEBUG). Nothing else about the call: a write error
   after queueing surfaces through nstr_step() instead. */
bool nstr_send(nstr_t * c, const char * client_msg);

/* The next complete inbound message (one JSON array) into out. out must be
   >= 4096 bytes - a smaller buffer drops that message and logs "[nstr]
   recv dropped" rather than writing truncated JSON. Returns false when
   there is none. */
bool nstr_recv(nstr_t * c, char * out, size_t outn);

nstr_state_t nstr_state(const nstr_t * c);

/* Why it is not UP - the carrier's status string, one of the §5 table's
   short reasons: "bad url", "no host", "no CA", "no link", "refused",
   "tls failed", "bad ws", "no reply", "disconnected". "" while it is
   still live (IDLE/CONNECTING/UP). */
const char * nstr_reason(const nstr_t * c);

/* Display form of the parsed URL, built by the parser itself: the host,
   or host:port when the port is not the scheme's default, no scheme,
   IPv6 in brackets - what the carrier's ONLINE status shows. "" when the
   URL did not parse. */
const char * nstr_label(const nstr_t * c);

/* Close and free the slot. Still-queued outbound messages are discarded
   (logged "[nstr] queue discarded <n>" under PDA_DEBUG): a connection
   death loses queued messages, honestly. NULL is a no-op. */
void nstr_close(nstr_t * c);

#endif
