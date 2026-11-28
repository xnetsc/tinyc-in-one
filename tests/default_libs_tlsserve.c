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
/* MBEDTLS_ALLOW_PRIVATE_ACCESS exposes ssl.state (the handshake
   state machine position) to this diagnostic test program: the
   stall is inside one handshake step, so the state at the stall
   is the diagnosis. It only affects this file's compilation;
   the library itself is built without it. */
#define MBEDTLS_ALLOW_PRIVATE_ACCESS
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
#include <sys/time.h>
#include <unistd.h>
#if defined(__ANDROID__)
#include <poll.h>
#endif
static volatile const char *tls_stage = "start";
static volatile long tls_handshake_iters = 0;
static volatile int tls_hs_state = -1;
static void tls_watchdog(int sig) {
    (void) sig;
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
        "tlsserve: STUCK at %s after 60s (handshake iterations=%ld, hs_state=%d)\n",
        tls_stage, tls_handshake_iters, tls_hs_state);
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

/* Handshake state names for the step log (mbedtls_ssl_states,
   public enum in mbedtls/ssl.h). */
static const char *tls_state_name(int s) {
    switch (s) {
    case MBEDTLS_SSL_HELLO_REQUEST: return "HELLO_REQUEST";
    case MBEDTLS_SSL_CLIENT_HELLO: return "CLIENT_HELLO";
    case MBEDTLS_SSL_SERVER_HELLO: return "SERVER_HELLO";
    case MBEDTLS_SSL_SERVER_CERTIFICATE: return "SERVER_CERTIFICATE";
    case MBEDTLS_SSL_SERVER_KEY_EXCHANGE: return "SERVER_KEY_EXCHANGE";
    case MBEDTLS_SSL_CERTIFICATE_REQUEST: return "CERTIFICATE_REQUEST";
    case MBEDTLS_SSL_SERVER_HELLO_DONE: return "SERVER_HELLO_DONE";
    case MBEDTLS_SSL_CLIENT_CERTIFICATE: return "CLIENT_CERTIFICATE";
    case MBEDTLS_SSL_CLIENT_KEY_EXCHANGE: return "CLIENT_KEY_EXCHANGE";
    case MBEDTLS_SSL_CERTIFICATE_VERIFY: return "CERTIFICATE_VERIFY";
    case MBEDTLS_SSL_CLIENT_CHANGE_CIPHER_SPEC: return "CLIENT_CHANGE_CIPHER_SPEC";
    case MBEDTLS_SSL_CLIENT_FINISHED: return "CLIENT_FINISHED";
    case MBEDTLS_SSL_SERVER_CHANGE_CIPHER_SPEC: return "SERVER_CHANGE_CIPHER_SPEC";
    case MBEDTLS_SSL_SERVER_FINISHED: return "SERVER_FINISHED";
    case MBEDTLS_SSL_FLUSH_BUFFERS: return "FLUSH_BUFFERS";
    case MBEDTLS_SSL_HANDSHAKE_WRAPUP: return "HANDSHAKE_WRAPUP";
    case MBEDTLS_SSL_NEW_SESSION_TICKET: return "NEW_SESSION_TICKET";
    case MBEDTLS_SSL_SERVER_HELLO_VERIFY_REQUEST_SENT: return "SERVER_HELLO_VERIFY_REQUEST_SENT";
    case MBEDTLS_SSL_HELLO_RETRY_REQUEST: return "HELLO_RETRY_REQUEST";
    case MBEDTLS_SSL_ENCRYPTED_EXTENSIONS: return "ENCRYPTED_EXTENSIONS";
    case MBEDTLS_SSL_END_OF_EARLY_DATA: return "END_OF_EARLY_DATA";
    case MBEDTLS_SSL_CLIENT_CERTIFICATE_VERIFY: return "CLIENT_CERTIFICATE_VERIFY";
    case MBEDTLS_SSL_CLIENT_CCS_AFTER_SERVER_FINISHED: return "CLIENT_CCS_AFTER_SERVER_FINISHED";
    case MBEDTLS_SSL_CLIENT_CCS_BEFORE_2ND_CLIENT_HELLO: return "CLIENT_CCS_BEFORE_2ND_CLIENT_HELLO";
    case MBEDTLS_SSL_SERVER_CCS_AFTER_SERVER_HELLO: return "SERVER_CCS_AFTER_SERVER_HELLO";
    case MBEDTLS_SSL_CLIENT_CCS_AFTER_CLIENT_HELLO: return "CLIENT_CCS_AFTER_CLIENT_HELLO";
    case MBEDTLS_SSL_SERVER_CCS_AFTER_HELLO_RETRY_REQUEST: return "SERVER_CCS_AFTER_HELLO_RETRY_REQUEST";
    case MBEDTLS_SSL_HANDSHAKE_OVER: return "HANDSHAKE_OVER";
    case MBEDTLS_SSL_TLS1_3_NEW_SESSION_TICKET: return "TLS1_3_NEW_SESSION_TICKET";
    case MBEDTLS_SSL_TLS1_3_NEW_SESSION_TICKET_FLUSH: return "TLS1_3_NEW_SESSION_TICKET_FLUSH";
    default: return "UNKNOWN";
    }
}

/* Logging entropy/RNG wrappers: the stall sits between the
   ClientHello read and the first ServerHello send, so the two
   calls that can block there without any I/O are the entropy
   gather and the random generation. Each call logs entry and
   return; an entry with no return names the blocking call. */
static int tls_entropy(void *data, unsigned char *output, size_t len) {
    int ret;
    printf("entropy: len=%lu\n", (unsigned long) len); fflush(stdout);
    ret = mbedtls_entropy_func(data, output, len);
    printf("entropy: ret=%d\n", ret); fflush(stdout);
    return ret;
}
static int tls_rng(void *p_rng, unsigned char *output, size_t len) {
    int ret;
    printf("rng: len=%lu\n", (unsigned long) len); fflush(stdout);
    ret = mbedtls_ctr_drbg_random(p_rng, output, len);
    printf("rng: ret=%d\n", ret); fflush(stdout);
    return ret;
}

/* Logging bio wrappers: the handshake can block inside the
   first recv with no output at all, so each recv/send logs
   its entry and its return. A recv entry with no return means
   the ClientHello never reached the bio; a return whose first
   byte is 0x16 means a TLS record arrived and the handshake
   is stuck parsing it, not waiting for it. */
static int tls_recv(void *ctx, unsigned char *buf, size_t len) {
    int fd = ((mbedtls_net_context *) ctx)->fd;
    int ret;
    printf("bio recv: fd=%d len=%lu\n", fd, (unsigned long) len);
    fflush(stdout);
    ret = mbedtls_net_recv(ctx, buf, len);
    printf("bio recv: fd=%d ret=%d first=0x%02x\n", fd, ret,
           ret > 0 ? buf[0] : 0);
    fflush(stdout);
    return ret;
}
static int tls_send(void *ctx, const unsigned char *buf, size_t len) {
    int fd = ((mbedtls_net_context *) ctx)->fd;
    int ret;
    printf("bio send: fd=%d len=%lu\n", fd, (unsigned long) len);
    fflush(stdout);
    ret = mbedtls_net_send(ctx, buf, len);
    printf("bio send: fd=%d ret=%d\n", fd, ret);
    fflush(stdout);
    return ret;
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
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, tls_entropy, &entropy, NULL, 0)) != 0) { tls_err("seed", ret); return 1; }
    if (load_file(certf, certbuf, sizeof(certbuf), &clen) || load_file(keyf, keybuf, sizeof(keybuf), &klen)) { printf("file load fail\n"); return 1; }
    if ((ret = mbedtls_x509_crt_parse(&srvcert, certbuf, clen)) != 0) { tls_err("crt parse", ret); return 1; }
    if ((ret = mbedtls_pk_parse_key(&pkey, keybuf, klen, NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0) { tls_err("key parse", ret); return 1; }
#ifndef _WIN32
    tls_stage = "bind";
#endif
    if ((ret = mbedtls_net_bind(&listen_fd, "127.0.0.1", port, MBEDTLS_NET_PROTO_TCP)) != 0) { tls_err("bind", ret); return 1; }
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) { tls_err("conf", ret); return 1; }
    mbedtls_ssl_conf_rng(&conf, tls_rng, &ctr_drbg);
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
    /* Readiness probe before the first handshake call: if the
       client already sent bytes, the accepted socket is
       readable and the handshake's first recv returns at once;
       if it reports nothing after 3 s, no ClientHello has
       arrived and the stall is on the client/network side, not
       in the bio. Android uses poll(): bionic's FD_SET is a
       fortified inline function referencing __builtin_object_size,
       which TCC does not implement (the same reason
       probe_nb_connect in default_libs_net.c avoids select()
       on Android); the round-6 select probe broke the android
       tlsserve build with an unresolved __builtin_object_size. */
#if defined(__ANDROID__)
    {
        struct pollfd pfd;
        pfd.fd = client_fd.fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, 3000);
        printf("pre-handshake poll: fd=%d ret=%d readable=%d\n",
               client_fd.fd, pr, pr > 0 && (pfd.revents & POLLIN));
        fflush(stdout);
    }
#else
    {
        fd_set rfds;
        struct timeval tv = { 3, 0 };
        FD_ZERO(&rfds);
        FD_SET(client_fd.fd, &rfds);
        int sr = select(client_fd.fd + 1, &rfds, NULL, NULL, &tv);
        printf("pre-handshake select: fd=%d ret=%d readable=%d\n",
               client_fd.fd, sr, sr > 0 && FD_ISSET(client_fd.fd, &rfds));
        fflush(stdout);
    }
#endif
#endif
    mbedtls_ssl_set_bio(&ssl, &client_fd, tls_send, tls_recv, NULL);
    /* Single-step handshake loop: mbedtls_ssl_handshake() is
       exactly this loop (while state != HANDSHAKE_OVER, call
       handshake_step and break on any non-zero return), but the
       single call gives no position when it never returns. The
       stall is inside one step after the ClientHello read, so
       the state logged before each step names the state the
       stall is in; the watchdog reports it as well. */
    while (ssl.state != MBEDTLS_SSL_HANDSHAKE_OVER) {
#ifndef _WIN32
        tls_hs_state = ssl.state;
        printf("handshake step: state=%s\n", tls_state_name(ssl.state));
        fflush(stdout);
#endif
        ret = mbedtls_ssl_handshake_step(&ssl);
#ifndef _WIN32
        tls_handshake_iters++;
#endif
        if (ret != 0 && ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) { tls_err("handshake", ret); return 1; }
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
