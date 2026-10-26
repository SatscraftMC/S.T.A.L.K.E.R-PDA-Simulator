#include "wallet/wallet.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* wallet_mock.c - the preview backend (kept out of the public header) */
int64_t wallet_mock_open(void);        /* load the state file, return the balance */
int64_t wallet_mock_balance(void);
bool wallet_mock_make_invoice(int64_t sats, const char * memo,
                              wallet_invoice_t * out, char * err, size_t err_n);
bool wallet_mock_invoice(wallet_invoice_t * out);
void wallet_mock_poll(void);
int wallet_mock_history(wallet_tx_t * out, int max);
bool wallet_mock_check(const char * bolt11, int64_t * sats_out,
                       char * err, size_t err_n);
int64_t wallet_mock_fee(int64_t sats);
bool wallet_mock_pay(const char * bolt11, const char * memo,
                     int64_t * sats_out, char * err, size_t err_n);

/* wallet_nwc.c - the NIP-47 backend, kept out of the public header the same
   way the mock is: only this file names the two backends. */
bool wallet_nwc_conn_present(void);
bool wallet_nwc_open(char * err, size_t err_n);
int64_t wallet_nwc_balance(void);
bool wallet_nwc_make_invoice(int64_t sats, const char * memo,
                             wallet_invoice_t * out, char * err, size_t err_n);
bool wallet_nwc_invoice(wallet_invoice_t * out);
void wallet_nwc_poll(void);
bool wallet_nwc_check(const char * bolt11, int64_t * sats_out,
                      char * err, size_t err_n);
int64_t wallet_nwc_fee(int64_t sats);
bool wallet_nwc_pay(const char * bolt11, const char * memo,
                    int64_t * sats_out, char * err, size_t err_n);
int wallet_nwc_history(wallet_tx_t * out, int max);
bool wallet_nwc_connect_uri(const char * uri, char * err, size_t err_n);

static bool send_gate(char * err, size_t err_n);

static bool s_done = false;
static wallet_state_t s_state = WALLET_UNSET;
static wallet_backend_t s_backend = WALLET_BACKEND_NONE;
static char s_name[48] = "none";
static char s_err[192];

/* every reason this file settles on, once, where the suites can read it back:
   the not-set-up reason and the unreachable-relay reason are both backend
   English (section 8), never the connection URI or its secret */
static void log_error(void)
{
    if(getenv("PDA_DEBUG") && s_err[0])
        fprintf(stderr, "[wallet] error: %s\n", s_err);
}

void wallet_init(void)
{
    if(s_done) return;
    s_done = true;
    s_err[0] = 0;

    const char * b = getenv("PDA_WALLET_BACKEND");
    if(!b || !*b) {
        snprintf(s_name, sizeof(s_name), "%s", "none");
        s_backend = WALLET_BACKEND_NONE;
        s_state = WALLET_UNSET;
        return;
    }

    snprintf(s_name, sizeof(s_name), "%s", b);
    if(!strcmp(b, "mock")) {
        s_backend = WALLET_BACKEND_MOCK;
        s_state = WALLET_READY;
        wallet_mock_open();      /* loads the balance and the history */
        return;
    }

    if(!strcmp(b, "nwc")) {
        s_backend = WALLET_BACKEND_NWC;
        if(!wallet_nwc_conn_present()) {
            /* the not-set-up screen tells the user what is missing - the
               state stays UNSET, the reason rides on wallet_error() (§1) */
            s_state = WALLET_UNSET;
            snprintf(s_err, sizeof(s_err), "%s",
                     "no wallet connection saved - paste one from your wallet app");
            log_error();
            return;
        }
        if(wallet_nwc_open(s_err, sizeof(s_err))) {
            s_state = WALLET_READY;
            s_err[0] = 0;
        }
        else {
            /* a saved connection the relay did not answer: never READY with
               no fetch behind it (§1) - the tick retries every ~30 s */
            s_state = WALLET_ERROR;
            log_error();
        }
        return;
    }

    s_backend = WALLET_BACKEND_UNKNOWN;
    s_state = WALLET_ERROR;
    snprintf(s_err, sizeof(s_err),
             "unknown wallet backend \"%s\" - this build has: mock, nwc", b);
    log_error();
}

/* The two state moves wallet_nwc.c needs and wallet.h must not grow for:
   a transport failure/timeout demotes the wallet to its honest error (§1),
   and the tick's retry promotes it back the moment a fetch lands. wallet.c
   owns the state; the backend only ever says what happened. */
void wallet_demote(const char * reason)
{
    s_state = WALLET_ERROR;
    snprintf(s_err, sizeof(s_err), "%s", (reason && *reason) ? reason : "the wallet did not answer");
    log_error();
}

void wallet_promote(void)
{
    if(s_backend != WALLET_BACKEND_NWC) return;
    s_state = WALLET_READY;
    s_err[0] = 0;
}

wallet_state_t wallet_state(void)
{
    return s_state;
}

const char * wallet_backend_name(void)
{
    return s_name;
}

const char * wallet_error(void)
{
    return s_err;
}

bool wallet_is_mock(void)
{
    return s_backend == WALLET_BACKEND_MOCK;
}

/* The balance is the backend's to keep: a payment or a settle moves it
 * between two reads of this, and a copy taken at startup would put a stale
 * number on the balance screen, the PAID panel and the result of a payment
 * that has already happened. nwc reads its cache and flags a refresh (§6):
 * the tick services the flag on a rate limit, so a screen build can never
 * spin the relay. */
int64_t wallet_balance_sats(void)
{
    if(s_state != WALLET_READY) return 0;
    if(s_backend == WALLET_BACKEND_MOCK) return wallet_mock_balance();
    if(s_backend == WALLET_BACKEND_NWC) return wallet_nwc_balance();
    return 0;
}

/* Make an invoice. Both real answers exist; anything that cannot answer
 * fails with a reason instead of pretending. */
bool wallet_make_invoice(int64_t sats, const char * memo,
                         char * out, size_t out_n, char * err, size_t err_n)
{
    if(out && out_n) out[0] = 0;
    if(!send_gate(err, err_n)) return false;

    wallet_invoice_t inv;
    bool ok;
    if(s_backend == WALLET_BACKEND_MOCK)
        ok = wallet_mock_make_invoice(sats, memo, &inv, err, err_n);
    else if(s_backend == WALLET_BACKEND_NWC)
        ok = wallet_nwc_make_invoice(sats, memo, &inv, err, err_n);
    else {
        if(err && err_n) snprintf(err, err_n,
                                  "%s", "this backend cannot make an invoice yet");
        return false;
    }
    if(!ok) return false;
    if(out && out_n) snprintf(out, out_n, "%s", inv.bolt11);
    return true;
}

bool wallet_invoice(wallet_invoice_t * out)
{
    if(!out) return false;
    memset(out, 0, sizeof(*out));
    if(s_backend == WALLET_BACKEND_MOCK) return wallet_mock_invoice(out);
    if(s_backend == WALLET_BACKEND_NWC) return wallet_nwc_invoice(out);
    return false;
}

void wallet_poll(void)
{
    if(s_backend == WALLET_BACKEND_MOCK) wallet_mock_poll();
    else if(s_backend == WALLET_BACKEND_NWC) wallet_nwc_poll();
}

/* the reason a call cannot be made, or "" when it can be: the wallet is
   not set up, a demoted nwc wallet (its own reason - §5.7's fast refusal,
   no pump until the retry succeeds), or a backend this build lacks */
static bool send_gate(char * err, size_t err_n)
{
    if(s_state == WALLET_ERROR && s_err[0]) {
        if(err && err_n) snprintf(err, err_n, "%s", s_err);
        return false;
    }
    if(s_state != WALLET_READY) {
        if(err && err_n) snprintf(err, err_n, "%s", "the wallet is not set up");
        return false;
    }
    if(s_backend != WALLET_BACKEND_MOCK && s_backend != WALLET_BACKEND_NWC) {
        if(err && err_n) snprintf(err, err_n, "%s",
                                  "this backend cannot move money yet");
        return false;
    }
    return true;
}

bool wallet_check_invoice(const char * bolt11, int64_t * sats_out,
                          char * err, size_t err_n)
{
    if(sats_out) *sats_out = 0;
    if(!send_gate(err, err_n)) return false;

    bool ok;
    if(s_backend == WALLET_BACKEND_NWC)
        ok = wallet_nwc_check(bolt11, sats_out, err, err_n);
    else
        ok = wallet_mock_check(bolt11, sats_out, err, err_n);

    /* read back by tools/ui_test.py - and never with "invoice:" in it, or
       it would be mistaken for the bolt11 the screen was handed */
    if(ok)
        fprintf(stderr, "[wallet] check: %lld sats\n", (long long)*sats_out);
    else
        fprintf(stderr, "[wallet] check refused: %s\n",
                (err && *err) ? err : "no reason given");
    return ok;
}

int64_t wallet_fee_sats(int64_t sats)
{
    if(s_backend == WALLET_BACKEND_MOCK && s_state == WALLET_READY)
        return wallet_mock_fee(sats);
    if(s_backend == WALLET_BACKEND_NWC)
        return wallet_nwc_fee(sats);
    return -1;                       /* unknown, never a fabricated zero */
}

bool wallet_pay_invoice(const char * bolt11, const char * memo,
                        int64_t * sats_out, char * err, size_t err_n)
{
    if(sats_out) *sats_out = 0;
    if(!send_gate(err, err_n)) return false;

    bool ok;
    if(s_backend == WALLET_BACKEND_MOCK)
        ok = wallet_mock_pay(bolt11, memo, sats_out, err, err_n);
    else
        ok = wallet_nwc_pay(bolt11, memo, sats_out, err, err_n);

    if(!ok) {
        fprintf(stderr, "[wallet] pay refused: %s\n",
                (err && *err) ? err : "no reason given");
        return false;
    }
    return true;
}

int wallet_history(wallet_tx_t * out, int max)
{
    if(s_backend == WALLET_BACKEND_MOCK) return wallet_mock_history(out, max);
    if(s_backend == WALLET_BACKEND_NWC) return wallet_nwc_history(out, max);
    return 0;
}

/* seam grow 1 of 2 (see wallet.h): the URI door. Only the nwc backend has
   a connection file; every other backend answers honestly and writes
   nothing. A saved connection re-runs this file's own init, so the caller
   never has to know what a connection means (spec DECIDE "Re-read after
   save": no new seam call for it). */
bool wallet_connect_uri(const char * uri, char * err, size_t err_n)
{
    if(err && err_n) err[0] = 0;
    if(s_backend != WALLET_BACKEND_NWC) {
        if(err && err_n) snprintf(err, err_n, "%s",
                                  "this backend does not take a connection");
        return false;
    }
    if(!wallet_nwc_connect_uri(uri, err, err_n)) {
        fprintf(stderr, "[wallet] connect refused: %s\n",
                (err && *err) ? err : "no reason given");
        return false;
    }

    s_done = false;
    s_err[0] = 0;
    wallet_init();                  /* idempotent: opens the new connection */
    return true;
}
