#include "wallet/wallet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* The preview backend: a balance, invoices that nothing really pays, and a
 * history of the payments that happened here. It is not a wallet, it says
 * so everywhere it is shown, and no real money can touch it.
 *
 * The state file is one "key = value" line per setting, the same shape as
 * prefs.conf, so it can be edited by hand. The balance only moves when the
 * preview settles an invoice, and then the file is written so the number
 * survives a restart.
 *
 * Settling is a timer on the wall clock, not a thread: an invoice is made
 * with the time it was made, wallet_mock_poll() looks at the clock when the
 * UI asks, and MOCK_SETTLE_SECS later the payment "arrives". That is what
 * makes the PAID screen demoable while there is no backend behind it. */

#define MOCK_DEFAULT_SATS  1234567
#define MOCK_INVOICE_TTL   300    /* s: an invoice may be paid for this long */
#define MOCK_SETTLE_SECS   8      /* s: when the preview pretends it was paid */
#define MOCK_PAYLOAD_LEN   160    /* filler after the bolt11 tag */
#define MOCK_HIST_MAX      64

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

/* load the state file; anything unreadable or wrong keeps the default.
 * Returns the balance. */
int64_t wallet_mock_open(void)
{
    resolve_path();
    s_balance = MOCK_DEFAULT_SATS;

    FILE * f = fopen(s_path, "r");
    if(!f) return s_balance;

    char line[256];
    while(fgets(line, sizeof(line), f)) {
        if(strncmp(line, "balance", 7) != 0) continue;
        char * eq = strchr(line, '=');
        if(!eq) continue;
        char * val = eq + 1;
        while(*val == ' ' || *val == '\t') val++;
        char * end = NULL;
        long long v = strtoll(val, &end, 10);
        if(end == val || v < 0) continue;   /* not a number: keep the default */
        s_balance = (int64_t)v;
        break;
    }
    fclose(f);
    return s_balance;
}

int64_t wallet_mock_balance(void)
{
    return s_balance;
}

/* ---------- the balance on disk ---------- */

static bool is_balance_line(const char * line)
{
    if(strncmp(line, "balance", 7) != 0) return false;
    return line[7] == ' ' || line[7] == '\t' || line[7] == '=';
}

/* write the balance back. Every other line of the file is carried over, so
 * a setting somebody added by hand is not thrown away. */
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

    FILE * in = fopen(s_path, "r");
    if(in) {
        char line[256];
        while(fgets(line, sizeof(line), in)) {
            if(is_balance_line(line)) continue;
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

/* lnbc<amount><multiplier>1p<filler>mock
 *
 * The amount is a real BOLT11 amount (nano- and pico-BTC, the two units a
 * small payment fits in), and the filler ends in the word "mock" so the
 * string can never be mistaken for an invoice anybody could pay. */
static bool make_bolt11(int64_t sats, char * out, size_t out_n)
{
    static const char bech[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
    long long num;
    char mult;

    if(sats % 10 == 0) { num = sats / 10; mult = 'n'; }      /* 10 sats a nano */
    else               { num = sats * 10000; mult = 'p'; }   /* 1 sat = 10^4 pico */

    char body[512];
    int k = snprintf(body, sizeof(body), "lnbc%lld%c1p", num, mult);
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

static void push_history(bool outgoing, int64_t sats, const char * memo, time_t when)
{
    int n = (s_hist_n < MOCK_HIST_MAX) ? s_hist_n + 1 : MOCK_HIST_MAX;
    for(int i = n - 1; i > 0; i--) s_hist[i] = s_hist[i - 1];
    memset(&s_hist[0], 0, sizeof(s_hist[0]));
    s_hist[0].outgoing = outgoing;
    s_hist[0].sats = sats;
    snprintf(s_hist[0].memo, sizeof(s_hist[0].memo), "%s", memo ? memo : "");
    s_hist[0].when = when;
    s_hist[0].settled = true;
    s_hist_n = n;
}

/* the preview settles: the balance moves and the row lands in the history */
void wallet_mock_poll(void)
{
    if(!s_inv.valid || s_settled) return;
    if(s_inv.state != WALLET_INVOICE_WAITING) return;

    time_t now = time(NULL);
    if(now >= s_inv.expires_at) return;            /* expired, nothing to settle */
    if(now < s_inv.made_at + MOCK_SETTLE_SECS) return;

    s_settled = true;
    s_inv.state = WALLET_INVOICE_PAID;
    s_balance += s_inv.sats;
    push_history(false, s_inv.sats, s_inv.memo, now);
    wallet_mock_save();
    if(getenv("PDA_DEBUG"))
        fprintf(stderr, "[wallet] mock settled %lld sats, balance %lld\n",
                (long long)s_inv.sats, (long long)s_balance);
}

int wallet_mock_history(wallet_tx_t * out, int max)
{
    if(!out || max <= 0) return 0;
    int n = (s_hist_n < max) ? s_hist_n : max;
    for(int i = 0; i < n; i++) out[i] = s_hist[i];
    return n;
}
