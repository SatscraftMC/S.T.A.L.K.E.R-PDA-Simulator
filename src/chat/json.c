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
                        /* a surrogate pair is one character: the python
                           relay re-dumps lines with ensure_ascii=True, which
                           turns non-BMP characters into \uD83D\uDE00 - without
                           the combining, a signed message with an emoji would
                           re-serialize differently at the receiver and fail
                           the id recheck on an honest message */
                        if(cp >= 0xd800 && cp <= 0xdbff && p[1] == '\\' && p[2] == 'u') {
                            unsigned lo = 0;
                            int ok2 = 1;
                            for(int i = 3; i <= 6; i++) {
                                char h = p[i];
                                lo <<= 4;
                                if(h >= '0' && h <= '9') lo |= (unsigned)(h - '0');
                                else if(h >= 'a' && h <= 'f') lo |= (unsigned)(h - 'a' + 10);
                                else if(h >= 'A' && h <= 'F') lo |= (unsigned)(h - 'A' + 10);
                                else { ok2 = 0; break; }
                            }
                            if(ok2 && lo >= 0xdc00 && lo <= 0xdfff) {
                                cp = 0x10000u + ((cp - 0xd800u) << 10) + (lo - 0xdc00u);
                                p += 6;
                            }
                        }
                        if(cp < 0x80) c = (char)cp;
                        else if(cp < 0x800) {
                            if(o + 1 < outn) out[o++] = (char)(0xC0 | (cp >> 6));
                            if(o + 1 < outn) out[o++] = (char)(0x80 | (cp & 0x3F));
                            c = 0;
                        }
                        else if(cp < 0x10000) {
                            if(o + 2 < outn) {
                                out[o++] = (char)(0xE0 | (cp >> 12));
                                out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                                out[o++] = (char)(0x80 | (cp & 0x3F));
                            }
                            c = 0;
                        }
                        else {
                            if(o + 3 < outn) {
                                out[o++] = (char)(0xF0 | (cp >> 18));
                                out[o++] = (char)(0x80 | ((cp >> 12) & 0x3F));
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
   end. returns the position after it. Text inside strings is copied byte
   for byte - escapes included: a dropped backslash turns a quote escape
   into a string end (and a unicode escape into literal letters) two parses
   later, which is exactly how a re-dumped emoji dies. */
static const char * read_raw(const char * p, char * out, size_t outn)
{
    size_t o = 0;
    int depth = 0;
    bool in_str = false;
    while(*p) {
        if(in_str && *p == '\\' && p[1]) {
            if(o + 1 < outn) out[o++] = *p;      /* the backslash */
            p++;
            if(o + 1 < outn) out[o++] = *p;      /* and what it escapes */
            p++;
            continue;
        }
        if(in_str) {
            if(*p == '"') in_str = false;
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

        /* read straight into the caller's buffer: an intermediate 512-byte
           scratch silently *truncated* any longer value, and a truncated
           NIP-44 payload then failed the id recheck as "a broken id".
           Non-matching keys reset the buffer, so the failure contract
           (out[0] == '\0') is unchanged for every existing caller. */
        if(*p == '"') p = read_string(p + 1, out, outn);
        else p = read_raw(p, out, outn);

        if(strcmp(kbuf, key) == 0) return true;
        out[0] = 0;
    }
    out[0] = 0;
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
    /* byte by byte as *unsigned*: on this signed-char dev box a byte >=0x80
       read into a `char` is negative, and the `c < 0x20` test would mangle
       every UTF-8 byte into a bogus \uXXXX (on ARM, where char is unsigned,
       it would not - the output would be platform-dependent). Bytes >=0x80
       pass through raw: valid UTF-8 in JSON, and closer to NIP-01's
       verbatim-UTF-8 rule. Without this a non-ASCII name or message
       re-serializes differently at the verifier and fails the id recheck. */
    for(const unsigned char * p = (const unsigned char *)(in ? in : ""); *p; p++) {
        unsigned char c = *p;
        if(c == '"' || c == '\\') {
            if(o + 2 >= outn) break;
            out[o++] = '\\';
            out[o++] = (char)c;
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
            if(o + 6 >= outn) break;
            o += (size_t)snprintf(out + o, outn - o, "\\u%04x", (unsigned)c);
        }
        else {
            if(o + 1 >= outn) break;
            out[o++] = (char)c;
        }
    }
    out[o] = 0;
    return o;
}

/* ---------- the tags array of a NIP-01 event ---------- */

/* step over one array element that we will not be using */
static const char * skip_value(const char * p)
{
    if(*p == '"') {
        char dump[8];
        return read_string(p + 1, dump, sizeof(dump));
    }
    if(*p == '{' || *p == '[') {
        char dump[4];
        return read_raw(p, dump, sizeof(dump));   /* stops before , or ] */
    }
    while(*p && *p != ',' && *p != ']' && *p != '}') p++;
    return p;
}

/* One element of the tags array, *pp at its '['.
   Returns 1 with the first two strings as a pair, 2 when the element is not
   a well-formed string pair (*pp is left at its start for skip_value), 0 at
   the end of the array, -1 when there is no array at all. The first
   occurrence of a tag is the one that counts (the nostr convention). */
static int tag_next(const char ** pp, char * name, size_t namen,
                    char * val, size_t valn)
{
    const char * p = skip_ws(*pp);
    if(*p != '[') return -1;

    p = skip_ws(p + 1);
    if(*p == ']') { *pp = p + 1; return 0; }      /* the array is empty */
    if(*p != '"') return 2;
    p = read_string(p + 1, name, namen);
    p = skip_ws(p);
    if(*p != ',') return 2;                       /* a name with no value */
    p = skip_ws(p + 1);
    if(*p != '"') return 2;
    p = read_string(p + 1, val, valn);
    p = skip_ws(p);
    if(*p != ']') return 2;                       /* slice A's serialization
                                                     is pairs, nothing more */
    *pp = p + 1;
    return 1;
}

bool json_tags_rebuild(const char * obj, char * out, size_t outn)
{
    if(!obj || !out || outn == 0) return false;
    out[0] = 0;

    char raw[512];
    if(!json_get(obj, "tags", raw, sizeof(raw))) {
        /* no tags at all is a legal event: the canonical form is [] */
        snprintf(out, outn, "%s", "[]");
        return true;
    }

    const char * p = skip_ws(raw);
    if(*p != '[') return false;
    p++;

    size_t o = 0;
    out[o++] = '[';                      /* the canonical array wrapper */
    out[o] = 0;
    bool first = true;
    for(;;) {
        p = skip_ws(p);
        if(*p == ']') { p++; break; }
        if(*p == 0) return false;                 /* truncated input */
        if(*p == ',') { p++; continue; }

        char name[128] = "", val[128] = "";
        int r = tag_next(&p, name, sizeof(name), val, sizeof(val));
        if(r < 0) return false;
        if(r == 2) {
            /* not a string pair: skip it (a signed event then fails the id
               recheck honestly, an unsigned one displays best-effort) */
            p = skip_value(p);
            continue;
        }
        if(r == 0) break;

        char e_name[768], e_val[768];
        json_escape(name, e_name, sizeof(e_name));
        json_escape(val, e_val, sizeof(e_val));
        int w = snprintf(out + o, outn - o, "%s[\"%s\",\"%s\"]",
                         first ? "" : ",", e_name, e_val);
        if(w < 0 || (size_t)w >= outn - o) return false;
        o += (size_t)w;
        first = false;
    }
    if(o + 2 > outn) return false;
    out[o++] = ']';
    out[o] = 0;
    return true;
}

bool json_tag(const char * obj, const char * name, char * out, size_t outn)
{
    if(!obj || !name || !out || outn == 0) return false;
    out[0] = 0;

    char raw[512];
    if(!json_get(obj, "tags", raw, sizeof(raw))) return false;

    const char * p = skip_ws(raw);
    if(*p != '[') return false;
    p++;

    for(;;) {
        p = skip_ws(p);
        if(*p == 0) return false;
        if(*p == ']') return false;
        if(*p == ',') { p++; continue; }

        char t_name[128] = "", val[128] = "";
        int r = tag_next(&p, t_name, sizeof(t_name), val, sizeof(val));
        if(r < 0) return false;
        if(r == 2) { p = skip_value(p); continue; }
        if(r == 0) return false;

        if(strcmp(t_name, name) == 0) {           /* first occurrence wins */
            snprintf(out, outn, "%s", val);
            return true;
        }
    }
}

/* ---------- positional reads from a top-level array ---------- */

/* the shared walk of json_arr_get/json_arr_str: *want_str says whether the
   element has to be a quoted string. Returns true with out filled. */
static bool arr_get(const char * arr, int idx, char * out, size_t outn,
                    bool want_str)
{
    if(!arr || !out || outn == 0 || idx < 0) return false;
    out[0] = 0;

    const char * p = skip_ws(arr);
    if(*p != '[') return false;
    p++;
    p = skip_ws(p);
    if(*p == ']') return false;              /* an empty array has no idx 0 */

    int i = 0;
    for(;;) {
        p = skip_ws(p);
        if(*p == 0) return false;            /* truncated input */
        if(*p == ']') return false;          /* the index is not there */
        if(*p == ',') { p++; continue; }

        if(i == idx) {
            if(*p == '"') {
                p = read_string(p + 1, out, outn);
                return true;
            }
            if(want_str) { out[0] = 0; return false; }   /* a value, not a string */
            p = read_raw(p, out, outn);
            return true;
        }
        p = skip_value(p);
        i++;
    }
}

bool json_arr_get(const char * arr, int idx, char * out, size_t outn)
{
    return arr_get(arr, idx, out, outn, false);
}

bool json_arr_str(const char * arr, int idx, char * out, size_t outn)
{
    return arr_get(arr, idx, out, outn, true);
}
