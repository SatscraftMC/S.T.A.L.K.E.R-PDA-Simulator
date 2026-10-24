#ifndef PDA_WALLET_H
#define PDA_WALLET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* The wallet seam.
 *
 * The Wallet tab talks to this and nothing else: which backend is up, what
 * it can do, and what went wrong. The first backend is the mock - a preview
 * that is visibly not real money (PDA_WALLET_BACKEND=mock). The real one
 * (LDK Node / NIP-47 / Cashu, decided in its own spec) plugs in behind the
 * same calls, so the UI does not change when it arrives.
 *
 * PDA_WALLET_BACKEND picks the backend:
 *   (unset)         - nothing installed: the honest "not set up" screen
 *   mock            - the preview backend
 *   anything else   - the error screen, naming what was asked for
 */

typedef enum {
    WALLET_BACKEND_NONE = 0,    /* nothing configured */
    WALLET_BACKEND_MOCK,        /* the preview backend */
    WALLET_BACKEND_UNKNOWN      /* asked for something this build does not have */
} wallet_backend_t;

typedef enum {
    WALLET_UNSET = 0,           /* not set up on this device */
    WALLET_READY,               /* a backend is up */
    WALLET_ERROR                /* the backend asked for does not exist */
} wallet_state_t;

/* one line of the history (the History screen puts these on screen) */
typedef struct {
    bool outgoing;              /* true = we paid, false = we were paid */
    int64_t sats;
    char memo[48];
    time_t when;
    bool settled;
} wallet_tx_t;

/* what an invoice this device made is doing. It is the same four answers
 * for every backend: the mock settles on a timer, a real one hears about
 * the payment over the network, but the screens only ever read this. */
typedef enum {
    WALLET_INVOICE_NONE = 0,    /* this device has made no invoice yet */
    WALLET_INVOICE_WAITING,     /* on the wire, waiting for somebody to pay */
    WALLET_INVOICE_PAID,        /* settled: the balance has already moved */
    WALLET_INVOICE_EXPIRED      /* too old to be paid now */
} wallet_invoice_state_t;

/* the last invoice this device made */
typedef struct {
    bool valid;                 /* false: there is none */
    char bolt11[640];
    int64_t sats;
    char memo[48];
    time_t made_at;
    time_t expires_at;
    wallet_invoice_state_t state;
} wallet_invoice_t;

/* reads PDA_WALLET_BACKEND once; safe to call more than once */
void wallet_init(void);

wallet_state_t wallet_state(void);
wallet_backend_t wallet_backend(void);
const char * wallet_backend_name(void);   /* "none" | "mock" | what was asked for */
const char * wallet_error(void);          /* WALLET_ERROR: what went wrong */

bool wallet_is_mock(void);
int64_t wallet_balance_sats(void);

/* The seam the later phases and the backend spec use. Anything that does
 * not have a screen yet answers honestly ("not built yet") - the UI never
 * pretends. */

/* make an invoice for sats, optionally saying what it is for, and put the
   bolt11 in out. The REQUEST screen shows it as a QR code. */
bool wallet_make_invoice(int64_t sats, const char * memo,
                         char * out, size_t out_n, char * err, size_t err_n);

/* the invoice wallet_make_invoice() made last. Returns false (and leaves
   out invalid) when this device has made none. It is a plain read: expiry
   is worked out from the clock, nothing is settled by asking. */
bool wallet_invoice(wallet_invoice_t * out);

/* let the backend advance - the mock settles a pending invoice here, so the
   UI calls this on a timer and then reads wallet_invoice() again. */
void wallet_poll(void);

/* What an invoice is asking for, without paying anything: the confirm
 * screen reads the amount off this before it offers a PAY button. False
 * with err set when the string is not a bolt11 invoice, when it carries no
 * amount (an amount-less invoice cannot be paid by this seam), or when it
 * asks for more than any wallet holds. The string is cleaned up first: a
 * paste may be padded with blanks or wrapped over a line. */
bool wallet_check_invoice(const char * bolt11, int64_t * sats_out,
                          char * err, size_t err_n);

/* What the backend charges on top of sats to move it - shown next to the
 * amount on the confirm screen, before anything is paid. */
int64_t wallet_fee_sats(int64_t sats);

/* Pay it: the amount (plus the fee) leaves the balance and a row lands in
 * the history. memo says what it was for (the merchant card supplies it
 * later; nothing here needs one). sats_out gets the amount paid, not the
 * fee. */
bool wallet_pay_invoice(const char * bolt11, const char * memo,
                        int64_t * sats_out, char * err, size_t err_n);

int wallet_history(wallet_tx_t * out, int max);   /* newest first */

#endif
