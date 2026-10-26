#include "wallet/wallet.h"
#include "wallet/wallet_bolt11.h"
#include "wallet/aes.h"
#include "chat/json.h"
#include "identity/identity.h"
#include "identity/state_file.h"
#include "nstr/nstr.h"
#include "ui/prefs.h"

#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_schnorrsig.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* The NIP-47 (NWC) wallet backend - slice B of spec-nip47-wallet.md.
 *
 * One connection file next to prefs.conf holds the user's own
 * nostr+walletconnect:// URI (pasted once on the not-set-up screen); this
 * file is the only place the URI's secret and the parsed connection ever
 * live - the rule identity.c draws around the device nsec, held the same
 * way here: the path string and the raw secret bytes stay in this file,
 * public parts (wallet key, relay URL) may be logged, the secret never is.
 *
 * The wire: a request event (kind 23194, authored by the connection key,
 * content NIP-04 encrypted) goes out over nstr - this file builds it and
 * waits for the answer, nstr moves the bytes. There is no transport in
 * here: no socket calls, no dials, no framing. src/nstr/nstr.h is the whole
 * outside world (spec section 4's consumer contract, and the checkable rule
 * that keeps the two apart).
 *
 * One operation at a time. A call the user made (make / pay / the first
 * fetch) blocks inside a bounded pump - steps of NWC_STEP_MS until the
 * answer lands or NWC_TOTAL_MS runs out, worst case one stall. Everything
 * the 250 ms tick drives (invoice lookups, balance refresh, the ERROR-state
 * retry) is stepped with a budget of 0: one non-blocking pass per tick, a
 * wait spans passes, the UI never stalls (section 5.7's hybrid).
 *
 * Files this backend owns:
 *   <config-dir>/wallet.nwc        the connection URI, one line, 0600
 *   <config-dir>/wallet.nwc.state  balance + history + the pending invoice,
 *                                  the mock's key=value shape with a 1024
 *                                  line reader, written by state_write()
 */

#define NDBG(...) do { if(getenv("PDA_DEBUG")) fprintf(stderr, __VA_ARGS__); } while(0)

#define NWC_URL_MAX      192      /* a relay URL */
#define NWC_LINE_MAX     1024     /* both file readers (section 5.6) */
#define NWC_CONTENT_MAX  2048     /* decrypted content, event builder out */
#define NWC_PLAIN_MAX    1400     /* payload before encryption (fits 2048) */
#define NWC_HIST_MAX     64
#define NWC_STEP_MS      400      /* one pump bite, the net.c relay style */
#define NWC_TOTAL_MS     5000     /* wall clock of one user-made operation */
#define NWC_REFRESH_SECS 10       /* balance refresh rate limit */
#define NWC_LOOKUP_SECS  3        /* invoice lookup cadence */
#define NWC_RETRY_SECS   30       /* ERROR-state retry cadence */
#define NWC_INV_FALLBACK 300      /* expiry when the invoice carries none */

#define NWC_KIND_REQUEST  23194   /* verified against NIP-47 at build */
#define NWC_KIND_RESPONSE 23195

/* the seam's two state moves, implemented in wallet.c - this file says what
   happened, wallet.c owns what the screens read */
void wallet_demote(const char * reason);
void wallet_promote(void);

/* ---------- the connection (parsed once, only ever in this file) ---------- */

typedef struct {
    char wallet_pub[65];          /* the service's x-only key, 64 hex */
    char relay[NWC_URL_MAX];      /* the first relay= parameter, decoded */
    uint8_t secret[32];           /* the URI's secret key - never logged */
} nwc_conn_t;

static char s_conn_path[560];
static char s_state_path[560];
static nwc_conn_t s_conn;
static bool s_have_conn = false;

static secp256k1_context * s_ctx = NULL;
static secp256k1_keypair s_kp;              /* the connection key */
static char s_client_pub[65] = "";           /* its x-only public hex */

/* ---------- local state (the mirror of wallet.nwc.state) ---------- */

static int64_t s_balance = 0;
static wallet_tx_t s_hist[NWC_HIST_MAX];     /* s_hist[0] is the newest */
static int s_hist_n = 0;
static wallet_invoice_t s_inv;
static bool s_inv_rowed = false;             /* the pending invoice's row landed */

static int64_t s_last_paid = -1;             /* the fee contract of section 6 */
static int64_t s_last_fee = -1;

static bool s_balance_wanted = false;        /* refresh-on-read flag (section 6) */
static time_t s_balance_done = 0;
static time_t s_last_lookup = 0;
static time_t s_last_retry = 0;

/* ---------- one operation ---------- */

typedef enum {
    NWC_OP_NONE = 0,
    NWC_OP_MAKE,
    NWC_OP_PAY,
    NWC_OP_LOOKUP,
    NWC_OP_BALANCE
} nwc_op_kind_t;

typedef enum { PH_CONNECT = 0, PH_SEND, PH_WAIT } nwc_ph_t;

static struct {
    nwc_op_kind_t kind;
    nwc_ph_t ph;
    nstr_t * h;
    char sub[65];                /* subscription id = our request event id */
    char req_id[65];
    bool done;                   /* the operation has an answer or a failure */
    bool ok;
    bool transport;              /* the failure was silence, not an answer */
    char err[192];
    time_t deadline;

    int64_t sats;                /* what this operation is about */
    char memo[48];

    char method[32];             /* what we asked, for result_type's check */
    char res[NWC_CONTENT_MAX];   /* the answer's result object */
} s_op;

static char s_pending_event[NWC_CONTENT_MAX];  /* built, not yet published */
static bool s_last_transport = false;          /* how the last finish failed */

/* ---------- small helpers ---------- */

static bool os_random(uint8_t * out, size_t n)
{
    /* /dev/urandom read right here (section 3): the wallet's keys and IVs
       never go through identity.c's randomness door - the two files share
       nothing, per the spec's bill of materials */
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd < 0) return false;
    size_t got = 0;
    while(got < n) {
        ssize_t r = read(fd, out + got, n - got);
        if(r <= 0) { close(fd); return false; }
        got += (size_t)r;
    }
    close(fd);
    return true;
}

static void bytes_to_hex(const uint8_t * in, size_t n, char * out)
{
    static const char h[] = "0123456789abcdef";
    for(size_t i = 0; i < n; i++) {
        out[i * 2] = h[in[i] >> 4];
        out[i * 2 + 1] = h[in[i] & 0x0f];
    }
    out[n * 2] = 0;
}

static int hex_val(char c)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool hex_ok(const char * s, size_t n)
{
    if(!s || strlen(s) != n) return false;
    for(size_t i = 0; i < n; i++) if(hex_val(s[i]) < 0) return false;
    return true;
}

static bool hex_to_bytes(const char * hex, size_t n, uint8_t * out, size_t outn)
{
    if(n % 2 != 0 || n / 2 > outn) return false;
    for(size_t i = 0; i < n / 2; i++) {
        int a = hex_val(hex[i * 2]), b = hex_val(hex[i * 2 + 1]);
        if(a < 0 || b < 0) return false;
        out[i] = (uint8_t)((a << 4) | b);
    }
    return true;
}

/* standard base64 with padding - NIP-04 content is base64 (an algorithm,
   not a dependency, per section 3) */
static size_t b64_encode(const uint8_t * in, size_t n, char * out, size_t outn)
{
    static const char a[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "abcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t o = 0;
    for(size_t i = 0; i < n; i += 3) {
        unsigned v = (unsigned)in[i] << 16;
        if(i + 1 < n) v |= (unsigned)in[i + 1] << 8;
        if(i + 2 < n) v |= in[i + 2];
        if(o + 4 >= outn) return 0;
        out[o++] = a[(v >> 18) & 63];
        out[o++] = a[(v >> 12) & 63];
        out[o++] = (i + 1 < n) ? a[(v >> 6) & 63] : '=';
        out[o++] = (i + 2 < n) ? a[v & 63] : '=';
    }
    if(o >= outn) return 0;
    out[o] = 0;
    return o;
}

static int b64_val(char c)
{
    if(c >= 'A' && c <= 'Z') return c - 'A';
    if(c >= 'a' && c <= 'z') return c - 'a' + 26;
    if(c >= '0' && c <= '9') return c - '0' + 52;
    if(c == '+') return 62;
    if(c == '/') return 63;
    return -1;
}

static bool b64_decode(const char * in, size_t n, uint8_t * out, size_t outn,
                       size_t * got)
{
    if(n == 0 || n % 4 != 0) return false;
    size_t o = 0;
    for(size_t i = 0; i < n; i += 4) {
        int v[4];
        int pad = 0;
        for(int k = 0; k < 4; k++) {
            char c = in[i + k];
            if(c == '=') { v[k] = 0; pad++; continue; }
            if(pad) return false;              /* padding only at the end */
            v[k] = b64_val(c);
            if(v[k] < 0) return false;
        }
        /* what this group really adds: the last group is short (pad), and
           an iv-sized output is exactly six groups - counting 3 here would
           refuse the final one of a 16-byte decode */
        int outb = 3 - pad;
        if(o + (size_t)outb > outn) return false;
        unsigned w = (unsigned)v[0] << 18 | (unsigned)v[1] << 12
                   | (unsigned)v[2] << 6 | (unsigned)v[3];
        out[o++] = (uint8_t)(w >> 16);
        if(pad < 2) out[o++] = (uint8_t)((w >> 8) & 0xff);
        if(pad < 1) out[o++] = (uint8_t)(w & 0xff);
    }
    if(got) *got = o;
    return true;
}

/* "wss://host[:port][/path]" -> "host[:port]": what the reason line names,
   built here from this file's own parse - opening the URL is nstr's call */
static void relay_host(char * out, size_t outn)
{
    const char * p = s_conn.relay;
    const char * scheme = strstr(p, "://");
    if(scheme) p = scheme + 3;
    size_t n = 0;
    while(p[n] && p[n] != '/') n++;
    if(!n) { snprintf(out, outn, "%s", "the relay"); return; }
    if(n >= outn) n = outn - 1;
    memcpy(out, p, n);
    out[n] = 0;
}

/* ---------- JSON past json.c's 512-byte staging buffer ---------- */

static const char * skip_ws(const char * p)
{
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

/* json_get() stages every value through a 512-byte buffer (chat/json.c), and
   a response event's encrypted content or a make_invoice result is longer
   than that - a truncated read here would fail the id recheck or hand a
   half-invoice to the screen. Same walk, value read into the caller's
   buffer as it goes. */
static bool big_json_get(const char * obj, const char * key,
                         char * out, size_t outn)
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
        p = skip_ws(p + 1);
        if(*p == 0) break;

        bool match = (strcmp(kbuf, key) == 0);
        bool quote = (*p == '"');
        size_t o = 0;
        bool over = false;

        if(quote) {
            const char * v = p + 1;
            p = v;
            while(*p && *p != '"') {
                if(*p == '\\' && p[1]) {
                    p++;
                    if(*p == 'u') {
                        unsigned cp = 0;
                        for(int i = 1; i <= 4; i++) {
                            char hch = p[i];
                            cp <<= 4;
                            if(hch >= '0' && hch <= '9') cp |= (unsigned)(hch - '0');
                            else if(hch >= 'a' && hch <= 'f') cp |= (unsigned)(hch - 'a' + 10);
                            else if(hch >= 'A' && hch <= 'F') cp |= (unsigned)(hch - 'A' + 10);
                            else { cp = '?'; break; }
                        }
                        p += 4;
                        if(match) {
                            if(cp < 0x80) {
                                if(o + 1 < outn) out[o++] = (char)cp;
                                else over = true;
                            }
                            else if(cp < 0x800) {
                                if(o + 2 < outn) {
                                    out[o++] = (char)(0xc0 | (cp >> 6));
                                    out[o++] = (char)(0x80 | (cp & 0x3f));
                                }
                                else over = true;
                            }
                            else {
                                if(o + 3 < outn) {
                                    out[o++] = (char)(0xe0 | (cp >> 12));
                                    out[o++] = (char)(0x80 | ((cp >> 6) & 0x3f));
                                    out[o++] = (char)(0x80 | (cp & 0x3f));
                                }
                                else over = true;
                            }
                        }
                        p++;
                        continue;
                    }
                    char c;
                    switch(*p) {
                        case 'n': c = '\n'; break;
                        case 't': c = '\t'; break;
                        case 'r': c = '\r'; break;
                        case 'b': c = '\b'; break;
                        case 'f': c = '\f'; break;
                        default: c = *p; break;
                    }
                    if(match) {
                        if(o + 1 < outn) out[o++] = c;
                        else over = true;
                    }
                    p++;
                    continue;
                }
                if(match) {
                    if(o + 1 < outn) out[o++] = *p;
                    else over = true;
                }
                p++;
            }
            if(*p == '"') p++;
            if(match) {
                if(over) return false;          /* did not fit: not ours */
                out[o] = 0;
                return true;
            }
        }
        else {
            const char * v = p;
            int depth = 0;
            bool in_str = false;
            while(*p) {
                if(in_str) {
                    if(*p == '\\' && p[1]) p += 2;
                    else { if(*p == '"') in_str = false; p++; }
                    continue;
                }
                if(*p == '"') { in_str = true; p++; continue; }
                if(*p == '{' || *p == '[') { depth++; p++; continue; }
                if(*p == '}' || *p == ']') {
                    if(depth == 0) break;
                    depth--; p++; continue;
                }
                if(*p == ',' && depth == 0) break;
                p++;
            }
            if(match) {
                const char * end = p;
                while(end > v &&
                      (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
                    end--;
                size_t len = (size_t)(end - v);
                if(len >= outn) return false;
                memcpy(out, v, len);
                out[len] = 0;
                return true;
            }
        }
    }
    return false;
}

static bool big_json_i64(const char * obj, const char * key, int64_t * out)
{
    char buf[32];
    if(!big_json_get(obj, key, buf, sizeof(buf))) return false;
    if(!buf[0]) return false;
    char * end = NULL;
    long long v = strtoll(buf, &end, 10);
    if(end == buf) return false;
    if(out) *out = (int64_t)v;
    return true;
}

/* ---------- the NIP-04 cipher (section 3) ---------- */

/* NIP-4's own note: nostr uses the shared X coordinate, un-hashed, as the
   AES key. libsecp256k1's default hashfp would SHA-256 the whole point,
   which is not what the other end computes - so this one copies X. */
static int nip04_x_only(unsigned char * out32, const unsigned char * x32,
                        const unsigned char * y32, void * data)
{
    (void)y32;
    (void)data;
    memcpy(out32, x32, 32);
    return 1;
}

static bool shared_secret(uint8_t out[32])
{
    if(!s_ctx || !s_have_conn) return false;
    uint8_t pub33[33];
    pub33[0] = 0x02;
    if(!hex_to_bytes(s_conn.wallet_pub, 64, pub33 + 1, 32)) return false;

    secp256k1_pubkey pk;
    if(!secp256k1_ec_pubkey_parse(s_ctx, &pk, pub33, sizeof(pub33))) return false;
    return secp256k1_ecdh(s_ctx, out, &pk, s_conn.secret, nip04_x_only, NULL) == 1;
}

static bool nip04_encrypt(const char * plain, char * out, size_t outn)
{
    size_t plen = strlen(plain);
    if(!plain || plen > NWC_PLAIN_MAX || !out || outn < 64) return false;

    uint8_t key[32];
    if(!shared_secret(key)) return false;
    uint8_t iv[16];
    if(!os_random(iv, sizeof(iv))) return false;

    uint8_t buf[NWC_PLAIN_MAX + 16];
    memcpy(buf, plain, plen);
    uint8_t pad = (uint8_t)(16 - (plen % 16));      /* PKCS#7 */
    memset(buf + plen, pad, pad);
    size_t total = plen + pad;

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, buf, total);

    char ct[NWC_PLAIN_MAX + 32];
    size_t cn = b64_encode(buf, total, ct, sizeof(ct));
    if(!cn) return false;
    char ivb[32];
    if(!b64_encode(iv, sizeof(iv), ivb, sizeof(ivb))) return false;

    int w = snprintf(out, outn, "%s?iv=%s", ct, ivb);
    if(w < 0 || (size_t)w >= outn) return false;
    return true;
}

static bool nip04_decrypt(const char * content, char * out, size_t outn)
{
    if(!content || !out || outn == 0) return false;
    out[0] = 0;

    const char * ivs = strstr(content, "?iv=");
    if(!ivs || ivs == content) return false;
    size_t ct_len = (size_t)(ivs - content);
    if(ct_len % 4 != 0) return false;

    uint8_t ct[NWC_CONTENT_MAX];
    uint8_t iv[16];
    size_t ct_n = 0, iv_n = 0;
    if(!b64_decode(content, ct_len, ct, sizeof(ct), &ct_n)) return false;
    if(!b64_decode(ivs + 4, strlen(ivs + 4), iv, sizeof(iv), &iv_n)) return false;
    if(iv_n != 16 || ct_n == 0 || ct_n % 16 != 0) return false;

    uint8_t key[32];
    if(!shared_secret(key)) return false;
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, ct, ct_n);

    uint8_t pad = ct[ct_n - 1];
    if(pad < 1 || pad > 16 || pad > ct_n) return false;
    for(size_t i = ct_n - pad; i < ct_n; i++) if(ct[i] != pad) return false;
    size_t plain_n = ct_n - pad;
    if(plain_n + 1 > outn) return false;
    memcpy(out, ct, plain_n);
    out[plain_n] = 0;
    return true;
}

/* The pinned self-check (section 3): at the top of wallet_nwc_open(), after
   the file parses and before any network, gated on PDA_DEBUG like slice A's
   [id] selftest. Two legs: the vendored AES against an answer OpenSSL
   produced at build time (an independent implementation - a symmetric bug
   would round trip happily), then a real NIP-04 round trip over this
   connection's own shared secret. */
static bool cipher_selftest(char * err, size_t err_n)
{
    static const uint8_t want[16] = {
        0xef, 0x82, 0xb6, 0x2b, 0xb1, 0xbc, 0xb5, 0xaf,
        0x78, 0x7c, 0xfc, 0x01, 0xe3, 0xd1, 0x91, 0x91
    };
    uint8_t key[32], iv[16];
    for(int i = 0; i < 32; i++) key[i] = (uint8_t)i;
    for(int i = 0; i < 16; i++) iv[i] = (uint8_t)i;

    uint8_t buf[16];
    memcpy(buf, "NIP04 selftest", 14);
    buf[14] = buf[15] = 0x02;                 /* PKCS#7 for a 14-byte input */

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, buf, sizeof(buf));
    if(memcmp(buf, want, sizeof(want)) != 0) goto broken;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, buf, sizeof(buf));
    if(memcmp(buf, "NIP04 selftest", 14) != 0 || buf[14] != 0x02 || buf[15] != 0x02)
        goto broken;

    char enc[NWC_CONTENT_MAX], dec[NWC_CONTENT_MAX];
    if(!nip04_encrypt("wallet selftest", enc, sizeof(enc))) goto broken;
    if(!nip04_decrypt(enc, dec, sizeof(dec)) || strcmp(dec, "wallet selftest") != 0)
        goto broken;

    if(getenv("PDA_DEBUG"))
        fprintf(stderr, "[wallet] nwc cipher selftest ok\n");
    return true;

broken:
    if(err && err_n)
        snprintf(err, err_n, "%s", "the wallet cipher failed its self test");
    fprintf(stderr, "[wallet] nwc cipher selftest FAILED\n");
    return false;
}

/* ---------- the connection file (section 2) ---------- */

static void resolve_paths(void)
{
    if(s_conn_path[0]) return;
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", prefs_path());
    char * slash = strrchr(dir, '/');
    if(slash) *slash = 0;
    snprintf(s_conn_path, sizeof(s_conn_path), "%s%swallet.nwc",
             dir[0] ? dir : "", dir[0] ? "/" : "");
    snprintf(s_state_path, sizeof(s_state_path), "%s%swallet.nwc.state",
             dir[0] ? dir : "", dir[0] ? "/" : "");
}

/* leading/trailing whitespace and line wraps off a pasted URI - the same
   treatment wallet_check_invoice documents for a pasted bolt11 */
static bool clean_text(const char * in, char * out, size_t out_n, size_t * len)
{
    size_t n = 0;
    for(const char * c = in ? in : ""; *c; c++) {
        if(*c == ' ' || *c == '\t' || *c == '\r' || *c == '\n') continue;
        if(n + 1 >= out_n) return false;
        out[n++] = *c;
    }
    out[n] = 0;
    if(len) *len = n;
    return true;
}

static bool pct_decode(const char * in, size_t n, char * out, size_t outn)
{
    size_t o = 0;
    for(size_t i = 0; i < n; i++) {
        char c = in[i];
        if(c == '%') {
            if(i + 2 >= n) return false;
            int a = hex_val(in[i + 1]), b = hex_val(in[i + 2]);
            if(a < 0 || b < 0) return false;
            c = (char)((a << 4) | b);
            i += 2;
        }
        if(o + 1 >= outn) return false;
        out[o++] = c;
    }
    out[o] = 0;
    return true;
}

/* Validate a URI into out. Nothing is written and nothing in s_conn moves
   until the caller has this back true - a refused paste must leave the
   previous connection (if any) exactly where it was. The pinned refusal
   strings below are the test plan's greps (rule 1). */
static bool parse_uri(const char * uri, nwc_conn_t * out, char * err, size_t err_n)
{
    static const char PFX[] = "nostr+walletconnect://";
    memset(out, 0, sizeof(*out));

    if(strncmp(uri, PFX, sizeof(PFX) - 1) != 0) {
        snprintf(err, err_n, "%s",
                 "not a wallet connection URI (expected nostr+walletconnect://)");
        return false;
    }

    const char * p = uri + sizeof(PFX) - 1;
    const char * q = strchr(p, '?');
    if(!q || q == p) {
        snprintf(err, err_n, "%s", "the connection is missing its wallet key");
        return false;
    }
    if((size_t)(q - p) != 64) {
        snprintf(err, err_n, "%s", "the connection is missing its wallet key");
        return false;
    }
    char key_hex[65];
    memcpy(key_hex, p, 64);
    key_hex[64] = 0;
    if(!hex_ok(key_hex, 64)) {
        snprintf(err, err_n, "%s", "the connection wallet key is not a valid key");
        return false;
    }
    if(s_ctx) {
        uint8_t pub33[33];
        pub33[0] = 0x02;
        hex_to_bytes(key_hex, 64, pub33 + 1, 32);
        secp256k1_pubkey pk;
        if(!secp256k1_ec_pubkey_parse(s_ctx, &pk, pub33, sizeof(pub33))) {
            snprintf(err, err_n, "%s", "the connection wallet key is not a valid key");
            return false;
        }
    }
    memcpy(out->wallet_pub, key_hex, 65);

    bool have_secret = false;
    const char * kv = q + 1;
    while(*kv) {
        const char * amp = strchr(kv, '&');
        size_t seg = amp ? (size_t)(amp - kv) : strlen(kv);
        const char * eq = memchr(kv, '=', seg);
        if(eq) {
            size_t kn = (size_t)(eq - kv);
            const char * val = eq + 1;
            size_t vn = seg - kn - 1;

            if(kn == 5 && strncmp(kv, "relay", 5) == 0) {
                if(!out->relay[0]) {            /* the first relay= wins (§2) */
                    char dec[NWC_URL_MAX];
                    if(!pct_decode(val, vn, dec, sizeof(dec)) || !dec[0]) {
                        snprintf(err, err_n, "%s", "the connection is missing its relay");
                        return false;
                    }
                    snprintf(out->relay, sizeof(out->relay), "%s", dec);
                }
                /* further relays are kept in the stored line untouched and
                   ignored here - fail-over is LATER (§2) */
            }
            else if(kn == 6 && strncmp(kv, "secret", 6) == 0) {
                have_secret = true;
                char sec[80];
                if(!pct_decode(val, vn, sec, sizeof(sec))) sec[0] = 0;
                if(strlen(sec) != 64 || !hex_ok(sec, 64)) {
                    snprintf(err, err_n, "%s",
                             "the connection secret is not 64 hex characters");
                    return false;
                }
                hex_to_bytes(sec, 64, out->secret, 32);
                if(s_ctx && !secp256k1_ec_seckey_verify(s_ctx, out->secret)) {
                    snprintf(err, err_n, "%s", "the connection secret is not a valid key");
                    return false;
                }
            }
            /* any other parameter (Alby's lud16 and friends) rides along in
               the stored line and is ignored here - never a refusal (§2) */
        }
        kv += seg;
        if(*kv == '&') kv++;
    }

    if(!out->relay[0]) {
        snprintf(err, err_n, "%s", "the connection is missing its relay");
        return false;
    }
    if(!have_secret) {
        snprintf(err, err_n, "%s", "the connection secret is not 64 hex characters");
        return false;
    }
    return true;
}

static bool load_conn(char * err, size_t err_n)
{
    resolve_paths();

    FILE * f = fopen(s_conn_path, "r");
    if(!f) {
        snprintf(err, err_n, "%s", "the wallet connection could not be read");
        return false;
    }
    char line[NWC_LINE_MAX];
    bool got = fgets(line, sizeof(line), f) != NULL;
    fclose(f);
    if(!got) {
        snprintf(err, err_n, "%s", "the wallet connection could not be read");
        return false;
    }

    size_t n = 0;
    char clean[NWC_LINE_MAX];
    if(!clean_text(line, clean, sizeof(clean), &n) || !n) {
        snprintf(err, err_n, "%s", "the wallet connection could not be read");
        return false;
    }

    nwc_conn_t parsed;
    if(!parse_uri(clean, &parsed, err, err_n)) return false;
    s_conn = parsed;
    return true;
}

bool wallet_nwc_conn_present(void)
{
    resolve_paths();
    struct stat st;
    return stat(s_conn_path, &st) == 0 && S_ISREG(st.st_mode);
}

static bool ensure_ctx(void)
{
    if(s_ctx) return true;
    s_ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
    uint8_t seed[32];
    if(os_random(seed, sizeof(seed))) {
        int blinded = secp256k1_context_randomize(s_ctx, seed);
        (void)blinded;             /* blinding is side-channel hygiene, best
                                      effort: the keys work the same without */
    }
    return true;
}

static bool adopt_conn(char * err, size_t err_n)
{
    ensure_ctx();
    if(!secp256k1_keypair_create(s_ctx, &s_kp, s_conn.secret)) {
        snprintf(err, err_n, "%s", "the connection secret is not a valid key");
        return false;
    }
    secp256k1_xonly_pubkey pk;
    uint8_t pub[32];
    if(!secp256k1_keypair_xonly_pub(s_ctx, &pk, NULL, &s_kp) ||
       !secp256k1_xonly_pubkey_serialize(s_ctx, pub, &pk)) {
        snprintf(err, err_n, "%s", "the connection secret is not a valid key");
        return false;
    }
    bytes_to_hex(pub, 32, s_client_pub);
    return true;
}

/* The URI door behind wallet_connect_uri(): validate, save atomically, then
   start the local state fresh - the cached balance and history belong to
   the previous wallet, and showing them under a new connection would be
   another wallet's numbers (§2). */
bool wallet_nwc_connect_uri(const char * uri, char * err, size_t err_n)
{
    if(err && err_n) err[0] = 0;
    ensure_ctx();

    char clean[NWC_LINE_MAX];
    size_t n = 0;
    if(!clean_text(uri, clean, sizeof(clean), &n) || !n) {
        snprintf(err, err_n, "%s", "the wallet connection could not be read");
        return false;
    }

    nwc_conn_t parsed;
    if(!parse_uri(clean, &parsed, err, err_n)) return false;

    /* only now does anything move: the file first, memory after */
    if(!state_write(s_conn_path, clean, n)) {
        snprintf(err, err_n, "%s", "the wallet connection could not be read");
        return false;
    }

    s_conn = parsed;
    if(!adopt_conn(err, err_n)) return false;
    s_have_conn = true;

    resolve_paths();
    remove(s_state_path);
    s_balance = 0;
    s_hist_n = 0;
    memset(&s_inv, 0, sizeof(s_inv));
    s_inv_rowed = false;
    s_last_paid = -1;
    s_last_fee = -1;
    s_balance_wanted = false;
    s_balance_done = 0;
    s_last_lookup = 0;
    s_last_retry = 0;
    memset(&s_op, 0, sizeof(s_op));

    NDBG("[wallet] nwc connection saved (wallet key %s, relay %s)\n",
         s_conn.wallet_pub, s_conn.relay);
    return true;
}

/* ---------- wallet.nwc.state ---------- */

static char s_save_buf[NWC_LINE_MAX * 10];

static void clean_memo(char * memo, size_t n)
{
    for(size_t i = 0; i < n && memo[i]; i++)
        if(memo[i] == '\n' || memo[i] == '\r' || memo[i] == '\t') memo[i] = ' ';
}

static void state_save(void)
{
    resolve_paths();
    size_t o = 0;
    int w = snprintf(s_save_buf, sizeof(s_save_buf),
                     "balance = %lld\n", (long long)s_balance);
    if(w < 0 || (size_t)w >= sizeof(s_save_buf)) return;
    o = (size_t)w;

    if(s_inv.valid && !s_inv_rowed) {
        char memo[48];
        snprintf(memo, sizeof(memo), "%s", s_inv.memo);
        clean_memo(memo, sizeof(memo));
        w = snprintf(s_save_buf + o, sizeof(s_save_buf) - o,
                     "invoice = %s|%lld|%lld|%lld|%s\n",
                     s_inv.bolt11, (long long)s_inv.made_at,
                     (long long)s_inv.expires_at, (long long)s_inv.sats, memo);
        if(w > 0 && o + (size_t)w < sizeof(s_save_buf)) o += (size_t)w;
    }

    for(int i = 0; i < s_hist_n && o + 160 < sizeof(s_save_buf); i++) {
        const wallet_tx_t * tx = &s_hist[i];
        char memo[48];
        snprintf(memo, sizeof(memo), "%s", tx->memo);
        clean_memo(memo, sizeof(memo));
        w = snprintf(s_save_buf + o, sizeof(s_save_buf) - o,
                     "history = %lld|%s|%lld|%s|%s\n",
                     (long long)tx->when, tx->outgoing ? "out" : "in",
                     (long long)tx->sats, tx->settled ? "paid" : "expired", memo);
        if(w < 0 || o + (size_t)w >= sizeof(s_save_buf)) break;
        o += (size_t)w;
    }

    if(!state_write(s_state_path, s_save_buf, o))
        NDBG("[wallet] nwc state not written\n");
}

static bool is_key(const char * line, const char * key)
{
    size_t n = strlen(key);
    if(strncmp(line, key, n) != 0) return false;
    return line[n] == ' ' || line[n] == '\t' || line[n] == '=';
}

static void chomp(char * line)
{
    size_t n = strlen(line);
    while(n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
}

static bool parse_ll(const char * s, long long * out)
{
    if(!s || !*s) return false;
    char * end = NULL;
    long long v = strtoll(s, &end, 10);
    if(end == s || *end) return false;
    *out = v;
    return true;
}

static bool slice(const char * a, const char * b, char * out, size_t n)
{
    if(!a || !b || b < a) return false;
    size_t len = (size_t)(b - a);
    if(len + 1 > n) return false;
    memcpy(out, a, len);
    out[len] = 0;
    return true;
}

/* "history = <when>|<in|out>|<sats>|<paid|expired>|<memo>" - the mock's row,
   memo last so a '|' in it is content, newest first on disk and in memory */
static void load_hist_line(const char * v)
{
    const char * p1 = strchr(v, '|'); if(!p1) return;
    const char * p2 = strchr(p1 + 1, '|'); if(!p2) return;
    const char * p3 = strchr(p2 + 1, '|'); if(!p3) return;
    const char * p4 = strchr(p3 + 1, '|'); if(!p4) return;

    char f_when[24], f_dir[12], f_sats[24], f_state[16];
    if(!slice(v, p1, f_when, sizeof(f_when))) return;
    if(!slice(p1 + 1, p2, f_dir, sizeof(f_dir))) return;
    if(!slice(p2 + 1, p3, f_sats, sizeof(f_sats))) return;
    if(!slice(p3 + 1, p4, f_state, sizeof(f_state))) return;

    long long when = 0, sats = 0;
    if(!parse_ll(f_when, &when) || !parse_ll(f_sats, &sats)) return;
    if(when < 0 || sats < 0) return;

    bool outgoing;
    if(!strcmp(f_dir, "out")) outgoing = true;
    else if(!strcmp(f_dir, "in")) outgoing = false;
    else return;

    bool settled;
    if(!strcmp(f_state, "paid")) settled = true;
    else if(!strcmp(f_state, "expired")) settled = false;
    else return;

    if(s_hist_n >= NWC_HIST_MAX) return;
    int i = s_hist_n++;
    memset(&s_hist[i], 0, sizeof(s_hist[i]));
    s_hist[i].outgoing = outgoing;
    s_hist[i].sats = sats;
    s_hist[i].when = (time_t)when;
    s_hist[i].settled = settled;
    snprintf(s_hist[i].memo, sizeof(s_hist[i].memo), "%s", p4 + 1);
}

/* "invoice = <bolt11>|<made_at>|<expires_at>|<sats>|<memo>" - everything the
   resume and the eventual history row need (section 6): a restart picks the
   lookup cadence back up instead of forgetting an invoice somebody may be
   about to pay */
static void load_invoice_line(const char * v)
{
    const char * p1 = strchr(v, '|'); if(!p1) return;
    const char * p2 = strchr(p1 + 1, '|'); if(!p2) return;
    const char * p3 = strchr(p2 + 1, '|'); if(!p3) return;
    const char * p4 = strchr(p3 + 1, '|'); if(!p4) return;

    char bolt[640], f_made[24], f_exp[24], f_sats[24];
    if(!slice(v, p1, bolt, sizeof(bolt))) return;
    if(!slice(p1 + 1, p2, f_made, sizeof(f_made))) return;
    if(!slice(p2 + 1, p3, f_exp, sizeof(f_exp))) return;
    if(!slice(p3 + 1, p4, f_sats, sizeof(f_sats))) return;

    long long made = 0, exp = 0, sats = 0;
    if(!parse_ll(f_made, &made) || !parse_ll(f_exp, &exp) || !parse_ll(f_sats, &sats))
        return;
    if(!bolt[0] || made <= 0 || exp <= 0 || sats <= 0) return;

    memset(&s_inv, 0, sizeof(s_inv));
    snprintf(s_inv.bolt11, sizeof(s_inv.bolt11), "%s", bolt);
    s_inv.made_at = (time_t)made;
    s_inv.expires_at = (time_t)exp;
    s_inv.sats = sats;
    snprintf(s_inv.memo, sizeof(s_inv.memo), "%s", p4 + 1);
    s_inv.valid = true;
    s_inv.state = WALLET_INVOICE_WAITING;   /* the clock decides EXPIRED */
    s_inv_rowed = false;
}

static void state_load(void)
{
    resolve_paths();
    s_balance = 0;
    s_hist_n = 0;
    memset(&s_inv, 0, sizeof(s_inv));
    s_inv_rowed = false;

    FILE * f = fopen(s_state_path, "r");
    if(!f) return;

    char line[NWC_LINE_MAX];
    while(fgets(line, sizeof(line), f)) {
        chomp(line);
        char * eq = strchr(line, '=');
        if(!eq) continue;
        char * v = eq + 1;
        while(*v == ' ' || *v == '\t') v++;

        if(is_key(line, "invoice")) { load_invoice_line(v); continue; }
        if(is_key(line, "history")) { load_hist_line(v); continue; }
        if(is_key(line, "balance")) {
            long long val = 0;
            if(parse_ll(v, &val) && val >= 0) s_balance = (int64_t)val;
        }
    }
    fclose(f);
}

static void push_history(bool outgoing, int64_t sats, const char * memo,
                         time_t when, bool settled)
{
    int n = (s_hist_n < NWC_HIST_MAX) ? s_hist_n + 1 : NWC_HIST_MAX;
    for(int i = n - 1; i > 0; i--) s_hist[i] = s_hist[i - 1];
    memset(&s_hist[0], 0, sizeof(s_hist[0]));
    s_hist[0].outgoing = outgoing;
    s_hist[0].sats = sats;
    snprintf(s_hist[0].memo, sizeof(s_hist[0].memo), "%s", memo ? memo : "");
    s_hist[0].when = when;
    s_hist[0].settled = settled;
    s_hist_n = n;
}

/* ---------- the request event (section 5.1) ---------- */

/* the canonical array of slice A's §3 - the same shape chat builds, over
   json.c: ["pubkey", created_at, kind, tags, content] -> sha256 -> BIP-340.
   chat_event_serialize() itself caps content at 240 bytes (chat.c:419),
   far below an encrypted NWC payload, hence this sibling. */
static bool nwc_serialize(char * out, size_t outn, const char * pubkey_hex,
                          long created_at, long kind, const char * tags_json,
                          const char * content)
{
    if(!out || outn == 0 || !pubkey_hex || !tags_json || !content) return false;
    out[0] = 0;
    char ec[NWC_CONTENT_MAX + 64];
    json_escape(content, ec, sizeof(ec));
    int w = snprintf(out, outn, "[\"%s\",%ld,%ld,%s,\"%s\"]",
                     pubkey_hex, created_at, kind, tags_json, ec);
    if(w < 0 || (size_t)w >= outn) { out[0] = 0; return false; }
    return true;
}

static bool nwc_sign(const char * id_hex, char * sig_hex)
{
    uint8_t msg[32];
    if(strlen(id_hex) != 64 || !hex_to_bytes(id_hex, 64, msg, sizeof(msg)))
        return false;
    uint8_t aux[32];
    if(!os_random(aux, sizeof(aux))) memset(aux, 0, sizeof(aux));
    uint8_t sig[64];
    if(!secp256k1_schnorrsig_sign32(s_ctx, sig, msg, &s_kp, aux)) return false;
    bytes_to_hex(sig, 64, sig_hex);
    return true;
}

static bool build_request(const char * method, const char * params_json,
                          char * event_out, size_t outn, char * id_out)
{
    char payload[NWC_PLAIN_MAX];
    int n = snprintf(payload, sizeof(payload), "{\"method\":\"%s\",\"params\":%s}",
                     method, params_json);
    if(n < 0 || (size_t)n >= sizeof(payload)) return false;

    char content[NWC_CONTENT_MAX];
    if(!nip04_encrypt(payload, content, sizeof(content))) return false;

    /* one p tag at the wallet service, plus the encryption tag NIP-47 asks
       the client to carry: this build speaks nip04 (verified against the
       NIP at build; reading the info event to negotiate is LATER - section
       3's monocypher contingency covers a service that demands nip44) */
    char tags[256];
    int tw = snprintf(tags, sizeof(tags),
                      "[[\"p\",\"%s\"],[\"encryption\",\"nip04\"]]",
                      s_conn.wallet_pub);
    if(tw < 0 || (size_t)tw >= sizeof(tags)) return false;

    char ser[NWC_CONTENT_MAX];
    long now = (long)time(NULL);
    if(!nwc_serialize(ser, sizeof(ser), s_client_pub, now, NWC_KIND_REQUEST,
                      tags, content))
        return false;
    if(!identity_event_id(ser, id_out)) return false;

    char sig[129];
    if(!nwc_sign(id_out, sig)) return false;

    int w = snprintf(event_out, outn,
                     "{\"id\":\"%s\",\"pubkey\":\"%s\",\"created_at\":%ld,"
                     "\"kind\":%d,\"tags\":%s,\"content\":\"%s\",\"sig\":\"%s\"}",
                     id_out, s_client_pub, now, NWC_KIND_REQUEST, tags,
                     content, sig);
    if(w < 0 || (size_t)w >= outn) { event_out[0] = 0; return false; }
    return true;
}

/* ---------- one session on the wire (sections 4, 5.2, 5.3) ---------- */

static void end_session(void)
{
    if(s_op.h) {
        if(nstr_state(s_op.h) == NSTR_UP && s_op.sub[0]) {
            char msg[160];
            int n = snprintf(msg, sizeof(msg), "[\"CLOSE\",\"%s\"]", s_op.sub);
            if(n > 0 && (size_t)n < sizeof(msg)) nstr_send(s_op.h, msg);
        }
        nstr_close(s_op.h);
    }
    s_op.h = NULL;
    s_op.sub[0] = 0;
    s_op.req_id[0] = 0;
}

static void fail_op(bool transport, const char * fmt, ...)
{
    s_op.ok = false;
    s_op.done = true;
    s_op.transport = transport;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_op.err, sizeof(s_op.err), fmt, ap);
    va_end(ap);
}

/* Service errors map through one fixed table (section 5.4): the situation
   the mock already words reuses its wording, every other known code gets
   one phrase from the NIP's own error list (read at build), and an unknown
   code is reported as itself - honest, no invention. Never the URI, never
   the secret. */
static void map_service_error(const char * code, const char * svc_msg)
{
    const char * phrase = NULL;
    bool balance_case = false;

    if(!code || !*code) code = "OTHER";

    if(!strcmp(code, "RATE_LIMITED"))
        phrase = "the wallet is rate limiting us - try again in a few seconds";
    else if(!strcmp(code, "NOT_IMPLEMENTED"))
        phrase = "the wallet does not know this command";
    else if(!strcmp(code, "INSUFFICIENT_BALANCE"))
        balance_case = true;
    else if(!strcmp(code, "QUOTA_EXCEEDED"))
        phrase = "the wallet has reached its spending quota";
    else if(!strcmp(code, "RESTRICTED"))
        phrase = "this connection is not allowed to do that";
    else if(!strcmp(code, "UNAUTHORIZED"))
        phrase = "this connection is not linked to a wallet";
    else if(!strcmp(code, "INTERNAL"))
        phrase = "the wallet hit an internal error";
    else if(!strcmp(code, "UNSUPPORTED_ENCRYPTION"))
        phrase = "the wallet does not accept this build's encryption";
    else if(!strcmp(code, "PAYMENT_FAILED"))
        phrase = "the payment did not go through";
    else if(!strcmp(code, "NOT_FOUND"))
        phrase = "the wallet does not know that invoice";
    else if(!strcmp(code, "OTHER"))
        phrase = (svc_msg && *svc_msg) ? svc_msg : "the wallet refused (OTHER)";
    else {
        snprintf(s_op.err, sizeof(s_op.err), "the wallet refused (%s)", code);
        fprintf(stderr, "[wallet] nwc refused: %s\n", s_op.err);
        return;
    }

    if(balance_case) {
        /* the mock's exact sentence (wallet_mock.c); Y is the amount only -
           the fee is unknown before the payment goes (section 6) */
        if(s_op.sats > 0)
            snprintf(s_op.err, sizeof(s_op.err),
                     "the balance is %lld sats and this needs %lld",
                     (long long)s_balance, (long long)s_op.sats);
        else
            snprintf(s_op.err, sizeof(s_op.err), "%s",
                     "the wallet does not have enough balance");
    }
    else {
        snprintf(s_op.err, sizeof(s_op.err), "%s", phrase);
    }
    fprintf(stderr, "[wallet] nwc refused: %s\n", s_op.err);
}

/* The answer arrived: author, e tag and a freshly recomputed id all check
   out (section 5.3 - a claimed id is never trusted on the money path), so
   this is the wallet talking. Then decrypt and parse. */
static void accept_response(const char * ev)
{
    if(s_op.done) return;

    char id[65] = "", pub[65] = "", sig[129] = "";
    char content[NWC_CONTENT_MAX];
    long created_at = 0, kind = 0;
    bool have_ts = json_get(ev, "id", id, sizeof(id)) &&
                   json_get(ev, "pubkey", pub, sizeof(pub)) &&
                   json_get(ev, "sig", sig, sizeof(sig)) &&
                   json_get_int(ev, "created_at", &created_at) &&
                   json_get_int(ev, "kind", &kind) &&
                   big_json_get(ev, "content", content, sizeof(content));

    if(!have_ts || kind != NWC_KIND_RESPONSE) return;
    if(!hex_ok(id, 64) || !hex_ok(pub, 64) || !hex_ok(sig, 128)) return;
    if(strcmp(pub, s_conn.wallet_pub) != 0) return;   /* the wallet, not a stranger */

    char e_tag[65] = "";
    if(!json_tag(ev, "e", e_tag, sizeof(e_tag)) || strcmp(e_tag, s_op.req_id) != 0)
        return;                                        /* not our answer */

    char tags_json[2048], ser[NWC_CONTENT_MAX], re_id[65];
    if(!json_tags_rebuild(ev, tags_json, sizeof(tags_json)) ||
       !nwc_serialize(ser, sizeof(ser), pub, created_at, kind, tags_json, content) ||
       !identity_event_id(ser, re_id) || strcmp(re_id, id) != 0 ||
       !identity_verify_id(pub, re_id, sig)) {
        NDBG("[wallet] nwc response failed its id check %s\n", id);
        return;                       /* not accepted: the wait goes on */
    }

    /* passed every check - now the payload has to be readable */
    char payload[NWC_CONTENT_MAX];
    if(!nip04_decrypt(content, payload, sizeof(payload))) {
        NDBG("[wallet] nwc unreadable response %s\n", id);
        fail_op(false, "%s", "the wallet sent something this build cannot read");
        return;
    }

    char rt[48] = "";
    big_json_get(payload, "result_type", rt, sizeof(rt));
    if(rt[0] && strcmp(rt, s_op.method) != 0)
        NDBG("[wallet] nwc result_type %s for %s\n", rt, s_op.method);

    char err_obj[NWC_CONTENT_MAX];
    if(big_json_get(payload, "error", err_obj, sizeof(err_obj)) &&
       err_obj[0] && strcmp(err_obj, "null") != 0) {
        char code[64] = "", msg[256] = "";
        big_json_get(err_obj, "code", code, sizeof(code));
        big_json_get(err_obj, "message", msg, sizeof(msg));
        s_op.ok = false;
        s_op.done = true;
        s_op.transport = false;
        map_service_error(code, msg);   /* fills s_op.err and logs once */
        return;
    }

    char res[NWC_CONTENT_MAX];
    if(!big_json_get(payload, "result", res, sizeof(res))) {
        NDBG("[wallet] nwc unreadable response %s\n", id);
        fail_op(false, "%s", "the wallet sent something this build cannot read");
        return;
    }
    snprintf(s_op.res, sizeof(s_op.res), "%s", res);
    s_op.ok = true;
    s_op.done = true;
}

static void dispatch(const char * msg)
{
    char verb[32];
    if(!json_arr_str(msg, 0, verb, sizeof(verb))) {
        NDBG("[wallet] nwc relay sent a message without a verb\n");
        return;
    }

    if(!strcmp(verb, "EVENT")) {
        char sub[64];
        if(!json_arr_str(msg, 1, sub, sizeof(sub)) || strcmp(sub, s_op.sub) != 0)
            return;                                  /* not our subscription */
        char ev[4096];
        if(!json_arr_get(msg, 2, ev, sizeof(ev))) return;
        accept_response(ev);
        return;
    }

    if(!strcmp(verb, "OK")) {
        char id[80];
        if(!json_arr_str(msg, 1, id, sizeof(id)) || strcmp(id, s_op.req_id) != 0)
            return;
        char flag[16];
        if(!json_arr_get(msg, 2, flag, sizeof(flag)) || !flag[0]) return;
        if(!strcmp(flag, "true")) return;           /* ack is not the answer */
        char why[256] = "";
        json_arr_get(msg, 3, why, sizeof(why));
        /* the relay answered us: its free text, not silence - no demote */
        fail_op(false, "the relay refused: %s", why[0] ? why : "no reason given");
        return;
    }

    if(!strcmp(verb, "CLOSED")) {
        /* the relay closed the subscription before an answer arrived: the
           wait rules treat that as silence, so the timeout reports honestly
           instead of inventing another failure word (decided at build) */
        char sub[64];
        if(json_arr_str(msg, 1, sub, sizeof(sub)) && strcmp(sub, s_op.sub) == 0)
            NDBG("[wallet] nwc subscription closed by the relay\n");
        return;
    }

    /* EOSE (the stored backlog ended; the answer may still arrive live),
       NOTICE and anything else: logged, ignored (section 5.2) */
    NDBG("[wallet] nwc relay: %s\n", verb);
}

static void op_step(int budget)
{
    if(s_op.kind == NWC_OP_NONE || !s_op.h) return;

    if(time(NULL) >= s_op.deadline) {
        fail_op(true, "%s", "the wallet did not answer");
        return;
    }

    if(!nstr_step(s_op.h, budget)) {
        const char * why = nstr_reason(s_op.h);
        NDBG("[wallet] nwc transport: %s\n", (why && *why) ? why : "gone");
        fail_op(true, "%s", "the wallet did not answer");
        return;
    }

    if(s_op.ph == PH_CONNECT && nstr_state(s_op.h) == NSTR_UP)
        s_op.ph = PH_SEND;

    if(s_op.ph == PH_SEND) {
        /* REQ first (the answer is matched against its id), then EVENT */
        char req[512];
        int n = snprintf(req, sizeof(req),
                         "[\"REQ\",\"%s\",{\"authors\":[\"%s\"],"
                         "\"kinds\":[%d],\"#e\":[\"%s\"],\"limit\":1}]",
                         s_op.sub, s_conn.wallet_pub, NWC_KIND_REQUEST,
                         s_op.req_id);
        if(n < 0 || (size_t)n >= sizeof(req) || !nstr_send(s_op.h, req)) {
            fail_op(true, "%s", "the wallet did not answer");
            return;
        }
        char publish[NWC_CONTENT_MAX + 64];
        n = snprintf(publish, sizeof(publish), "[\"EVENT\",%s]", s_pending_event);
        if(n < 0 || (size_t)n >= sizeof(publish) || !nstr_send(s_op.h, publish)) {
            fail_op(true, "%s", "the wallet did not answer");
            return;
        }
        s_op.ph = PH_WAIT;
        return;
    }

    if(s_op.ph == PH_WAIT) {
        char msg[4096];
        int got = 0;
        while(!s_op.done && got < 16 && nstr_recv(s_op.h, msg, sizeof(msg))) {
            got++;
            dispatch(msg);
        }
    }
}

static bool start_op(nwc_op_kind_t kind, const char * method,
                     const char * params_json, int64_t sats, const char * memo,
                     char * err, size_t err_n)
{
    /* a tick-driven op in flight yields to this one (section 5.7: the user
       asked; the background can start again afterwards) */
    if(s_op.kind != NWC_OP_NONE) {
        NDBG("[wallet] nwc op preempted (%d)\n", (int)s_op.kind);
        end_session();
    }
    memset(&s_op, 0, sizeof(s_op));
    s_op.kind = kind;
    snprintf(s_op.method, sizeof(s_op.method), "%s", method);
    s_op.sats = sats;
    snprintf(s_op.memo, sizeof(s_op.memo), "%s", memo ? memo : "");

    if(!build_request(method, params_json, s_pending_event,
                      sizeof(s_pending_event), s_op.req_id)) {
        snprintf(err, err_n, "%s",
                 "the wallet asked for something this build cannot send");
        s_op.kind = NWC_OP_NONE;
        return false;
    }
    snprintf(s_op.sub, sizeof(s_op.sub), "%s", s_op.req_id);

    s_op.h = nstr_open(s_conn.relay);
    if(!s_op.h) {
        snprintf(err, err_n, "%s", "no link");
        s_op.kind = NWC_OP_NONE;
        return false;
    }
    if(nstr_state(s_op.h) == NSTR_FAILED) {
        const char * why = nstr_reason(s_op.h);
        nstr_close(s_op.h);
        s_op.h = NULL;
        snprintf(err, err_n, "%s", (why && *why) ? why : "no link");
        s_op.kind = NWC_OP_NONE;
        return false;
    }

    s_op.ph = PH_CONNECT;
    s_op.deadline = time(NULL) + (NWC_TOTAL_MS + 999) / 1000;
    NDBG("[wallet] nwc op %s -> %s\n", method, s_conn.relay);
    return true;
}

/* user-initiated: a bounded pump, one stall at worst (section 5.7) */
static void run_op_blocking(void)
{
    while(!s_op.done) {
        op_step(NWC_STEP_MS);
        if(!s_op.done) usleep(5000);
    }
}

/* Tear the session down and report: a successful answer is applied to the
   local state, a failure is logged once, and the wallet demotes - only for
   silence (section 1). Returns the final verdict. */
static bool finish_op(char * err, size_t err_n)
{
    nwc_op_kind_t kind = s_op.kind;

    if(s_op.ok && kind == NWC_OP_MAKE) {
        char bolt[1024];
        if(!big_json_get(s_op.res, "invoice", bolt, sizeof(bolt)) || !bolt[0]) {
            fail_op(false, "%s", "the wallet sent something this build cannot read");
        }
        else if(strlen(bolt) >= sizeof(s_inv.bolt11)) {
            /* never truncated into an unpayable QR (section 6) */
            NDBG("[wallet] nwc invoice too long: %d\n", (int)strlen(bolt));
            fail_op(false, "%s", "the wallet returned an invoice this build cannot hold");
        }
        else {
            time_t now = time(NULL);
            memset(&s_inv, 0, sizeof(s_inv));
            snprintf(s_inv.bolt11, sizeof(s_inv.bolt11), "%s", bolt);
            s_inv.valid = true;
            s_inv.sats = s_op.sats;
            snprintf(s_inv.memo, sizeof(s_inv.memo), "%s", s_op.memo);
            s_inv.made_at = now;
            long secs = 0;
            if(bolt11_expiry(bolt, &secs) && secs > 0)
                s_inv.expires_at = now + (time_t)secs;
            else
                s_inv.expires_at = now + NWC_INV_FALLBACK;   /* mock's TTL */
            s_inv.state = WALLET_INVOICE_WAITING;
            s_inv_rowed = false;
            state_save();

            /* the same two lines the mock prints: the UI test reads the
               invoice back out of the log and checks it against the QR */
            fprintf(stderr, "[wallet] invoice: %s\n", s_inv.bolt11);
            if(s_inv.memo[0])
                fprintf(stderr, "[wallet] memo: %s\n", s_inv.memo);
            NDBG("[wallet] nwc invoice expires in %ld s\n",
                 (long)(s_inv.expires_at - s_inv.made_at));
        }
    }

    if(s_op.ok && kind == NWC_OP_PAY) {
        int64_t fee_msat = 0;
        bool have_fee = big_json_i64(s_op.res, "fees_paid", &fee_msat);
        s_last_paid = s_op.sats;
        s_last_fee = have_fee ? (fee_msat + 999) / 1000 : -1;   /* ceil (§6) */
        /* the cache is a fetched number moved by a confirmed payment: the
           amount leaves now, the fee too when the service named it, and the
           tick's refresh reconciles with a fetch either way */
        s_balance -= s_op.sats + (have_fee ? s_last_fee : 0);
        if(s_balance < 0) s_balance = 0;
        push_history(true, s_op.sats, s_op.memo, time(NULL), true);
        state_save();
        fprintf(stderr, "[wallet] nwc pay: %lld sats + %lld fee, balance %lld\n",
                (long long)s_op.sats, (long long)(have_fee ? s_last_fee : -1),
                (long long)s_balance);
    }

    if(s_op.ok && kind == NWC_OP_BALANCE) {
        int64_t msat = 0;
        if(!big_json_i64(s_op.res, "balance", &msat) || msat < 0)
            fail_op(false, "%s", "the wallet sent something this build cannot read");
        else {
            s_balance = msat / 1000;              /* floor: no sub-sat is shown */
            s_balance_done = time(NULL);
            s_balance_wanted = false;
            state_save();
            NDBG("[wallet] nwc balance: %lld sats\n", (long long)s_balance);
            wallet_promote();
        }
    }

    if(s_op.ok && kind == NWC_OP_LOOKUP) {
        char st[24] = "";
        big_json_get(s_op.res, "state", st, sizeof(st));
        int64_t settled_at = 0;
        bool have_settled = big_json_i64(s_op.res, "settled_at", &settled_at) &&
                            settled_at > 0;
        int64_t amount_msat = 0;
        bool have_amount = big_json_i64(s_op.res, "amount", &amount_msat);

        bool paid = !strcmp(st, "settled") ||
                    (!st[0] && have_settled);     /* state is optional (NIP) */
        bool dead = !strcmp(st, "expired") || !strcmp(st, "failed");

        if(paid) {
            int64_t sats = have_amount ? amount_msat / 1000 : s_inv.sats;
            time_t when = have_settled ? (time_t)settled_at : time(NULL);
            push_history(false, sats, s_inv.memo, when, true);
            s_balance += sats;
            if(s_balance < 0) s_balance = 0;
            s_balance_wanted = true;
            s_inv.state = WALLET_INVOICE_PAID;
            s_inv_rowed = true;
            state_save();
            NDBG("[wallet] nwc invoice paid (%lld sats)\n", (long long)sats);
        }
        else if(dead) {
            /* the row's when is expires_at (mock parity, section 6), and
               only an answer may write the row - never the clock */
            push_history(false, s_inv.sats, s_inv.memo, s_inv.expires_at, false);
            s_inv.state = WALLET_INVOICE_EXPIRED;
            s_inv_rowed = true;
            state_save();
            NDBG("[wallet] nwc invoice expired unpaid\n");
        }
        else {
            NDBG("[wallet] nwc invoice still %s\n", st[0] ? st : "pending");
        }
    }

    bool ok = s_op.ok;
    s_last_transport = s_op.transport;
    if(!ok) {
        fprintf(stderr, "[wallet] nwc refused: %s\n", s_op.err);
        if(s_op.transport) {
            wallet_demote(s_op.err);
            s_last_retry = time(NULL);   /* the first retry comes in ~30 s */
        }
    }
    else if(kind != NWC_OP_BALANCE) {
        s_balance_wanted = true;          /* refresh after every op (§6) */
    }

    if(err && err_n) snprintf(err, err_n, "%s", ok ? "" : s_op.err);

    end_session();
    memset(&s_op, 0, sizeof(s_op));
    s_op.kind = NWC_OP_NONE;
    return ok;
}

static bool run_op(nwc_op_kind_t kind, const char * method,
                   const char * params_json, int64_t sats, const char * memo,
                   char * err, size_t err_n)
{
    if(!start_op(kind, method, params_json, sats, memo, err, err_n))
        return false;
    run_op_blocking();
    return finish_op(err, err_n);
}

/* ---------- the entry points wallet.c calls ---------- */

bool wallet_nwc_open(char * err, size_t err_n)
{
    if(err && err_n) err[0] = 0;

    ensure_ctx();
    if(!load_conn(err, err_n)) return false;
    if(!adopt_conn(err, err_n)) return false;
    s_have_conn = true;

    if(!cipher_selftest(err, err_n)) return false;   /* before any network */

    state_load();
    time_t now = time(NULL);
    s_balance_done = now;
    s_last_lookup = now;
    s_last_retry = now;
    s_balance_wanted = false;

    /* READY must mean at least one fetched balance (section 6): the first
       fetch runs here, bounded - an unreachable relay comes back as an
       honest error carrying the retry phrase, never READY with no number */
    char why[192] = "";
    if(start_op(NWC_OP_BALANCE, "get_balance", "{}", 0, NULL, why, sizeof(why))) {
        run_op_blocking();
        if(finish_op(why, sizeof(why))) return true;
        if(!s_last_transport) {              /* the service itself refused */
            snprintf(err, err_n, "%s", why);
            return false;
        }
    }
    char host[NWC_URL_MAX];
    relay_host(host, sizeof(host));
    snprintf(err, err_n, "could not reach %s - the wallet will retry", host);
    return false;
}

int64_t wallet_nwc_balance(void)
{
    s_balance_wanted = true;         /* refresh-on-read; the tick rate-limits */
    return s_balance;
}

bool wallet_nwc_make_invoice(int64_t sats, const char * memo,
                             wallet_invoice_t * out, char * err, size_t err_n)
{
    if(!out) return false;
    memset(out, 0, sizeof(*out));
    if(sats < 1) sats = 1;
    if(sats > 100000000LL) sats = 100000000LL;      /* parity with the mock */

    char params[256];
    char esc[160];
    json_escape(memo ? memo : "", esc, sizeof(esc));
    if(esc[0])
        snprintf(params, sizeof(params),
                 "{\"amount\":%lld,\"description\":\"%s\"}",
                 (long long)(sats * 1000), esc);
    else
        snprintf(params, sizeof(params), "{\"amount\":%lld}",
                 (long long)(sats * 1000));

    if(!run_op(NWC_OP_MAKE, "make_invoice", params, sats, memo, err, err_n))
        return false;
    *out = s_inv;
    return true;
}

bool wallet_nwc_invoice(wallet_invoice_t * out)
{
    if(!out) return false;
    if(!s_inv.valid) return false;
    *out = s_inv;
    /* the clock contract: past expires_at the screen says EXPIRED locally,
       no network needed - the history row still waits for a real answer */
    if(out->state == WALLET_INVOICE_WAITING && time(NULL) >= out->expires_at)
        out->state = WALLET_INVOICE_EXPIRED;
    return true;
}

bool wallet_nwc_check(const char * bolt11, int64_t * sats_out,
                      char * err, size_t err_n)
{
    int64_t sats = 0;
    if(!bolt11_parse(bolt11, &sats, err, err_n)) return false;

    /* the cached-balance gate, enforced: the mock's exact sentence with
       Y = the amount only (the fee is unknown pre-pay, section 6) */
    if(sats > s_balance) {
        if(err && err_n)
            snprintf(err, err_n, "the balance is %lld sats and this needs %lld",
                     (long long)s_balance, (long long)sats);
        return false;
    }
    if(sats_out) *sats_out = sats;
    return true;
}

int64_t wallet_nwc_fee(int64_t sats)
{
    if(s_last_paid >= 0 && sats == s_last_paid) return s_last_fee;
    return -1;                        /* unknown: before paying, or no fee in
                                         the pay response (section 6) */
}

bool wallet_nwc_pay(const char * bolt11, const char * memo,
                    int64_t * sats_out, char * err, size_t err_n)
{
    int64_t sats = 0;
    if(!bolt11_parse(bolt11, &sats, err, err_n)) return false;

    char params[768];
    char esc[704];
    json_escape(bolt11 ? bolt11 : "", esc, sizeof(esc));
    snprintf(params, sizeof(params), "{\"invoice\":\"%s\"}", esc);

    if(!run_op(NWC_OP_PAY, "pay_invoice", params, sats, memo, err, err_n))
        return false;
    if(sats_out) *sats_out = sats;
    return true;
}

int wallet_nwc_history(wallet_tx_t * out, int max)
{
    if(!out || max <= 0) return 0;
    int n = (s_hist_n < max) ? s_hist_n : max;
    for(int i = 0; i < n; i++) out[i] = s_hist[i];
    return n;
}

/* The tick (section 5.7, section 6): one non-blocking pass over an op in
   flight, then the cadences - an op is only ever started when nothing else
   is running, so the relay sees one conversation at a time. Nothing is in
   flight and nothing is due means nothing is sent (no idle traffic). */
void wallet_nwc_poll(void)
{
    if(!s_have_conn) return;
    time_t now = time(NULL);

    if(s_op.kind != NWC_OP_NONE) {
        op_step(0);                       /* ~0 ms: never stalls a tick */
        if(s_op.done) finish_op(NULL, 0);
        return;
    }

    wallet_state_t st = wallet_state();

    if(st == WALLET_ERROR) {
        /* only the ~30 s retry runs while the transport is presumed dead;
           the reason line keeps the words it earned (decided at build) */
        if(now - s_last_retry >= NWC_RETRY_SECS) {
            s_last_retry = now;
            char why[192];
            if(!start_op(NWC_OP_BALANCE, "get_balance", "{}", 0, NULL,
                         why, sizeof(why)))
                NDBG("[wallet] nwc retry not started: %s\n", why);
        }
        return;
    }

    if(st != WALLET_READY) return;        /* UNSET: nothing to say */

    bool bal_due = s_balance_wanted &&
                   now - s_balance_done >= NWC_REFRESH_SECS;
    bool look_due = s_inv.valid && !s_inv_rowed &&
                    now - s_last_lookup >= NWC_LOOKUP_SECS;

    if(bal_due) {
        char why[192];
        if(!start_op(NWC_OP_BALANCE, "get_balance", "{}", 0, NULL,
                     why, sizeof(why)))
            NDBG("[wallet] nwc refresh not started: %s\n", why);
        return;
    }

    if(look_due) {
        s_last_lookup = now;             /* the cadence spans the flight */
        char ph[65];
        if(!bolt11_payment_hash(s_inv.bolt11, ph)) {
            /* no key to ask by, and none may be invented: the row waits
               (a real bolt11 always carries its p tag) */
            NDBG("[wallet] nwc invoice has no payment hash - not looked up\n");
            return;
        }
        char params[160];
        snprintf(params, sizeof(params), "{\"payment_hash\":\"%s\"}", ph);
        char why[192];
        if(!start_op(NWC_OP_LOOKUP, "lookup_invoice", params, s_inv.sats,
                     s_inv.memo, why, sizeof(why)))
            NDBG("[wallet] nwc lookup not started: %s\n", why);
    }
}
