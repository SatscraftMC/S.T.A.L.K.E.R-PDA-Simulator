#include "nstr/nstr_int.h"

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the OpenSSL reason behind a fatal handshake/read/write, under PDA_DEBUG:
   "tls failed" is the carrier's word, this is the layer's own - it is how
   a certificate problem is told apart from a protocol one without a
   debugger. Not a contract line: the suites assert the `reason:` strings. */
static void tls_debug(const char * where)
{
    if(!getenv("PDA_DEBUG")) return;
    unsigned long e = ERR_get_error();
    char buf[160];
    if(e) ERR_error_string_n(e, buf, sizeof(buf));
    else snprintf(buf, sizeof(buf), "%s", "no OpenSSL error queued");
    fprintf(stderr, "[nstr] tls %s: %s\n", where, buf);
}

/* The OpenSSL call site (spec-nstr-relay §2): system OpenSSL, no vendored
   tree, verification on with no bypass anywhere.

   One SSL_CTX for the process - TLS_client_method(), no compression, the
   system CA store and SSL_VERIFY_PEER - created on first use. The attempt
   specific part is PDA_NSTR_CA: re-read on every attempt (nstr.c calls
   nstr_tls_ca_load() from nstr_open()) and *added* to the ctx, so a
   self-hosted relay's bundle works, a change of path takes effect on the
   next attempt without a restart, and a bundle that will not load is that
   attempt's "no CA" - never a skip of verification.

   The handshake runs as SSL_do_handshake() on the nonblocking socket from
   nstr_step(), against SSL_ERROR_WANT_READ/WANT_WRITE - a slow Pi handshake
   spans ticks, it never blocks one. */

static SSL_CTX * s_ctx = NULL;

bool nstr_tls_open(void)
{
    if(s_ctx) return true;
    s_ctx = SSL_CTX_new(TLS_client_method());
    if(!s_ctx) return false;
    SSL_CTX_set_options(s_ctx, SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_verify(s_ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_default_verify_paths(s_ctx);   /* the system store */
    return true;
}

bool nstr_tls_ca_load(void)
{
    const char * path = getenv("PDA_NSTR_CA");
    if(!path || !*path) return true;           /* nothing set: the store stands */
    if(!nstr_tls_open()) return false;
    if(SSL_CTX_load_verify_locations(s_ctx, path, NULL) != 1) return false;
    return true;
}

bool nstr_tls_begin(struct nstr * c)
{
    if(!c || !nstr_tls_open()) return false;

    SSL * ssl = SSL_new(s_ctx);
    if(!ssl) return false;
    c->ssl = ssl;
    if(SSL_set_fd(ssl, c->fd) != 1) return false;

    if(c->ip_literal) {
        /* an IP-literal URL is checked in IP form, and gets no SNI: RFC 6066
           forbids IP addresses in the SNI extension */
        if(X509_VERIFY_PARAM_set1_ip_asc(SSL_get0_param(ssl), c->host) != 1) return false;
    }
    else {
        if(SSL_set1_host(ssl, c->host) != 1) return false;
        if(SSL_set_tlsext_host_name(ssl, c->host) != 1) return false;
    }
    SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);
    /* SSL_do_handshake() only drives a handshake whose side is already
       chosen: without this the first call fails with "connection type not
       set" (SSL_connect() would set it for us; we must not use it - it
       blocks) */
    SSL_set_connect_state(ssl);
    c->want = POLLIN;
    return true;
}

/* map the SSL error of a non-want call: WANT_READ/WANT_WRITE -> 0 with the
   direction recorded, a clean shutdown -> -2 (the peer closed), anything
   else -> -1 (the caller names the reason by its stage: "tls failed" while
   handshaking, "disconnected" after) */
static int io_result(struct nstr * c, SSL * ssl, int ret)
{
    int e = SSL_get_error(ssl, ret);
    if(e == SSL_ERROR_WANT_READ) { c->want = POLLIN; return 0; }
    if(e == SSL_ERROR_WANT_WRITE) { c->want = POLLOUT; return 0; }
    if(e == SSL_ERROR_ZERO_RETURN) { ERR_clear_error(); return -2; }
    tls_debug("io");
    ERR_clear_error();
    return -1;
}

int nstr_tls_handshake(struct nstr * c)
{
    SSL * ssl = (SSL *)c->ssl;
    if(!ssl) return -1;
    int r = SSL_do_handshake(ssl);
    if(r == 1) return 1;
    return io_result(c, ssl, r);
}

int nstr_tls_read(struct nstr * c, void * buf, int n)
{
    SSL * ssl = (SSL *)c->ssl;
    if(!ssl || n <= 0) return -1;
    int r = SSL_read(ssl, buf, n);
    if(r > 0) return r;
    return io_result(c, ssl, r);
}

int nstr_tls_write(struct nstr * c, const void * buf, int n)
{
    SSL * ssl = (SSL *)c->ssl;
    if(!ssl || n < 0) return -1;
    if(n == 0) return 0;
    int r = SSL_write(ssl, buf, n);
    if(r > 0) return r;
    return io_result(c, ssl, r);
}

void nstr_tls_free(struct nstr * c)
{
    if(c && c->ssl) {
        SSL_free((SSL *)c->ssl);
        c->ssl = NULL;
    }
}
