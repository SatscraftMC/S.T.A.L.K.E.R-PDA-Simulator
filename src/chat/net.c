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
#include <netinet/tcp.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* NET - the local network: plain packets over the LAN, no server, no
 * account. Broadcast on port NET_PORT (7788) reaches everyone in earshot,
 * the devices the walk has found are talked to directly as well, and an
 * optional relay (tools/relay.py, PDA_RELAY_HOST) carries the room out to
 * the real internet when one is configured.
 *
 * The same rules as everywhere on the LAN apply: the firewall has to let
 * the port in (ufw allow 7788/udp), and the broadcast is the packet that
 * gets dropped first - which is why the peers are dialled directly too.
 *
 *   PDA_NET_PORT   the port                 (default 7788)
 *   PDA_NET_SCAN   the subnets the walk covers ("off", or "10.0.0.0/24,...")
 *   PDA_RELAY_HOST / PDA_RELAY_PORT          the optional relay uplink
 */

#define NET_PORT     7788

/* the discovery walk: the local subnet is walked every NET_SWEEP_SEC
   seconds, a few addresses per poll, so a /24 is covered in a few seconds
   without flooding the network */
#define NET_SWEEP_SEC   60
#define NET_PING_SEC    10
#define NET_PROBES      8
#define NET_SCAN_MAX    4
#define NET_SCAN_HOSTS  1024

typedef struct {
    struct in_addr net;     /* the first address of the subnet */
    struct in_addr mask;
    uint32_t hosts;         /* the addresses to walk (network and broadcast left out) */
} scan_t;

typedef struct {
    int udp;
    int port;
    struct in_addr bcast[8];    /* where the mesh message goes */
    int n_bcast;
    scan_t scan[NET_SCAN_MAX];  /* the subnets the walk covers */
    int n_scan;
    int scan_at;                /* the subnet the cursor is in */
    uint32_t scan_host;         /* the address inside it */
    struct in_addr local[8];    /* our own addresses: the walk skips them */
    int n_local;
    bool have_link;             /* a network we could actually talk on */
    time_t last_refresh;
    time_t next_sweep;
    time_t next_ping;
    bool scan_off;

    /* the optional relay uplink (the internet) */
    bool relay_want;
    char relay_host[128];
    int relay_port;
    int relay_fd;
    time_t relay_try;
    int relay_backoff;
} net_t;

static net_t s_net;
static chat_transport_t s_tr;

static void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* the addresses a netmask holds, without the network and the broadcast
   address */
static uint32_t host_count(struct in_addr mask)
{
    uint32_t m = ntohl(mask.s_addr);
    uint32_t all = ~m;             /* the size of the network */
    if(all < 3) return 0;
    uint32_t hosts = all - 1;      /* the network and the broadcast are out */
    if(hosts > NET_SCAN_HOSTS) hosts = NET_SCAN_HOSTS;
    return hosts;
}

/* our own addresses: the walk must not cover them (a datagram sent to our
   own address comes straight back to us - the device would take itself for
   a peer of the mesh) */
static bool is_local(const net_t * n, const struct in_addr * ip)
{
    for(int i = 0; i < n->n_local; i++)
        if(n->local[i].s_addr == ip->s_addr) return true;
    return false;
}

/* the networks this machine is on right now: the broadcast addresses, the
   subnets the walk covers, our own addresses - and whether there is a
   network at all (a machine whose only address is the loopback has no
   radio, and saying ONLINE anyway is how "the net ate my message" gets
   reported) */
static void collect_broadcasts(net_t * n)
{
    n->n_bcast = 0;
    n->have_link = false;
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
            for(int i = 0; i < n->n_bcast; i++)
                if(n->bcast[i].s_addr == br->sin_addr.s_addr) have = true;
            if(!have && n->n_bcast < 6) n->bcast[n->n_bcast++] = br->sin_addr;
            (void)sa;
            n->have_link = true;    /* an address on a real interface */
        }
        freeifaddrs(ifa);
    }
    struct in_addr all;
    all.s_addr = htonl(INADDR_BROADCAST);
    bool have_all = false;
    for(int i = 0; i < n->n_bcast; i++)
        if(n->bcast[i].s_addr == all.s_addr) have_all = true;
    if(!have_all && n->n_bcast < 8) n->bcast[n->n_bcast++] = all;

    struct in_addr loop;
    loop.s_addr = htonl(INADDR_LOOPBACK);
    bool have_loop = false;
    for(int i = 0; i < n->n_bcast; i++)
        if(n->bcast[i].s_addr == loop.s_addr) have_loop = true;
    if(!have_loop && n->n_bcast < 8) n->bcast[n->n_bcast++] = loop;
}

/* the subnets the walk covers: the ones this machine is on. PDA_NET_SCAN
   overrides them ("off" to switch the walk off, or a list of networks:
   "10.0.0.0/24,192.168.1.0/24") - the test uses that to walk a loopback
   range instead of the real network. */
static void collect_scan(net_t * n)
{
    n->n_scan = 0;
    n->scan_at = 0;
    n->scan_host = 0;

    const char * env = getenv("PDA_NET_SCAN");
    if(!env || !*env) env = getenv("PDA_BIT_SCAN");    /* the name it had first */
    if(env && *env) {
        if(strcmp(env, "off") == 0 || strcmp(env, "none") == 0) {
            n->scan_off = true;
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
            if(hosts < 1 || n->n_scan >= NET_SCAN_MAX) continue;
            scan_t * s = &n->scan[n->n_scan++];
            s->net = net;
            s->mask = mask;
            s->hosts = hosts;
        }
        return;
    }

    struct ifaddrs * ifa = NULL;
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
            if(hosts < 1 || n->n_scan >= NET_SCAN_MAX) continue;
            scan_t * s = &n->scan[n->n_scan++];
            s->net = net;
            s->mask = mask;
            s->hosts = hosts;
        }
        freeifaddrs(ifa);
    }
}

/* one datagram to one address */
static bool send_to(net_t * n, const struct in_addr * ip, int port,
                    const char * buf, size_t len)
{
    if(n->udp < 0 || !ip || port <= 0) return false;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr = *ip;
    addr.sin_port = htons((uint16_t)port);
    if(sendto(n->udp, buf, len, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if(getenv("PDA_DEBUG"))
            fprintf(stderr, "[chat] net sendto %s:%d failed: %s\n",
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
    int r = snprintf(out, outn,
                     "{\"v\":1,\"t\":\"%s\",\"from\":\"%s\",\"port\":%d,\"ts\":%ld}",
                     type, e_from, port, (long)time(NULL));
    /* the length that is sent must be the length that fits */
    if(r < 0) return 0;
    if((size_t)r >= outn) return (int)outn - 1;
    return r;
}

/* the walk: a few addresses of the local subnet per poll */
static void scan_step(net_t * n)
{
    if(n->scan_off || n->n_scan == 0) return;

    time_t now = time(NULL);
    if(now >= n->next_sweep) {
        /* the network may have changed since the last walk */
        collect_scan(n);
        n->scan_at = 0;
        n->scan_host = 0;
        n->next_sweep = now + NET_SWEEP_SEC;
        if(getenv("PDA_DEBUG")) {
            fprintf(stderr, "[chat] net: walking %d subnet(s)\n", n->n_scan);
        }
    }

    char hello[192];
    int len = hello_packet(hello, sizeof(hello), "hi", n->port);
    if(len <= 0) return;

    for(int i = 0; i < NET_PROBES; i++) {
        if(n->scan_at >= n->n_scan) break;
        scan_t * s = &n->scan[n->scan_at];
        if(n->scan_host >= s->hosts) {
            n->scan_at++;
            n->scan_host = 0;
            continue;
        }
        struct in_addr ip;
        ip.s_addr = htonl(ntohl(s->net.s_addr) + n->scan_host + 1);
        n->scan_host++;
        if(is_local(n, &ip)) continue;    /* that one is us */
        send_to(n, &ip, n->port, hello, (size_t)len);
    }
}

/* the keep-alive: a "hi" to every peer we know. It keeps the flow open in
   the firewalls of both machines (a firewall that only lets the traffic it
   has already seen through needs this) and it says that we are still here. */
static void ping_peers(net_t * n)
{
    if(n->udp < 0) return;
    char hello[192];
    int len = hello_packet(hello, sizeof(hello), "hi", n->port);
    if(len <= 0) return;

    int cnt = chat_peer_count();
    for(int i = 0; i < cnt; i++) {
        const chat_peer_t * p = chat_peer_at(i);
        if(!p || !p->addr[0]) continue;
        struct in_addr ip;
        if(inet_pton(AF_INET, p->addr, &ip) != 1) continue;
        send_to(n, &ip, p->port > 0 ? p->port : n->port, hello, (size_t)len);
    }
}

/* ---------- the optional relay uplink (the internet) ---------- */

static const char * net_relay_reason(int err)
{
    switch(err) {
        case ECONNREFUSED: return "no relay";
        case ENETUNREACH:
        case EHOSTUNREACH:
        case ENETDOWN:
        case EADDRNOTAVAIL:
            return "no link";
        default: return "no relay";
    }
}

static void relay_disconnect(net_t * n)
{
    if(getenv("PDA_DEBUG"))
        fprintf(stderr, "[chat] net relay disconnected (fd %d)\n", n->relay_fd);
    if(n->relay_fd >= 0) close(n->relay_fd);
    n->relay_fd = -1;
}

static bool relay_connect(net_t * n)
{
    time_t now = time(NULL);
    n->relay_try = now;

    char portbuf[16];
    snprintf(portbuf, sizeof(portbuf), "%d", n->relay_port);

    struct addrinfo hints, * res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(n->relay_host, portbuf, &hints, &res) != 0 || !res) {
        n->relay_backoff = n->relay_backoff ? n->relay_backoff * 2 : 2;
        if(n->relay_backoff > 30) n->relay_backoff = 30;
        return false;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(fd < 0) {
        freeaddrinfo(res);
        n->relay_backoff = n->relay_backoff ? n->relay_backoff * 2 : 2;
        return false;
    }
    set_nonblock(fd);

    int rc = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if(rc != 0) {
        struct pollfd pfd = { .fd = fd, .events = POLLOUT };
        int pr = poll(&pfd, 1, 400);
        int err = 0;
        socklen_t elen = sizeof(err);
        if(pr <= 0 || getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &elen) != 0 || err != 0) {
            close(fd);
            if(getenv("PDA_DEBUG"))
                fprintf(stderr, "[chat] net relay: %s (%s:%d)\n",
                        net_relay_reason(err), n->relay_host, n->relay_port);
            n->relay_backoff = n->relay_backoff ? n->relay_backoff * 2 : 2;
            if(n->relay_backoff > 30) n->relay_backoff = 30;
            return false;
        }
    }

    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    n->relay_fd = fd;

    /* announce ourselves, so the relay knows who is in the room */
    char hello[256];
    chat_envelope(hello, sizeof(hello), "hello", chat_identity(), "", (long)time(NULL));
    {
        ls_t tmp;
        ls_init(&tmp);
        tmp.fd = fd;
        ls_write_line(&tmp, hello);
    }

    n->relay_backoff = 0;
    if(getenv("PDA_DEBUG"))
        fprintf(stderr, "[chat] net relay up (%s:%d)\n", n->relay_host, n->relay_port);
    return true;
}

static void relay_poll(net_t * n)
{
    if(!n->relay_want) return;

    if(n->relay_fd < 0) {
        time_t now = time(NULL);
        if(n->relay_try && now - n->relay_try < n->relay_backoff) return;
        relay_connect(n);
        return;
    }

    char line[LS_BUF];
    for(;;) {
        ls_t tmp;
        ls_init(&tmp);
        tmp.fd = n->relay_fd;
        int r = ls_read_line(&tmp, line, sizeof(line));
        if(r <= 0) {
            if(r < 0) relay_disconnect(n);
            return;
        }
        char type[24] = "";
        json_get(line, "t", type, sizeof(type));
        if(strcmp(type, "msg") != 0) continue;

        char id[CHAT_ID_MAX] = "", from[CHAT_NAME_MAX] = "", text[CHAT_TEXT_MAX] = "";
        json_get(line, "id", id, sizeof(id));
        json_get(line, "from", from, sizeof(from));
        json_get(line, "text", text, sizeof(text));
        if(!text[0]) continue;
        chat_deliver(chat_transport_index(&s_tr), id, from, text);
    }
}

static bool relay_send(net_t * n, const char * env)
{
    if(n->relay_fd < 0) return false;
    ls_t tmp;
    ls_init(&tmp);
    tmp.fd = n->relay_fd;
    if(ls_write_line(&tmp, env) != 0) {
        relay_disconnect(n);
        return false;
    }
    return true;
}

/* ---------- the mesh ---------- */

static void deliver_line(const char * line)
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
    /* the net carries one channel: what is said elsewhere is not for us */
    if(chan[0] && strcmp(chan, chat_mesh_channel()) != 0) return;
    chat_deliver(chat_transport_index(&s_tr), id, from, text);
}

/* a "hi" from a device on the local network: it is how a peer is found.
   The answer tells the other side who we are, so both sides know each
   other and the messages can go straight to the peer from then on. */
static void handle_hello(net_t * n, const struct sockaddr_in * from,
                         const char * buf, bool is_hello)
{
    char who[CHAT_NAME_MAX] = "";
    json_get(buf, "from", who, sizeof(who));

    long port = 0;
    if(!json_get_int(buf, "port", &port) || port <= 0) port = ntohs(from->sin_port);
    if(port > 65535) port = n->port;

    char addr[40] = "";
    inet_ntop(AF_INET, &from->sin_addr, addr, sizeof(addr));

    /* our own datagrams come back to us: we are not a peer of ourselves */
    if(is_local(n, &from->sin_addr)) return;

    int known = chat_peer_count();
    chat_note_peer(addr, (int)port, who);
    if(getenv("PDA_DEBUG") && chat_peer_count() > known)
        fprintf(stderr, "[chat] net: peer %s at %s:%d\n",
                who[0] ? who : "?", addr, (int)port);

    if(is_hello) {
        char ack[192];
        int len = hello_packet(ack, sizeof(ack), "hi-ack", n->port);
        if(len > 0) send_to(n, &from->sin_addr, (int)port, ack, (size_t)len);
    }
}

static bool net_connect(chat_transport_t * t)
{
    net_t * n = (net_t *)t->priv;
    time_t now = time(NULL);

    if(n->udp < 0) {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(fd < 0) {
            snprintf(t->status, sizeof(t->status), "net down");
            return false;
        }
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons((uint16_t)n->port);
        if(bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
            close(fd);
            snprintf(t->status, sizeof(t->status), "port busy");
            return false;
        }
        set_nonblock(fd);
        n->udp = fd;
        collect_broadcasts(n);
        collect_scan(n);
        n->last_refresh = now;
        n->next_sweep = 0;      /* the first walk goes out at once */
        n->next_ping = 0;
    }
    else if(!n->last_refresh || now - n->last_refresh >= 2) {
        /* the interfaces may have come or gone while we were not up */
        collect_broadcasts(n);
        collect_scan(n);
        n->last_refresh = now;
    }

    chat_carrier_state_t was = t->state;
    if(n->have_link) {
        t->state = CHAT_CARRIER_ONLINE;
        snprintf(t->status, sizeof(t->status), "lan %d", n->port);
    }
    else {
        /* no interface with an address: there is nobody to talk to */
        t->state = CHAT_CARRIER_OFFLINE;
        snprintf(t->status, sizeof(t->status), "no link");
    }
    if(getenv("PDA_DEBUG") && t->state != was)
        fprintf(stderr, "[chat] net: %s\n", t->status);

    return n->have_link;
}

static void net_disconnect(chat_transport_t * t)
{
    net_t * n = (net_t *)t->priv;
    if(n->udp >= 0) { close(n->udp); n->udp = -1; }
    if(n->relay_fd >= 0) relay_disconnect(n);
    t->state = CHAT_CARRIER_OFFLINE;
    snprintf(t->status, sizeof(t->status), "net down");
}

static int net_poll(chat_transport_t * t)
{
    net_t * n = (net_t *)t->priv;
    int got = 0;

    char buf[2048];
    for(;;) {
        struct sockaddr_in from;
        socklen_t flen = sizeof(from);
        ssize_t r = recvfrom(n->udp, buf, sizeof(buf) - 1, MSG_DONTWAIT,
                             (struct sockaddr *)&from, &flen);
        if(r <= 0) break;
        buf[r] = 0;

        char type[24] = "";
        json_get(buf, "t", type, sizeof(type));

        if(strcmp(type, "hi") == 0 || strcmp(type, "hi-ack") == 0) {
            /* the device that sent it is a node of the mesh */
            handle_hello(n, &from, buf, strcmp(type, "hi") == 0);
            got++;
            continue;
        }

        /* the device that sent it is a node of the mesh */
        char addr[40] = "";
        inet_ntop(AF_INET, &from.sin_addr, addr, sizeof(addr));
        char who[CHAT_NAME_MAX] = "";
        json_get(buf, "from", who, sizeof(who));
        /* our own broadcast comes back to us: we are not a peer of
           ourselves */
        if(!is_local(n, &from.sin_addr)) chat_note_peer(addr, ntohs(from.sin_port), who);
        deliver_line(buf);
        got++;
    }

    /* the walk finds the peers that a broadcast never reaches */
    scan_step(n);

    /* and the keep-alive keeps the flows open */
    time_t now = time(NULL);
    if(n->udp >= 0 && now >= n->next_ping) {
        n->next_ping = now + NET_PING_SEC;
        ping_peers(n);
    }

    /* the relay uplink, when one is configured */
    relay_poll(n);

    /* the link comes and goes (wifi off, sleep): keep an eye on it, so the
       badge says so and the messages are not posted into the void */
    if(now - n->last_refresh >= 5) {
        n->last_refresh = now;
        collect_broadcasts(n);
        collect_scan(n);
        if(!n->have_link) {
            t->state = CHAT_CARRIER_OFFLINE;
            snprintf(t->status, sizeof(t->status), "no link");
            if(getenv("PDA_DEBUG")) fprintf(stderr, "[chat] net: no link\n");
        }
    }
    return got;
}

static bool net_send(chat_transport_t * t, const char * text, const char * id)
{
    net_t * n = (net_t *)t->priv;
    char env[1024];
    chat_envelope(env, sizeof(env), id, chat_identity(), text, (long)time(NULL));

    /* the broadcast: whoever is in earshot hears it */
    for(int i = 0; i < n->n_bcast; i++) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr = n->bcast[i];
        addr.sin_port = htons((uint16_t)n->port);
        if(sendto(n->udp, env, strlen(env), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            if(getenv("PDA_DEBUG"))
                fprintf(stderr, "[chat] net sendto failed: %s\n", strerror(errno));
        }
    }

    /* and the peers we know, one by one: the broadcast is the one packet a
       firewall drops first, a direct one is not */
    int cnt = chat_peer_count();
    for(int i = 0; i < cnt; i++) {
        const chat_peer_t * p = chat_peer_at(i);
        if(!p || !p->addr[0]) continue;
        struct in_addr ip;
        if(inet_pton(AF_INET, p->addr, &ip) != 1) continue;
        send_to(n, &ip, p->port > 0 ? p->port : n->port, env, strlen(env));
    }

    /* and the relay, when there is one */
    relay_send(n, env);
    return true;
}

/* what the walk is covering - the Bitchat page shows it */
const char * net_scan_text(char * out, size_t outn)
{
    net_t * n = &s_net;
    if(n->scan_off || n->n_scan == 0) {
        snprintf(out, outn, "off");
        return out;
    }
    size_t o = 0;
    out[0] = 0;
    for(int i = 0; i < n->n_scan && o + 1 < outn; i++) {
        char one[32];
        inet_ntop(AF_INET, &n->scan[i].net, one, sizeof(one));
        /* the prefix is the number of leading ones of the netmask */
        uint32_t m = ntohl(n->scan[i].mask.s_addr);
        int bits = 0;
        while(bits < 32 && (m & 0x80000000u)) { bits++; m <<= 1; }
        int w = snprintf(out + o, outn - o, "%s%s/%d", i ? " " : "", one, bits);
        if(w < 0) break;
        o += (size_t)w;
    }
    return out;
}

void chat_net_register(void)
{
    memset(&s_net, 0, sizeof(s_net));
    s_net.udp = -1;
    s_net.relay_fd = -1;
    s_net.port = getenv("PDA_NET_PORT") ? atoi(getenv("PDA_NET_PORT")) : NET_PORT;
    if(s_net.port <= 0) s_net.port = NET_PORT;

    const char * relay = getenv("PDA_RELAY_HOST");
    if(relay && *relay) {
        s_net.relay_want = true;
        snprintf(s_net.relay_host, sizeof(s_net.relay_host), "%s", relay);
        s_net.relay_port = getenv("PDA_RELAY_PORT") ? atoi(getenv("PDA_RELAY_PORT")) : 7788;
    }

    s_tr.tag = "NET";
    s_tr.label = "Local net";
    s_tr.connect = net_connect;
    s_tr.disconnect = net_disconnect;
    s_tr.poll = net_poll;
    s_tr.send = net_send;
    s_tr.priv = &s_net;
    s_tr.state = CHAT_CARRIER_OFFLINE;
    snprintf(s_tr.status, sizeof(s_tr.status), "no link");
    chat_add_transport(&s_tr);
}
