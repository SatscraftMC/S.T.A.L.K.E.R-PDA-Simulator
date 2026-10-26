#include "chat/chat.h"
#include "chat/json.h"
#include "nstr/nstr.h"
#include "ui/prefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* NSTR - the internet carrier: one WebSocket (+ TLS on wss://) connection
 * to one nostr relay, driven from the chat bus's tick like every other
 * carrier. The LAN half of the old NET carrier is gone with it: BIT (2425)
 * is the local mesh and already runs that machinery, and this one decides
 * from the connection attempt itself - resolve/connect/TLS/WS outcomes
 * mapped by the status table (spec-nstr-relay §5), no interface walk.
 *
 *   PDA_NSTR_RELAY   the relay URL (env wins over the saved setting)
 *   pref nstr_relay  the same URL from the Connections > Info row
 *
 * The module (src/nstr/) moves messages and knows nothing else; the
 * subscription, the room scope and the reconnect backoff live here. One
 * handle at a time: open -> connect -> REQ -> read, and a fresh open per
 * attempt behind the 2->30 s backoff.
 */

#define NDBG(...) do { if(getenv("PDA_DEBUG")) fprintf(stderr, __VA_ARGS__); } while(0)

#define NSTR_URL_MAX   192
#define NSTR_SUB       "pda"          /* the chat subscription id */
#define NSTR_DRAIN     16             /* inbound messages per tick */

typedef struct {
    nstr_t * h;                        /* the current attempt/connection */
    char opened[NSTR_URL_MAX];         /* the URL that handle was opened with */
    char bad_url[NSTR_URL_MAX];        /* the URL that failed to parse: per string */
    time_t next_try;                   /* the backoff: while now < this, no open */
    int backoff;                       /* 2 -> 4 ... capped 30 s */
    char sub_room[CHAT_ROOM_MAX];      /* the room the live REQ was built with */
    bool have_sub;                     /* the REQ has been written */
    long newest;                       /* the newest created_at seen (since on a
                                          re-subscribe) */
} nstrc_t;

static nstrc_t s_c;
static chat_transport_t s_tr;

/* ---------- the effective URL: env wins, read every attempt ---------- */

static void effective_url(char * buf, size_t n)
{
    const char * env = getenv("PDA_NSTR_RELAY");
    if(env && *env) { snprintf(buf, n, "%s", env); return; }
    const char * saved = prefs_str("nstr_relay", "");
    snprintf(buf, n, "%s", (saved && *saved) ? saved : "");
}

/* ---------- the state + status table ---------- */

/* a per-tick entry check: log only when the status actually changes, so
   the line stays meaningful and greppable */
static void want_offline(chat_transport_t * t, const char * why)
{
    bool changed = t->state != CHAT_CARRIER_OFFLINE || strcmp(t->status, why) != 0;
    t->state = CHAT_CARRIER_OFFLINE;
    snprintf(t->status, sizeof(t->status), "%s", why);
    if(changed) NDBG("[nstr] reason: %s\n", why);
}

/* a failed attempt or a terminal step: always worth one line (each one is
   backoff-spaced, so this is never a tick-rate log) */
static void fail_offline(chat_transport_t * t, const char * why)
{
    t->state = CHAT_CARRIER_OFFLINE;
    snprintf(t->status, sizeof(t->status), "%s", why);
    NDBG("[nstr] reason: %s\n", why);
}

static void backoff_start(nstrc_t * c)
{
    c->backoff = c->backoff ? c->backoff * 2 : 2;
    if(c->backoff > 30) c->backoff = 30;
    c->next_try = time(NULL) + c->backoff;
}

/* the subscription, in one line. The #t value goes through json_escape()
   like every wire string (the room may hold quotes and backslashes), and
   since/limit bound the catch-up: a reconnect resumes near the newest event
   seen rather than the whole backlog. */
static bool send_req(nstrc_t * c)
{
    char room[CHAT_ROOM_MAX * 6];
    json_escape(chat_room(), room, sizeof(room));
    long since = c->newest ? (long)(c->newest - 60) : (long)(time(NULL) - 300);
    char msg[512];
    int n = snprintf(msg, sizeof(msg),
                     "[\"REQ\",\"" NSTR_SUB "\",{\"kinds\":[1],\"#t\":[\"%s\"],"
                     "\"since\":%ld,\"limit\":50}]",
                     room, since);
    if(n < 0 || (size_t)n >= sizeof(msg)) return false;
    if(!nstr_send(c->h, msg)) return false;
    snprintf(c->sub_room, sizeof(c->sub_room), "%s", chat_room());
    return true;
}

/* close the handle and forget the subscription (the reason is the caller's) */
static void drop_handle(nstrc_t * c)
{
    if(c->h) nstr_close(c->h);
    c->h = NULL;
    c->have_sub = false;
    c->sub_room[0] = 0;
}

/* ---------- the bus entry points ---------- */

/* called every tick while the state is not ONLINE */
static bool nstr_connect(chat_transport_t * t)
{
    nstrc_t * c = &s_c;
    char url[NSTR_URL_MAX];
    effective_url(url, sizeof(url));

    /* step 0: a superseded URL dies at once, in every state - and the
       changed string clears any remembered "bad url" verdict */
    if(strcmp(url, c->opened) != 0) {
        drop_handle(c);
        c->bad_url[0] = 0;
        c->backoff = 0;
        c->next_try = 0;
        snprintf(c->opened, sizeof(c->opened), "%s", url);
        t->state = CHAT_CARRIER_OFFLINE;
        if(!url[0]) want_offline(t, "no relay");   /* a real transition: log it */
        return false;
    }

    if(!url[0]) { want_offline(t, "no relay"); return false; }

    /* a syntax error does not fix itself: the verdict is per URL string */
    if(c->bad_url[0] && strcmp(c->bad_url, url) == 0) {
        want_offline(t, "bad url");
        return false;
    }

    if(time(NULL) < c->next_try) return false;   /* behind the backoff, last
                                                    failure's reason still up */

    if(!c->h) {
        c->h = nstr_open(url);
        if(!c->h) return false;                  /* slots full - cannot occur
                                                    here: this is the only
                                                    caller, one handle */
        NDBG("[nstr] open %s\n", url);
        if(nstr_state(c->h) == NSTR_FAILED) {
            const char * why = nstr_reason(c->h);
            bool bad = strcmp(why, "bad url") == 0;
            if(bad) snprintf(c->bad_url, sizeof(c->bad_url), "%s", url);
            drop_handle(c);
            if(bad) { want_offline(t, "bad url"); return false; }   /* never retried */
            fail_offline(t, why);
            backoff_start(c);
            return false;
        }
        t->state = CHAT_CARRIER_CONNECTING;
        snprintf(t->status, sizeof(t->status), "%s", "connecting");
    }

    if(!nstr_step(c->h, 0)) {                    /* budget 0: never wait here */
        const char * why = nstr_reason(c->h);
        drop_handle(c);
        fail_offline(t, why);
        backoff_start(c);
        return false;
    }

    if(nstr_state(c->h) == NSTR_UP && !c->have_sub) {
        if(send_req(c)) {
            c->have_sub = true;
            c->backoff = 0;                      /* reached ONLINE: reset */
            c->next_try = 0;
            t->state = CHAT_CARRIER_ONLINE;
            snprintf(t->status, sizeof(t->status), "%s", nstr_label(c->h));
            NDBG("[nstr] up\n");
        }
        else {
            /* the queue refused the REQ: stay CONNECTING, write it next tick */
            t->state = CHAT_CARRIER_CONNECTING;
            snprintf(t->status, sizeof(t->status), "%s", "connecting");
        }
    }
    return true;
}

/* one inbound message, dispatched exactly by the verb (spec §5) */
static void dispatch(chat_transport_t * t, char * msg)
{
    nstrc_t * c = &s_c;
    char verb[32];
    if(!json_arr_str(msg, 0, verb, sizeof(verb)))
        snprintf(verb, sizeof(verb), "%s", "?");    /* no verb: logged, ignored */
    NDBG("[nstr] recv %s\n", verb);

    if(strcmp(verb, "EVENT") == 0) {
        char sub[64];
        if(!json_arr_str(msg, 1, sub, sizeof(sub)) || strcmp(sub, NSTR_SUB) != 0)
            return;                                 /* not ours */
        char ev[4096];
        if(!json_arr_get(msg, 2, ev, sizeof(ev))) return;

        /* the room backstop, in this carrier's inbound path only - never in
           chat.c, so BIT and FIPS are provably untouched. A missing room
           tag passes, the same rule the chan filter follows. */
        char room[CHAT_ROOM_MAX];
        if(json_tag(ev, "room", room, sizeof(room)) && room[0] &&
           strcmp(room, chat_room()) != 0) {
            char id[CHAT_ID_MAX] = "";
            json_get(ev, "id", id, sizeof(id));
            NDBG("[nstr] room drop %s\n", id);
            return;
        }
        chat_deliver_line(chat_transport_index(&s_tr), ev);

        long ts = 0;
        if(json_get_int(ev, "created_at", &ts) && ts > c->newest) c->newest = ts;
        return;
    }

    if(strcmp(verb, "OK") == 0) {
        /* true and duplicates: nothing beyond the recv line above. false is
           the relay's verdict, logged verbatim - no demotion, no SYSTEM
           line: the transport took it, the relay's opinion is its own. */
        char flag[16];
        if(json_arr_get(msg, 2, flag, sizeof(flag)) && flag[0] &&
           strcmp(flag, "true") != 0) {
            char why[256] = "";
            json_arr_get(msg, 3, why, sizeof(why));
            NDBG("[nstr] ok false: %s\n", why);
        }
        return;
    }

    if(strcmp(verb, "CLOSED") == 0) {
        char sub[64];
        if(!json_arr_str(msg, 1, sub, sizeof(sub)) || strcmp(sub, NSTR_SUB) != 0)
            return;
        /* our subscription is dead: "subscribed and reading" would be a lie */
        drop_handle(c);
        fail_offline(t, "closed");
        backoff_start(c);
        return;
    }

    /* EOSE, NOTICE and every unknown verb: nothing further */
}

/* called only while ONLINE */
static int nstr_poll(chat_transport_t * t)
{
    nstrc_t * c = &s_c;
    if(!c->h) { fail_offline(t, "disconnected"); return 0; }

    /* step 0: the same comparison as connect() - a live connection never
       outlives a superseded URL */
    char url[NSTR_URL_MAX];
    effective_url(url, sizeof(url));
    if(strcmp(url, c->opened) != 0) {
        drop_handle(c);
        c->bad_url[0] = 0;
        c->backoff = 0;
        c->next_try = 0;
        snprintf(c->opened, sizeof(c->opened), "%s", url);
        t->state = CHAT_CARRIER_OFFLINE;
        if(!url[0]) want_offline(t, "no relay");   /* a real transition: log it */
        return 0;
    }

    if(!nstr_step(c->h, 0)) {
        const char * why = nstr_reason(c->h);
        drop_handle(c);
        fail_offline(t, why);
        backoff_start(c);
        return 0;
    }

    /* the room moved at runtime: a fresh REQ with the same subscription id
       replaces the live one (NIP-01), so no CLOSE, no reconnect, no state
       change - and the new room is remembered only when the REQ was taken,
       so a refused switch retries next tick instead of stranding the sub */
    if(strcmp(c->sub_room, chat_room()) != 0) send_req(c);

    int got = 0;
    char msg[4096];
    while(got < NSTR_DRAIN && c->h && nstr_recv(c->h, msg, sizeof(msg))) {
        got++;
        dispatch(t, msg);
    }
    return got;
}

/* queued is success: the module owns the writes from here */
static bool nstr_send_ev(chat_transport_t * t, const char * event, const char * id)
{
    (void)t;
    (void)id;                          /* the id is the event's, already built */
    nstrc_t * c = &s_c;
    if(!c->h) return false;
    char msg[2160];
    int n = snprintf(msg, sizeof(msg), "[\"EVENT\",%s]", event);
    if(n < 0 || (size_t)n >= sizeof(msg)) return false;
    return nstr_send(c->h, msg);
}

/* interface completeness, exactly like the three carriers before it:
   nothing in src/ calls ->disconnect */
static void nstr_disconnect(chat_transport_t * t)
{
    drop_handle(&s_c);
    t->state = CHAT_CARRIER_OFFLINE;
    snprintf(t->status, sizeof(t->status), "%s", "no relay");
}

void chat_nstr_register(void)
{
    memset(&s_c, 0, sizeof(s_c));
    s_tr.tag = "NSTR";
    s_tr.label = "Nostr relay";
    s_tr.connect = nstr_connect;
    s_tr.disconnect = nstr_disconnect;
    s_tr.poll = nstr_poll;
    s_tr.send = nstr_send_ev;
    s_tr.priv = &s_c;
    s_tr.state = CHAT_CARRIER_OFFLINE;
    snprintf(s_tr.status, sizeof(s_tr.status), "%s", "offline");
    chat_add_transport(&s_tr);
}
