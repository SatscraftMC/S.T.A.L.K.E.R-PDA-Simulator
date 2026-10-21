#include "chat/chat.h"
#include "chat/json.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

/* FIPS - the mesh, through the daemon's native datagram API.
 *
 * The fips daemon is the node; this is a plain client of it, written against
 * the upstream line protocol:
 *
 *   - the setup connection is an AF_UNIX stream socket; one JSON command per
 *     connection, and the reply carries the flow's descriptor as ancillary
 *     data (SCM_RIGHTS), which is why every read of it is a recvmsg.
 *   - `listen` gives back a listener; an arriving flow is one SOCK_SEQPACKET
 *     message on it, and `connect` gives back a flow to a peer.
 *   - a flow carries datagrams with no framing at all: one send is one
 *     datagram, and the same JSON envelope the other carriers use.
 *
 * Everything here happens on a thread of its own: a setup call is a blocking
 * round trip to the daemon, and the LVGL loop must never wait for one.
 *
 *   PDA_FIPS_SOCK   the daemon's api socket  (default: the packaged path)
 *   PDA_FIPS_CTL    the daemon's control socket (for the peer list)
 *   PDA_FIPS_PORT   the FSP port to talk on   (default 4600)
 */

#define FIPS_MAX_FLOWS   8
#define FIPS_MAX_PEERS   16
#define FIPS_INBOX       24
#define FIPS_OUTBOX      16
#define FIPS_DEFAULT_PORT 4600

typedef struct {
    int fd;
    char peer[96];
    int remote_port;
    int max_payload;
    bool used;
} fips_flow_t;

typedef struct {
    char from[64];
    char text[CHAT_TEXT_MAX];
} fips_in_t;

typedef struct {
    char id[CHAT_ID_MAX];
    char text[CHAT_TEXT_MAX];
} fips_out_t;

typedef struct {
    pthread_mutex_t lock;
    pthread_t thread;
    bool thread_running;
    bool stop;

    char api_sock[128];
    char ctl_sock[128];
    int port;

    int listener;
    fips_flow_t flows[FIPS_MAX_FLOWS];
    int n_flows;

    fips_in_t inbox[FIPS_INBOX];
    int in_head, in_count;
    fips_out_t outbox[FIPS_OUTBOX];
    int out_head, out_count;

    time_t last_peers;
    int peer_count;
} fips_t;

static fips_t s_fips;
static chat_transport_t s_tr;

/* ---------- the daemon's paths ---------- */

static void resolve_paths(void)
{
    if(s_fips.api_sock[0]) return;

    const char * env = getenv("PDA_FIPS_SOCK");
    if(env && *env) {
        snprintf(s_fips.api_sock, sizeof(s_fips.api_sock), "%s", env);
    }
    else {
        const char * xdg = getenv("XDG_RUNTIME_DIR");
        struct stat st;
        if(stat("/run/fips", &st) == 0 && S_ISDIR(st.st_mode))
            snprintf(s_fips.api_sock, sizeof(s_fips.api_sock), "/run/fips/api.sock");
        else if(xdg && *xdg)
            snprintf(s_fips.api_sock, sizeof(s_fips.api_sock), "%s/fips/api.sock", xdg);
        else
            snprintf(s_fips.api_sock, sizeof(s_fips.api_sock), "/tmp/fips-api.sock");
    }

    env = getenv("PDA_FIPS_CTL");
    if(env && *env) snprintf(s_fips.ctl_sock, sizeof(s_fips.ctl_sock), "%s", env);
    else {
        char * slash = strrchr(s_fips.api_sock, '/');
        if(slash) {
            size_t n = (size_t)(slash - s_fips.api_sock);
            snprintf(s_fips.ctl_sock, sizeof(s_fips.ctl_sock), "%.*s/control.sock",
                     (int)n, s_fips.api_sock);
        }
        else snprintf(s_fips.ctl_sock, sizeof(s_fips.ctl_sock), "/run/fips/control.sock");
    }
}

/* ---------- the setup connection ---------- */

static int unix_connect(const char * path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd < 0) return -1;
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%.*s",
             (int)sizeof(addr.sun_path) - 1, path);
    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }
    return fd;
}

/* One command on a fresh setup connection. The reply line comes back in
   reply[], and the descriptor the daemon passes with it (if any) is returned.
   -1 means no descriptor, -2 means the call failed. */
static int fips_setup(const char * path, const char * cmd, char * reply, size_t replyn)
{
    int fd = unix_connect(path);
    if(fd < 0) return -2;

    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    size_t clen = strlen(cmd);
    const char * p = cmd;
    size_t off = 0;
    while(off < clen) {
        ssize_t w = send(fd, p + off, clen - off, MSG_NOSIGNAL);
        if(w <= 0) {
            if(w < 0 && errno == EINTR) continue;
            close(fd);
            return -2;
        }
        off += (size_t)w;
    }

    char buf[8192];
    size_t used = 0;
    int got_fd = -1;
    for(;;) {
        char cbuf[CMSG_SPACE(sizeof(int) * 4)];
        struct iovec iov = { .iov_base = buf + used, .iov_len = sizeof(buf) - 1 - used };
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cbuf;
        msg.msg_controllen = sizeof(cbuf);

        ssize_t n = recvmsg(fd, &msg, MSG_CMSG_CLOEXEC);
        if(n < 0) {
            if(errno == EINTR) continue;
            if(got_fd >= 0) close(got_fd);
            close(fd);
            return -2;
        }
        if(n == 0) {
            if(got_fd >= 0) close(got_fd);
            close(fd);
            return -2;
        }
        used += (size_t)n;
        buf[used] = 0;

        /* take every descriptor the kernel handed over, close the extras */
        for(struct cmsghdr * cm = CMSG_FIRSTHDR(&msg); cm; cm = CMSG_NXTHDR(&msg, cm)) {
            if(cm->cmsg_level != SOL_SOCKET || cm->cmsg_type != SCM_RIGHTS) continue;
            int count = (int)((cm->cmsg_len - CMSG_LEN(0)) / sizeof(int));
            int * fds = (int *)CMSG_DATA(cm);
            for(int i = 0; i < count; i++) {
                if(got_fd < 0) got_fd = fds[i];
                else close(fds[i]);
            }
        }
        if(msg.msg_flags & MSG_CTRUNC) {
            if(got_fd >= 0) close(got_fd);
            close(fd);
            return -2;
        }
        /* a descriptor that arrived with no complete line is a defect we
           cannot resolve: report it instead of holding it */
        if(got_fd >= 0 && !memchr(buf, '\n', used)) {
            close(got_fd);
            close(fd);
            return -2;
        }
        if(memchr(buf, '\n', used)) break;
        if(used >= sizeof(buf) - 1) {
            if(got_fd >= 0) close(got_fd);
            close(fd);
            return -2;
        }
    }

    char * nl = memchr(buf, '\n', used);
    if(nl) *nl = 0;
    if(replyn) snprintf(reply, replyn, "%.*s", (int)replyn - 1, buf);
    close(fd);      /* the connection owns nothing; the descriptor does */
    return got_fd;
}

/* ---------- the control socket (peer list) ---------- */

static int control_query(const char * path, const char * cmd, char * out, size_t outn)
{
    int fd = unix_connect(path);
    if(fd < 0) return -1;
    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    size_t clen = strlen(cmd);
    if(send(fd, cmd, clen, MSG_NOSIGNAL) != (ssize_t)clen) {
        close(fd);
        return -1;
    }
    size_t used = 0;
    for(;;) {
        if(used >= outn - 1) break;
        ssize_t n = read(fd, out + used, outn - 1 - used);
        if(n <= 0) break;
        used += (size_t)n;
        if(memchr(out, '\n', used)) break;
    }
    close(fd);
    out[used] = 0;
    char * nl = memchr(out, '\n', used);
    if(nl) *nl = 0;
    return used ? 0 : -1;
}

/* pull the npubs out of a show_peers reply */
static int extract_peers(const char * reply, char out[][96], int max)
{
    const char * p = strstr(reply, "\"peers\"");
    if(!p) return 0;
    p = strchr(p, '[');
    if(!p) return 0;
    p++;

    int n = 0;
    while(*p && n < max) {
        while(*p == ' ' || *p == ',' || *p == '\n' || *p == '\r') p++;
        if(*p != '{') break;
        const char * start = p;
        int depth = 0;
        bool in_str = false;
        while(*p) {
            if(in_str) {
                if(*p == '\\' && p[1]) p++;
                else if(*p == '"') in_str = false;
            }
            else if(*p == '"') in_str = true;
            else if(*p == '{') depth++;
            else if(*p == '}') {
                depth--;
                if(depth == 0) { p++; break; }
            }
            p++;
        }
        size_t len = (size_t)(p - start);
        if(len >= 96) len = 95;
        char obj[1024];
        if(len >= sizeof(obj)) len = sizeof(obj) - 1;
        memcpy(obj, start, len);
        obj[len] = 0;

        char npub[96] = "";
        if(json_get(obj, "npub", npub, sizeof(npub)) && npub[0]) {
            snprintf(out[n], 96, "%s", npub);
            n++;
        }
    }
    return n;
}

/* ---------- the worker thread ---------- */

static void push_inbox(const char * from, const char * text)
{
    pthread_mutex_lock(&s_fips.lock);
    if(s_fips.in_count < FIPS_INBOX) {
        int idx = (s_fips.in_head + s_fips.in_count) % FIPS_INBOX;
        snprintf(s_fips.inbox[idx].from, sizeof(s_fips.inbox[idx].from), "%s", from);
        snprintf(s_fips.inbox[idx].text, sizeof(s_fips.inbox[idx].text), "%s", text);
        s_fips.in_count++;
    }
    pthread_mutex_unlock(&s_fips.lock);
}

static bool flow_add(int fd, const char * peer, int remote_port, int max_payload)
{
    pthread_mutex_lock(&s_fips.lock);
    bool ok = false;
    for(int i = 0; i < FIPS_MAX_FLOWS; i++) {
        if(s_fips.flows[i].used) continue;
        s_fips.flows[i].fd = fd;
        snprintf(s_fips.flows[i].peer, sizeof(s_fips.flows[i].peer), "%.*s",
                 (int)sizeof(s_fips.flows[i].peer) - 1, peer);
        s_fips.flows[i].remote_port = remote_port;
        s_fips.flows[i].max_payload = max_payload;
        s_fips.flows[i].used = true;
        s_fips.n_flows++;
        ok = true;
        break;
    }
    pthread_mutex_unlock(&s_fips.lock);
    return ok;
}

static void flow_remove_at(int i)
{
    pthread_mutex_lock(&s_fips.lock);
    if(s_fips.flows[i].used) {
        if(s_fips.flows[i].fd >= 0) close(s_fips.flows[i].fd);
        memset(&s_fips.flows[i], 0, sizeof(s_fips.flows[i]));
        s_fips.flows[i].fd = -1;
        s_fips.n_flows--;
    }
    pthread_mutex_unlock(&s_fips.lock);
}

static void handle_arrival(const char * msg)
{
    char peer[96] = "", npub[96] = "";
    long port = 0, maxp = 0;
    json_get(msg, "peer", peer, sizeof(peer));
    json_get(msg, "npub", npub, sizeof(npub));
    json_get_int(msg, "remote_port", &port);
    json_get_int(msg, "max_payload", &maxp);
    if(!peer[0] && npub[0]) snprintf(peer, sizeof(peer), "%s", npub);
    if(!peer[0]) return;

    /* a flow to this peer: the peer may already be in the table */
    pthread_mutex_lock(&s_fips.lock);
    int existing = -1;
    for(int i = 0; i < FIPS_MAX_FLOWS; i++) {
        if(s_fips.flows[i].used && strcmp(s_fips.flows[i].peer, peer) == 0) {
            existing = i;
            break;
        }
    }
    pthread_mutex_unlock(&s_fips.lock);
    if(existing >= 0) return;      /* already talking to them */

    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "{\"command\":\"connect\",\"params\":{\"peer\":\"%.95s\",\"remote_port\":%ld}}\n",
             peer, port ? port : (long)s_fips.port);
    char reply[1024];
    int fd = fips_setup(s_fips.api_sock, cmd, reply, sizeof(reply));
    if(fd < 0) return;
    if(!flow_add(fd, peer, (int)port, (int)maxp)) close(fd);
}

static void read_flow(int idx)
{
    for(;;) {
        char buf[2048];
        char cbuf[CMSG_SPACE(sizeof(int) * 4)];
        struct iovec iov = { .iov_base = buf, .iov_len = sizeof(buf) - 1 };
        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cbuf;
        msg.msg_controllen = sizeof(cbuf);

        ssize_t n = recvmsg(s_fips.flows[idx].fd, &msg, MSG_CMSG_CLOEXEC);
        if(n < 0) {
            if(errno == EINTR) continue;
            if(errno == EAGAIN || errno == EWOULDBLOCK) return;
            flow_remove_at(idx);
            return;
        }
        if(n == 0) {                 /* the peer is gone */
            flow_remove_at(idx);
            return;
        }
        buf[n] = 0;
        if(msg.msg_flags & MSG_TRUNC) continue;    /* payload too long: drop it */

        char type[24] = "", from[64] = "", text[CHAT_TEXT_MAX] = "", id[CHAT_ID_MAX] = "";
        json_get(buf, "t", type, sizeof(type));
        if(strcmp(type, "msg") != 0) continue;
        json_get(buf, "from", from, sizeof(from));
        json_get(buf, "text", text, sizeof(text));
        json_get(buf, "id", id, sizeof(id));
        if(!text[0]) continue;
        push_inbox(from, text);
    }
}

static void refresh_peers(void)
{
    time_t now = time(NULL);
    if(s_fips.last_peers && now - s_fips.last_peers < 20) return;
    s_fips.last_peers = now;

    static char reply[32768];
    if(control_query(s_fips.ctl_sock, "{\"command\":\"show_peers\"}\n",
                     reply, sizeof(reply)) != 0)
        return;

    char npubs[FIPS_MAX_PEERS][96];
    int n = extract_peers(reply, npubs, FIPS_MAX_PEERS);
    pthread_mutex_lock(&s_fips.lock);
    s_fips.peer_count = n;
    pthread_mutex_unlock(&s_fips.lock);
    if(n <= 0) return;

    /* open a flow to every peer we do not have one for yet */
    for(int i = 0; i < n; i++) {
        char peer[96];
        memset(peer, 0, sizeof(peer));
        size_t plen = strnlen(npubs[i], sizeof(peer) - 1);
        memcpy(peer, npubs[i], plen);

        bool have = false;
        pthread_mutex_lock(&s_fips.lock);
        for(int f = 0; f < FIPS_MAX_FLOWS; f++) {
            if(s_fips.flows[f].used && strcmp(s_fips.flows[f].peer, peer) == 0) have = true;
        }
        pthread_mutex_unlock(&s_fips.lock);
        if(have) continue;

        char cmd[256];
        snprintf(cmd, sizeof(cmd),
                 "{\"command\":\"connect\",\"params\":{\"peer\":\"%.95s\",\"remote_port\":%d}}\n",
                 peer, s_fips.port);
        char reply2[1024];
        int fd = fips_setup(s_fips.api_sock, cmd, reply2, sizeof(reply2));
        if(fd < 0) continue;
        if(!flow_add(fd, peer, s_fips.port, 1200)) close(fd);
    }
}

static void flush_outbox(void)
{
    for(;;) {
        fips_out_t out;
        pthread_mutex_lock(&s_fips.lock);
        if(s_fips.out_count == 0) {
            pthread_mutex_unlock(&s_fips.lock);
            return;
        }
        out = s_fips.outbox[s_fips.out_head];
        s_fips.out_head = (s_fips.out_head + 1) % FIPS_OUTBOX;
        s_fips.out_count--;
        pthread_mutex_unlock(&s_fips.lock);

        char env[1024];
        chat_envelope(env, sizeof(env), out.id, chat_identity(), out.text, (long)time(NULL));

        pthread_mutex_lock(&s_fips.lock);
        for(int i = 0; i < FIPS_MAX_FLOWS; i++) {
            if(!s_fips.flows[i].used) continue;
            ssize_t w = send(s_fips.flows[i].fd, env, strlen(env), MSG_NOSIGNAL);
            if(w < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                /* the flow is broken: drop it and let the next peer list rebuild it */
                s_fips.flows[i].used = false;
                if(s_fips.flows[i].fd >= 0) close(s_fips.flows[i].fd);
                s_fips.flows[i].fd = -1;
                s_fips.n_flows--;
            }
        }
        pthread_mutex_unlock(&s_fips.lock);
    }
}

static void * fips_worker(void * arg)
{
    (void)arg;
    resolve_paths();

    while(!s_fips.stop) {
        struct stat st;
        if(stat(s_fips.api_sock, &st) != 0) {
            pthread_mutex_lock(&s_fips.lock);
            snprintf(s_tr.status, sizeof(s_tr.status), "no daemon");
            pthread_mutex_unlock(&s_fips.lock);
            s_tr.up = false;
            sleep(3);
            continue;
        }

        /* (re)open the listener */
        if(s_fips.listener < 0) {
            char cmd[128];
            snprintf(cmd, sizeof(cmd),
                     "{\"command\":\"listen\",\"params\":{\"local_port\":%d}}\n", s_fips.port);
            char reply[1024];
            int fd = fips_setup(s_fips.api_sock, cmd, reply, sizeof(reply));
            if(fd < 0) {
                pthread_mutex_lock(&s_fips.lock);
                snprintf(s_tr.status, sizeof(s_tr.status), "daemon busy");
                pthread_mutex_unlock(&s_fips.lock);
                s_tr.up = false;
                sleep(3);
                continue;
            }
            s_fips.listener = fd;
            s_tr.up = true;
            pthread_mutex_lock(&s_fips.lock);
            snprintf(s_tr.status, sizeof(s_tr.status), "mesh port %d", s_fips.port);
            pthread_mutex_unlock(&s_fips.lock);
        }

        refresh_peers();
        flush_outbox();

        struct pollfd pfd[FIPS_MAX_FLOWS + 1];
        int nfd = 0;
        pfd[nfd].fd = s_fips.listener;
        pfd[nfd].events = POLLIN;
        nfd++;
        int map[FIPS_MAX_FLOWS + 1];
        map[0] = -1;
        pthread_mutex_lock(&s_fips.lock);
        for(int i = 0; i < FIPS_MAX_FLOWS; i++) {
            if(!s_fips.flows[i].used) continue;
            pfd[nfd].fd = s_fips.flows[i].fd;
            pfd[nfd].events = POLLIN;
            map[nfd] = i;
            nfd++;
        }
        pthread_mutex_unlock(&s_fips.lock);

        int pr = poll(pfd, (nfds_t)nfd, 500);
        if(pr <= 0) continue;

        if(pfd[0].revents & POLLIN) {
            char buf[2048];
            ssize_t n = recv(s_fips.listener, buf, sizeof(buf) - 1, 0);
            if(n > 0) {
                buf[n] = 0;
                handle_arrival(buf);
            }
            else if(n == 0) {
                close(s_fips.listener);
                s_fips.listener = -1;
            }
        }
        for(int i = 1; i < nfd; i++) {
            if(!(pfd[i].revents & (POLLIN | POLLHUP))) continue;
            int idx = map[i];
            if(idx < 0) continue;
            read_flow(idx);
        }
    }

    return NULL;
}

/* ---------- the transport ---------- */

static bool fips_connect(chat_transport_t * t)
{
    if(s_fips.thread_running) {
        struct stat st;
        return stat(s_fips.api_sock, &st) == 0;
    }
    resolve_paths();
    if(s_fips.port <= 0) s_fips.port = FIPS_DEFAULT_PORT;

    pthread_mutex_init(&s_fips.lock, NULL);
    s_fips.listener = -1;
    s_fips.stop = false;
    for(int i = 0; i < FIPS_MAX_FLOWS; i++) s_fips.flows[i].fd = -1;

    if(pthread_create(&s_fips.thread, NULL, fips_worker, NULL) != 0) {
        snprintf(t->status, sizeof(t->status), "no thread");
        return false;
    }
    s_fips.thread_running = true;
    snprintf(t->status, sizeof(t->status), "starting");
    return true;
}

static void fips_disconnect(chat_transport_t * t)
{
    (void)t;
    s_fips.stop = true;
}

static int fips_poll(chat_transport_t * t)
{
    int got = 0;
    for(;;) {
        fips_in_t in;
        pthread_mutex_lock(&s_fips.lock);
        if(s_fips.in_count == 0) {
            pthread_mutex_unlock(&s_fips.lock);
            break;
        }
        in = s_fips.inbox[s_fips.in_head];
        s_fips.in_head = (s_fips.in_head + 1) % FIPS_INBOX;
        s_fips.in_count--;
        pthread_mutex_unlock(&s_fips.lock);

        chat_deliver(chat_transport_index(t), "", in.from, in.text);
        got++;
    }
    return got;
}

static bool fips_send(chat_transport_t * t, const char * text, const char * id)
{
    (void)t;
    pthread_mutex_lock(&s_fips.lock);
    if(s_fips.out_count < FIPS_OUTBOX) {
        int idx = (s_fips.out_head + s_fips.out_count) % FIPS_OUTBOX;
        snprintf(s_fips.outbox[idx].id, sizeof(s_fips.outbox[idx].id), "%s", id ? id : "");
        snprintf(s_fips.outbox[idx].text, sizeof(s_fips.outbox[idx].text), "%s", text);
        s_fips.out_count++;
    }
    pthread_mutex_unlock(&s_fips.lock);
    return true;
}

/* how many peers the daemon knows about */
int chat_fips_peer_count(void)
{
    pthread_mutex_lock(&s_fips.lock);
    int n = s_fips.peer_count;
    pthread_mutex_unlock(&s_fips.lock);
    return n;
}

void chat_fips_register(void)
{
    memset(&s_fips, 0, sizeof(s_fips));
    s_fips.port = getenv("PDA_FIPS_PORT") ? atoi(getenv("PDA_FIPS_PORT")) : FIPS_DEFAULT_PORT;
    if(s_fips.port <= 0) s_fips.port = FIPS_DEFAULT_PORT;

    s_tr.tag = "FIPS";
    s_tr.label = "FIPS mesh";
    s_tr.connect = fips_connect;
    s_tr.disconnect = fips_disconnect;
    s_tr.poll = fips_poll;
    s_tr.send = fips_send;
    s_tr.priv = &s_fips;
    snprintf(s_tr.status, sizeof(s_tr.status), "no daemon");
    chat_add_transport(&s_tr);
}
