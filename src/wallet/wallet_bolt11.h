#ifndef PDA_WALLET_BOLT11_H
#define PDA_WALLET_BOLT11_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

/* The shared bolt11 reader, used by both backends behind wallet.h.
 *
 * bolt11_parse() is the amount reader that lived inside wallet_mock_check(),
 * moved out unchanged: the make->check round trip must answer the same sats
 * and the same refusal strings it always did, mock or nwc.
 *
 * The two tagged fields are this slice's (spec-nip47-wallet.md section 6):
 * the payment hash, which is the lookup key for an invoice we made, and the
 * expiry, which says when the screen stops offering it. The tags are walked
 * out of the bech32 data part as BOLT-11 defines them (type 5 bits, length
 * 10 bits big-endian, data length x 5 bits); the bech32 checksum is NOT
 * verified here - the amount path never did, and the mock's invoices are
 * deliberately not bech32. */

/* sats_out gets the amount; false + err set when the string is not a bolt11,
   carries no amount, or asks for more than any wallet holds. Whitespace and
   line wraps are dropped first (a paste is not always one clean line). */
bool bolt11_parse(const char * bolt11, int64_t * sats_out,
                  char * err, size_t err_n);

/* The payment hash tag (p, 256 bits) as 64 lowercase hex, or false when the
   invoice carries no readable one (no caller may invent a key: false is the
   honest answer). */
bool bolt11_payment_hash(const char * bolt11, char * hex65);

/* The expiry tag (x, seconds since the invoice's own timestamp) as a raw
   second count, or false when the tag is absent - the caller applies its own
   fallback, this helper never guesses one. */
bool bolt11_expiry(const char * bolt11, long * secs_out);

#endif
