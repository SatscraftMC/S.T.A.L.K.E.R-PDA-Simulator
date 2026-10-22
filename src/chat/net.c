#include "chat/chat.h"
#include "chat/json.h"
#include "chat/linesock.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* NET - the internet, through a relay.
 *
 * The relay is a dumb hub: it takes a line from one client and hands it to
 * every other client. tools/relay.py is the reference implementation and can
 * bridge to an ntfy topic, which is how a PDA with no reachable server of its
 * own still reaches the internet.
 *
 *   PDA_RELAY_HOST / PDA_RELAY_PORT   where the relay lives
 *                                      (default 127.0.0.1:7788)
 */

#define NET_DEFAULT_PORT 7788

typedef struct {
    ls_t ls;
    char host[128];
    int port;
    time_t last_try;
    int backoff;
} net_t;

static net_t s_net;
static chat_transport_t s_tr;

static void set_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* why the relay could not be reached: "no relay" is a dead port on a live
   network, "no link" is no network to speak of at all */
static const char * net_reason(int err)
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

static bool net_connect(chat_transport_t * t)
{
    net_t * n = (net_t *)t->priv;
    time_t now = time(NULL);
    if(n->last_try && now - n->last_try < n->backoff) {
        t->state = CHAT_CARRIER_CONNECTING;   /* waiting for the next try */
        return false;
    }
    n->last_try = now;
    t->state = CHAT_CARRIER_CONNECTING;

    char portbuf[16];
    snprintf(portbuf, sizeof(portbuf), "%d", n->port);

    struct addrinfo hints, * res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if(getaddrinfo(n->host, portbuf, &hints, &res) != 0 || !res) {
        snprintf(t->status, sizeof(t->status), "no relay");
        n->backoff = n->backoff ? n->backoff * 2 : 2;
        if(n->backoff > 30) n->backoff = 30;
        return false;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(fd < 0) {
        freeaddrinfo(res);
        snprintf(t->status, sizeof(t->status), "no relay");
        n->backoff = n->backoff ? n->backoff * 2 : 2;
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
            snprintf(t->status, sizeof(t->status), "%s", net_reason(err));
            n->backoff = n->backoff ? n->backoff * 2 : 2;
            if(n->backoff > 30) n->backoff = 30;
            return false;
        }
    }

    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    ls_init(&n->ls);
    n->ls.fd = fd;

    /* announce ourselves, so the relay knows who is in the room */
    char hello[256];
    chat_envelope(hello, sizeof(hello), "hello", chat_identity(), "", (long)time(NULL));
    ls_write_line(&n->ls, hello);

    n->backoff = 0;
    t->state = CHAT_CARRIER_ONLINE;
    snprintf(t->status, sizeof(t->status), "relay %.30s:%d", n->host, n->port);
    return true;
}

static void net_disconnect(chat_transport_t * t)
{
    net_t * n = (net_t *)t->priv;
    if(getenv("PDA_DEBUG"))
        fprintf(stderr, "[chat] net disconnected (fd %d)\n", n->ls.fd);
    if(n->ls.fd >= 0) close(n->ls.fd);
    ls_init(&n->ls);
    t->state = CHAT_CARRIER_OFFLINE;
    snprintf(t->status, sizeof(t->status), "no relay");
}

static int net_poll(chat_transport_t * t)
{
    net_t * n = (net_t *)t->priv;
    int got = 0;
    char line[LS_BUF];
    for(;;) {
        int r = ls_read_line(&n->ls, line, sizeof(line));
        if(r <= 0) {
            if(r < 0) {
                net_disconnect(t);
            }
            break;
        }
        char type[24] = "";
        json_get(line, "t", type, sizeof(type));
        if(getenv("PDA_DEBUG"))
            fprintf(stderr, "[chat] net line (t=%s): %s\n", type, line);
        if(strcmp(type, "msg") != 0) continue;

        char id[CHAT_ID_MAX] = "", from[CHAT_NAME_MAX] = "", text[CHAT_TEXT_MAX] = "";
        json_get(line, "id", id, sizeof(id));
        json_get(line, "from", from, sizeof(from));
        json_get(line, "text", text, sizeof(text));
        if(!text[0]) continue;
        chat_deliver(chat_transport_index(t), id, from, text);
        got++;
    }
    return got;
}

static bool net_send(chat_transport_t * t, const char * text, const char * id)
{
    net_t * n = (net_t *)t->priv;
    char env[1024];
    chat_envelope(env, sizeof(env), id, chat_identity(), text, (long)time(NULL));
    if(ls_write_line(&n->ls, env) != 0) {
        if(getenv("PDA_DEBUG"))
            fprintf(stderr, "[chat] net write failed: %s\n", strerror(errno));
        net_disconnect(t);
        return false;
    }
    return true;
}

void chat_net_register(void)
{
    memset(&s_net, 0, sizeof(s_net));
    ls_init(&s_net.ls);
    snprintf(s_net.host, sizeof(s_net.host), "%s",
             getenv("PDA_RELAY_HOST") ? getenv("PDA_RELAY_HOST") : "127.0.0.1");
    s_net.port = getenv("PDA_RELAY_PORT") ? atoi(getenv("PDA_RELAY_PORT")) : NET_DEFAULT_PORT;

    s_tr.tag = "NET";
    s_tr.label = "Internet";
    s_tr.connect = net_connect;
    s_tr.disconnect = net_disconnect;
    s_tr.poll = net_poll;
    s_tr.send = net_send;
    s_tr.priv = &s_net;
    s_tr.state = CHAT_CARRIER_OFFLINE;
    snprintf(s_tr.status, sizeof(s_tr.status), "no relay");
    chat_add_transport(&s_tr);
}
