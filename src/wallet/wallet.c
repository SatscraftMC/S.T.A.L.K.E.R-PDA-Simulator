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

static bool s_done = false;
static wallet_state_t s_state = WALLET_UNSET;
static wallet_backend_t s_backend = WALLET_BACKEND_NONE;
static char s_name[48] = "none";
static char s_err[192];

void wallet_init(void)
{
    if(s_done) return;
    s_done = true;

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

    s_backend = WALLET_BACKEND_UNKNOWN;
    s_state = WALLET_ERROR;
    snprintf(s_err, sizeof(s_err),
             "unknown wallet backend \"%s\" - this build has: mock", b);
}

wallet_state_t wallet_state(void)
{
    return s_state;
}

wallet_backend_t wallet_backend(void)
{
    return s_backend;
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
 * that has already happened. */
int64_t wallet_balance_sats(void)
{
    if(s_state != WALLET_READY) return 0;
    if(s_backend == WALLET_BACKEND_MOCK) return wallet_mock_balance();
    return 0;
}

/* Make an invoice. The mock can; anything that has not landed yet fails
 * with a reason instead of pretending. */
bool wallet_make_invoice(int64_t sats, const char * memo,
                         char * out, size_t out_n, char * err, size_t err_n)
{
    if(out && out_n) out[0] = 0;
    if(s_state != WALLET_READY) {
        if(err && err_n) snprintf(err, err_n, "%s", "the wallet is not set up");
        return false;
    }
    if(s_backend != WALLET_BACKEND_MOCK) {
        if(err && err_n) snprintf(err, err_n,
                                  "%s", "this backend cannot make an invoice yet");
        return false;
    }
    wallet_invoice_t inv;
    if(!wallet_mock_make_invoice(sats, memo, &inv, err, err_n)) return false;
    if(out && out_n) snprintf(out, out_n, "%s", inv.bolt11);
    return true;
}

bool wallet_invoice(wallet_invoice_t * out)
{
    if(!out) return false;
    memset(out, 0, sizeof(*out));
    if(s_backend != WALLET_BACKEND_MOCK) return false;
    return wallet_mock_invoice(out);
}

void wallet_poll(void)
{
    if(s_backend == WALLET_BACKEND_MOCK) wallet_mock_poll();
}

/* the reason a call cannot be made, or "" when it can be: the wallet is
 * not set up, or the backend in front of us is not one this build has */
static bool send_gate(char * err, size_t err_n)
{
    if(s_state != WALLET_READY) {
        if(err && err_n) snprintf(err, err_n, "%s", "the wallet is not set up");
        return false;
    }
    if(s_backend != WALLET_BACKEND_MOCK) {
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

    bool ok = wallet_mock_check(bolt11, sats_out, err, err_n);
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
    if(s_state != WALLET_READY || s_backend != WALLET_BACKEND_MOCK) return 0;
    return wallet_mock_fee(sats);
}

bool wallet_pay_invoice(const char * bolt11, const char * memo,
                        int64_t * sats_out, char * err, size_t err_n)
{
    if(sats_out) *sats_out = 0;
    if(!send_gate(err, err_n)) return false;

    if(!wallet_mock_pay(bolt11, memo, sats_out, err, err_n)) {
        fprintf(stderr, "[wallet] pay refused: %s\n",
                (err && *err) ? err : "no reason given");
        return false;
    }
    return true;
}

int wallet_history(wallet_tx_t * out, int max)
{
    if(s_backend != WALLET_BACKEND_MOCK) return 0;
    return wallet_mock_history(out, max);
}
