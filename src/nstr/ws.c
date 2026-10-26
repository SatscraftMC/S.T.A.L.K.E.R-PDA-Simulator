#include "nstr/nstr_int.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>

/* RFC 6455 - the client half of the WebSocket protocol, written here
   because the app offers no extensions and needs nothing else: the
   handshake (with a random key and the server's accept verified against
   it), masked client frames, and the accept hash the other side computes
   back. One complete text message is one NIP-01 array; there is no
   permessage-deflate to negotiate or decompress. */

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* SHA1(key + GUID), base64 - what the server must send back as
   Sec-WebSocket-Accept. One OpenSSL call each; no crypto is written here. */
static bool accept_of(const char * key, char * out, size_t outn)
{
    char both[128];
    unsigned char sha[EVP_MAX_MD_SIZE];
    unsigned int n = 0;

    int w = snprintf(both, sizeof(both), "%s%s", key, WS_GUID);
    if(w <= 0 || (size_t)w >= sizeof(both)) return false;
    if(!EVP_Digest(both, (size_t)w, sha, &n, EVP_sha1(), NULL) || n != 20) return false;
    if(outn < 25) return false;
    out[0] = 0;
    if(EVP_EncodeBlock((unsigned char *)out, sha, 20) < 0) { out[0] = 0; return false; }
    return true;
}

bool ws_key_new(char * out, size_t outn)
{
    unsigned char raw[16];
    if(!out || outn < 25) return false;
    if(RAND_bytes(raw, (int)sizeof(raw)) != 1) return false;
    out[0] = 0;
    if(EVP_EncodeBlock((unsigned char *)out, raw, 16) < 0) { out[0] = 0; return false; }
    return true;
}

int ws_request(char * out, size_t outn, const char * path,
               const char * host_hdr, const char * key)
{
    char acc[32];
    if(!out || !path || !host_hdr || !key) return -1;
    if(!accept_of(key, acc, sizeof(acc))) return -1;

    /* No Sec-WebSocket-Extensions line at all: nothing is offered, so
       nothing has to be negotiated or decompressed. */
    int w = snprintf(out, outn,
                     "GET %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Upgrade: websocket\r\n"
                     "Connection: Upgrade\r\n"
                     "Sec-WebSocket-Key: %s\r\n"
                     "Sec-WebSocket-Version: 13\r\n"
                     "\r\n",
                     path, host_hdr, key);
    if(w < 0 || (size_t)w >= outn) return -1;
    return w;
}

/* the value of one header of a header block (case-insensitive name),
   trimmed, or NULL. hdr points just after the status line. */
static const char * header_val(const char * hdr, const char * name, char * out,
                               size_t outn)
{
    size_t namelen = strlen(name);
    out[0] = 0;
    for(const char * line = hdr; line && *line; ) {
        const char * eol = strstr(line, "\r\n");
        const char * end = eol ? eol : line + strlen(line);
        if(end == line) break;                       /* the blank line */
        const char * colon = memchr(line, ':', (size_t)(end - line));
        if(colon && (size_t)(colon - line) == namelen &&
           strncasecmp(line, name, namelen) == 0) {
            const char * v = colon + 1;
            while(v < end && (*v == ' ' || *v == '\t')) v++;
            const char * ve = end;
            while(ve > v && (ve[-1] == ' ' || ve[-1] == '\t')) ve--;
            size_t n = (size_t)(ve - v);
            if(n >= outn) n = outn - 1;
            memcpy(out, v, n);
            out[n] = 0;
            return out;
        }
        line = eol ? eol + 2 : NULL;
    }
    return NULL;
}

/* a case-insensitive substring search: HTTP header values are tokens, and
   "Upgrade"/"Connection" are matched on the token, not its case */
static bool ci_has(const char * hay, const char * needle)
{
    size_t n = strlen(needle);
    if(!hay) return false;
    for(const char * p = hay; *p; p++)
        if(strncasecmp(p, needle, n) == 0) return true;
    return false;
}

bool ws_response_ok(const char * resp, const char * key)
{
    if(!resp || !key) return false;

    /* the status line: something/1.x 101 ... */
    const char * eol = strstr(resp, "\r\n");
    if(!eol) return false;
    const char * sp = strchr(resp, ' ');
    if(!sp || sp > eol || strncmp(sp + 1, "101", 3) != 0) return false;

    const char * hdrs = eol + 2;
    char val[128];
    if(!header_val(hdrs, "Upgrade", val, sizeof(val)) || !ci_has(val, "websocket"))
        return false;
    if(!header_val(hdrs, "Connection", val, sizeof(val)) || !ci_has(val, "upgrade"))
        return false;
    if(!header_val(hdrs, "Sec-WebSocket-Accept", val, sizeof(val))) return false;

    char acc[32];
    if(!accept_of(key, acc, sizeof(acc))) return false;
    return strcmp(val, acc) == 0;
}

static void ws_mask(unsigned char * buf, size_t len, const unsigned char key[4])
{
    for(size_t i = 0; i < len; i++) buf[i] ^= key[i % 4];
}

int ws_frame(unsigned char * out, size_t outn, int opcode,
             const void * payload, size_t len, bool fin)
{
    if(!out) return -1;
    size_t h = (len < 126) ? 2 : (len <= 0xffffu ? 4 : 10);
    size_t need = h + 4 + len;
    if(need > outn) return -1;

    out[0] = (unsigned char)((fin ? 0x80 : 0x00) | (opcode & 0x0f));
    if(len < 126) {
        out[1] = (unsigned char)(0x80 | len);
    }
    else if(len <= 0xffffu) {
        out[1] = 0x80 | 126;
        out[2] = (unsigned char)((len >> 8) & 0xff);
        out[3] = (unsigned char)(len & 0xff);
    }
    else {
        out[1] = (unsigned char)(0x80 | 127);
        for(int i = 0; i < 8; i++)
            out[2 + i] = (unsigned char)((len >> (56 - 8 * i)) & 0xff);
    }

    unsigned char key[4];
    if(RAND_bytes(key, 4) != 1) return -1;
    memcpy(out + h, key, 4);
    if(len && payload) memcpy(out + h + 4, payload, len);
    ws_mask(out + h + 4, len, key);            /* client frames are masked */
    return (int)(h + 4 + len);
}
