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

static bool s_done = false;
static wallet_state_t s_state = WALLET_UNSET;
static wallet_backend_t s_backend = WALLET_BACKEND_NONE;
static char s_name[48] = "none";
static char s_err[192];
static int64_t s_balance = 0;

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
        s_balance = wallet_mock_open();
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

int64_t wallet_balance_sats(void)
{
    if(s_state != WALLET_READY) return 0;
    return s_balance;
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

/* Paying arrives with the SEND screen (phase 3); until then it fails with
 * the reason instead of pretending to work. */
bool wallet_pay_invoice(const char * bolt11, int64_t * sats_out,
                        char * err, size_t err_n)
{
    (void)bolt11;
    if(sats_out) *sats_out = 0;
    if(err && err_n) {
        if(s_state != WALLET_READY)
            snprintf(err, err_n, "%s", "the wallet is not set up");
        else
            snprintf(err, err_n, "%s", "paying arrives with the SEND screen");
    }
    return false;
}

int wallet_history(wallet_tx_t * out, int max)
{
    if(s_backend != WALLET_BACKEND_MOCK) return 0;
    return wallet_mock_history(out, max);
}
