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
 * The wire format is the same on all three: one JSON object per message.
 *
 *      {"v":1,"t":"msg","id":"7f2a91","from":"Satscraft",
 *       "room":"SECTOR","text":"anyone got the drop coords?","ts":1730000000}
 */

#define CHAT_MAX_MSG       300
#define CHAT_TEXT_MAX      240
#define CHAT_NAME_MAX      32
#define CHAT_ID_MAX        24
#define CHAT_ROOM_MAX      24
#define CHAT_TRANSPORT_MAX 4

typedef enum {
    CHAT_KIND_SYS = 0,   /* a status line written by the bus itself */
    CHAT_KIND_MSG = 1    /* something somebody said */
} chat_kind_t;

typedef struct {
    char id[CHAT_ID_MAX];
    char sender[CHAT_NAME_MAX];
    char text[CHAT_TEXT_MAX];
    chat_kind_t kind;
    int transport;        /* index into the transport list, -1 for local */
    bool outbound;        /* typed here, not received */
    time_t ts;
} chat_msg_t;

/* A carrier. The bus never blocks on one: poll() must return quickly. */
typedef struct chat_transport {
    const char * tag;         /* short label shown in the UI: "NET" */
    const char * label;       /* long label: "Internet" */
    bool (*connect)(struct chat_transport * t);
    void (*disconnect)(struct chat_transport * t);
    int  (*poll)(struct chat_transport * t);   /* deliver what arrived */
    bool (*send)(struct chat_transport * t, const char * text, const char * id);
    bool up;                  /* the carrier is usable right now */
    char status[48];          /* one line for the UI */
    void * priv;
} chat_transport_t;

void chat_init(void);
void chat_poll(void);

/* Say something: goes out on every carrier that is up, and shows up in the
   local log. Returns false when there was nothing to send. */
bool chat_send(const char * text);

int chat_count(void);
const chat_msg_t * chat_get(int i);          /* 0 = oldest */
const chat_msg_t * chat_newest(void);

void chat_deliver(int transport, const char * id, const char * sender,
                  const char * text);
void chat_system(const char * text);

int  chat_unread(void);
void chat_reset_unread(void);

int chat_transport_count(void);
const chat_transport_t * chat_transport_get(int i);
int chat_transport_index(const chat_transport_t * t);
void chat_add_transport(chat_transport_t * t);

const char * chat_room(void);
const char * chat_identity(void);
void chat_set_identity(const char * name);
void chat_set_room(const char * room);

/* Build the wire envelope for a message. */
void chat_envelope(char * out, size_t outn, const char * id, const char * sender,
                   const char * text, long ts);

#endif
