#ifndef PDA_IDENTITY_BECH32_H
#define PDA_IDENTITY_BECH32_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* BIP-173 bech32 (the plain checksum, not bech32m), one implementation:
   nostr's npub/nsec encoding. 32-byte keys in, 59 characters out.

   bech32_encode("npub", key32, ...) - lower-case on the wire.
   bech32_decode(str, "npub", ...)   - accepts upper-case as a whole
   (BIP-173 says a decoder must), rejects mixed case. */

bool bech32_encode(const char * hrp, const uint8_t * data, size_t data_len,
                   char * out, size_t outn);

bool bech32_decode(const char * str, const char * hrp,
                   uint8_t * out, size_t outn, size_t * out_len);

#endif
