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
 * The broadcast alone is not enough, though: a broadcast is the one packet
 * that is dropped first - by a firewall that only lets the established
 * traffic through, by an access point that does not forward broadcasts
 * between the wired and the wireless side, by a router that puts the two on
 * different bridges. One of those and the message is gone with nothing to
 * show for it, while the other direction (the one that only has to leave
 * the machine) works fine - which is exactly the kind of bug that looks
 * like the app is broken in one direction only.
 *
 * So the mesh talks to the peers it knows *directly* as well: every device
 * that has been heard is remembered with its address, and the messages go
 * out to those addresses one by one, next to the broadcast. The peers are
 * found by walking the local subnet: a small "hi" goes to every address of
 * the network the machine is on, and the device that answers says who it is
 * and which port it listens on. Talking to a peer directly also keeps the
 * flow open in the firewall of both machines, so the traffic that comes
 * back is the traffic the firewall already knows.
 *
 * A real bitchat node can be wired in through a bridge: point
 * PDA_BITCHAT_SOCK (a unix socket) or PDA_BITCHAT_HOST/PDA_BITCHAT_PORT at
 * it and the bridge carries the same signed event line in and out of the
 * real mesh.
 * tools/bitchat_bridge.py is the reference bridge.
 */

#define BIT_PORT 2425

/* the discovery sweep: the local subnet is walked every BIT_SWEEP_SEC
   seconds, a few addresses per poll, so a /24 is covered in a few seconds
   without flooding the network */
#define BIT_SWEEP_SEC   60
#define BIT_PING_SEC    10
#define BIT_PROBES      8
#define BIT_SCAN_MAX    4
#define BIT_SCAN_HOSTS  1024

typedef struct {
    struct in_addr net;     /* the first address of the subnet */
    struct in_addr mask;
    uint32_t hosts;         /* the addresses to walk (network and broadcast left out) */
} scan_t;

typedef struct {
    int udp;
    int port;
    ls_t bridge;            /* optional link to a real bitchat node */
    bool bridge_want;
    time_t last_try;
    int backoff;
    struct in_addr bcast[8];    /* where the mesh message goes */
    int n_bcast;
    scan_t scan[BIT_SCAN_MAX];  /* the subnets the sweep walks */
    int n_scan;
    int scan_at;            /* the subnet the cursor is in */
    uint32_t scan_host;     /* the address inside it */
    struct in_addr local[8];    /* our own addresses: the sweep skips them */
    int n_local;
    bool have_link;             /* a network we could actually talk on */
    time_t last_refresh;        /* when the interfaces were last looked at */
    time_t next_sweep;
    time_t next_ping;
    bool scan_off;
} bit_t;

static bit_t s_bit;
static chat_transport_t s_tr;

/* the addresses a mesh message goes to: every interface's broadcast address,
   the limited broadcast, and the loopback (so two PDAs on the same machine
   hear each other - the kernel does not hand a local broadcast back).
   It also says whether there is a network at all: a machine whose only
   address is the loopback has no radio, and saying ONLINE anyway is how
   "the mesh ate my message" gets reported. */
static void collect_broadcasts(bit_t * b)
{
    b->n_bcast = 0;
    b->have_link = false;
    struct ifaddrs * ifa = NULL;
    if(getifaddrs(&ifa) == 0) {
        for(struct ifaddrs * p = ifa; p; p = p->ifa_next) {
            if(!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
            if(!(p->ifa_flags & IFF_BROADCAST)) continue;
            struct sockaddr_in * sa = (struct sockaddr_in *)p->ifa_addr;
            struct sockaddr_in * br = (struct sockaddr_in *)p->ifa_broadaddr;
            if(!br) continue;
            if(p->ifa_flags & IFF_LOOPBACK) continue;
            bool have = false;
            for(int i = 0; i < b->n_bcast; i++)
                if(b->bcast[i].s_addr == br->sin_addr.s_addr) have = true;
            if(!have && b->n_bcast < 6) b->bcast[b->n_bcast++] = br->sin_addr;
            (void)sa;
            b->have_link = true;    /* an address on a real interface */
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

/* how many addresses a netmask holds, without the network and the
   broadcast address */
static uint32_t host_count(struct in_addr mask)
{
    uint32_t m = ntohl(mask.s_addr);
    uint32_t all = ~m;             /* the size of the network */
    if(all < 3) return 0;
    uint32_t hosts = all - 1;      /* the network and the broadcast are out */
    if(hosts > BIT_SCAN_HOSTS) hosts = BIT_SCAN_HOSTS;
    return hosts;
}

/* our own addresses: the sweep must not walk them (a datagram sent to our
   own address comes straight back to us - the device would take itself for
   a peer of the mesh) */
static bool is_local(const bit_t * b, const struct in_addr * ip)
{
    for(int i = 0; i < b->n_local; i++)
        if(b->local[i].s_addr == ip->s_addr) return true;
    return false;
}

/* the subnets the sweep walks: the ones this machine is on. PDA_BIT_SCAN
   overrides them ("off" to switch the sweep off, or a list of networks:
   "10.0.0.0/24,192.168.1.0/24") - the test uses that to walk a loopback
   range instead of the real network. */
static void collect_scan(bit_t * b)
{
    b->n_scan = 0;
    b->scan_at = 0;
    b->scan_host = 0;
    b->n_local = 0;

    /* our own addresses, every interface (the loopback too) */
    struct ifaddrs * ifa = NULL;
    if(getifaddrs(&ifa) == 0) {
        for(struct ifaddrs * p = ifa; p; p = p->ifa_next) {
            if(!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
            if(b->n_local >= 8) break;
            struct sockaddr_in * sa = (struct sockaddr_in *)p->ifa_addr;
            b->local[b->n_local++] = sa->sin_addr;
        }
        freeifaddrs(ifa);
    }

    const char * env = getenv("PDA_BIT_SCAN");
    if(env && *env) {
        if(strcmp(env, "off") == 0 || strcmp(env, "none") == 0) {
            b->scan_off = true;
            return;
        }
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", env);
        char * save = NULL;
        for(char * tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
            char * slash = strchr(tok, '/');
            int bits = 24;
            if(slash) { *slash = 0; bits = atoi(slash + 1); }
            struct in_addr net, mask;
            if(inet_pton(AF_INET, tok, &net) != 1) continue;
            if(bits <= 0) mask.s_addr = 0;
            else if(bits >= 32) mask.s_addr = 0xffffffffu;
            else mask.s_addr = htonl(0xffffffffu << (32 - bits));
            net.s_addr &= mask.s_addr;
            uint32_t hosts = host_count(mask);
            if(hosts < 1 || b->n_scan >= BIT_SCAN_MAX) continue;
            scan_t * s = &b->scan[b->n_scan++];
            s->net = net;
            s->mask = mask;
            s->hosts = hosts;
        }
        return;
    }

    if(getifaddrs(&ifa) == 0) {
        for(struct ifaddrs * p = ifa; p; p = p->ifa_next) {
            if(!p->ifa_addr || p->ifa_addr->sa_family != AF_INET) continue;
            if(!p->ifa_netmask) continue;
            /* the loopback is not a place where other devices live, and a
               point to point link (a tunnel, a vpn) has no neighbours */
            if(p->ifa_flags & IFF_LOOPBACK) continue;
            if(p->ifa_flags & IFF_POINTOPOINT) continue;
            struct sockaddr_in * sa = (struct sockaddr_in *)p->ifa_addr;
            struct sockaddr_in * nm = (struct sockaddr_in *)p->ifa_netmask;
            struct in_addr mask = nm->sin_addr;
            struct in_addr net;
            net.s_addr = sa->sin_addr.s_addr & mask.s_addr;
            uint32_t hosts = host_count(mask);
            if(hosts < 1 || b->n_scan >= BIT_SCAN_MAX) continue;
            scan_t * s = &b->scan[b->n_scan++];
            s->net = net;
            s->mask = mask;
            s->hosts = hosts;
        }
        freeifaddrs(ifa);
    }
}

static void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* one datagram to one address */
static bool send_to(bit_t * b, const struct in_addr * ip, int port,
                    const char * buf, size_t len)
{
    if(b->udp < 0 || !ip || port <= 0) return false;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *ip;
    addr.sin_port = htons((uint16_t)port);
    if(sendto(b->udp, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if(getenv("PDA_DEBUG"))
            fprintf(stderr, "[chat] bit sendto %s:%d failed: %s\n",
                    inet_ntoa(*ip), port, strerror(errno));
        return false;
    }
    return true;
}

/* the "hi": who we are and which port we listen on. It is how a device is
   found on the local network - and it is answered, so both sides learn each
   other. */
static int hello_packet(char * out, size_t outn, const char * type, int port)
{
    char e_from[CHAT_NAME_MAX * 6];
    json_escape(chat_identity(), e_from, sizeof(e_from));
    int n = snprintf(out, outn,
                     "{\"v\":1,\"t\":\"%s\",\"from\":\"%s\",\"port\":%d,\"ts\":%ld}",
                     type, e_from, port, (long)time(NULL));
    /* the length that is sent must be the length that fits */
    if(n < 0) return 0;
    if((size_t)n >= outn) return (int)outn - 1;
    return n;
}

/* the sweep: a few addresses of the local subnet per poll */
static void scan_step(bit_t * b)
{
    if(b->scan_off || b->n_scan == 0) return;

    time_t now = time(NULL);
    if(now >= b->next_sweep) {
        /* the network may have changed since the last walk */
        collect_scan(b);
        b->scan_at = 0;
        b->scan_host = 0;
        b->next_sweep = now + BIT_SWEEP_SEC;
        if(getenv("PDA_DEBUG")) {
            fprintf(stderr, "[chat] bit: sweeping %d subnet(s)\n", b->n_scan);
        }
    }

    char hello[192];
    int len = hello_packet(hello, sizeof(hello), "hi", b->port);
    if(len <= 0) return;

    for(int i = 0; i < BIT_PROBES; i++) {
        if(b->scan_at >= b->n_scan) break;
        scan_t * s = &b->scan[b->scan_at];
        if(b->scan_host >= s->hosts) {
            b->scan_at++;
            b->scan_host = 0;
            continue;
        }
        struct in_addr ip;
        ip.s_addr = htonl(ntohl(s->net.s_addr) + b->scan_host + 1);
        b->scan_host++;
        if(is_local(b, &ip)) continue;    /* that one is us */
        send_to(b, &ip, b->port, hello, (size_t)len);
    }
}

/* the keep-alive: a "hi" to every peer we know. It keeps the flow open in
   the firewalls of both machines (a firewall that only lets the traffic it
   has already seen through needs this) and it says that we are still here. */
static void ping_peers(bit_t * b)
{
    if(b->udp < 0) return;
    char hello[192];
    int len = hello_packet(hello, sizeof(hello), "hi", b->port);
    if(len <= 0) return;

    int n = chat_peer_count();
    for(int i = 0; i < n; i++) {
        const chat_peer_t * p = chat_peer_at(i);
        if(!p || !p->addr[0]) continue;
        struct in_addr ip;
        if(inet_pton(AF_INET, p->addr, &ip) != 1) continue;
        send_to(b, &ip, p->port > 0 ? p->port : b->port, hello, (size_t)len);
    }
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
    time_t now = time(NULL);

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
        collect_scan(b);
        b->last_refresh = now;
        b->next_sweep = 0;      /* the first sweep goes out at once */
        b->next_ping = 0;
    }
    else if(!b->last_refresh || now - b->last_refresh >= 2) {
        /* the interfaces may have come or gone while we were not up */
        collect_broadcasts(b);
        collect_scan(b);
        b->last_refresh = now;
    }

    chat_carrier_state_t was = t->state;
    if(b->have_link) {
        t->state = CHAT_CARRIER_ONLINE;
        snprintf(t->status, sizeof(t->status), "mesh %d", b->port);
    }
    else {
        /* no interface with an address: there is nobody to talk to */
        t->state = CHAT_CARRIER_OFFLINE;
        snprintf(t->status, sizeof(t->status), "no link");
    }
    if(getenv("PDA_DEBUG") && t->state != was)
        fprintf(stderr, "[chat] bit: %s\n", t->status);

    if(b->bridge_want && b->bridge.fd < 0) {
        if(!b->last_try || now - b->last_try > b->backoff) {
            bridge_connect(b);
            if(b->backoff == 0) b->backoff = 2;
            else if(b->backoff < 30) b->backoff *= 2;
        }
    }
    return b->have_link;
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
    /* the dispatch rule, owned by the carriers: a line with a t key stays
       here (discovery as it always was - hi/hi-ack never reach this
       function - and the old chat envelope is dropped), a line without t
       but with a kind is an event, neither is dropped */
    char type[24] = "";
    json_get(line, "t", type, sizeof(type));
    if(type[0]) {
        if(strcmp(type, "msg") == 0 && getenv("PDA_DEBUG"))
            fprintf(stderr, "[chat] legacy line dropped\n");
        return;
    }
    char kind[24] = "";
    if(!json_get(line, "kind", kind, sizeof(kind))) {
        if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] legacy line dropped\n");
        return;
    }

    /* the mesh carries one channel: what is said elsewhere is not for us.
       Only when a chan tag is present and differs - no tag passes, so the
       python tools' events behave as they always did */
    char chan[CHAT_ROOM_MAX] = "";
    json_tag(line, "chan", chan, sizeof(chan));
    if(chan[0] && strcmp(chan, chat_mesh_channel()) != 0) return;

    chat_deliver_line(chat_transport_index(t), line);
}

/* a "hi" from a device on the local network: it is how a peer is found.
   The answer tells the other side who we are, so both sides know each
   other and the messages can go straight to the peer from then on. */
static void handle_hello(bit_t * b, const struct sockaddr_in * from,
                         const char * buf, bool is_hello)
{
    char who[CHAT_NAME_MAX] = "";
    json_get(buf, "from", who, sizeof(who));

    long port = 0;
    if(!json_get_int(buf, "port", &port) || port <= 0) port = ntohs(from->sin_port);
    if(port > 65535) port = b->port;

    char addr[40] = "";
    inet_ntop(AF_INET, &from->sin_addr, addr, sizeof(addr));

    /* our own datagrams come back to us: we are not a peer of ourselves */
    if(is_local(b, &from->sin_addr)) return;

    int known = chat_peer_count();
    chat_note_peer(addr, (int)port, who);
    if(getenv("PDA_DEBUG") && chat_peer_count() > known)
        fprintf(stderr, "[chat] bit: peer %s at %s:%d\n",
                who[0] ? who : "?", addr, (int)port);

    if(is_hello) {
        char ack[192];
        int len = hello_packet(ack, sizeof(ack), "hi-ack", b->port);
        if(len > 0) send_to(b, &from->sin_addr, (int)port, ack, (size_t)len);
    }
}

static int bit_poll(chat_transport_t * t)
{
    bit_t * b = t->priv;
    int got = 0;

    char buf[2048];
    for(;;) {
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);
        ssize_t n = recvfrom(b->udp, buf, sizeof(buf) - 1, MSG_DONTWAIT,
                             (struct sockaddr *)&from, &flen);
        if(n <= 0) break;
        buf[n] = 0;

        char type[24] = "";
        json_get(buf, "t", type, sizeof(type));

        if(strcmp(type, "hi") == 0 || strcmp(type, "hi-ack") == 0) {
            /* the device that sent it is a node of the mesh */
            handle_hello(b, &from, buf, strcmp(type, "hi") == 0);
            got++;
            continue;
        }

        /* the device that sent it is a node of the mesh. Its name: the
           name tag when the line is an event, else the legacy from field */
        char addr[40] = "";
        inet_ntop(AF_INET, &from.sin_addr, addr, sizeof(addr));
        char who[CHAT_NAME_MAX] = "";
        json_tag(buf, "name", who, sizeof(who));
        if(!who[0]) json_get(buf, "from", who, sizeof(who));
        /* our own broadcast comes back to us: we are not a peer of
           ourselves */
        if(!is_local(b, &from.sin_addr)) chat_note_peer(addr, ntohs(from.sin_port), who);
        deliver_line(t, buf);
        got++;
    }

    /* the sweep finds the peers that a broadcast never reaches */
    scan_step(b);

    /* and the keep-alive keeps the flows open */
    time_t now = time(NULL);
    if(b->udp >= 0 && now >= b->next_ping) {
        b->next_ping = now + BIT_PING_SEC;
        ping_peers(b);
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

    /* the link comes and goes (wifi off, sleep): keep an eye on it, so the
       badge says so and the messages are not posted into the void */
    if(now - b->last_refresh >= 5) {
        b->last_refresh = now;
        collect_broadcasts(b);
        collect_scan(b);
        if(!b->have_link) {
            t->state = CHAT_CARRIER_OFFLINE;
            snprintf(t->status, sizeof(t->status), "no link");
            if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] bit: no link\n");
        }
    }
    return got;
}

static bool bit_send(chat_transport_t * t, const char * event, const char * id)
{
    (void)id;                     /* the id is the event's, already built */
    bit_t * b = t->priv;
    const char * env = event;

    /* the broadcast: whoever is in earshot hears it */
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

    /* and the peers we know, one by one: the broadcast is the one packet a
       firewall drops first, a direct one is not */
    int n = chat_peer_count();
    for(int i = 0; i < n; i++) {
        const chat_peer_t * p = chat_peer_at(i);
        if(!p || !p->addr[0]) continue;
        struct in_addr ip;
        if(inet_pton(AF_INET, p->addr, &ip) != 1) continue;
        send_to(b, &ip, p->port > 0 ? p->port : b->port, env, strlen(env));
    }

    if(b->bridge.fd >= 0) {
        if(ls_write_line(&b->bridge, env) != 0) {
            close(b->bridge.fd);
            ls_init(&b->bridge);
        }
    }
    return true;
}

/* what the sweep is walking - the Bitchat page shows it */
const char * bit_scan_text(char * out, size_t outn)
{
    bit_t * b = &s_bit;
    if(b->scan_off) {
        snprintf(out, outn, "off");
        return out;
    }
    if(b->n_scan == 0) {
        snprintf(out, outn, "off");
        return out;
    }
    size_t o = 0;
    out[0] = 0;
    for(int i = 0; i < b->n_scan && o + 1 < outn; i++) {
        char one[32];
        inet_ntop(AF_INET, &b->scan[i].net, one, sizeof(one));
        /* the prefix is the number of leading ones of the netmask */
        uint32_t m = ntohl(b->scan[i].mask.s_addr);
        int bits = 0;
        while(bits < 32 && (m & 0x80000000u)) { bits++; m <<= 1; }
        int w = snprintf(out + o, outn - o, "%s%s/%d", i ? " " : "", one, bits);
        if(w < 0) break;
        o += (size_t)w;
    }
    return out;
}

/* what the bluetooth radio is (address and channel, or "no adapter") */
const char * bit_info_text(char * out, size_t outn)
{
    bit_t * b = &s_bit;
    if(b->udp < 0) {
        snprintf(out, outn, "not started");
        return out;
    }
    if(!b->have_link) {
        snprintf(out, outn, "no link");
        return out;
    }
    if(b->bridge_want) {
        snprintf(out, outn, "bridged");
        return out;
    }
    snprintf(out, outn, "port %d", b->port);
    return out;
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
