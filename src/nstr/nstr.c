#include "nstr/nstr_int.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* The nstr relay client: the URL, the bounded-step connection machine
   (TCP -> TLS -> WebSocket handshake -> up), the inbound frame parser and
   the NIP-01 message layer. Stepped from the LVGL tick like every carrier;
   no thread, no malloc in the message path, one blocking call per attempt
   (the resolve, inside nstr_open()).

   It knows nothing about nostr: a message in is one complete WebSocket
   text message (a JSON array) in the ring, a message out is a JSON array
   the caller built. No subscriptions, no reconnect, no room filter - the
   callers own those, which is what lets three callers use one module. */

#define NDBG(...) do { if(getenv("PDA_DEBUG")) fprintf(stderr, __VA_ARGS__); } while(0)

static struct nstr s_pool[NSTR_CONN_MAX];

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* ---------- lifecycle ---------- */

static void teardown(nstr_t * c)
{
    nstr_tls_free(c);
    if(c->fd >= 0) { close(c->fd); c->fd = -1; }
    if(c->res) { freeaddrinfo(c->res); c->res = NULL; }
}

/* terminal: the reason is what the carrier will show, the handle is dead */
static bool fail(nstr_t * c, const char * why)
{
    snprintf(c->reason, sizeof(c->reason), "%s", why ? why : "disconnected");
    c->state = NSTR_FAILED;
    teardown(c);
    return false;
}

/* an I/O error mid-flight, named by where it happened: the peer going away
   is "disconnected" everywhere, a TLS-layer failure while handshaking is
   "tls failed" (spec §5's table) */
static const char * io_fail_reason(nstr_t * c, bool eof)
{
    if(eof) return "disconnected";
    if(c->stage == NS_TLS) return "tls failed";
    return "disconnected";
}

/* ---------- the URL ---------- */

/* wss://host[:port][/path] | ws://...  - reject, never truncate:
   uppercase scheme, no scheme, a bare host, userinfo, a query or fragment,
   an empty/0/>65535 port, a host over 127 bytes, a path over 63 bytes.
   IPv6 comes in brackets (wss://[::1]:443) and keeps them for the Host
   header and the label; the dial always uses the full parsed host. */
static bool parse_url(nstr_t * c, const char * url)
{
    if(!url) return false;
    const char * p = url;
    if(strncmp(p, "wss://", 6) == 0) { c->tls = true; p += 6; }
    else if(strncmp(p, "ws://", 5) == 0) { c->tls = false; p += 5; }
    else return false;

    if(strpbrk(p, "?#")) return false;              /* no query, no fragment */

    const char * hs;
    size_t hl;
    if(*p == '[') {
        const char * e = strchr(p, ']');
        if(!e) return false;
        hs = p + 1;
        hl = (size_t)(e - hs);
        p = e + 1;
        if(*p && *p != ':' && *p != '/') return false;
    }
    else {
        hs = p;
        while(*p && *p != ':' && *p != '/') p++;
        hl = (size_t)(p - hs);
        for(size_t i = 0; i < hl; i++)
            if(hs[i] == '@') return false;         /* userinfo is rejected */
    }
    if(hl == 0 || hl > NSTR_HOST_MAX) return false;
    memcpy(c->host, hs, hl);
    c->host[hl] = 0;

    c->port = c->tls ? 443 : 80;
    if(*p == ':') {
        p++;
        if(*p < '0' || *p > '9') return false;     /* empty port */
        int port = 0;
        while(*p >= '0' && *p <= '9') {
            port = port * 10 + (*p - '0');
            if(port > 65535) return false;
            p++;
        }
        if(port == 0) return false;
        c->port = port;
    }

    if(*p == 0) {
        snprintf(c->path, sizeof(c->path), "/");
    }
    else if(*p == '/') {
        size_t n = strlen(p);
        if(n > NSTR_PATH_MAX) return false;        /* reject, never truncate */
        memcpy(c->path, p, n + 1);
    }
    else return false;                             /* something we do not dial */

    struct in_addr v4;
    struct in6_addr v6;
    c->ip_literal = inet_pton(AF_INET, c->host, &v4) == 1 ||
                    inet_pton(AF_INET6, c->host, &v6) == 1;

    /* Host header and the display label are one and the same string: the
       host, brackets around an IPv6 literal, ":port" when it is not the
       scheme's default. No scheme, ever. */
    bool brackets = strchr(c->host, ':') != NULL;
    int def = c->tls ? 443 : 80;
    if(c->port == def)
        snprintf(c->host_hdr, sizeof(c->host_hdr), "%s%s%s",
                 brackets ? "[" : "", c->host, brackets ? "]" : "");
    else
        snprintf(c->host_hdr, sizeof(c->host_hdr), "%s%s%s:%d",
                 brackets ? "[" : "", c->host, brackets ? "]" : "", c->port);
    snprintf(c->label, sizeof(c->label), "%s", c->host_hdr);
    return true;
}

/* the one blocking call of an attempt - the house precedent (net.c's
   resolver), bounded by the resolver itself. The attempt deadline starts
   when nstr_open() returns, so a slow resolver can never eat it. */
static bool resolve(nstr_t * c)
{
    char portbuf[16];
    struct addrinfo hints, * res = NULL;
    snprintf(portbuf, sizeof(portbuf), "%d", c->port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(c->host, portbuf, &hints, &res) != 0 || !res) {
        if(res) freeaddrinfo(res);
        return false;
    }
    c->res = res;
    return true;
}

nstr_t * nstr_open(const char * url)
{
    nstr_t * c = NULL;
    for(int i = 0; i < NSTR_CONN_MAX; i++)
        if(!s_pool[i].used) { c = &s_pool[i]; break; }
    if(!c) return NULL;                            /* full: never evict one */

    memset(c, 0, sizeof(*c));
    c->used = true;
    c->fd = -1;
    c->state = NSTR_IDLE;
    c->stage = NS_TCP;
    c->hdr_need = 2;
    c->want = POLLIN;
    c->last_io_ms = now_ms();

    if(!parse_url(c, url)) {
        c->label[0] = 0;
        fail(c, "bad url");
        return c;
    }
    if(!resolve(c)) { fail(c, "no host"); return c; }
    if(c->tls && !nstr_tls_ca_load()) { fail(c, "no CA"); return c; }

    c->open_ms = now_ms();                         /* ... the deadline starts here */
    return c;
}

void nstr_close(nstr_t * c)
{
    if(!c || !c->used) return;
    int n = c->outq_n + (c->wframe_off < c->wframe_len ? 1 : 0);
    if(n > 0) NDBG("[nstr] queue discarded %d\n", n);
    teardown(c);
    c->used = false;
}

/* ---------- the transport under the frames ---------- */

/* >0 bytes written, 0 = wants io, -1 = failed. TLS goes through OpenSSL,
   plain sockets through send(MSG_NOSIGNAL) - the process ignores SIGPIPE
   as well, so a vanished peer is an error, never a dead app. */
static int wr(nstr_t * c, const void * buf, int n)
{
    if(n <= 0) return 0;
    if(c->tls && c->ssl) return nstr_tls_write(c, buf, n);
    ssize_t r = send(c->fd, buf, (size_t)n, MSG_NOSIGNAL);
    if(r > 0) return (int)r;
    if(r == 0) return 0;
    if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
    return -1;
}

/* >0 bytes read, 0 = wants io, -1 = failed, -2 = the peer closed */
static int rd(nstr_t * c, void * buf, int n)
{
    if(n <= 0) return 0;
    if(c->tls && c->ssl) return nstr_tls_read(c, buf, n);
    ssize_t r = recv(c->fd, buf, (size_t)n, 0);
    if(r > 0) return (int)r;
    if(r == 0) return -2;
    if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
    return -1;
}

static bool out_pending(const nstr_t * c)
{
    return c->outq_n > 0 || c->wframe_off < c->wframe_len;
}

static bool feed(nstr_t * c, const unsigned char * data, int n, int * used,
                 bool * stopped);
static bool frame_done(nstr_t * c);

/* ---------- the stages ---------- */

static bool ws_req_begin(nstr_t * c);

static const char * tcp_reason(int e)
{
    switch(e) {
        case ECONNREFUSED: return "refused";
        case ENETUNREACH:
        case EHOSTUNREACH:
        case ENETDOWN:
        case EADDRNOTAVAIL:
            return "no link";
        default: return "disconnected";
    }
}

static bool tcp_done(nstr_t * c)
{
    if(c->res) { freeaddrinfo(c->res); c->res = NULL; }
    if(c->tls) {
        c->stage = NS_TLS;
        c->want = POLLIN;
        c->entry = true;
        return true;
    }
    return ws_req_begin(c);
}

static bool tcp_start(nstr_t * c)
{
    c->state = NSTR_CONNECTING;
    if(!c->res) return fail(c, "no host");

    int fd = socket(c->res->ai_family, SOCK_STREAM, 0);
    if(fd < 0) return fail(c, "disconnected");
    set_nonblock(fd);
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    if(connect(fd, c->res->ai_addr, c->res->ai_addrlen) == 0) {
        c->fd = fd;
        return tcp_done(c);
    }
    if(errno == EINPROGRESS) { c->fd = fd; return true; }
    int e = errno;
    close(fd);
    return fail(c, tcp_reason(e));
}

static bool tcp_complete(nstr_t * c, bool * progressed)
{
    int err = 0;
    socklen_t elen = sizeof(err);
    if(getsockopt(c->fd, SOL_SOCKET, SO_ERROR, &err, &elen) != 0)
        return fail(c, "disconnected");
    if(err != 0) return fail(c, tcp_reason(err));
    *progressed = true;
    return tcp_done(c);
}

static bool ws_req_begin(nstr_t * c)
{
    if(!ws_key_new(c->ws_key, sizeof(c->ws_key))) return fail(c, "bad ws");
    int n = ws_request(c->ws_req, sizeof(c->ws_req), c->path, c->host_hdr, c->ws_key);
    if(n < 0) return fail(c, "bad ws");
    c->ws_req_len = n;
    c->ws_req_off = 0;
    c->resp_n = 0;
    c->stage = NS_WS_REQ;
    c->entry = true;
    return true;
}

static bool ws_req_step(nstr_t * c, bool * progressed)
{
    while(c->ws_req_off < c->ws_req_len) {
        int n = wr(c, c->ws_req + c->ws_req_off, c->ws_req_len - c->ws_req_off);
        if(n < 0) return fail(c, io_fail_reason(c, false));
        if(n == 0) return true;
        c->ws_req_off += n;
        c->last_io_ms = now_ms();
        *progressed = true;
    }
    c->stage = NS_WS_RESP;
    c->entry = true;
    *progressed = true;
    return true;
}

static bool ws_resp_step(nstr_t * c, bool * progressed)
{
    char buf[512];
    int n = rd(c, buf, sizeof(buf));
    if(n == 0) return true;
    if(n < 0) return fail(c, io_fail_reason(c, n == -2));
    if(c->resp_n + n > NSTR_RESP_MAX) return fail(c, "bad ws");

    memcpy(c->resp + c->resp_n, buf, (size_t)n);
    c->resp_n += n;
    c->resp[c->resp_n] = 0;
    c->last_io_ms = now_ms();
    *progressed = true;

    char * end = strstr(c->resp, "\r\n\r\n");
    if(!end) return true;                          /* the rest is coming */

    if(!ws_response_ok(c->resp, c->ws_key)) return fail(c, "bad ws");

    c->stage = NS_UP;
    c->entry = true;
    c->state = NSTR_UP;

    /* the upgrade response and the first frames can share one segment:
       stage what came after the header for the normal read path */
    int used = (int)(end + 4 - c->resp);
    if(c->resp_n > used) {
        int left = c->resp_n - used;
        if(left > (int)sizeof(c->rin)) return fail(c, "bad ws");
        memcpy(c->rin, c->resp + used, (size_t)left);
        c->rin_n = left;
    }
    c->resp_n = 0;
    return c->state != NSTR_FAILED;
}

static bool tls_step(nstr_t * c, bool * progressed)
{
    if(!c->ssl && !nstr_tls_begin(c)) return fail(c, "tls failed");
    int r = nstr_tls_handshake(c);
    if(r < 0) return fail(c, "tls failed");
    *progressed = true;                            /* done, or io is pending */
    if(r == 1) return ws_req_begin(c);
    return true;
}

/* ---------- the inbound frame parser ---------- */

static bool deliver(nstr_t * c)
{
    int n = c->msg_len;
    c->msg[n] = 0;
    c->msg_len = 0;

    /* one complete message is one NIP-01 array - anything else never
       enters the ring, so it never counts against the 16-message cap and
       can never fail the connection */
    const char * p = c->msg;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if(*p != '[') {
        NDBG("[nstr] bad message\n");
        return true;
    }
    if(c->ring_n >= NSTR_RING) {
        NDBG("[nstr] queue full\n");               /* a burst must never tear the link */
        return true;
    }
    int slot = (c->ring_head + c->ring_n) % NSTR_RING;
    memcpy(c->ring[slot], c->msg, (size_t)n + 1);
    c->ring_n++;
    return true;
}

static bool frame_done(nstr_t * c)
{
    int op = c->opcode;
    if(op == 0x8) {                                /* close: reply in kind, then go */
        unsigned char cf[8];
        int n = ws_frame(cf, sizeof(cf), 0x8, c->ctl, (size_t)c->plen, true);
        if(n > 0) { int w = wr(c, cf, n); (void)w; }
        return fail(c, "disconnected");
    }
    if(op == 0x9) {                                /* ping -> pong, same payload */
        unsigned char pf[132];
        int n = ws_frame(pf, sizeof(pf), 0xA, c->ctl, (size_t)c->plen, true);
        if(n > 0) { int w = wr(c, pf, n); (void)w; }
        return true;
    }
    if(op == 0xA) return true;                     /* pong: consumed, never surfaced */
    if(c->fin) return deliver(c);
    return true;                                   /* a fragment: keep accumulating */
}

/* the full header is in c->hdr: validate it and set up the payload phase.
   Every violation here is an honest disconnect with "bad ws" (§5's table),
   never a stall. */
static bool frame_header(nstr_t * c)
{
    const unsigned char * h = c->hdr;
    size_t need = 2;

    if(h[0] & 0x70) return fail(c, "bad ws");      /* RSV: no extension offered */
    if(h[1] & 0x80) return fail(c, "bad ws");      /* a server never masks */

    long long len = h[1] & 0x7f;
    if(len == 126) need = 4;
    else if(len == 127) need = 10;
    if(c->hdr_n < (int)need) {                     /* the length is still coming */
        c->hdr_need = (int)need;
        return true;
    }
    if(len == 126) len = ((long long)h[2] << 8) | h[3];
    else if(len == 127) {
        len = 0;
        for(int i = 2; i < 10; i++) len = (len << 8) | h[i];
        if(len < 0) return fail(c, "bad ws");
    }

    int op = h[0] & 0x0f;
    bool fin = (h[0] & 0x80) != 0;
    c->fin = fin;
    c->opcode = op;

    if(op == 0x8 || op == 0x9 || op == 0xA) {      /* control frame */
        if(!fin || len > 125) return fail(c, "bad ws");
        c->pay_is_ctl = true;
    }
    else if(op == 0x0 || op == 0x1) {              /* continuation / text */
        if(op == 0x1 && c->msg_len != 0) return fail(c, "bad ws");
        if(op == 0x0 && c->msg_len == 0) return fail(c, "bad ws");
        c->pay_is_ctl = false;
        if(c->msg_len + len > NSTR_MSG_MAX - 1) return fail(c, "bad ws");
    }
    else return fail(c, "bad ws");                 /* binary and the rest are out */

    c->plen = len;
    c->pay_left = len;
    c->hdr_n = 0;
    c->hdr_need = 2;
    c->in_payload = true;
    return true;
}

/* Parse data[0..n). Stops at a frame boundary when the completed-message
   ring is full (*stopped, *used short of n): the caller stashes the
   remainder and the rest of the burst waits in the socket - the ring is
   exactly one tick's drain, so a burst beyond it must never be dropped at
   the ring when the kernel is still holding it. Returns false on a
   terminal frame error (the connection is then dead). */
static bool feed(nstr_t * c, const unsigned char * data, int n, int * used,
                 bool * stopped)
{
    int i = 0;
    *stopped = false;
    while(i < n) {
        if(c->ring_n >= NSTR_RING) { *stopped = true; break; }

        if(!c->in_payload) {
            int take = c->hdr_need - c->hdr_n;
            if(take > n - i) take = n - i;
            memcpy(c->hdr + c->hdr_n, data + i, (size_t)take);
            c->hdr_n += take;
            i += take;
            if(c->hdr_n < c->hdr_need) break;        /* the header is still coming */
            if(!frame_header(c)) { *used = i; return false; }
            if(c->in_payload && c->pay_left == 0) {  /* a header with no payload */
                c->in_payload = false;
                if(!frame_done(c)) { *used = i; return false; }
            }
            continue;
        }

        long long take = c->pay_left;
        if(take > (long long)(n - i)) take = n - i;
        if(take > 0) {
            if(c->pay_is_ctl)
                memcpy(c->ctl + (int)(c->plen - c->pay_left), data + i, (size_t)take);
            else {
                memcpy(c->msg + c->msg_len, data + i, (size_t)take);
                c->msg_len += (int)take;
            }
            c->pay_left -= take;
            i += (int)take;
        }
        if(c->pay_left == 0) {
            c->in_payload = false;
            if(!frame_done(c)) { *used = i; return false; }
        }
    }
    *used = i;
    return c->state != NSTR_FAILED;
}

/* ---------- writing ---------- */

/* hand the socket whatever is queued: pop the next message into a frame and
   write it as far as the socket takes it. Returns false on a hard write
   error (the caller names it "disconnected"). */
static bool flush_out(nstr_t * c, bool * progressed)
{
    for(;;) {
        if(c->wframe_off >= c->wframe_len) {
            c->wframe_off = 0;
            c->wframe_len = 0;
            if(c->outq_n == 0) return true;
            int idx = c->outq_head;
            int len = ws_frame(c->wframe, sizeof(c->wframe), 0x1,
                               c->outq[idx], strlen(c->outq[idx]), true);
            c->outq_head = (c->outq_head + 1) % NSTR_OUTQ;
            c->outq_n--;
            if(len < 0) continue;                  /* cannot happen: 4095 cap */
            c->wframe_len = len;
        }
        int n = wr(c, c->wframe + c->wframe_off, c->wframe_len - c->wframe_off);
        if(n < 0) return false;
        if(n == 0) return true;
        c->wframe_off += n;
        c->last_io_ms = now_ms();
        *progressed = true;
    }
}

/* one non-blocking round of the current stage. *progressed says whether
   anything moved, which is how nstr_step() spots a stall. */
/* stage what is already read, then read more while the ring has room */
static bool pump_in(nstr_t * c, bool * progressed)
{
    for(;;) {
        int used = 0;
        bool stopped = false;
        if(!feed(c, c->rin, c->rin_n, &used, &stopped)) return false;
        if(used > 0) {
            *progressed = true;
            if(used < c->rin_n)
                memmove(c->rin, c->rin + used, (size_t)(c->rin_n - used));
            c->rin_n -= used;
        }
        if(stopped || c->ring_n >= NSTR_RING) return true;   /* wait a tick */
        if(c->rin_n == (int)sizeof(c->rin)) return true;      /* full frame pending */

        int space = (int)sizeof(c->rin) - c->rin_n;
        int n = rd(c, c->rin + c->rin_n, space);
        if(n == 0) return true;                    /* wants io */
        if(n < 0) return fail(c, io_fail_reason(c, n == -2));
        c->rin_n += n;
        c->last_io_ms = now_ms();
        *progressed = true;
    }
}

static bool up_work(nstr_t * c, bool * progressed)
{
    if(!flush_out(c, progressed)) return fail(c, "disconnected");
    if(!pump_in(c, progressed)) return false;

    /* quiet for 60 s: one ping, silently - the harness's pong log is how a
       test observes it (§3) */
    if(now_ms() - c->last_io_ms >= (long long)NSTR_PING_SEC * 1000) {
        unsigned char pf[8];
        int n = ws_frame(pf, sizeof(pf), 0x9, NULL, 0, true);
        if(n > 0 && wr(c, pf, n) > 0) c->last_io_ms = now_ms();
    }
    return true;
}

static bool do_work(nstr_t * c, bool * progressed)
{
    *progressed = false;
    switch(c->stage) {
        case NS_TCP:     return tcp_complete(c, progressed);
        case NS_TLS:     return tls_step(c, progressed);
        case NS_WS_REQ:  return ws_req_step(c, progressed);
        case NS_WS_RESP: return ws_resp_step(c, progressed);
        case NS_UP:      return up_work(c, progressed);
    }
    return fail(c, "disconnected");
}

static short stage_events(const nstr_t * c)
{
    short ev = POLLIN | POLLERR | POLLHUP;
    switch(c->stage) {
        case NS_TCP:
        case NS_WS_REQ:  return POLLOUT | POLLERR | POLLHUP;
        case NS_TLS:     return (short)(c->want | POLLERR | POLLHUP);
        case NS_WS_RESP: return POLLIN | POLLERR | POLLHUP;
        case NS_UP:      return out_pending(c) ? (short)(ev | POLLOUT) : ev;
    }
    return ev;
}

bool nstr_step(nstr_t * c, int budget_ms)
{
    if(!c || !c->used) return false;
    if(c->state == NSTR_FAILED) return false;
    if(c->state == NSTR_IDLE) { c->state = NSTR_CONNECTING; c->entry = true; }

    long long until = now_ms() + (budget_ms > 0 ? budget_ms : 0);
    for(;;) {
        /* starting the connect needs no readiness signal */
        if(c->stage == NS_TCP && c->fd < 0) {
            if(!tcp_start(c)) return false;
            continue;
        }

        int timeout = 0;
        if(budget_ms > 0) {
            long long rem = until - now_ms();
            if(rem <= 0) break;
            timeout = (int)rem;
        }

        bool progressed = false;
        /* Two things need a work round even when poll() has nothing to say:
           a stage that has not run yet (the handshake's first call), and
           bytes already staged in rin that last tick could not parse because
           the ring was full. Without the second one the tail of a burst can
           sit in rin forever: the socket it came from is empty by then, so
           poll() never reports readable again. */
        if(c->entry || (c->stage == NS_UP && c->rin_n > 0)) {
            c->entry = false;
            if(!do_work(c, &progressed)) return false;
            if(progressed) continue;
        }

        struct pollfd p = { .fd = c->fd, .events = stage_events(c), .revents = 0 };
        int pr = poll(&p, 1, timeout);
        if(pr < 0) {
            if(errno == EINTR) continue;
            return fail(c, "disconnected");
        }
        if(pr == 0) break;                          /* the budget ran out / nothing yet */
        if(!do_work(c, &progressed)) return false;
        if(!progressed) break;                      /* progress stalls */
    }

    /* one deadline for the whole attempt, whatever stage stalled in it */
    if(c->state != NSTR_UP && now_ms() - c->open_ms >= NSTR_DEADLINE_MS)
        return fail(c, "no reply");
    return c->state != NSTR_FAILED;
}

/* ---------- the message layer ---------- */

bool nstr_send(nstr_t * c, const char * client_msg)
{
    bool ok = c && c->used && c->state == NSTR_UP && client_msg;
    if(ok) {
        size_t n = strlen(client_msg);
        if(n >= NSTR_MSG_MAX) ok = false;                       /* > 4095: never exists */
        else if(c->outq_n + (c->wframe_off < c->wframe_len ? 1 : 0) >= NSTR_OUTQ)
            ok = false;                                         /* the queue is full */
        else {
            int slot = (c->outq_head + c->outq_n) % NSTR_OUTQ;
            memcpy(c->outq[slot], client_msg, n + 1);
            c->outq_n++;
            return true;
        }
    }
    NDBG("[nstr] send refused\n");
    return false;
}

bool nstr_recv(nstr_t * c, char * out, size_t outn)
{
    if(!c || !c->used || !out || outn == 0) return false;
    if(c->ring_n == 0) return false;

    if(outn < NSTR_MSG_MAX) {
        /* a caller that under-sizes gets the drop, not truncated JSON */
        NDBG("[nstr] recv dropped\n");
        c->ring_head = (c->ring_head + 1) % NSTR_RING;
        c->ring_n--;
        return false;
    }
    memcpy(out, c->ring[c->ring_head], strlen(c->ring[c->ring_head]) + 1);
    c->ring_head = (c->ring_head + 1) % NSTR_RING;
    c->ring_n--;
    return true;
}

nstr_state_t nstr_state(const nstr_t * c)
{
    return c ? c->state : NSTR_FAILED;
}

const char * nstr_reason(const nstr_t * c)
{
    return c ? c->reason : "disconnected";
}

const char * nstr_label(const nstr_t * c)
{
    return c ? c->label : "";
}
