#include "chat/json.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char * skip_ws(const char * p)
{
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

/* read a JSON string body (the opening quote is already consumed) into out,
   unescaping as it goes. returns the position after the closing quote. */
static const char * read_string(const char * p, char * out, size_t outn)
{
    size_t o = 0;
    if(outn == 0) return p;
    while(*p && *p != '"') {
        char c = *p;
        if(c == '\\' && p[1]) {
            p++;
            char e = *p;
            switch(e) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case 'b': c = '\b'; break;
                case 'f': c = '\f'; break;
                case '/': c = '/'; break;
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case 'u': {
                    unsigned cp = 0;
                    int ok = 1;
                    for(int i = 1; i <= 4; i++) {
                        char h = p[i];
                        cp <<= 4;
                        if(h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
                        else if(h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
                        else if(h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
                        else { ok = 0; break; }
                    }
                    if(!ok) { cp = '?'; }
                    else {
                        p += 4;
                        if(cp < 0x80) c = (char)cp;
                        else if(cp < 0x800) {
                            if(o + 1 < outn) out[o++] = (char)(0xC0 | (cp >> 6));
                            if(o + 1 < outn) out[o++] = (char)(0x80 | (cp & 0x3F));
                            c = 0;
                        }
                        else {
                            if(o + 2 < outn) {
                                out[o++] = (char)(0xE0 | (cp >> 12));
                                out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                                out[o++] = (char)(0x80 | (cp & 0x3F));
                            }
                            c = 0;
                        }
                    }
                    break;
                }
                default: c = e; break;
            }
        }
        if(c && o + 1 < outn) out[o++] = c;
        p++;
    }
    out[o] = 0;
    if(*p == '"') p++;
    return p;
}

/* read a raw value (number, bool, null, object, array) up to the matching
   end. returns the position after it. */
static const char * read_raw(const char * p, char * out, size_t outn)
{
    const char * start = p;
    size_t o = 0;
    int depth = 0;
    bool in_str = false;
    while(*p) {
        if(in_str) {
            if(*p == '\\' && p[1]) p++;
            else if(*p == '"') in_str = false;
        }
        else if(*p == '"') in_str = true;
        else if(*p == '{' || *p == '[') depth++;
        else if(*p == '}' || *p == ']') {
            if(depth == 0) break;
            depth--;
        }
        else if(*p == ',' && depth == 0) break;
        if(o + 1 < outn) out[o++] = *p;
        p++;
    }
    if(outn) out[o] = 0;
    (void)start;
    return p;
}

bool json_get(const char * obj, const char * key, char * out, size_t outn)
{
    if(!obj || !key || !out || outn == 0) return false;
    out[0] = 0;

    const char * p = skip_ws(obj);
    if(*p != '{') return false;
    p++;

    while(*p) {
        p = skip_ws(p);
        if(*p == '}' || *p == 0) break;
        if(*p == ',') { p++; continue; }
        if(*p != '"') break;

        char kbuf[64];
        size_t n = 0;
        p++;
        while(*p && *p != '"') {
            if(*p == '\\' && p[1]) p++;
            if(n + 1 < sizeof(kbuf)) kbuf[n++] = *p;
            p++;
        }
        if(*p != '"') break;
        kbuf[n] = 0;
        p++;

        p = skip_ws(p);
        if(*p != ':') break;
        p++;
        p = skip_ws(p);
        if(*p == 0) break;

        char val[512];
        val[0] = 0;
        if(*p == '"') p = read_string(p + 1, val, sizeof(val));
        else p = read_raw(p, val, sizeof(val));

        if(strcmp(kbuf, key) == 0) {
            snprintf(out, outn, "%s", val);
            return true;
        }
    }
    return false;
}

bool json_get_int(const char * obj, const char * key, long * out)
{
    char buf[64];
    if(!json_get(obj, key, buf, sizeof(buf))) return false;
    char * end = NULL;
    long v = strtol(buf, &end, 10);
    if(end == buf) return false;
    if(out) *out = v;
    return true;
}

size_t json_escape(const char * in, char * out, size_t outn)
{
    size_t o = 0;
    if(!out || outn == 0) return 0;
    for(const unsigned char * p = (const unsigned char *)(in ? in : ""); *p; p++) {
        char c = (char)*p;
        if(c == '"' || c == '\\') {
            if(o + 2 >= outn) break;
            out[o++] = '\\';
            out[o++] = c;
        }
        else if(c == '\n') {
            if(o + 2 >= outn) break;
            out[o++] = '\\';
            out[o++] = 'n';
        }
        else if(c == '\r') {
            if(o + 2 >= outn) break;
            out[o++] = '\\';
            out[o++] = 'r';
        }
        else if(c == '\t') {
            if(o + 2 >= outn) break;
            out[o++] = '\\';
            out[o++] = 't';
        }
        else if(c < 0x20) {
            if(o + 4 >= outn) break;
            o += (size_t)snprintf(out + o, outn - o, "\\u%04x", (unsigned)c);
        }
        else {
            if(o + 1 >= outn) break;
            out[o++] = c;
        }
    }
    out[o] = 0;
    return o;
}
