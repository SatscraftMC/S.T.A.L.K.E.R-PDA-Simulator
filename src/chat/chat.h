#ifndef PDA_CHAT_H
#define PDA_CHAT_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* The unified chat bus.
 *
 * One room, many carriers: every message that comes in - whatever medium it
 * arrived on - lands in the same log, and a message typed here goes out on
 * every medium that is currently up. The carriers ("transports") are:
 *
 *   NET   the internet, through a relay (tools/relay.py)
 *   BIT   the local mesh, no infrastructure (UDP broadcast, port 2425)
 *   FIPS  the FIPS mesh, through the daemon's native datagram API
 *
 * The wire format is a nostr NIP-01 event, one JSON object per line, the
 * same on all three carriers. The display name is a signed tag, so a peer
 * cannot become somebody else by changing its name - the key underneath is
 * the identity and the badge says whether you pressed Trust on it.
 *
 *   {"id":"<64 hex sha256 of the serialization>","pubkey":"<64 hex>",
 *    "created_at":1730000000,"kind":1,
 *    "tags":[["room","SECTOR"],["chan","mesh"],["name","Satscraft"]],
 *    "content":"anyone got the drop coords?","sig":"<128 hex BIP340>"}
 *
 * Unsigned lines (tests, the relay's ntfy bridge, this device's own
 * identity-failure sends) keep the same shape with "pubkey":"" and no
 * "sig" field. Discovery chatter (hi/hi-ack) stays unsigned legacy
 * {"v":1,"t":...} - it carries no speech and grants no trust.
 */

#define CHAT_MAX_MSG       300
#define CHAT_TEXT_MAX      240
#define CHAT_NAME_MAX      32
#define CHAT_ID_MAX        65      /* event ids are 64 hex + the NUL */
#define CHAT_ROOM_MAX      24
#define CHAT_TRANSPORT_MAX 4

typedef enum {
    CHAT_KIND_SYS = 0,   /* a status line written by the bus itself */
    CHAT_KIND_MSG = 1    /* something somebody said */
} chat_kind_t;

/* what verification said about a received line (see chat_deliver_line) */
typedef enum {
    CHAT_SIG_NONE = 0,   /* no verifiable claim: at least one of pubkey/sig
                            is absent or empty */
    CHAT_SIG_OK = 1,     /* id recomputed and matched, BIP-340 verified */
    CHAT_SIG_BAD = 2     /* claimed to be signed and did not verify */
} chat_sig_t;

typedef struct {
    char id[CHAT_ID_MAX];
    char sender[CHAT_NAME_MAX];
    char text[CHAT_TEXT_MAX];
    chat_kind_t kind;
    int transport;        /* index into the transport list, -1 for local */
    bool outbound;        /* typed here, not received */
    time_t ts;
    char pub[65];         /* the event's pubkey, "" when unsigned */
    chat_sig_t sig;
} chat_msg_t;

/* A carrier. The bus never blocks on one: poll() must return quickly. */
/* what a carrier is doing right now */
typedef enum {
    CHAT_CARRIER_OFFLINE = 0,     /* red: nothing there */
    CHAT_CARRIER_CONNECTING = 1,  /* grey: trying */
    CHAT_CARRIER_ONLINE = 2       /* green: up */
} chat_carrier_state_t;

typedef struct chat_transport {
    const char * tag;         /* short label shown in the UI: "NET" */
    const char * label;       /* long label: "Internet" */
    bool (*connect)(struct chat_transport * t);
    void (*disconnect)(struct chat_transport * t);
    int  (*poll)(struct chat_transport * t);   /* deliver what arrived */
    /* Ship one already-built event line. The bus builds it once per send
       (one created_at, one id, one signature) so every carrier carries the
       identical bytes - the id names the event, the transport only moves
       it. */
    bool (*send)(struct chat_transport * t, const char * event, const char * id);
    chat_carrier_state_t state;
    char status[48];          /* one line for the UI */
    void * priv;
} chat_transport_t;

void chat_init(void);
void chat_poll(void);

/* Say something: goes out on every carrier that is up, and shows up in the
   local log. Returns false when there was nothing to send. */
bool chat_send(const char * text);

/* Which carriers a message goes out on. The user picks them in the
   Messages tab - one carrier, two or all of them. A carrier that is off
   still receives; it just does not send. */
void chat_set_carrier_enabled(int index, bool on);
bool chat_carrier_enabled(int index);

int chat_count(void);
const chat_msg_t * chat_get(int i);          /* 0 = oldest */
const chat_msg_t * chat_newest(void);

/* The single inbound choke point: the raw event line a carrier already
   handed over. The t/legacy dispatch lives in the carriers. */
void chat_deliver_line(int transport, const char * line);
void chat_system(const char * text);
int chat_unread(void);
void chat_reset_unread(void);

/* empty the log */
void chat_clear(void);


int chat_transport_count(void);
const chat_transport_t * chat_transport_get(int i);
int chat_transport_index(const chat_transport_t * t);
void chat_add_transport(chat_transport_t * t);

const char * chat_room(void);
const char * chat_identity(void);
void chat_set_identity(const char * name);
void chat_set_room(const char * room);

/* how many peers the fips daemon knows about */
int chat_fips_peer_count(void);

/* The devices the mesh has been heard from - the nodes of the mesh map.
   A peer that has been quiet for a few minutes is gone. */
#define CHAT_PEER_MAX 12
typedef struct {
    char name[CHAT_NAME_MAX];
    char addr[40];
    int  port;              /* the port the peer was heard on */
    time_t last;
} chat_peer_t;

/* a device the mesh has heard from. The port matters: the mesh talks to a
   peer directly on that port, not only through the broadcast. */
void chat_note_peer(const char * addr, int port, const char * name);

/* the subnets the local net walks looking for the other devices
   ("192.168.1.0/24", or "off") */
const char * net_scan_text(char * out, size_t outn);

/* what the bluetooth radio is (address and channel, or "no adapter") */
const char * bit_info_text(char * out, size_t outn);
int chat_peer_count(void);
const chat_peer_t * chat_peer_at(int i);

/* Build the canonical NIP-01 serialization of an event: the array the id
   is the SHA-256 of.
      [0,"<pubkey hex>",<created_at>,<kind>,<tags>,<content>]
   compact, UTF-8, tags in the order they appear; strings inside are
   escaped (bytes >=0x80 pass through raw), which is symmetric by
   construction: the signer escapes its logical values, the verifier
   escapes the values it parsed back out of the line. */
bool chat_event_serialize(char * out, size_t outn, const char * pubkey_hex,
                          long created_at, long kind, const char * tags_json,
                          const char * content);

/* Serialize -> hash -> sign -> the full event line. With no usable key it
   builds the unsigned shape instead (pubkey "", no sig field): same call,
   one caller, no second code path. Returns false only if serialization,
   the id hash or the buffers fail - then chat_send keeps the message
   locally with a make_id() row id and sends nothing half-built. */
bool chat_event_build(char * out, size_t outn, char * id_out /* 65 */,
                      long created_at, const char * text);

/* what a carrier is doing, in words */
const char * chat_carrier_state_name(chat_carrier_state_t st);

/* The channel the local mesh talks on ("mesh" by default, shown as #mesh).
   The mesh carrier only carries the messages of its own channel. */
void chat_set_mesh_channel(const char * chan);
const char * chat_mesh_channel(void);

/* The speakers heard this session, keyed by pubkey (the SEEN section of
   the Contacts tab). Only events that verified - CHAT_SIG_OK - are noted,
   never our own key. Session-only: nothing here is persisted. */
typedef struct {
    char    pub[65];
    char    name[CHAT_NAME_MAX];
    time_t  last;
} chat_speaker_t;

void chat_note_speaker(const char * pub, const char * name);
const chat_speaker_t * chat_speaker_at(int i);
int  chat_speaker_count(void);
/* bumped on note, update, LRU eviction and quiet-timeout removal - the
   SEEN change signal the Contacts tab's refresh timer watches */
unsigned chat_speaker_version(void);

#endif
