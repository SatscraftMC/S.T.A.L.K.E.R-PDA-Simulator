#include "wallet/wallet.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* The preview backend: a balance, invoices that nothing really pays, and a
 * history of the payments that happened here. It is not a wallet, it says
 * so everywhere it is shown, and no real money can touch it.
 *
 * The state file is one "key = value" line per setting, the same shape as
 * prefs.conf, so it can be edited by hand. The balance only moves when the
 * preview settles an invoice or a payment goes out, and then the file is
 * written - so both the balance and the history survive a restart. The file
 * owns the "history =" lines: they are read at open and written back whole,
 * while every other line somebody added by hand is carried over untouched.
 *
 * Settling is a timer on the wall clock, not a thread: an invoice is made
 * with the time it was made, wallet_mock_poll() looks at the clock when the
 * UI asks, and MOCK_SETTLE_SECS later the payment "arrives". That is what
 * makes the PAID screen demoable while there is no backend behind it. An
 * invoice nobody paid in time expires the same way, and lands in the
 * history as unpaid rather than quietly disappearing. */

#define MOCK_DEFAULT_SATS  1234567
#define MOCK_INVOICE_TTL   300    /* s: an invoice may be paid for this long */
#define MOCK_SETTLE_SECS   8      /* s: when the preview pretends it was paid */
#define MOCK_PAYLOAD_LEN   160    /* filler after the bolt11 tag */
#define MOCK_HIST_MAX      64
#define MOCK_FEE_SATS      1      /* what sending costs, on top of the amount */
#define MOCK_MAX_SATS      2100000000000000LL   /* 21 000 000 BTC, all of it */

static int64_t s_balance = MOCK_DEFAULT_SATS;
static char s_path[512];

static wallet_invoice_t s_inv;
static bool s_settled = false;               /* the balance has moved already */
static wallet_tx_t s_hist[MOCK_HIST_MAX];    /* s_hist[0] is the newest */
static int s_hist_n = 0;

static void resolve_path(void)
{
    if(s_path[0]) return;
    const char * env = getenv("PDA_WALLET_MOCK");
    if(env && *env) {
        snprintf(s_path, sizeof(s_path), "%s", env);
        return;
    }
    const char * xdg = getenv("XDG_CONFIG_HOME");
    if(xdg && *xdg) {
        snprintf(s_path, sizeof(s_path), "%s/pda/wallet.mock", xdg);
        return;
    }
    const char * home = getenv("HOME");
    if(home && *home) {
        snprintf(s_path, sizeof(s_path), "%s/.config/pda/wallet.mock", home);
        return;
    }
    snprintf(s_path, sizeof(s_path), "pda-wallet.mock");
}

/* ---------- reading the state file ---------- */

/* the newline off the end of a line (and a stray \r before it) */
static void chomp(char * line)
{
    size_t n = strlen(line);
    while(n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
}

static bool is_key(const char * line, const char * key)
{
    size_t n = strlen(key);
    if(strncmp(line, key, n) != 0) return false;
    return line[n] == ' ' || line[n] == '\t' || line[n] == '=';
}

/* the part of a string between two pointers, into a fixed buffer */
static bool slice(const char * a, const char * b, char * out, size_t n)
{
    if(!a || !b || b < a) return false;
    size_t len = (size_t)(b - a);
    if(len + 1 > n) return false;
    memcpy(out, a, len);
    out[len] = 0;
    return true;
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

/* "history = <when>|<in|out>|<sats>|<paid|expired>|<memo>"
 *
 * The memo is last on purpose: a '|' that a hand-edited memo happens to
 * hold is then just part of the memo instead of a broken line. */
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

    /* the file holds them newest first, and so does memory */
    if(s_hist_n >= MOCK_HIST_MAX) return;
    int i = s_hist_n++;
    memset(&s_hist[i], 0, sizeof(s_hist[i]));
    s_hist[i].outgoing = outgoing;
    s_hist[i].sats = sats;
    s_hist[i].when = (time_t)when;
    s_hist[i].settled = settled;
    snprintf(s_hist[i].memo, sizeof(s_hist[i].memo), "%s", p4 + 1);
}

/* load the state file; anything unreadable or wrong keeps the default.
 * Returns the balance. */
int64_t wallet_mock_open(void)
{
    resolve_path();
    s_balance = MOCK_DEFAULT_SATS;
    s_hist_n = 0;

    FILE * f = fopen(s_path, "r");
    if(!f) return s_balance;

    char line[512];
    bool got_balance = false;
    while(fgets(line, sizeof(line), f)) {
        chomp(line);
        if(is_key(line, "history")) {
            char * eq = strchr(line, '=');
            if(!eq) continue;
            char * v = eq + 1;
            while(*v == ' ' || *v == '\t') v++;
            load_hist_line(v);
            continue;
        }
        if(got_balance || !is_key(line, "balance")) continue;
        char * eq = strchr(line, '=');
        if(!eq) continue;
        char * val = eq + 1;
        while(*val == ' ' || *val == '\t') val++;
        char * end = NULL;
        long long v = strtoll(val, &end, 10);
        if(end == val || v < 0) continue;   /* not a number: keep the default */
        s_balance = (int64_t)v;
        got_balance = true;
    }
    fclose(f);
    return s_balance;
}

int64_t wallet_mock_balance(void)
{
    return s_balance;
}

/* ---------- the state on disk ---------- */

/* one history row: the four fixed fields first, the memo last so it can
 * hold a '|' without breaking the line */
static void write_hist_line(FILE * out, const wallet_tx_t * tx)
{
    char memo[sizeof(tx->memo)];
    snprintf(memo, sizeof(memo), "%s", tx->memo);
    for(char * c = memo; *c; c++) if(*c == '\n' || *c == '\r' || *c == '\t') *c = ' ';

    fprintf(out, "history = %lld|%s|%lld|%s|%s\n",
            (long long)tx->when,
            tx->outgoing ? "out" : "in",
            (long long)tx->sats,
            tx->settled ? "paid" : "expired",
            memo);
}

/* write the balance and the history back. Every other line of the file is
 * carried over, so a setting somebody added by hand is not thrown away -
 * but the balance and the history are ours now, and the ones on disk are
 * replaced by the ones in memory rather than added to them. */
static void wallet_mock_save(void)
{
    resolve_path();

    char dir[544];
    snprintf(dir, sizeof(dir), "%s", s_path);
    char * slash = strrchr(dir, '/');
    if(slash) {
        *slash = 0;
        if(dir[0]) mkdir(dir, 0755);
    }

    char tmp[544];
    snprintf(tmp, sizeof(tmp), "%s.tmp", s_path);
    FILE * out = fopen(tmp, "w");
    if(!out) return;
    fprintf(out, "balance = %lld\n", (long long)s_balance);
    for(int i = 0; i < s_hist_n; i++) write_hist_line(out, &s_hist[i]);

    FILE * in = fopen(s_path, "r");
    if(in) {
        char line[512];
        while(fgets(line, sizeof(line), in)) {
            if(is_key(line, "balance") || is_key(line, "history")) continue;
            fputs(line, out);
        }
        fclose(in);
    }
    fclose(out);
    rename(tmp, s_path);
}

/* ---------- the fake bolt11 ---------- */

/* a small xorshift of our own: the invoice must be different every time
 * without touching whatever random state the rest of the program uses */
static uint32_t s_rng = 0;

static uint32_t mock_rand(void)
{
    if(!s_rng) {
        s_rng = (uint32_t)time(NULL) ^ ((uint32_t)getpid() * 2654435761u);
        if(!s_rng) s_rng = 0x9E3779B9u;
    }
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

/* lnbc<amount>n1p<filler>mock
 *
 * One sat is exactly ten nano-BTC, so every whole-sat amount encodes as
 * lnbc<10*sats>n with nothing rounded: a 500 sat request comes back out of
 * the invoice as 500 sats, which is what the confirm screen has to show.
 * The filler ends in the word "mock" so the string can never be mistaken
 * for an invoice anybody could pay. */
static bool make_bolt11(int64_t sats, char * out, size_t out_n)
{
    static const char bech[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    long long num = (long long)sats * 10;

    char body[512];
    int k = snprintf(body, sizeof(body), "lnbc%lldn1p", num);
    if(k < 0 || (size_t)k + MOCK_PAYLOAD_LEN + 5 >= sizeof(body)) return false;
    if((size_t)k + MOCK_PAYLOAD_LEN + 5 >= out_n) return false;

    for(int i = 0; i < MOCK_PAYLOAD_LEN; i++)
        body[k++] = bech[mock_rand() % 32];
    memcpy(body + k, "mock", 5);
    k += 5;

    memcpy(out, body, (size_t)k + 1);
    return true;
}

/* ---------- the invoices ---------- */

bool wallet_mock_make_invoice(int64_t sats, const char * memo,
                              wallet_invoice_t * out, char * err, size_t err_n)
{
    if(!out) return false;
    memset(out, 0, sizeof(*out));
    if(sats < 1) sats = 1;
    if(sats > 100000000LL) sats = 100000000LL;      /* one bitcoin */

    if(!make_bolt11(sats, out->bolt11, sizeof(out->bolt11))) {
        if(err && err_n) snprintf(err, err_n, "%s", "the amount does not fit in an invoice");
        return false;
    }

    out->valid = true;
    out->sats = sats;
    snprintf(out->memo, sizeof(out->memo), "%s", memo ? memo : "");
    out->made_at = time(NULL);
    out->expires_at = out->made_at + MOCK_INVOICE_TTL;
    out->state = WALLET_INVOICE_WAITING;

    s_inv = *out;
    s_settled = false;

    /* the UI test reads this back out of the log and checks it against the
       QR code the screen drew */
    fprintf(stderr, "[wallet] invoice: %s\n", out->bolt11);
    return true;
}

/* a plain read: expiry comes off the clock, settling does not happen here */
bool wallet_mock_invoice(wallet_invoice_t * out)
{
    if(!out) return false;
    if(!s_inv.valid) return false;
    *out = s_inv;
    if(out->state == WALLET_INVOICE_WAITING && time(NULL) >= out->expires_at)
        out->state = WALLET_INVOICE_EXPIRED;
    return true;
}

static void push_history(bool outgoing, int64_t sats, const char * memo,
                         time_t when, bool settled)
{
    int n = (s_hist_n < MOCK_HIST_MAX) ? s_hist_n + 1 : MOCK_HIST_MAX;
    for(int i = n - 1; i > 0; i--) s_hist[i] = s_hist[i - 1];
    memset(&s_hist[0], 0, sizeof(s_hist[0]));
    s_hist[0].outgoing = outgoing;
    s_hist[0].sats = sats;
    snprintf(s_hist[0].memo, sizeof(s_hist[0].memo), "%s", memo ? memo : "");
    s_hist[0].when = when;
    s_hist[0].settled = settled;
    s_hist_n = n;
}

/* the preview settles: the balance moves and the row lands in the history.
 * An invoice nobody paid in time expires instead - same row, marked
 * unpaid, and the balance is left alone. */
void wallet_mock_poll(void)
{
    if(!s_inv.valid || s_settled) return;
    if(s_inv.state != WALLET_INVOICE_WAITING) return;

    time_t now = time(NULL);
    if(now >= s_inv.expires_at) {
        s_inv.state = WALLET_INVOICE_EXPIRED;
        push_history(false, s_inv.sats, s_inv.memo, s_inv.expires_at, false);
        wallet_mock_save();
        if(getenv("PDA_DEBUG"))
            fprintf(stderr, "[wallet] mock invoice expired unpaid, balance %lld\n",
                    (long long)s_balance);
        return;
    }
    if(now < s_inv.made_at + MOCK_SETTLE_SECS) return;

    s_settled = true;
    s_inv.state = WALLET_INVOICE_PAID;
    s_balance += s_inv.sats;
    push_history(false, s_inv.sats, s_inv.memo, now, true);
    wallet_mock_save();
    if(getenv("PDA_DEBUG"))
        fprintf(stderr, "[wallet] mock settled %lld sats, balance %lld\n",
                (long long)s_inv.sats, (long long)s_balance);
}

/* ---------- paying ---------- */

/* mant * 10^exp10 -> whole sats. exp10 runs from -16 to 8 for anything a
 * bolt11 amount can say, so the multiply side is the one that can run away
 * and it is capped before it gets near unsigned long long. */
static bool mant_to_sats(unsigned long long mant, int exp10, int64_t * out)
{
    if(exp10 >= 0) {
        for(int i = 0; i < exp10; i++) {
            if(mant > (unsigned long long)(MOCK_MAX_SATS / 10)) return false;
            mant *= 10;
        }
        *out = (int64_t)mant;
        return true;
    }

    unsigned long long div = 1;
    for(int i = 0; i < -exp10; i++) div *= 10;   /* 10^16 at most: it fits */
    unsigned long long q = mant / div;
    if((mant % div) * 2 >= div) q++;             /* round half up */
    if(q > (unsigned long long)MOCK_MAX_SATS) return false;
    *out = (int64_t)q;
    return true;
}

/* what an invoice is asking for.
 *
 * A bolt11 is "<hrp>1<data>": a human readable part, the *last* '1' in the
 * string, then bech32 data - which never holds a '1' of its own, so the
 * separator cannot be confused with one. The hrp is the currency
 * (lnbc/lntb/lnbcrt) followed by the amount: digits, an optional fraction,
 * and the unit - nothing for BTC, m, u, n (nano) or p (pico).
 *
 * Blanks and line breaks are dropped first: what comes out of a paste is
 * not always one clean line. */
bool wallet_mock_check(const char * bolt11, int64_t * sats_out,
                       char * err, size_t err_n)
{
    if(sats_out) *sats_out = 0;

    char clean[640];
    size_t n = 0;
    for(const char * c = bolt11 ? bolt11 : ""; *c && n + 1 < sizeof(clean); c++) {
        if(isspace((unsigned char)*c)) continue;
        clean[n++] = *c;
    }
    clean[n] = 0;

    if(!n) {
        if(err && err_n) snprintf(err, err_n, "%s", "there is no invoice in the field");
        return false;
    }

    const char * sep = NULL;
    for(const char * c = clean; *c; c++) if(*c == '1') sep = c;
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

int64_t wallet_mock_fee(int64_t sats)
{
    return sats >= 1 ? MOCK_FEE_SATS : 0;
}

bool wallet_mock_pay(const char * bolt11, const char * memo,
                     int64_t * sats_out, char * err, size_t err_n)
{
    if(sats_out) *sats_out = 0;

    int64_t sats = 0;
    if(!wallet_mock_check(bolt11, &sats, err, err_n)) return false;

    int64_t fee = wallet_mock_fee(sats);
    if(sats + fee > s_balance) {
        if(err && err_n) snprintf(err, err_n,
                                  "the balance is %lld sats and this needs %lld",
                                  (long long)s_balance, (long long)(sats + fee));
        return false;
    }

    s_balance -= sats + fee;
    push_history(true, sats, memo, time(NULL), true);
    wallet_mock_save();

    if(sats_out) *sats_out = sats;
    /* read back by tools/ui_test.py - and never with "invoice:" in it, or
       it would be mistaken for the bolt11 the screen was handed */
    fprintf(stderr, "[wallet] pay: %lld sats + %lld fee, balance %lld\n",
            (long long)sats, (long long)fee, (long long)s_balance);
    return true;
}

int wallet_mock_history(wallet_tx_t * out, int max)
{
    if(!out || max <= 0) return 0;
    int n = (s_hist_n < max) ? s_hist_n : max;
    for(int i = 0; i < n; i++) out[i] = s_hist[i];
    return n;
}
