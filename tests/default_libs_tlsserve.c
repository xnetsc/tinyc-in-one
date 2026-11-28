/* Default libraries battery: standalone mbedTLS TLS server
   for interoperability checks. Serves one fixed HTTP response
   over TLS, then exits. The driver runs it with a test
   certificate and connects with the system TLS tools (the
   system curl / openssl), and the other way around, so the
   bundled stack is verified against an independent one.
   Usage: default_libs_tlsserve <port> <cert-file> <key-file>
   The server binds the loopback address explicitly: binding the
   wildcard (NULL) asks getaddrinfo for AF_UNSPEC, which on
   macOS returns the IPv6 wildcard first, and a macOS IPv6
   socket defaults to IPV6_V6ONLY, so an IPv4 client to
   127.0.0.1 is refused while Linux (V6ONLY off by default)
   accepts it. The interop client connects to 127.0.0.1, so
   the server binds 127.0.0.1. */
#include <stdio.h>
#include <string.h>
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "mbedtls/error.h"

/* Stage logging + watchdog: on macOS the system-curl leg
   stalls for the full 300 s curl timeout with no server-side
   output at all, so a silent hang is indistinguishable from
   "never accepted" vs "handshake looping". Every stage prints
   as it is reached, handshake iterations are counted, and a
   60 s watchdog reports the stage and iteration count, then
   exits, instead of letting the client burn its whole timeout
   with nothing in the log. */
#ifndef _WIN32
#include <signal.h>
#include <unistd.h>
static volatile const char *tls_stage = "start";
static volatile long tls_handshake_iters = 0;
static void tls_watchdog(int sig) {
    (void) sig;
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "tlsserve: STUCK at %s after 60s (handshake iterations=%ld)\n",
        tls_stage, tls_handshake_iters);
    if (n > 0) { ssize_t w = write(2, buf, (size_t) n); (void) w; }
    _exit(2);
}
#endif

static void tls_err(const char *what, int ret) {
    char buf[128];
    mbedtls_strerror(ret, buf, sizeof(buf));
    printf("%s %d (%s)\n", what, ret, buf);
    fflush(stdout);
}

static int load_file(const char *path, unsigned char *buf, size_t cap, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(buf, 1, cap - 1, f);
    fclose(f);
    buf[n] = 0;
    *len = n + 1; /* include NUL: parse functions accept len incl. terminator */
    return 0;
}

int main(int argc, char **argv) {
    const char *port = argc > 1 ? argv[1] : "8902";
    const char *certf = argc > 2 ? argv[2] : "/tmp/tlscert/srv.crt";
    const char *keyf = argc > 3 ? argv[3] : "/tmp/tlscert/srv.key";
    static unsigned char certbuf[8192], keybuf[8192];
    size_t clen, klen;
    mbedtls_net_context listen_fd, client_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
    int ret;
    mbedtls_net_init(&listen_fd); mbedtls_net_init(&client_fd);
    mbedtls_ssl_init(&ssl); mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&srvcert); mbedtls_pk_init(&pkey);
    mbedtls_entropy_init(&entropy); mbedtls_ctr_drbg_init(&ctr_drbg);
#ifndef _WIN32
    tls_stage = "seed";
#endif
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) != 0) { tls_err("seed", ret); return 1; }
    if (load_file(certf, certbuf, sizeof(certbuf), &clen) || load_file(keyf, keybuf, sizeof(keybuf), &klen)) { printf("file load fail\n"); return 1; }
    if ((ret = mbedtls_x509_crt_parse(&srvcert, certbuf, clen)) != 0) { tls_err("crt parse", ret); return 1; }
    if ((ret = mbedtls_pk_parse_key(&pkey, keybuf, klen, NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) { tls_err("key parse", ret); return 1; }
#ifndef _WIN32
    tls_stage = "bind";
#endif
    if ((ret = mbedtls_net_bind(&listen_fd, "127.0.0.1", port, MBEDTLS_NET_PROTO_TCP)) != 0) { tls_err("bind", ret); return 1; }
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) { tls_err("conf", ret); return 1; }
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_ca_chain(&conf, srvcert.next, NULL);
    if ((ret = mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey)) != 0) { tls_err("own cert", ret); return 1; }
    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) { tls_err("setup", ret); return 1; }
    printf("tls server ready on %s\n", port); fflush(stdout);
#ifndef _WIN32
    tls_stage = "accept";
    signal(SIGALRM, tls_watchdog);
    alarm(60);
#endif
    if ((ret = mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL)) != 0) { tls_err("accept", ret); return 1; }
    printf("accepted\n"); fflush(stdout);
#ifndef _WIN32
    tls_stage = "handshake";
#endif
    mbedtls_ssl_set_bio(&ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
#ifndef _WIN32
        tls_handshake_iters++;
#endif
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) { tls_err("handshake", ret); return 1; }
    }
    printf("handshake ok: %s (iterations=%ld)\n", mbedtls_ssl_get_version(&ssl),
#ifndef _WIN32
           tls_handshake_iters
#else
           0L
#endif
           ); fflush(stdout);
    unsigned char rbuf[2048];
#ifndef _WIN32
    tls_stage = "read";
#endif
    do { ret = mbedtls_ssl_read(&ssl, rbuf, sizeof(rbuf) - 1); } while (ret == MBEDTLS_ERR_SSL_WANT_READ);
    if (ret <= 0) { tls_err("read", ret); return 1; }
    rbuf[ret] = 0;
    printf("request: %.40s\n", (char *)rbuf); fflush(stdout);
    static const char resp[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\ntinyc-tls-ok\n";
    const unsigned char *p = (const unsigned char *)resp;
    size_t left = sizeof(resp) - 1;
#ifndef _WIN32
    tls_stage = "write";
#endif
    while (left) {
        ret = mbedtls_ssl_write(&ssl, p, left);
        if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
        if (ret < 0) { tls_err("write", ret); return 1; }
        p += ret; left -= ret;
    }
    mbedtls_ssl_close_notify(&ssl);
#ifndef _WIN32
    alarm(0);
    tls_stage = "done";
#endif
    printf("tls server done\n");
    return 0;
}
