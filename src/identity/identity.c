#include "identity/identity.h"
#include "identity/bech32.h"
#include "identity/sha256.h"
#include "identity/state_file.h"
#include "ui/prefs.h"
#include "chat/chat.h"

#include <secp256k1.h>
#include <secp256k1_schnorrsig.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>

/* The device key: identity.nsec next to the prefs file, generated on the
   first run, never in the repo, never in a log.

   Every sign/hash call goes through one internal signer table (the seam the
   remote signer of build-order.md #4 will plug into). Callers only ever see
   identity.h - chat.c and the User page never know which backend answered.
   This slice wires the local libsecp256k1 backend and nothing else: no
   bunker strings, no relay path, no fake "remote" UI. */

typedef struct {
    bool (*event_id)(const char * serialization, char * hex65);
    bool (*sign_id)(const char * id_hex, char * sig_hex);
    bool (*verify_id)(const char * pub_hex, const char * id_hex, const char * sig_hex);
} id_signer_t;

static id_signer_t s_signer;              /* this file is the only one that knows it */

static secp256k1_context * s_ctx = NULL;
static secp256k1_keypair    s_kp;
static bool s_ok = false;
static bool s_inited = false;
static char s_pub_hex[65] = "";
static char s_npub[64] = "";
static char s_nsec[64] = "";

/* ---------- small helpers ---------- */

static void bytes_to_hex(const uint8_t * in, size_t n, char * out)
{
    static const char * h = "0123456789abcdef";
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

static const char * trim_ws(const char * s, size_t * out_len)
{
    while(*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    size_t n = strlen(s);
    while(n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r' || s[n - 1] == '\n'))
        n--;
    if(out_len) *out_len = n;
    return s;
}

static bool os_random(uint8_t * out, size_t n)
{
#ifdef __linux__
    size_t got = 0;
    while(got < n) {
        ssize_t r = getrandom(out + got, n - got, 0);
        if(r < 0) return false;
        got += (size_t)r;
    }
    return true;
#else
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
#endif
}

static void key_path(char * out, size_t outn)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", prefs_path());
    char * slash = strrchr(dir, '/');
    if(slash) *slash = 0;
    snprintf(out, outn, "%s%sidentity.nsec", dir[0] ? dir : "", dir[0] ? "/" : "");
}

static void log_pub(void)
{
    if(!getenv("PDA_DEBUG")) return;
    fprintf(stderr, "[id] pub=%s npub=%s\n", s_pub_hex, s_npub);
}

/* The sha256 vectors, once, from identity_init(): hashing needs no key, so
   even a corrupt identity.nsec run prints it. The driver suites assert the
   published NIST constants - how a no-C-harness project tests the hash. */
static void log_selftest(void)
{
    if(!getenv("PDA_DEBUG")) return;
    static const char * abc =
        "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    uint8_t d[32];
    char a[65], b[65], c[65];
    sha256("", 0, d);          bytes_to_hex(d, 32, a);
    sha256("abc", 3, d);       bytes_to_hex(d, 32, b);
    sha256(abc, strlen(abc), d); bytes_to_hex(d, 32, c);
    fprintf(stderr, "[id] selftest %s %s %s\n", a, b, c);
}

/* ---------- adopt a secret key (validate, then everything derives from it) */

static void forget(void);

static bool adopt(const uint8_t sec[32])
{
    secp256k1_xonly_pubkey pk;
    uint8_t pub[32];
    if(!secp256k1_keypair_create(s_ctx, &s_kp, sec)) { forget(); return false; }
    if(!secp256k1_keypair_xonly_pub(s_ctx, &pk, NULL, &s_kp)) { forget(); return false; }
    if(!secp256k1_xonly_pubkey_serialize(s_ctx, pub, &pk)) { forget(); return false; }

    bytes_to_hex(pub, 32, s_pub_hex);
    if(!bech32_encode("npub", pub, 32, s_npub, sizeof(s_npub))) { forget(); return false; }
    if(!bech32_encode("nsec", sec, 32, s_nsec, sizeof(s_nsec))) { forget(); return false; }
    s_ok = true;
    return true;
}

static void forget(void)
{
    s_ok = false;
    s_pub_hex[0] = 0;
    s_npub[0] = 0;
    s_nsec[0] = 0;
    memset(&s_kp, 0, sizeof(s_kp));
}

/* write the key file; only the caller swaps memory, and only if this worked */
static bool save_sec(const uint8_t sec[32])
{
    char nsec[64];
    if(!bech32_encode("nsec", sec, 32, nsec, sizeof(nsec))) return false;
    char body[80];
    snprintf(body, sizeof(body), "%s\n", nsec);

    char path[600];
    key_path(path, sizeof(path));
    if(!state_write(path, body, strlen(body))) return false;
    return true;
}

/* ---------- the local signer backend (the only one this slice wires) ---------- */

static bool local_event_id(const char * serialization, char * hex65)
{
    if(!serialization || !hex65) return false;
    uint8_t d[32];
    sha256(serialization, strlen(serialization), d);
    bytes_to_hex(d, 32, hex65);
    return true;
}

static bool local_sign_id(const char * id_hex, char * sig_hex)
{
    if(!s_ok || !id_hex || !sig_hex) return false;
    uint8_t msg[32];
    if(strlen(id_hex) != 64) return false;
    if(!hex_to_bytes(id_hex, 64, msg, sizeof(msg))) return false;

    uint8_t sig[64];
    /* deterministic: same id -> same sig (RFC 6979-style nonce, aux = NULL),
       so rebuilding the same event anywhere yields byte-identical output */
    if(!secp256k1_schnorrsig_sign32(s_ctx, sig, msg, &s_kp, NULL)) return false;
    bytes_to_hex(sig, 64, sig_hex);
    return true;
}

static bool local_verify_id(const char * pub_hex, const char * id_hex, const char * sig_hex)
{
    if(!pub_hex || !id_hex || !sig_hex) return false;
    if(strlen(pub_hex) != 64 || strlen(id_hex) != 64 || strlen(sig_hex) != 128)
        return false;

    uint8_t pub[32], msg[32], sig[64];
    if(!hex_to_bytes(pub_hex, 64, pub, sizeof(pub))) return false;
    if(!hex_to_bytes(id_hex, 64, msg, sizeof(msg))) return false;
    if(!hex_to_bytes(sig_hex, 128, sig, sizeof(sig))) return false;

    secp256k1_xonly_pubkey pk;
    if(!secp256k1_xonly_pubkey_parse(s_ctx, &pk, pub)) return false;
    return secp256k1_schnorrsig_verify(s_ctx, sig, msg, 32, &pk) == 1;
}

/* ---------- the door ---------- */

static void fail(const char * why)
{
    forget();
    if(why && *why) chat_system(why);
}

bool identity_init(void)
{
    /* the table is wired even when the key is unusable: hashing and verify
       do not need our key, and the remote backend lands on the same seam */
    s_signer.event_id = local_event_id;
    s_signer.sign_id = local_sign_id;
    s_signer.verify_id = local_verify_id;

    if(!s_ctx) {
        s_ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);
        uint8_t seed[32];
        /* randomizing is side-channel blinding and strictly best effort:
           the key and its signatures work the same without it */
        if(os_random(seed, sizeof(seed))) {
            int blinded = secp256k1_context_randomize(s_ctx, seed);
            (void)blinded;
        }
    }

    log_selftest();
    if(s_inited) return s_ok;
    s_inited = true;

    char path[600];
    key_path(path, sizeof(path));

    struct stat st;
    if(stat(path, &st) != 0) {
        /* first run: a keypair, saved before anything claims it exists */
        uint8_t sec[32];
        if(!os_random(sec, sizeof(sec)) ) {
            fail("no identity key: no usable randomness, messages go out unsigned");
            return false;
        }
        if(!secp256k1_ec_seckey_verify(s_ctx, sec)) {
            fail("no identity key: randomness was not a valid key, messages go out unsigned");
            return false;
        }
        if(!save_sec(sec)) {
            fail("no identity key: identity.nsec could not be written, messages go out unsigned");
            return false;
        }
        if(!adopt(sec)) {
            fail("no identity key: the generated key could not be used, messages go out unsigned");
            return false;
        }
        log_pub();
        return true;
    }

    /* the file exists: read it, accept nsec or 64-hex, and never ever
       regenerate a key we could not understand (quiet destroy) */
    FILE * f = fopen(path, "rb");
    if(!f) {
        fail("no identity key: identity.nsec could not be read, messages go out unsigned");
        return false;
    }
    char buf[512];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = 0;

    size_t tn = 0;
    const char * text = trim_ws(buf, &tn);
    uint8_t sec[32];
    bool good = false;
    if(tn == 64 && hex_to_bytes(text, 64, sec, sizeof(sec))) {
        good = true;
    }
    else if(tn > 5) {
        char tmp[128];
        if(tn < sizeof(tmp)) {
            memcpy(tmp, text, tn);
            tmp[tn] = 0;
            size_t got = 0;
            good = bech32_decode(tmp, "nsec", sec, sizeof(sec), &got) && got == 32;
        }
    }
    if(good) good = secp256k1_ec_seckey_verify(s_ctx, sec) == 1;

    if(!good) {
        /* honest: the ways out are [NEW] (always works), [PASTE], or the
           user deleting the file himself - never a silent regeneration */
        fail("no identity key: identity.nsec is not a valid key (not regenerated), "
             "messages go out unsigned");
        return false;
    }
    if(!adopt(sec)) {
        fail("no identity key: identity.nsec could not be used, messages go out unsigned");
        return false;
    }
    log_pub();
    return true;
}

const char * identity_pub_hex(void)
{
    return s_pub_hex;
}

const char * identity_npub(void)
{
    return s_npub;
}

const char * identity_nsec(void)
{
    return s_nsec;
}

/* one parse function for the file and for the paste field: bech32 nsec… or
   64 hex, case-insensitive, no 0x, surrounding whitespace/CR trimmed */
static bool parse_secret(const char * text, uint8_t sec[32])
{
    if(!text) return false;
    size_t tn = 0;
    const char * t = trim_ws(text, &tn);
    if(!tn) return false;

    if(tn == 64 && hex_to_bytes(t, 64, sec, 32))
        return secp256k1_ec_seckey_verify(s_ctx, sec) == 1;

    if(tn < 8 || tn >= 128) return false;
    char tmp[128];
    memcpy(tmp, t, tn);
    tmp[tn] = 0;
    size_t got = 0;
    if(!bech32_decode(tmp, "nsec", sec, 32, &got)) return false;
    if(got != 32) return false;
    return secp256k1_ec_seckey_verify(s_ctx, sec) == 1;
}

bool identity_import_nsec(const char * nsec_or_hex)
{
    if(!s_ctx && !identity_init()) return false;

    uint8_t sec[32];
    if(!parse_secret(nsec_or_hex, sec)) {
        chat_system("key not imported: that is not an nsec or a 64-hex secret key");
        return false;
    }
    if(!save_sec(sec)) {
        chat_system("key not imported: identity.nsec could not be written");
        return false;
    }
    /* only now does memory change - disk and memory never diverge */
    if(!adopt(sec)) {
        chat_system("key not imported: the key could not be used");
        return false;
    }
    log_pub();
    return true;
}

bool identity_regenerate(void)
{
    if(!s_ctx && !identity_init()) return false;

    uint8_t sec[32];
    bool made = false;
    for(int i = 0; i < 8 && !made; i++) {
        if(!os_random(sec, sizeof(sec))) break;
        made = secp256k1_ec_seckey_verify(s_ctx, sec) == 1;
    }
    if(!made) {
        chat_system("new key not made: no usable randomness");
        return false;
    }
    if(!save_sec(sec)) {
        chat_system("new key not made: identity.nsec could not be written");
        return false;
    }
    if(!adopt(sec)) {
        chat_system("new key not made: the key could not be used");
        return false;
    }
    log_pub();
    return true;
}

bool identity_event_id(const char * serialization, char * hex65)
{
    bool (*fn)(const char *, char *) =
        s_signer.event_id ? s_signer.event_id : local_event_id;
    return fn(serialization, hex65);
}

bool identity_sign_id(const char * id_hex, char * sig_hex)
{
    bool (*fn)(const char *, char *) =
        s_signer.sign_id ? s_signer.sign_id : local_sign_id;
    return fn(id_hex, sig_hex);
}

bool identity_verify_id(const char * pub_hex, const char * id_hex, const char * sig_hex)
{
    bool (*fn)(const char *, const char *, const char *) =
        s_signer.verify_id ? s_signer.verify_id : local_verify_id;
    return fn(pub_hex, id_hex, sig_hex);
}

bool identity_npub_from_pub_hex(const char * pub_hex, char * out, size_t outn)
{
    if(!pub_hex || !out || outn == 0) return false;
    out[0] = 0;
    if(strlen(pub_hex) != 64) return false;
    uint8_t pub[32];
    if(!hex_to_bytes(pub_hex, 64, pub, sizeof(pub))) return false;
    return bech32_encode("npub", pub, 32, out, outn);
}
