#ifndef PDA_CHAT_LINESOCK_H
#define PDA_CHAT_LINESOCK_H

#include <stdbool.h>
#include <stddef.h>

/* A tiny line-oriented socket helper: the carriers that speak the JSON
   envelope over a stream socket (the relay, the bitchat bridge) all need the
   same three things - accumulate bytes until a newline, hand out one line at
   a time, and write a line without caring about partial writes. */

#define LS_BUF 4096

typedef struct {
    int fd;
    int len;
    char buf[LS_BUF];
} ls_t;

void ls_init(ls_t * l);
/* 1 = a line was read (newline stripped), 0 = nothing yet, -1 = closed */
int ls_read_line(ls_t * l, char * out, size_t outn);
/* 0 = ok, -1 = the socket is gone */
int ls_write(ls_t * l, const char * data, size_t n);
int ls_write_line(ls_t * l, const char * line);

#endif
