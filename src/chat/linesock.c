#include "chat/linesock.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void ls_init(ls_t * l)
{
    l->fd = -1;
    l->len = 0;
}

int ls_read_line(ls_t * l, char * out, size_t outn)
{
    if(l->fd < 0) return -1;
    for(;;) {
        /* a complete line already in the buffer? */
        char * nl = memchr(l->buf, '\n', (size_t)l->len);
        if(nl) {
            int n = (int)(nl - l->buf);
            size_t copy = (size_t)n < outn - 1 ? (size_t)n : outn - 1;
            memcpy(out, l->buf, copy);
            out[copy] = 0;
            int rest = l->len - n - 1;
            if(rest > 0) memmove(l->buf, nl + 1, (size_t)rest);
            l->len = rest;
            return 1;
        }
        if(l->len == (int)sizeof(l->buf)) {   /* runaway line: drop it */
            l->len = 0;
        }
        ssize_t n = read(l->fd, l->buf + l->len, sizeof(l->buf) - (size_t)l->len);
        if(n > 0) {
            l->len += (int)n;
            continue;
        }
        if(n == 0) return -1;                 /* peer closed */
        if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return 0;
        return -1;
    }
}

int ls_write(ls_t * l, const char * data, size_t n)
{
    if(l->fd < 0) return -1;
    size_t off = 0;
    int tries = 0;
    while(off < n && tries < 200) {
        ssize_t w = write(l->fd, data + off, n - off);
        if(w > 0) {
            off += (size_t)w;
            tries = 0;
            continue;
        }
        if(w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            tries++;
            continue;
        }
        return -1;
    }
    return off == n ? 0 : -1;
}

int ls_write_line(ls_t * l, const char * line)
{
    char buf[LS_BUF];
    size_t n = strlen(line);
    if(n + 2 > sizeof(buf)) n = sizeof(buf) - 2;
    memcpy(buf, line, n);
    buf[n] = '\n';
    return ls_write(l, buf, n + 1);
}
