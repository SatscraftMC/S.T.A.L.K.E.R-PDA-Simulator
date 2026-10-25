#include "chat/chat.h"
#include "chat/json.h"
#include "ui/prefs.h"
#include "identity/identity.h"
#include "identity/contacts.h"

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

/* the carriers a message goes out on - the user picks them in the Messages
   tab. All of them are on to begin with. */
static bool s_enabled[CHAT_TRANSPORT_MAX];

static chat_transport_t * s_tr[CHAT_TRANSPORT_MAX];
static int s_n_tr = 0;

static char s_id[CHAT_NAME_MAX] = "STALKER";
static char s_room[CHAT_ROOM_MAX] = "SECTOR";
static char s_chan[CHAT_ROOM_MAX] = "mesh";

/* the devices the mesh has been heard from: the nodes of the mesh map */
static chat_peer_t s_peers[CHAT_PEER_MAX];
static int s_n_peers = 0;

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
    /* no control characters: the name goes into a file and onto the wire */
    size_t o = 0;
    for(const char * p = name; *p && o + 1 < sizeof(s_id); p++) {
        unsigned char c = (unsigned char)*p;
        if(c < 0x20 || c == 0x7f) continue;
        s_id[o++] = *p;
    }
    if(o == 0) return;
    s_id[o] = 0;
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
    size_t o = 0;
    for(const char * p = room; *p && o + 1 < sizeof(s_room); p++) {
        unsigned char c = (unsigned char)*p;
        if(c < 0x20 || c == 0x7f) continue;
        s_room[o++] = *p;
    }
    if(o == 0) return;
    s_room[o] = 0;
}

void chat_add_transport(chat_transport_t * t)
{
    if(!t || s_n_tr >= CHAT_TRANSPORT_MAX) return;
    t->state = CHAT_CARRIER_OFFLINE;
    snprintf(t->status, sizeof(t->status), "offline");
    s_tr[s_n_tr++] = t;
}

/* which carriers a message goes out on */
void chat_set_carrier_enabled(int index, bool on)
{
    if(index < 0 || index >= CHAT_TRANSPORT_MAX) return;
    s_enabled[index] = on;
}

bool chat_carrier_enabled(int index)
{
    if(index < 0 || index >= CHAT_TRANSPORT_MAX) return false;
    return s_enabled[index];
}

const char * chat_carrier_state_name(chat_carrier_state_t st)
{
    switch(st) {
        case CHAT_CARRIER_ONLINE: return "ONLINE";
        case CHAT_CARRIER_CONNECTING: return "CONNECTING";
        default: return "OFFLINE";
    }
}

void chat_set_mesh_channel(const char * chan)
{
    if(!chan || !*chan) return;
    snprintf(s_chan, sizeof(s_chan), "%.*s", (int)sizeof(s_chan) - 1, chan);
}

const char * chat_mesh_channel(void)
{
    return s_chan;
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
    /* the key first (it may write a SYSTEM line and go on unsigned), then
       the trust list - both run right after prefs_init(), so prefs_path()
       resolves the config directory they live in */
    identity_init();
    contacts_load();

    /* every carrier sends to begin with */
    for(int i = 0; i < CHAT_TRANSPORT_MAX; i++) s_enabled[i] = true;

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

    const char * chan = getenv("PDA_MESH_CHANNEL");
    if(chan && *chan) chat_set_mesh_channel(chan);
    else {
        const char * saved = prefs_str("mesh_chan", NULL);
        if(saved && *saved) chat_set_mesh_channel(saved);
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

/* empty the log (the Advanced page's "Reset log") */
void chat_clear(void)
{
    s_head = 0;
    s_count = 0;
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

/* is this exactly n lowercase hex characters? (the wire format of id,
   pubkey and sig - uppercase is malformed, per NIP-01's lowercase rules) */
static bool hex_ok(const char * s, size_t n)
{
    if(!s || strlen(s) != n) return false;
    for(size_t i = 0; i < n; i++) {
        char c = s[i];
        if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return true;
}

/* The one inbound choke point: an event line a carrier already handed over
   (the t/legacy dispatch lives in the carriers). Exactly these rules, none
   of them left to the builder:

     1. kind must parse to 1, else the line is dropped as legacy
     2. parse with our own scratch buffers; content > 240 and a truncated
        tags substring are dropped before any verify attempt
     3. empty content is never delivered
     4. verify -> CHAT_SIG_OK / CHAT_SIG_BAD / CHAT_SIG_NONE, by these
        boundaries alone
     5. note the speaker (only a verified, non-own key), push the message  */
void chat_deliver_line(int transport, const char * line)
{
    if(!line || !*line) return;

    long kind = 0;
    if(!json_get_int(line, "kind", &kind) || kind != 1) {
        if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] legacy line dropped\n");
        return;
    }

    char id[CHAT_ID_MAX] = "", pub[65] = "", sig[129] = "";
    char content[512] = "", tags_raw[512] = "";
    json_get(line, "id", id, sizeof(id));
    json_get(line, "pubkey", pub, sizeof(pub));
    json_get(line, "sig", sig, sizeof(sig));
    json_get(line, "content", content, sizeof(content));
    bool have_tags = json_get(line, "tags", tags_raw, sizeof(tags_raw));
    long created_at = 0;
    bool have_ts = json_get_int(line, "created_at", &created_at);

    /* the size caps: never display truncated content, never recompute an id
       over truncated tags (a truncation at 511 is over 240 too, so
       content's rule catches its own truncations) */
    if(strlen(content) > 240) {
        if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] event dropped: content too long\n");
        return;
    }
    if(have_tags && strlen(tags_raw) >= 511) {
        if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] event dropped: tags too long\n");
        return;
    }

    if(!content[0]) return;              /* empty content is never delivered */

    /* verify -> one of three states, decided by the boundaries alone */
    bool claim = pub[0] != 0 && sig[0] != 0;
    chat_sig_t st = claim ? CHAT_SIG_BAD : CHAT_SIG_NONE;
    if(claim && hex_ok(pub, 64) && hex_ok(sig, 128) && hex_ok(id, 64) && have_ts) {
        char tags_json[2048];
        char ser[2048];
        char re_id[65];
        if(json_tags_rebuild(line, tags_json, sizeof(tags_json)) &&
           chat_event_serialize(ser, sizeof(ser), pub, created_at, 1,
                                tags_json, content) &&
           identity_event_id(ser, re_id) &&
           strcmp(re_id, id) == 0 &&          /* the mandatory id recheck */
           identity_verify_id(pub, id, sig)) {
            st = CHAT_SIG_OK;
        }
    }

    char name_tag[128] = "";
    json_tag(line, "name", name_tag, sizeof(name_tag));
    char who[CHAT_NAME_MAX];
    snprintf(who, sizeof(who), "%.*s", (int)sizeof(who) - 1,
             name_tag[0] ? name_tag : "UNKNOWN");
    time_t ts = have_ts ? (time_t)created_at : time(NULL);

    if(getenv("PDA_DEBUG")) {
        if(st == CHAT_SIG_OK) fprintf(stderr, "[chat] sig ok pub=%s\n", pub);
        else if(st == CHAT_SIG_BAD)
            fprintf(stderr, "[chat] sig bad pub=%s from=%s\n", pub, who);
        else fprintf(stderr, "[chat] unsigned from=%s\n", who);
    }

    /* de-dup on the event id when it is well formed, else a local one */
    char mid[CHAT_ID_MAX];
    if(hex_ok(id, 64)) snprintf(mid, sizeof(mid), "%s", id);
    else make_id(mid, sizeof(mid));

    /* a verified key we have heard: pressable in the SEEN section. An
       unsigned or forged claim never becomes one, and neither does our own
       key (you do not trust yourself) */
    if(st == CHAT_SIG_OK && pub[0] && strcmp(pub, identity_pub_hex()) != 0)
        chat_note_speaker(pub, who);

    /* the label of a trusted contact follows the name it signs with - but
       only when a name tag is there at all (a missing one is not a rename) */
    if(st == CHAT_SIG_OK && name_tag[0] && contacts_has(pub)) {
        const char * stored = contacts_name(pub);
        if(!stored || strcmp(stored, name_tag) != 0) contacts_trust(pub, name_tag);
    }

    if(seen_before(mid, who, content)) return;
    remember(mid, who, content);

    chat_msg_t m;
    memset(&m, 0, sizeof(m));
    snprintf(m.id, sizeof(m.id), "%s", mid);
    snprintf(m.sender, sizeof(m.sender), "%s", who);
    snprintf(m.text, sizeof(m.text), "%.*s", (int)sizeof(m.text) - 1, content);
    m.kind = CHAT_KIND_MSG;
    m.transport = transport;
    m.outbound = false;
    m.ts = ts;
    snprintf(m.pub, sizeof(m.pub), "%s", claim ? pub : "");
    m.sig = st;
    push(&m);
    s_unread++;
    if(getenv("PDA_DEBUG")) {
        const chat_transport_t * tr = chat_transport_get(transport);
        fprintf(stderr, "[chat] recv via %s from %s: %s\n",
                tr ? tr->tag : "?", who, content);
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
    /* the SYSTEM line lives in the message log; under PDA_DEBUG it also
       reaches the log the driver suites read (an honest failure that only
       ever showed on a screen nobody is looking at is not a report) */
    if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] system: %s\n", m.text);
}

bool chat_event_serialize(char * out, size_t outn, const char * pubkey_hex,
                          long created_at, long kind, const char * tags_json,
                          const char * content)
{
    if(!out || outn == 0 || !pubkey_hex || !tags_json || !content) return false;
    out[0] = 0;
    if(strlen(content) > 240) return false;    /* the caps are defined here */

    char e_content[1536];                      /* 240 * 6 worst escaped */
    json_escape(content, e_content, sizeof(e_content));
    int w = snprintf(out, outn, "[0,\"%s\",%ld,%ld,%s,\"%s\"]",
                     pubkey_hex, created_at, kind, tags_json, e_content);
    if(w < 0 || (size_t)w >= outn) { out[0] = 0; return false; }
    return true;
}

/* the line as it goes on the wire (the unsigned shape has no sig field) */
static bool emit_event(char * out, size_t outn, const char * id, const char * pub,
                       long created_at, const char * tags_json,
                       const char * content, const char * sig)
{
    char e_content[1536];
    json_escape(content, e_content, sizeof(e_content));
    int w;
    if(sig) {
        w = snprintf(out, outn,
                     "{\"id\":\"%s\",\"pubkey\":\"%s\",\"created_at\":%ld,\"kind\":1,"
                     "\"tags\":%s,\"content\":\"%s\",\"sig\":\"%s\"}",
                     id, pub, created_at, tags_json, e_content, sig);
    }
    else {
        w = snprintf(out, outn,
                     "{\"id\":\"%s\",\"pubkey\":\"%s\",\"created_at\":%ld,\"kind\":1,"
                     "\"tags\":%s,\"content\":\"%s\"}",
                     id, pub, created_at, tags_json, e_content);
    }
    if(w < 0 || (size_t)w >= outn) { out[0] = 0; return false; }
    return true;
}

/* true when the last chat_event_build() went out signed (chat_send records
   that honestly in its own log row: a real pubkey and CHAT_SIG_OK) */
static bool s_last_built_signed = false;

bool chat_event_build(char * out, size_t outn, char * id_out,
                      long created_at, const char * text)
{
    if(!out || outn == 0 || !id_out || !text) return false;
    out[0] = 0;
    id_out[0] = 0;
    s_last_built_signed = false;

    char e_room[160], e_chan[160], e_name[224];
    json_escape(s_room, e_room, sizeof(e_room));
    json_escape(s_chan, e_chan, sizeof(e_chan));
    json_escape(s_id, e_name, sizeof(e_name));

    char tags[640];
    int tw = snprintf(tags, sizeof(tags),
                      "[[\"room\",\"%s\"],[\"chan\",\"%s\"],[\"name\",\"%s\"]]",
                      e_room, e_chan, e_name);
    if(tw < 0 || (size_t)tw >= sizeof(tags)) return false;

    char ser[2048], id[65], sig[129];
    const char * pub = identity_pub_hex();

    if(pub[0] &&
       chat_event_serialize(ser, sizeof(ser), pub, created_at, 1, tags, text) &&
       identity_event_id(ser, id) &&
       identity_sign_id(id, sig) &&
       emit_event(out, outn, id, pub, created_at, tags, text, sig)) {
        s_last_built_signed = true;
        snprintf(id_out, 65, "%s", id);
        return true;
    }

    /* no usable key (init failed, or the sign refused): the unsigned
       shape, same call, one caller - hashing still gives the line a real
       id, and if even that fails we return false and send nothing */
    out[0] = 0;
    if(chat_event_serialize(ser, sizeof(ser), "", created_at, 1, tags, text) &&
       identity_event_id(ser, id) &&
       emit_event(out, outn, id, "", created_at, tags, text, NULL)) {
        snprintf(id_out, 65, "%s", id);
        return true;
    }
    out[0] = 0;
    id_out[0] = 0;
    return false;
}

bool chat_send(const char * text)
{
    if(!text || !*text) return false;

    /* one event, one created_at, one id and - with deterministic signing -
       one signature: every carrier ships the identical bytes */
    char event[2048];
    char id[CHAT_ID_MAX] = "";
    bool built = chat_event_build(event, sizeof(event), id, (long)time(NULL), text);
    bool signed_out = built && s_last_built_signed;

    if(!built) make_id(id, sizeof(id));   /* local-only row id, never sent */

    bool any = false;
    char why[200] = "";       /* the carriers that did not take it, and why */

    if(built) for(int i = 0; i < s_n_tr; i++) {
        chat_transport_t * t = s_tr[i];
        if(!t) continue;

        char one[64] = "";
        if(!chat_carrier_enabled(i)) {
            snprintf(one, sizeof(one), "%s off", t->tag);
        }
        else if(t->state != CHAT_CARRIER_ONLINE || !t->send) {
            if(t->status[0]) snprintf(one, sizeof(one), "%s %s", t->tag, t->status);
            else snprintf(one, sizeof(one), "%s down", t->tag);
        }
        else if(t->send(t, event, id)) {
            any = true;
        }
        else {
            snprintf(one, sizeof(one), "%s failed", t->tag);
        }

        if(one[0] && strlen(why) + strlen(one) + 3 < sizeof(why)) {
            if(why[0]) strncat(why, ", ", sizeof(why) - strlen(why) - 1);
            strncat(why, one, sizeof(why) - strlen(why) - 1);
        }
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
    /* outbound rows record what actually happened: a signed send carries
       our own pubkey, an identity-failure send "" - honest either way, and
       neither ever shows the badge (you don't need one for yourself) */
    if(signed_out) snprintf(m.pub, sizeof(m.pub), "%s", identity_pub_hex());
    m.sig = signed_out ? CHAT_SIG_OK : CHAT_SIG_NONE;
    push(&m);
    remember(m.id, m.sender, m.text);   /* so our own echo is not shown twice */

    if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] sent: %s\n", text);
    if(!built) {
        chat_system("could not build the message - kept locally, not sent");
    }
    else if(!any) {
        char line[CHAT_TEXT_MAX + 64];
        snprintf(line, sizeof(line), "no carrier up - kept locally (%s)",
                 why[0] ? why : "nothing enabled");
        chat_system(line);
    }
    return true;
}

void chat_poll(void)
{
    for(int i = 0; i < s_n_tr; i++) {
        chat_transport_t * t = s_tr[i];
        if(!t) continue;
        if(t->state != CHAT_CARRIER_ONLINE && t->connect) {
            t->connect(t);          /* the carrier says what state it is in */
        }
        if(t->state == CHAT_CARRIER_ONLINE && t->poll) t->poll(t);
    }
}

/* ---------- the peers of the mesh ---------- */

void chat_note_peer(const char * addr, int port, const char * name)
{
    if(!addr || !*addr) return;
    time_t now = time(NULL);

    /* the same device again */
    for(int i = 0; i < s_n_peers; i++) {
        if(strcmp(s_peers[i].addr, addr) != 0) continue;
        s_peers[i].last = now;
        if(port > 0) s_peers[i].port = port;
        if(name && *name) snprintf(s_peers[i].name, sizeof(s_peers[i].name), "%s", name);
        return;
    }

    /* a new one: the quietest goes */
    if(s_n_peers >= CHAT_PEER_MAX) {
        int oldest = 0;
        for(int i = 1; i < s_n_peers; i++)
            if(s_peers[i].last < s_peers[oldest].last) oldest = i;
        for(int j = oldest; j < s_n_peers - 1; j++) s_peers[j] = s_peers[j + 1];
        s_n_peers--;
    }

    snprintf(s_peers[s_n_peers].addr, sizeof(s_peers[s_n_peers].addr), "%s", addr);
    snprintf(s_peers[s_n_peers].name, sizeof(s_peers[s_n_peers].name), "%s",
             (name && *name) ? name : "?");
    s_peers[s_n_peers].port = port;
    s_peers[s_n_peers].last = now;
    s_n_peers++;
}

int chat_peer_count(void)
{
    time_t now = time(NULL);
    /* the ones that went quiet are gone */
    for(int i = 0; i < s_n_peers; ) {
        if(now - s_peers[i].last > 300) {
            for(int j = i; j < s_n_peers - 1; j++) s_peers[j] = s_peers[j + 1];
            s_n_peers--;
            continue;
        }
        i++;
    }
    return s_n_peers;
}

const chat_peer_t * chat_peer_at(int i)
{
    if(i < 0 || i >= s_n_peers) return NULL;
    return &s_peers[i];
}

/* ---------- the speakers heard this session (the SEEN section) ---------- */

/* Noted from chat_deliver_line() only for events with CHAT_SIG_OK and a
   non-empty pubkey that is not our own - an unsigned or forged claim is
   exactly what cannot become pressable in the Trust UI. Session-only:
   saved contacts persist, this list does not. */

static chat_speaker_t s_speakers[CHAT_PEER_MAX];
static int s_n_speakers = 0;
static unsigned s_speaker_version = 0;

void chat_note_speaker(const char * pub, const char * name)
{
    if(!pub || !*pub) return;
    time_t now = time(NULL);

    for(int i = 0; i < s_n_speakers; i++) {
        if(strcmp(s_speakers[i].pub, pub) != 0) continue;
        s_speakers[i].last = now;
        if(name && *name)
            snprintf(s_speakers[i].name, sizeof(s_speakers[i].name), "%s", name);
        s_speaker_version++;
        return;
    }

    /* the quietest goes, the way the mesh peers do */
    if(s_n_speakers >= CHAT_PEER_MAX) {
        int oldest = 0;
        for(int i = 1; i < s_n_speakers; i++)
            if(s_speakers[i].last < s_speakers[oldest].last) oldest = i;
        for(int j = oldest; j < s_n_speakers - 1; j++)
            s_speakers[j] = s_speakers[j + 1];
        s_n_speakers--;
        s_speaker_version++;
    }

    snprintf(s_speakers[s_n_speakers].pub, sizeof(s_speakers[s_n_speakers].pub),
             "%s", pub);
    snprintf(s_speakers[s_n_speakers].name, sizeof(s_speakers[s_n_speakers].name),
             "%s", (name && *name) ? name : "UNKNOWN");
    s_speakers[s_n_speakers].last = now;
    s_n_speakers++;
    s_speaker_version++;
}

/* the quiet-timeout sweep lives in the count, exactly like
   chat_peer_count() (chat.c's mesh peers at the same 300 s) */
int chat_speaker_count(void)
{
    time_t now = time(NULL);
    for(int i = 0; i < s_n_speakers; ) {
        if(now - s_speakers[i].last > 300) {
            for(int j = i; j < s_n_speakers - 1; j++)
                s_speakers[j] = s_speakers[j + 1];
            s_n_speakers--;
            s_speaker_version++;
            continue;
        }
        i++;
    }
    return s_n_speakers;
}

const chat_speaker_t * chat_speaker_at(int i)
{
    if(i < 0 || i >= s_n_speakers) return NULL;
    return &s_speakers[i];
}

unsigned chat_speaker_version(void)
{
    return s_speaker_version;
}
