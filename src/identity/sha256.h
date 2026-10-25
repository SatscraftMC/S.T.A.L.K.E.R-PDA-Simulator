#ifndef PDA_IDENTITY_SHA256_H
#define PDA_IDENTITY_SHA256_H

#include <stddef.h>
#include <stdint.h>

/* SHA-256 (FIPS 180-4), written here because libsecp256k1 keeps its own
   hash internal and exposes no public SHA-256 API.

   It only ever hashes public serializations (event ids): secret keys are
   hashed exclusively inside libsecp256k1's constant-time code. Correctness
   is the bar, not side-channel hardness - the test plan checks the
   published NIST vectors through the [id] selftest line. */

typedef struct {
    uint32_t h[8];
    uint64_t bytes;                 /* total message length */
    uint8_t  buf[64];
    size_t   fill;                  /* bytes currently in buf */
} sha256_ctx_t;

void sha256_init(sha256_ctx_t * c);
void sha256_update(sha256_ctx_t * c, const void * data, size_t len);
void sha256_final(sha256_ctx_t * c, uint8_t out[32]);

/* one shot */
void sha256(const void * data, size_t len, uint8_t out[32]);

#endif
