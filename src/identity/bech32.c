#include "identity/bech32.h"

#include <string.h>

#define BECH32_MAX 90      /* BIP-173: a bech32 string is never longer */

static const char CHARSET[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

static int charset_rev(char c)
{
    const char * p = strchr(CHARSET, c);
    if(!p || c == 0) return -1;
    return (int)(p - CHARSET);
}

static uint32_t polymod(const uint8_t * values, size_t len)
{
    static const uint32_t GEN[5] = {
        0x3b6a57b2u, 0x26508e6du, 0x1ea119fau, 0x3d4233ddu, 0x2a1462b3u
    };
    uint32_t chk = 1;
    for(size_t i = 0; i < len; i++) {
        uint8_t top = (uint8_t)(chk >> 25);
        chk = ((chk & 0x1ffffffu) << 5) ^ (uint32_t)values[i];
        for(int k = 0; k < 5; k++)
            if((top >> k) & 1u) chk ^= GEN[k];
    }
    return chk;
}

/* hrp expansion, as it goes into the checksum: high bits, 0, low bits */
static size_t expand_hrp(const char * hrp, uint8_t * out, size_t outn)
{
    size_t n = 0;
    size_t hl = strlen(hrp);
    for(size_t i = 0; i < hl && n + 1 < outn; i++) out[n++] = (uint8_t)(hrp[i] >> 5);
    if(n + 1 < outn) out[n++] = 0;
    for(size_t i = 0; i < hl && n + 1 < outn; i++) out[n++] = (uint8_t)(hrp[i] & 0x1f);
    return n;
}

/* hrp expansion + payload + the 6 zeros, as one buffer (the checksummed
   input of encoding) */
static size_t expand(const char * hrp, const uint8_t * data, size_t data_len,
                     uint8_t * out, size_t outn)
{
    size_t n = expand_hrp(hrp, out, outn);
    for(size_t i = 0; i < data_len && n + 1 < outn; i++) out[n++] = data[i];
    for(int i = 0; i < 6 && n + 1 < outn; i++) out[n++] = 0;
    return n;
}

/* regroup the bits: frombits -> tobits, with or without padding */
static bool convert_bits(const uint8_t * in, size_t inlen, int frombits, int tobits,
                         bool pad, uint8_t * out, size_t * outlen)
{
    uint32_t acc = 0;
    int bits = 0;
    uint32_t maxv = (1u << tobits) - 1u;
    size_t o = 0;
    for(size_t i = 0; i < inlen; i++) {
        uint32_t v = in[i];
        if((v >> frombits) != 0) return false;
        acc = (acc << frombits) | v;
        bits += frombits;
        while(bits >= tobits) {
            bits -= tobits;
            if(o >= *outlen) return false;
            out[o++] = (uint8_t)((acc >> bits) & maxv);
        }
    }
    if(pad) {
        if(bits > 0) {
            if(o >= *outlen) return false;
            out[o++] = (uint8_t)((acc << (tobits - bits)) & maxv);
        }
    }
    else if(bits >= frombits || ((acc << (tobits - bits)) & maxv) != 0) {
        return false;                      /* not full bytes: not valid */
    }
    *outlen = o;
    return true;
}

bool bech32_encode(const char * hrp, const uint8_t * data, size_t data_len,
                   char * out, size_t outn)
{
    if(!hrp || !*hrp || !data || !out || outn == 0) return false;

    uint8_t groups[64];
    size_t glen = sizeof(groups);
    if(data_len > sizeof(groups)) return false;
    if(!convert_bits(data, data_len, 8, 5, true, groups, &glen)) return false;

    size_t hl = strlen(hrp);
    if(hl + 1 + glen + 6 + 1 > BECH32_MAX || hl + 1 + glen + 6 + 1 > outn)
        return false;

    uint8_t buf[BECH32_MAX];
    size_t bn = expand(hrp, groups, glen, buf, sizeof(buf));
    uint32_t mod = polymod(buf, bn) ^ 1u;

    size_t o = 0;
    for(size_t i = 0; i < hl; i++) out[o++] = (char)(hrp[i] | 0x20);   /* lower */
    out[o++] = '1';
    for(size_t i = 0; i < glen; i++) out[o++] = CHARSET[groups[i]];
    for(int i = 0; i < 6; i++) out[o++] = CHARSET[(mod >> (5 * (5 - i))) & 31u];
    out[o] = 0;
    return true;
}

bool bech32_decode(const char * str, const char * hrp,
                   uint8_t * out, size_t outn, size_t * out_len)
{
    if(!str || !*hrp || !out || !out_len) return false;
    *out_len = 0;

    size_t len = strlen(str);
    if(len < 8 || len > BECH32_MAX) return false;

    char lower[BECH32_MAX + 1];
    bool has_up = false, has_low = false;
    for(size_t i = 0; i < len; i++) {
        if(str[i] >= 'A' && str[i] <= 'Z') has_up = true;
        else if(str[i] >= 'a' && str[i] <= 'z') has_low = true;
        if(str[i] < 33 || str[i] > 126) return false;
    }
    if(has_up && has_low) return false;              /* mixed case is invalid */
    for(size_t i = 0; i < len; i++)
        lower[i] = (char)(str[i] >= 'A' && str[i] <= 'Z' ? str[i] - 'A' + 'a' : str[i]);
    lower[len] = 0;

    const char * one = strrchr(lower, '1');
    if(!one) return false;
    size_t split = (size_t)(one - lower);
    size_t data_len = len - split - 1;
    if(split < 1 || data_len < 6) return false;
    if(split != strlen(hrp)) return false;
    for(size_t i = 0; i < split; i++)
        if(lower[i] != hrp[i]) return false;

    uint8_t values[BECH32_MAX];
    size_t vn = 0;
    for(size_t i = split + 1; i < len; i++) {
        int v = charset_rev(lower[i]);
        if(v < 0) return false;
        values[vn++] = (uint8_t)v;
    }

    uint8_t buf[BECH32_MAX];
    /* the checksum covers hrp expansion + payload + checksum, verbatim:
       polymod of that must be 1 */
    size_t bn = expand_hrp(hrp, buf, sizeof(buf));
    for(size_t i = 0; i < vn && bn + 1 < sizeof(buf); i++) buf[bn++] = values[i];
    if(polymod(buf, bn) != 1u) return false;

    size_t olen = outn;
    if(!convert_bits(values, vn - 6, 5, 8, false, out, &olen)) return false;
    *out_len = olen;
    return true;
}
