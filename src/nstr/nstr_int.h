#ifndef PDA_NSTR_INT_H
#define PDA_NSTR_INT_H

/* The internals of the nstr relay client, shared between its three
   implementation files:

     nstr.c  the URL parser, the connection state machine, the inbound
             frame parser and the NIP-01 message layer (the ring, the
             outbound queue)
     ws.c    RFC 6455 client framing - the handshake request/response and
             the frame codec (masking uses OpenSSL's RNG, the accept hash
             its SHA-1 and base64)
     tls.c   the OpenSSL call site: one SSL_CTX per process, the per-
             attempt CA load, the stepped handshake and the SSL read/write

   Nothing here allocates: every buffer is fixed and sized by the spec. */

#include "nstr/nstr.h"

#include <netdb.h>
#include <poll.h>
#include <stdbool.h>
#include <stddef.h>

#define NSTR_MSG_MAX      4096     /* one message: reassembly + one ring slot */
#define NSTR_RING         16       /* one full drain of the carrier's per-tick cap */
#define NSTR_OUTQ         2        /* chat send + a wallet REQ while chat is chatty */
#define NSTR_HOST_MAX     127      /* reject longer, never truncate */
#define NSTR_PATH_MAX     63       /* ditto */
#define NSTR_CONN_MAX     4        /* chat + one wallet op + headroom for #4 */
#define NSTR_DEADLINE_MS  20000    /* the attempt deadline, from nstr_open()'s return */
#define NSTR_PING_SEC     60       /* keep-alive when nothing has been sent/received */
#define NSTR_RESP_MAX     2048     /* the WebSocket handshake response */

typedef enum {
    NS_TCP = 0,      /* nonblocking connect in flight */
    NS_TLS,          /* wss:// only: the stepped handshake */
    NS_WS_REQ,       /* writing the HTTP upgrade request */
    NS_WS_RESP,      /* reading and validating the upgrade response */
    NS_UP            /* the pipe: frames in, frames out */
} nstr_stage_t;

struct nstr {
    bool used;
    nstr_state_t state;
    char reason[24];                /* the §5 table's short strings */

    /* the parsed URL: dial with host/port/path, show label */
    bool tls;                       /* wss:// */
    bool ip_literal;                /* the host is an IP: IP-form verify, no SNI */
    char host[NSTR_HOST_MAX + 1];
    int port;
    char path[NSTR_PATH_MAX + 1];
    char host_hdr[NSTR_HOST_MAX + 10];   /* Host: header (brackets + port) */
    char label[NSTR_HOST_MAX + 10];      /* the ONLINE status string */

    /* the attempt */
    struct addrinfo * res;          /* the one blocking call of an attempt */
    int fd;
    void * ssl;                     /* SSL*, owned by tls.c */
    nstr_stage_t stage;
    bool entry;                     /* this stage has not been worked yet */
    long long open_ms;              /* monotonic ms when nstr_open() returned */

    /* the WebSocket handshake */
    char ws_key[32];
    char ws_req[640];
    int ws_req_len;
    int ws_req_off;
    char resp[NSTR_RESP_MAX + 1];
    int resp_n;

    /* the inbound frame parser (server frames are never masked) */
    unsigned char hdr[16];
    int hdr_n;
    int hdr_need;
    bool in_payload;                   /* the header is parsed; bytes go to msg/ctl */
    bool fin;
    int opcode;
    long long plen;
    long long pay_left;
    bool pay_is_ctl;
    char msg[NSTR_MSG_MAX];         /* reassembled message text */
    int msg_len;
    unsigned char ctl[126];         /* ping/close payload */

    /* the read staging area: bytes taken off the socket but not parsed yet,
       because the ring above is one tick's drain and a burst beyond it must
       wait in the kernel buffer instead of being dropped at the ring */
    unsigned char rin[NSTR_MSG_MAX + 16];
    int rin_n;

    /* completed inbound messages, waiting for nstr_recv() */
    char ring[NSTR_RING][NSTR_MSG_MAX];
    int ring_head;
    int ring_n;

    /* outbound: queued messages, plus the one frame being written */
    char outq[NSTR_OUTQ][NSTR_MSG_MAX];
    int outq_head;
    int outq_n;
    unsigned char wframe[NSTR_MSG_MAX + 16];
    int wframe_len;
    int wframe_off;

    short want;                     /* what the TLS layer wants polled */
    long long last_io_ms;           /* the keep-alive clock */
};

/* --- ws.c --- */

/* a fresh random 16-byte key, base64 into out (needs 25 bytes) */
bool ws_key_new(char * out, size_t outn);

/* the HTTP upgrade request; the length written, or -1 (won't fit / no key) */
int ws_request(char * out, size_t outn, const char * path,
               const char * host_hdr, const char * key);

/* the server's upgrade response is a real 101 with our accept back */
bool ws_response_ok(const char * resp, const char * key);

/* one frame, client style: masked, big lengths, FIN per arg. Returns the
   bytes written or -1 when outn is too small (cannot happen for a message
   that passed nstr_send()'s 4095-byte cap). */
int ws_frame(unsigned char * out, size_t outn, int opcode,
             const void * payload, size_t len, bool fin);

/* --- tls.c --- */

/* once per process: the client SSL_CTX with the system store and
   verification on. False only when OpenSSL itself is unusable. */
bool nstr_tls_open(void);

/* per attempt, wss:// only: re-read PDA_NSTR_CA and add that bundle to the
   ctx. False = the bundle is set and will not load -> the attempt's "no CA". */
bool nstr_tls_ca_load(void);

/* per attempt, after TCP: a new SSL on this fd, hostname (or IP) verify
   parameters and SNI. False -> "tls failed". */
bool nstr_tls_begin(struct nstr * c);

/* 1 = handshake done, 0 = needs io (c->want set), -1 = failed */
int nstr_tls_handshake(struct nstr * c);

/* >0 = bytes, 0 = needs io (c->want set), -1 = failed */
int nstr_tls_read(struct nstr * c, void * buf, int n);
int nstr_tls_write(struct nstr * c, const void * buf, int n);

void nstr_tls_free(struct nstr * c);

#endif
