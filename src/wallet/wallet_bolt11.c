#include "wallet/wallet_bolt11.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

/* The shared bolt11 reader (see wallet_bolt11.h). The amount half is the
   mock's own parser, moved out of wallet_mock_check() unchanged - every
   refusal string and every rounding rule below is the one the wallet tests
   already pin. The tag half is this slice's, walked in 5-bit groups the way
   BOLT-11 lays the data part out (7 groups timestamp, then tagged fields of
   type:5 / length:10 / data, then the 104-group signature). */

#define BOLT11_MAX       640
#define BOLT11_MAX_SATS  2100000000000000LL   /* 21 000 000 BTC, all of it */

/* ---------- the amount reader (moved verbatim from wallet_mock.c) ---------- */

/* mant * 10^exp10 -> whole sats. exp10 runs from -16 to 8 for anything a
 * bolt11 amount can say, so the multiply side is the one that can run away
 * and it is capped before it gets near unsigned long long. */
static bool mant_to_sats(unsigned long long mant, int exp10, int64_t * out)
{
    if(exp10 >= 0) {
        for(int i = 0; i < exp10; i++) {
            if(mant > (unsigned long long)(BOLT11_MAX_SATS / 10)) return false;
            mant *= 10;
        }
        *out = (int64_t)mant;
        return true;
    }

    unsigned long long div = 1;
    for(int i = 0; i < -exp10; i++) div *= 10;   /* 10^16 at most: it fits */
    unsigned long long q = mant / div;
    if((mant % div) * 2 >= div) q++;             /* round half up */
    if(q > (unsigned long long)BOLT11_MAX_SATS) return false;
    *out = (int64_t)q;
    return true;
}

/* Blanks and line breaks are dropped first: what comes out of a paste is
   not always one clean line. */
static size_t clean_copy(const char * bolt11, char * out, size_t out_n)
{
    size_t n = 0;
    for(const char * c = bolt11 ? bolt11 : ""; *c && n + 1 < out_n; c++) {
        if(isspace((unsigned char)*c)) continue;
        out[n++] = *c;
    }
    out[n] = 0;
    return n;
}

/* the last '1' in the string: bech32 data never holds one, so it is the
   separator (a digit '1' inside the hrp's amount comes before it) */
static const char * last_sep(const char * clean)
{
    const char * sep = NULL;
    for(const char * c = clean; *c; c++) if(*c == '1') sep = c;
    return sep;
}

bool bolt11_parse(const char * bolt11, int64_t * sats_out,
                  char * err, size_t err_n)
{
    if(sats_out) *sats_out = 0;

    char clean[BOLT11_MAX + 8];
    size_t n = clean_copy(bolt11, clean, sizeof(clean));

    if(!n) {
        if(err && err_n) snprintf(err, err_n, "%s", "there is no invoice in the field");
        return false;
    }

    const char * sep = last_sep(clean);
    if(!sep) {
        if(err && err_n) snprintf(err, err_n, "%s", "that is not a bolt11 invoice");
        return false;
    }

    size_t hrp_len = (size_t)(sep - clean);
    size_t data_len = n - hrp_len - 1;
    if(data_len < 8) {                      /* bech32 data is never this short */
        if(err && err_n) snprintf(err, err_n, "%s", "that is not a bolt11 invoice");
        return false;
    }

    size_t pre;
    if(hrp_len >= 6 && strncasecmp(clean, "lnbcrt", 6) == 0) pre = 6;
    else if(hrp_len >= 4 && strncasecmp(clean, "lnbc", 4) == 0) pre = 4;
    else if(hrp_len >= 4 && strncasecmp(clean, "lntb", 4) == 0) pre = 4;
    else {
        if(err && err_n) snprintf(err, err_n, "%s", "that is not a bolt11 invoice");
        return false;
    }

    const char * a = clean + pre;
    size_t alen = hrp_len - pre;
    if(!alen) {
        if(err && err_n) snprintf(err, err_n, "%s",
                                   "that invoice does not say how much to pay");
        return false;
    }

    unsigned long long mant = 0;
    int nsig = 0;                 /* digits actually kept */
    int nf = 0;                   /* ...of which the fraction's */
    int mult_exp = 0;             /* value = mant / 10^nf / 10^mult_exp BTC */
    bool digits = false, dot = false;

    size_t i = 0;
    for(; i < alen; i++) {
        char c = a[i];
        if(c == '.') {
            if(dot) break;
            dot = true;
            continue;
        }
        if(c < '0' || c > '9') break;
        digits = true;
        if(dot) {
            if(nsig < 18 && nf < 12) { mant = mant * 10 + (unsigned)(c - '0'); nf++; nsig++; }
        }
        else {
            if(nsig >= 18) {                 /* more than any wallet holds */
                if(err && err_n) snprintf(err, err_n,
                                          "%s", "that invoice asks for more than any wallet holds");
                return false;
            }
            mant = mant * 10 + (unsigned)(c - '0');
            nsig++;
        }
    }

    if(!digits) {
        if(err && err_n) snprintf(err, err_n, "%s",
                                   "that invoice does not say how much to pay");
        return false;
    }

    if(i < alen) {
        char u = (char)tolower((unsigned char)a[i]);
        if(u == 'm') mult_exp = 3;
        else if(u == 'u') mult_exp = 6;
        else if(u == 'n') mult_exp = 9;
        else if(u == 'p') mult_exp = 12;
        else {
            if(err && err_n) snprintf(err, err_n, "%s", "that is not a bolt11 invoice");
            return false;
        }
        i++;
    }
    if(i != alen) {
        if(err && err_n) snprintf(err, err_n, "%s", "that is not a bolt11 invoice");
        return false;
    }

    /* sats = mant * 10^-nf * 10^-mult_exp * 10^8 */
    int64_t sats = 0;
    if(!mant_to_sats(mant, 8 - nf - mult_exp, &sats)) {
        if(err && err_n) snprintf(err, err_n, "%s",
                                   "that invoice asks for more than any wallet holds");
        return false;
    }
    if(sats < 1) {
        if(err && err_n) snprintf(err, err_n, "%s",
                                   "that invoice is for less than one sat");
        return false;
    }

    if(sats_out) *sats_out = sats;
    return true;
}

/* ---------- the tagged fields ---------- */

static int bech_val(char c)
{
    static const char cs[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    const char * p = strchr(cs, c);
    if(!p) return -1;
    return (int)(p - cs);
}

/* the data part as 5-bit groups, checksum (6 groups) already dropped */
static bool groups(const char * clean, const char * sep, uint8_t * out,
                   size_t out_n, size_t * out_len)
{
    const char * data = sep + 1;
    size_t n = strlen(data);
    if(n < 6) return false;
    n -= 6;
    if(n > out_n) return false;
    for(size_t i = 0; i < n; i++) {
        int v = bech_val(data[i]);
        if(v < 0) return false;
        out[i] = (uint8_t)v;
    }
    *out_len = n;
    return true;
}

typedef struct {
    const uint8_t * g;
    size_t ng;
    size_t bit;                 /* the next bit index */
} bits_t;

static int bit_take(bits_t * b)
{
    size_t byte = b->bit / 5, off = b->bit % 5;
    if(byte >= b->ng) return -1;
    int v = (b->g[byte] >> (4 - off)) & 1;
    b->bit++;
    return v;
}

/* Walk the tagged fields; fills whichever outputs are non-NULL. Returns false
   only when the data part is not walkable at all - a missing tag is not a
   failure, the caller applies its own fallback. */
static bool walk_tags(const char * clean, const char * sep,
                      char * hash_hex, long * expiry)
{
    static uint8_t gs[BOLT11_MAX + 8];
    size_t ng = 0;
    if(!groups(clean, sep, gs, sizeof(gs), &ng)) return false;

    bits_t b = { gs, ng, 0 };

    /* 35 bits of timestamp, then the fields, then the 104-group signature
       (520 bits) which is not a tagged field and never walked */
    if(ng * 5 < 35 + 520) return false;
    b.bit = 35;
    size_t end = ng * 5 - 520;

    bool got_hash = false, got_exp = false;
    while(b.bit + 15 <= end) {
        int type = 0, len = 0;
        for(int k = 0; k < 5; k++) type = (type << 1) | bit_take(&b);
        for(int k = 0; k < 10; k++) len = (len << 1) | bit_take(&b);
        if(b.bit + (size_t)len * 5 > end) break;      /* truncated: stop here */

        if(type == 1 && hash_hex && !got_hash && len * 5 >= 256) {
            char out[65];
            static const char h[] = "0123456789abcdef";
            for(int byte = 0; byte < 32; byte++) {
                int v = 0;
                for(int k = 0; k < 8; k++) v = (v << 1) | bit_take(&b);
                out[byte * 2] = h[(v >> 4) & 0xf];
                out[byte * 2 + 1] = h[v & 0xf];
            }
            out[64] = 0;
            /* the field may be a little longer than 256 bits: skip the rest */
            for(size_t k = 256; k < (size_t)len * 5; k++) (void)bit_take(&b);
            memcpy(hash_hex, out, 65);
            got_hash = true;
        }
        else if(type == 6 && expiry && !got_exp) {
            long v = 0;
            bool over = false;
            for(int k = 0; k < len * 5; k++) {
                int bit = bit_take(&b);
                if(v > 100000000L) { over = true; continue; }
                v = v * 2 + bit;
            }
            if(!over) { *expiry = v; got_exp = true; }
        }
        else {
            for(int k = 0; k < len * 5; k++) (void)bit_take(&b);
        }

        if((!hash_hex || got_hash) && (!expiry || got_exp)) break;
    }
    return true;
}

bool bolt11_payment_hash(const char * bolt11, char * hex65)
{
    if(!hex65) return false;
    hex65[0] = 0;

    char clean[BOLT11_MAX + 8];
    if(!clean_copy(bolt11, clean, sizeof(clean))) return false;
    const char * sep = last_sep(clean);
    if(!sep) return false;

    char out[65];
    if(!walk_tags(clean, sep, out, NULL)) return false;
    if(!out[0]) return false;
    memcpy(hex65, out, 65);
    return true;
}

bool bolt11_expiry(const char * bolt11, long * secs_out)
{
    if(!secs_out) return false;
    *secs_out = 0;

    char clean[BOLT11_MAX + 8];
    if(!clean_copy(bolt11, clean, sizeof(clean))) return false;
    const char * sep = last_sep(clean);
    if(!sep) return false;

    long v = 0;
    if(!walk_tags(clean, sep, NULL, &v)) return false;
    if(v <= 0) return false;
    *secs_out = v;
    return true;
}
