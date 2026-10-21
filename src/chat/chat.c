#include "chat/chat.h"
#include "chat/json.h"
#include "ui/prefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* registered by the carrier modules */
void chat_bit_register(void);
void chat_fips_register(void);
void chat_net_register(void);

static chat_msg_t s_msgs[CHAT_MAX_MSG];
static int s_head = 0;      /* index of the oldest message */
static int s_count = 0;
static int s_unread = 0;

static chat_transport_t * s_tr[CHAT_TRANSPORT_MAX];
static int s_n_tr = 0;

static char s_id[CHAT_NAME_MAX] = "STALKER";
static char s_room[CHAT_ROOM_MAX] = "SECTOR";

/* recently seen ids, for de-duplicating the same message arriving on two
   carriers at once (a relay that bridges, for instance) */
#define SEEN_MAX 32
typedef struct {
    char id[CHAT_ID_MAX];
    char sender[CHAT_NAME_MAX];
    char text[64];
    time_t ts;
} seen_t;

static seen_t s_seen[SEEN_MAX];
static int s_seen_next = 0;

static void make_id(char * out, size_t outn)
{
    static unsigned counter = 0;
    unsigned long v = (unsigned long)time(NULL) * 1000u + (counter++);
    v ^= (unsigned long)(uintptr_t)out;
    snprintf(out, outn, "%08lx", v & 0xffffffffUL);
}

void chat_set_identity(const char * name)
{
    if(!name || !*name) return;
    snprintf(s_id, sizeof(s_id), "%.*s", (int)sizeof(s_id) - 1, name);
}

const char * chat_identity(void)
{
    return s_id;
}

const char * chat_room(void)
{
    return s_room;
}

void chat_set_room(const char * room)
{
    if(!room || !*room) return;
    snprintf(s_room, sizeof(s_room), "%.*s", (int)sizeof(s_room) - 1, room);
}

void chat_add_transport(chat_transport_t * t)
{
    if(!t || s_n_tr >= CHAT_TRANSPORT_MAX) return;
    t->up = false;
    snprintf(t->status, sizeof(t->status), "offline");
    s_tr[s_n_tr++] = t;
}

int chat_transport_count(void)
{
    return s_n_tr;
}

int chat_transport_index(const chat_transport_t * t)
{
    for(int i = 0; i < s_n_tr; i++)
        if(s_tr[i] == t) return i;
    return -1;
}

const chat_transport_t * chat_transport_get(int i)
{
    if(i < 0 || i >= s_n_tr) return NULL;
    return s_tr[i];
}

void chat_init(void)
{
    /* the environment wins over the saved settings: it is the explicit one */
    const char * name = getenv("PDA_NAME");
    if(name && *name) chat_set_identity(name);
    else {
        const char * saved = prefs_str("name", NULL);
        if(saved && *saved) chat_set_identity(saved);
    }

    const char * room = getenv("PDA_ROOM");
    if(room && *room) chat_set_room(room);
    else {
        const char * saved = prefs_str("room", NULL);
        if(saved && *saved) chat_set_room(saved);
    }

    chat_bit_register();
    chat_fips_register();
    chat_net_register();
}

int chat_count(void)
{
    return s_count;
}

const chat_msg_t * chat_get(int i)
{
    if(i < 0 || i >= s_count) return NULL;
    return &s_msgs[(s_head + i) % CHAT_MAX_MSG];
}

const chat_msg_t * chat_newest(void)
{
    if(s_count == 0) return NULL;
    return &s_msgs[(s_head + s_count - 1) % CHAT_MAX_MSG];
}

int chat_unread(void)
{
    return s_unread;
}

void chat_reset_unread(void)
{
    s_unread = 0;
}

static bool seen_before(const char * id, const char * sender, const char * text)
{
    time_t now = time(NULL);
    for(int i = 0; i < SEEN_MAX; i++) {
        if(s_seen[i].ts == 0) continue;
        if(now - s_seen[i].ts > 30) continue;
        if(id && *id) {
            /* the carriers all carry the id: only the very same message is
               a duplicate, never somebody saying the same words again */
            if(s_seen[i].id[0] && strcmp(s_seen[i].id, id) == 0) return true;
        }
        else if(strcmp(s_seen[i].sender, sender) == 0 && strcmp(s_seen[i].text, text) == 0) {
            return true;      /* a carrier that does not carry an id */
        }
    }
    return false;
}

static void remember(const char * id, const char * sender, const char * text)
{
    seen_t * e = &s_seen[s_seen_next];
    snprintf(e->id, sizeof(e->id), "%.*s", (int)sizeof(e->id) - 1, id ? id : "");
    snprintf(e->sender, sizeof(e->sender), "%.*s", (int)sizeof(e->sender) - 1, sender ? sender : "");
    snprintf(e->text, sizeof(e->text), "%.*s", (int)sizeof(e->text) - 1, text ? text : "");
    e->ts = time(NULL);
    s_seen_next = (s_seen_next + 1) % SEEN_MAX;
}

static void push(const chat_msg_t * m)
{
    if(s_count == CHAT_MAX_MSG) {
        s_head = (s_head + 1) % CHAT_MAX_MSG;
        s_count--;
    }
    s_msgs[(s_head + s_count) % CHAT_MAX_MSG] = *m;
    s_count++;
}

void chat_deliver(int transport, const char * id, const char * sender,
                  const char * text)
{
    if(!text || !*text) return;

    char mid[CHAT_ID_MAX];
    if(id && *id) snprintf(mid, sizeof(mid), "%.*s", (int)sizeof(mid) - 1, id);
    else make_id(mid, sizeof(mid));

    char who[CHAT_NAME_MAX];
    snprintf(who, sizeof(who), "%.*s", (int)sizeof(who) - 1,
             (sender && *sender) ? sender : "UNKNOWN");

    if(seen_before(mid, who, text)) return;
    remember(mid, who, text);

    chat_msg_t m;
    memset(&m, 0, sizeof(m));
    snprintf(m.id, sizeof(m.id), "%s", mid);
    snprintf(m.sender, sizeof(m.sender), "%s", who);
    snprintf(m.text, sizeof(m.text), "%.*s", (int)sizeof(m.text) - 1, text);
    m.kind = CHAT_KIND_MSG;
    m.transport = transport;
    m.outbound = false;
    m.ts = time(NULL);
    push(&m);
    s_unread++;
    if(getenv("PDA_DEBUG")) {
        const chat_transport_t * tr = chat_transport_get(transport);
        fprintf(stderr, "[chat] recv via %s from %s: %s\n",
                tr ? tr->tag : "?", who, text);
    }
}

void chat_system(const char * text)
{
    chat_msg_t m;
    memset(&m, 0, sizeof(m));
    make_id(m.id, sizeof(m.id));
    snprintf(m.sender, sizeof(m.sender), "%s", "SYSTEM");
    snprintf(m.text, sizeof(m.text), "%.*s", (int)sizeof(m.text) - 1, text ? text : "");
    m.kind = CHAT_KIND_SYS;
    m.transport = -1;
    m.outbound = false;
    m.ts = time(NULL);
    push(&m);
}

void chat_envelope(char * out, size_t outn, const char * id, const char * sender,
                   const char * text, long ts)
{
    char e_id[CHAT_ID_MAX * 6], e_from[CHAT_NAME_MAX * 6], e_text[CHAT_TEXT_MAX * 6];
    json_escape(id ? id : "", e_id, sizeof(e_id));
    json_escape(sender ? sender : "", e_from, sizeof(e_from));
    json_escape(text ? text : "", e_text, sizeof(e_text));
    snprintf(out, outn,
             "{\"v\":1,\"t\":\"msg\",\"id\":\"%s\",\"from\":\"%s\",\"room\":\"%s\","
             "\"text\":\"%s\",\"ts\":%ld}",
             e_id, e_from, s_room, e_text, ts);
}

bool chat_send(const char * text)
{
    if(!text || !*text) return false;

    char id[CHAT_ID_MAX];
    make_id(id, sizeof(id));

    bool any = false;
    for(int i = 0; i < s_n_tr; i++) {
        chat_transport_t * t = s_tr[i];
        if(!t->up || !t->send) continue;
        if(t->send(t, text, id)) any = true;
    }

    chat_msg_t m;
    memset(&m, 0, sizeof(m));
    snprintf(m.id, sizeof(m.id), "%s", id);
    snprintf(m.sender, sizeof(m.sender), "%s", s_id);
    snprintf(m.text, sizeof(m.text), "%.*s", (int)sizeof(m.text) - 1, text);
    m.kind = CHAT_KIND_MSG;
    m.transport = -1;
    m.outbound = true;
    m.ts = time(NULL);
    push(&m);
    remember(m.id, m.sender, m.text);   /* so our own echo is not shown twice */

    if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] sent: %s\n", text);
    if(!any) chat_system("no carrier up - message kept locally");
    return true;
}

void chat_poll(void)
{
    for(int i = 0; i < s_n_tr; i++) {
        chat_transport_t * t = s_tr[i];
        if(!t) continue;
        if(!t->up && t->connect) {
            if(t->connect(t)) {
                t->up = true;
                if(getenv("PDA_DEBUG"))
                    fprintf(stderr, "[chat] %s up\n", t->tag);
            }
        }
        if(t->up && t->poll) t->poll(t);
    }
}
