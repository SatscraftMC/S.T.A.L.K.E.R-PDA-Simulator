#include "chat/chat.h"
#include "chat/json.h"
#include "chat/linesock.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/* BIT - the local mesh: no infrastructure, no server, just the devices that
 * are within earshot of each other.
 *
 * The carrier is UDP broadcast on port 2425 (the port the bitchat mesh uses),
 * so two PDAs on the same WiFi or LAN hear each other without anyone running
 * a thing. There is no infrastructure and no address book: whoever is in range
 * is in the room.
 *
 * A real bitchat node can be wired in through a bridge: point
 * PDA_BITCHAT_SOCK (a unix socket) or PDA_BITCHAT_HOST/PDA_BITCHAT_PORT at
 * it and the bridge carries the same envelope in and out of the real mesh.
 * tools/bitchat_bridge.py is the reference bridge.
 */

#define BIT_PORT 2425

typedef struct {
    int udp;
    int port;
    ls_t bridge;            /* optional link to a real bitchat node */
    bool bridge_want;
    time_t last_try;
    int backoff;
    struct in_addr bcast[8];    /* where the mesh message goes */
    int n_bcast;
} bit_t;

static bit_t s_bit;
static chat_transport_t s_tr;

/* the addresses a mesh message goes to: every interface's broadcast address,
   the limited broadcast, and the loopback (so two PDAs on the same machine
   hear each other - the kernel does not hand a local broadcast back) */
static void collect_broadcasts(bit_t * b)
{
    b->n_bcast = 0;
    struct ifaddrs * ifa = NULL;
    if(getifaddrs(&ifa) == 0) {
        for(struct ifaddrs * p = ifa; p; p = p->ifa_next) {
            if(!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
            if(!(p->ifa_flags & IFF_BROADCAST)) continue;
            struct sockaddr_in * sa = (struct sockaddr_in *)p->ifa_addr;
            struct sockaddr_in * br = (struct sockaddr_in *)p->ifa_broadaddr;
            if(!br) continue;
            bool have = false;
            for(int i = 0; i < b->n_bcast; i++)
                if(b->bcast[i].s_addr == br->sin_addr.s_addr) have = true;
            if(!have && b->n_bcast < 6) b->bcast[b->n_bcast++] = br->sin_addr;
            (void)sa;
        }
        freeifaddrs(ifa);
    }
    struct in_addr all;
    all.s_addr = htonl(INADDR_BROADCAST);
    bool have_all = false;
    for(int i = 0; i < b->n_bcast; i++)
        if(b->bcast[i].s_addr == all.s_addr) have_all = true;
    if(!have_all && b->n_bcast < 8) b->bcast[b->n_bcast++] = all;

    struct in_addr loop;
    loop.s_addr = htonl(INADDR_LOOPBACK);
    bool have_loop = false;
    for(int i = 0; i < b->n_bcast; i++)
        if(b->bcast[i].s_addr == loop.s_addr) have_loop = true;
    if(!have_loop && b->n_bcast < 8) b->bcast[b->n_bcast++] = loop;
}

static void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static bool bridge_connect(bit_t * b)
{
    const char * sock = getenv("PDA_BITCHAT_SOCK");
    const char * host = getenv("PDA_BITCHAT_HOST");
    int port = getenv("PDA_BITCHAT_PORT") ? atoi(getenv("PDA_BITCHAT_PORT")) : 0;

    ls_init(&b->bridge);
    int fd = -1;

    if(sock && *sock) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", sock);
        fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(fd >= 0 && connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            close(fd);
            fd = -1;
        }
    }
    else if(host && *host && port > 0) {
        char portbuf[16];
        snprintf(portbuf, sizeof(portbuf), "%d", port);
        struct addrinfo hints, * res = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if(getaddrinfo(host, portbuf, &hints, &res) == 0 && res) {
            fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if(fd >= 0 && connect(fd, res->ai_addr, res->ai_addrlen) != 0) {
                close(fd);
                fd = -1;
            }
            freeaddrinfo(res);
        }
    }
    if(fd < 0) return false;

    set_nonblock(fd);
    b->bridge.fd = fd;
    b->last_try = time(NULL);
    b->backoff = 0;
    return true;
}

static bool bit_connect(chat_transport_t * t)
{
    bit_t * b = (bit_t *)t->priv;
    if(b->udp < 0) {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(fd < 0) {
            snprintf(t->status, sizeof(t->status), "mesh down");
            return false;
        }
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons((uint16_t)b->port);
        if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            close(fd);
            snprintf(t->status, sizeof(t->status), "port busy");
            return false;
        }
        set_nonblock(fd);
        b->udp = fd;
        collect_broadcasts(b);
    }

    t->state = CHAT_CARRIER_ONLINE;
    snprintf(t->status, sizeof(t->status), "mesh %d", b->port);

    if(b->bridge_want && b->bridge.fd < 0) {
        time_t now = time(NULL);
        if(!b->last_try || now - b->last_try > b->backoff) {
            bridge_connect(b);
            if(b->backoff == 0) b->backoff = 2;
            else if(b->backoff < 30) b->backoff *= 2;
        }
    }
    return true;
}

static void bit_disconnect(chat_transport_t * t)
{
    bit_t * b = (bit_t *)t->priv;
    if(b->udp >= 0) { close(b->udp); b->udp = -1; }
    if(b->bridge.fd >= 0) { close(b->bridge.fd); ls_init(&b->bridge); }
    t->state = CHAT_CARRIER_OFFLINE;
    snprintf(t->status, sizeof(t->status), "mesh down");
}

static void deliver_line(chat_transport_t * t, const char * line)
{
    char type[24] = "";
    json_get(line, "t", type, sizeof(type));
    if(strcmp(type, "msg") != 0) return;

    char id[CHAT_ID_MAX] = "", from[CHAT_NAME_MAX] = "", text[CHAT_TEXT_MAX] = "";
    char chan[CHAT_ROOM_MAX] = "";
    json_get(line, "id", id, sizeof(id));
    json_get(line, "from", from, sizeof(from));
    json_get(line, "text", text, sizeof(text));
    json_get(line, "chan", chan, sizeof(chan));
    if(!text[0]) return;
    /* the mesh carries one channel: what is said elsewhere is not for us */
    if(chan[0] && strcmp(chan, chat_mesh_channel()) != 0) return;
    chat_deliver(chat_transport_index(t), id, from, text);
}

static int bit_poll(chat_transport_t * t)
{
    bit_t * b = (bit_t *)t->priv;
    int got = 0;

    char buf[2048];
    for(;;) {
        ssize_t n = recv(b->udp, buf, sizeof(buf) - 1, MSG_DONTWAIT);
        if(n <= 0) break;
        buf[n] = 0;
        deliver_line(t, buf);
        got++;
    }

    if(b->bridge.fd >= 0) {
        char line[LS_BUF];
        for(;;) {
            int r = ls_read_line(&b->bridge, line, sizeof(line));
            if(r <= 0) {
                if(r < 0) {
                    close(b->bridge.fd);
                    ls_init(&b->bridge);
                }
                break;
            }
            deliver_line(t, line);
            got++;
        }
    }
    return got;
}

static bool bit_send(chat_transport_t * t, const char * text, const char * id)
{
    bit_t * b = (bit_t *)t->priv;
    char env[1024];
    chat_envelope(env, sizeof(env), id, chat_identity(), text, (long)time(NULL));

    for(int i = 0; i < b->n_bcast; i++) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr = b->bcast[i];
        addr.sin_port = htons((uint16_t)b->port);
        if(sendto(b->udp, env, strlen(env), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            if(getenv("PDA_DEBUG"))
                fprintf(stderr, "[chat] bit sendto failed: %s\n", strerror(errno));
        }
    }

    if(b->bridge.fd >= 0) {
        if(ls_write_line(&b->bridge, env) != 0) {
            close(b->bridge.fd);
            ls_init(&b->bridge);
        }
    }
    return true;
}

void chat_bit_register(void)
{
    memset(&s_bit, 0, sizeof(s_bit));
    s_bit.udp = -1;
    s_bit.port = getenv("PDA_BITCHAT_PORT") ? atoi(getenv("PDA_BITCHAT_PORT")) : BIT_PORT;
    if(s_bit.port <= 0) s_bit.port = BIT_PORT;
    ls_init(&s_bit.bridge);
    s_bit.bridge_want = (getenv("PDA_BITCHAT_SOCK") || getenv("PDA_BITCHAT_HOST")) ? true : false;

    s_tr.tag = "BIT";
    s_tr.label = "Local mesh";
    s_tr.connect = bit_connect;
    s_tr.disconnect = bit_disconnect;
    s_tr.poll = bit_poll;
    s_tr.send = bit_send;
    s_tr.priv = &s_bit;
    s_tr.state = CHAT_CARRIER_OFFLINE;
    snprintf(s_tr.status, sizeof(s_tr.status), "mesh down");
    chat_add_transport(&s_tr);
}
