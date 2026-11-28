/* Default libraries battery, part 2: network checks.
   One process hosts three loopback servers on threads (a raw
   HTTP responder, an mbedTLS HTTPS responder and a raw
   WebSocket echo) and drives them with the bundled libcurl:
   HTTP GET and POST, HTTPS GET with certificate verification
   on (and a negative run without the CA), and a WebSocket
   echo round trip. Built with the packaged compiler against
   the staged libraries and run on the target.
   Usage: default_libs_net [base-port]   (default 18231) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* Socket and thread portability: the same battery runs on the
   POSIX targets and on Windows (winsock + CreateThread), so
   the platform CI legs execute one identical test program. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET sock_t;
#define sock_close closesocket
#define sock_valid(s) ((s) != INVALID_SOCKET)
#define sleep_ms(ms) Sleep(ms)
typedef HANDLE thread_t;
#define thread_create(th, fn) \
    ((*(th) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(fn), \
                           NULL, 0, NULL)) != NULL ? 0 : -1)
#define thread_join(th) (WaitForSingleObject((th), INFINITE), CloseHandle(th))
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
typedef int sock_t;
#define sock_close close
#define sock_valid(s) ((s) >= 0)
#define sleep_ms(ms) usleep((ms) * 1000)
typedef pthread_t thread_t;
#define thread_create(th, fn) pthread_create((th), NULL, (fn), NULL)
#define thread_join(th) pthread_join((th), NULL)
#endif
#include <curl/curl.h>
#include "mbedtls/net_sockets.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/pk.h"
#include "default_libs_cert.h"

static int base_port;

/* The server threads bind asynchronously; each sets its
   ready flag once it is listening, and main waits on the
   flags before driving that server. wait_ready returns 0
   (and says so) when a server never becomes ready, so a
   failed check reports the missing server instead of a
   misleading client error. */
static volatile int ready_http, ready_ws, ready_tls;
static int wait_ready(volatile int *flag, const char *name, int port) {
    for (int i = 0; i < 400 && !*flag; i++) sleep_ms(25);
    if (!*flag) {
        printf("server not ready: %s on 127.0.0.1:%d after 10s\n", name, port);
        return 0;
    }
    return 1;
}

/* Millisecond clock for probe sequencing: the CI log merges
   stdout and stderr, so log order alone cannot sequence events
   (the servers print to stderr, the probes to stdout); the
   timestamps can. */
static unsigned long long now_ms(void) {
#ifdef _WIN32
    /* GetTickCount, not GetTickCount64: the bundled headers do
       not declare the 64-bit one. 32-bit milliseconds wrap
       every ~49 days, which cannot matter for ordering events
       inside one battery run. */
    return (unsigned long long)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)tv.tv_sec * 1000ull +
           (unsigned long long)tv.tv_usec / 1000ull;
#endif
}

/* ---------------- raw HTTP server ---------------- */
static sock_t listen_on(int port) {
    sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons((unsigned short)port);
    if (bind(fd, (struct sockaddr *)&a, sizeof(a)) || listen(fd, 4)) {
#ifdef _WIN32
        fprintf(stderr, "listen_on: port %d bind/listen failed: WSA error %d\n",
                port, WSAGetLastError());
#else
        fprintf(stderr, "listen_on: port %d bind/listen failed: %s\n",
                port, strerror(errno));
#endif
        exit(1);
    }
    fprintf(stderr, "listening on 127.0.0.1:%d\n", port);
    /* Report the address the socket actually bound to, not just
       the requested one: if the bind landed elsewhere (wrong
       byte order, wrong address family) the log line above
       would still claim the requested port. */
    {
        struct sockaddr_in actual;
#ifdef _WIN32
        int alen = sizeof(actual);
#else
        socklen_t alen = sizeof(actual);
#endif
        if (getsockname(fd, (struct sockaddr *)&actual, &alen) == 0) {
            const unsigned char *b = (const unsigned char *)&actual.sin_addr;
            fprintf(stderr, "listening actual %u.%u.%u.%u:%d\n",
                    b[0], b[1], b[2], b[3], ntohs(actual.sin_port));
        }
    }
    return fd;
}

static int read_until(sock_t fd, char *buf, int cap, const char *end) {
    int n = 0;
    buf[0] = 0;
    while (n < cap - 1) {
        int r = (int)recv(fd, buf + n, 1, 0);
        if (r <= 0) break;
        n += r;
        buf[n] = 0;
        if (strstr(buf, end)) break;
    }
    return n;
}

static void *http_server(void *arg) {
    sock_t lfd = listen_on(base_port);
    ready_http = 1;
    for (int i = 0; i < 2; i++) {           /* one GET, one POST */
        sock_t fd = accept(lfd, NULL, NULL);
        if (!sock_valid(fd)) {
#ifdef _WIN32
            fprintf(stderr, "http server: accept failed: WSA error %d\n",
                    WSAGetLastError());
#else
            fprintf(stderr, "http server: accept failed: %s\n", strerror(errno));
#endif
            return NULL;
        }
        /* Connection count and time: the nb probe's connect is
           refused (SO_ERROR 10061) only if this listener is
           already closed, which happens after 2 served
           connections -- so the count at probe time decides
           whether the probe hit a live or a closed listener. */
        fprintf(stderr, "http server: accepted conn #%d at t=%llu ms\n",
                i + 1, now_ms());
        static char buf[8192];
        int n = read_until(fd, buf, sizeof(buf), "\r\n\r\n");
        int clen = 0;
        const char *cl = strstr(buf, "Content-Length:");
        if (cl) clen = atoi(cl + 15);
        while (n - (int)(strstr(buf, "\r\n\r\n") - buf) - 4 < clen) {
            int r = (int)recv(fd, buf + n, sizeof(buf) - 1 - n, 0);
            if (r <= 0) break;
            n += r;
            buf[n] = 0;
        }
        const char *body = strstr(buf, "\r\n\r\n") + 4;
        char resp[8192];
        int rn;
        if (!strncmp(buf, "POST", 4))
            rn = snprintf(resp, sizeof(resp),
                          "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s",
                          clen, body);
        else
            rn = snprintf(resp, sizeof(resp),
                          "HTTP/1.1 200 OK\r\nContent-Length: 14\r\n\r\n"
                          "tinyc-http-ok\n");
        send(fd, resp, rn, 0);
        sock_close(fd);
    }
    fprintf(stderr, "http server: served 2, closing listener at t=%llu ms\n",
            now_ms());
    sock_close(lfd);
    return NULL;
}

/* ---------------- WebSocket echo server ---------------- */

/* The Sec-WebSocket-Accept value needs SHA-1 + base64; both
   come from the bundled mbedTLS. */
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"
static void *ws_server(void *arg) {
    sock_t lfd = listen_on(base_port + 2);
    ready_ws = 1;
    sock_t fd = accept(lfd, NULL, NULL);
    if (!sock_valid(fd)) {
#ifdef _WIN32
        fprintf(stderr, "ws server: accept failed: WSA error %d\n",
                WSAGetLastError());
#else
        fprintf(stderr, "ws server: accept failed: %s\n", strerror(errno));
#endif
        return NULL;
    }
    fprintf(stderr, "ws server: accepted conn at t=%llu ms\n", now_ms());
    static char buf[8192];
    read_until(fd, buf, sizeof(buf), "\r\n\r\n");
    char key[128] = { 0 };
    const char *k = strstr(buf, "Sec-WebSocket-Key:");
    if (k) sscanf(k + 18, "%127s", key);
    char acc_in[256];
    snprintf(acc_in, sizeof(acc_in), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);
    unsigned char digest[20];
    mbedtls_sha1((const unsigned char *)acc_in, strlen(acc_in), digest);
    unsigned char accept[64];
    size_t alen = 0;
    mbedtls_base64_encode(accept, sizeof(accept), &alen, digest, 20);
    accept[alen] = 0;
    char resp[512];
    int rn = snprintf(resp, sizeof(resp),
                      "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                      "Sec-WebSocket-Accept: %s\r\n\r\n", (char *)accept);
    send(fd, resp, rn, 0);
    /* one masked text frame in, one unmasked frame out */
    unsigned char h[2];
    if (recv(fd, h, 2, MSG_WAITALL) != 2) { sock_close(fd); return NULL; }
    int op = h[0] & 0x0f;
    int masked = h[1] & 0x80;
    unsigned long len = h[1] & 0x7f;
    if (len == 126) {
        unsigned char e[2];
        recv(fd, e, 2, MSG_WAITALL);
        len = (e[0] << 8) | e[1];
    }
    unsigned char mask[4] = { 0 };
    if (masked) recv(fd, mask, 4, MSG_WAITALL);
    static unsigned char payload[4096];
    if (len > sizeof(payload)) len = sizeof(payload);
    if (recv(fd, payload, (int)len, MSG_WAITALL) < 0) { sock_close(fd); return NULL; }
    for (unsigned long i = 0; i < len; i++) payload[i] ^= mask[i % 4];
    unsigned char oh[4];
    oh[0] = 0x80 | op;
    int hn = 2;
    if (len < 126) oh[1] = (unsigned char)len;
    else { oh[1] = 126; oh[2] = (len >> 8) & 0xff; oh[3] = len & 0xff; hn = 4; }
    send(fd, oh, hn, 0);
    send(fd, payload, (int)len, 0);
    sock_close(fd);
    sock_close(lfd);
    return NULL;
}

/* ---------------- HTTPS server (mbedTLS) ---------------- */
static void *https_server(void *arg) {
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
    char port[8];
    snprintf(port, sizeof(port), "%d", base_port + 1);
    if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0)) return NULL;
    if (mbedtls_x509_crt_parse(&srvcert, (const unsigned char *)TEST_SRV_CERT_PEM,
                               strlen(TEST_SRV_CERT_PEM) + 1)) return NULL;
    if (mbedtls_pk_parse_key(&pkey, (const unsigned char *)TEST_SRV_KEY_PEM,
                             strlen(TEST_SRV_KEY_PEM) + 1, NULL, 0,
                             mbedtls_ctr_drbg_random, &ctr_drbg)) return NULL;
    if (mbedtls_net_bind(&listen_fd, "127.0.0.1", port, MBEDTLS_NET_PROTO_TCP)) {
        fprintf(stderr, "tls server: bind failed on 127.0.0.1:%s\n", port);
        return NULL;
    }
    fprintf(stderr, "listening on 127.0.0.1:%s (tls)\n", port);
    ready_tls = 1;
    if (mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_SERVER,
                                    MBEDTLS_SSL_TRANSPORT_STREAM,
                                    MBEDTLS_SSL_PRESET_DEFAULT)) return NULL;
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_ca_chain(&conf, srvcert.next, NULL);
    if (mbedtls_ssl_conf_own_cert(&conf, &srvcert, &pkey)) return NULL;
    /* Two connections: the verified GET, then the no-CA run
       whose handshake the client aborts (accept either a clean
       response or a handshake failure on the second one). */
    for (int conn = 0; conn < 2; conn++) {
        mbedtls_net_init(&client_fd);
        mbedtls_ssl_init(&ssl);
        if (mbedtls_ssl_setup(&ssl, &conf)) return NULL;
        if (mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL)) return NULL;
        mbedtls_ssl_set_bio(&ssl, &client_fd, mbedtls_net_send, mbedtls_net_recv, NULL);
        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
                break;
        }
        if (ret != 0) {         /* aborted handshake: expected on conn 2 */
            mbedtls_ssl_free(&ssl);
            mbedtls_net_free(&client_fd);
            continue;
        }
        unsigned char rbuf[2048];
        do { ret = mbedtls_ssl_read(&ssl, rbuf, sizeof(rbuf) - 1); }
        while (ret == MBEDTLS_ERR_SSL_WANT_READ);
        if (ret <= 0) return NULL;
        static const char resp[] =
            "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\ntinyc-tls-ok\n";
        const unsigned char *p = (const unsigned char *)resp;
        size_t left = sizeof(resp) - 1;
        while (left) {
            ret = mbedtls_ssl_write(&ssl, p, left);
            if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
            if (ret < 0) return NULL;
            p += ret; left -= (size_t)ret;
        }
        mbedtls_ssl_close_notify(&ssl);
        mbedtls_ssl_free(&ssl);
        mbedtls_net_free(&client_fd);
    }
    return NULL;
}

/* ---------------- libcurl client checks ---------------- */
static char cbuf[8192];
static size_t clen;
static size_t wr(void *p, size_t sz, size_t nm, void *u) {
    size_t n = sz * nm;
    memcpy(cbuf + clen, p, n);
    clen += n;
    cbuf[clen] = 0;
    return n;
}

static int check_http(void) {
    char url[64];
    char errbuf[CURL_ERROR_SIZE];
    CURL *c = curl_easy_init();
    if (!c) { printf("curl: init failed\n"); return 1; }
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, wr);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);
    /* Verbose connect trace: on the Windows runner the GET fails
       with rc 7 while a raw socket connect to the same port
       succeeds, so the failure is inside libcurl's connect path;
       the trace shows which step stalls or fails there. */
    curl_easy_setopt(c, CURLOPT_VERBOSE, 1L);
    errbuf[0] = 0;
    clen = 0; cbuf[0] = 0;
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/hello", base_port);
    curl_easy_setopt(c, CURLOPT_URL, url);
    CURLcode rc = curl_easy_perform(c);
    if (rc != CURLE_OK) {
        printf("curl: GET failed: %s (%d)%s%s\n", curl_easy_strerror(rc),
               (int)rc, errbuf[0] ? ": " : "", errbuf);
        return 1;
    }
    if (!strstr(cbuf, "tinyc-http-ok")) { printf("curl: GET body: %s\n", cbuf); return 1; }
    clen = 0; cbuf[0] = 0;
    snprintf(url, sizeof(url), "http://127.0.0.1:%d/echo", base_port);
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_POSTFIELDS, "ping=42");
    errbuf[0] = 0;
    rc = curl_easy_perform(c);
    if (rc != CURLE_OK) {
        printf("curl: POST failed: %s (%d)%s%s\n", curl_easy_strerror(rc),
               (int)rc, errbuf[0] ? ": " : "", errbuf);
        return 1;
    }
    if (!strstr(cbuf, "ping=42")) { printf("curl: POST body: %s\n", cbuf); return 1; }
    curl_easy_cleanup(c);
    printf("curl: http GET and POST ok\n");
    return 0;
}

static int check_https(const char *cafile) {
    char url[64];
    char errbuf[CURL_ERROR_SIZE];
    snprintf(url, sizeof(url), "https://127.0.0.1:%d/", base_port + 1);
    CURL *c = curl_easy_init();
    if (!c) { printf("curl: init failed\n"); return 1; }
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, wr);
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_CAINFO, cafile);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);
    errbuf[0] = 0;
    clen = 0; cbuf[0] = 0;
    CURLcode rc = curl_easy_perform(c);
    if (rc != CURLE_OK) {
        printf("curl: HTTPS failed: %s (%d)%s%s\n", curl_easy_strerror(rc),
               (int)rc, errbuf[0] ? ": " : "", errbuf);
        return 1;
    }
    if (!strstr(cbuf, "tinyc-tls-ok")) { printf("curl: HTTPS body: %s\n", cbuf); return 1; }
    curl_easy_cleanup(c);
    /* Negative: without the test CA the same server must be
       rejected (verification stays on). */
    c = curl_easy_init();
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, wr);
    curl_easy_setopt(c, CURLOPT_URL, url);
    rc = curl_easy_perform(c);
    curl_easy_cleanup(c);
    if (rc == CURLE_OK) {
        printf("curl: HTTPS without CA unexpectedly succeeded\n");
        return 1;
    }
    printf("curl: https GET with verification ok, no-CA rejected\n");
    return 0;
}

static int check_ws(void) {
    char url[64];
    char errbuf[CURL_ERROR_SIZE];
    snprintf(url, sizeof(url), "ws://127.0.0.1:%d/", base_port + 2);
    CURL *c = curl_easy_init();
    if (!c) { printf("curl: init failed\n"); return 1; }
    curl_easy_setopt(c, CURLOPT_URL, url);
    curl_easy_setopt(c, CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);
    errbuf[0] = 0;
    CURLcode crc = curl_easy_perform(c);
    if (crc != CURLE_OK) {
        printf("curl: ws connect failed: %s (%d)%s%s\n", curl_easy_strerror(crc),
               (int)crc, errbuf[0] ? ": " : "", errbuf);
        return 1;
    }
    const char *msg = "tinyc-ws-ping";
    size_t sent = 0;
    crc = curl_ws_send(c, msg, strlen(msg), &sent, 0, CURLWS_TEXT);
    if (crc != CURLE_OK) {
        printf("curl: ws send failed: %s (%d)%s%s\n", curl_easy_strerror(crc),
               (int)crc, errbuf[0] ? ": " : "", errbuf);
        return 1;
    }
    curl_socket_t sock;
    curl_easy_getinfo(c, CURLINFO_ACTIVESOCKET, &sock);
    char buf[256];
    size_t nread = 0;
    const struct curl_ws_frame *meta = NULL;
    CURLcode rc = CURLE_AGAIN;
    for (int i = 0; i < 100 && rc == CURLE_AGAIN; i++) {
        /* Wait briefly for readability between recv retries. */
        fd_set rfds;
        struct timeval tv = { 0, 100000 };
        FD_ZERO(&rfds);
        FD_SET((int)sock, &rfds);
        select((int)sock + 1, &rfds, NULL, NULL, &tv);
        rc = curl_ws_recv(c, buf, sizeof(buf) - 1, &nread, &meta);
    }
    if (rc != CURLE_OK) {
        printf("curl: ws recv failed: %s (%d)%s%s\n", curl_easy_strerror(rc),
               (int)rc, errbuf[0] ? ": " : "", errbuf);
        return 1;
    }
    buf[nread] = 0;
    if (strcmp(buf, msg)) { printf("curl: ws echo was: %s\n", buf); return 1; }
    curl_easy_cleanup(c);
    printf("curl: websocket echo ok\n");
    return 0;
}

/* A plain socket connect to the HTTP port, run only after the
   libcurl GET has failed: it separates "the server is not
   reachable on this port" from "libcurl's connect path is
   broken". A minimal well-formed GET is sent so the server's
   read path sees a complete request (a bare connect-and-close
   would leave it parsing an empty buffer). */
static void probe_raw_connect(void) {
    sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!sock_valid(fd)) { printf("raw connect: socket failed\n"); return; }
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons((unsigned short)base_port);
    if (connect(fd, (struct sockaddr *)&a, sizeof(a)) != 0) {
#ifdef _WIN32
        printf("raw connect: failed, WSA error %d\n", WSAGetLastError());
#else
        printf("raw connect: failed, errno %d (%s)\n", errno, strerror(errno));
#endif
        sock_close(fd);
        return;
    }
    static const char req[] = "GET /probe HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    send(fd, req, (int)sizeof(req) - 1, 0);
    char rbuf[256];
    int rn = (int)recv(fd, rbuf, sizeof(rbuf) - 1, 0);
    if (rn > 0) {
        rbuf[rn] = 0;
        printf("raw connect: ok, server replied: %.40s\n", rbuf);
    } else {
        printf("raw connect: ok, no reply\n");
    }
    sock_close(fd);
}

/* Replicate libcurl's connect steps with plain socket calls:
   non-blocking connect, select for writability, then SO_ERROR.
   The raw blocking connect above can pass while this one fails,
   which is exactly the difference between "the server is
   reachable" and "libcurl's non-blocking connect path works";
   each step prints its own result so the log shows where the
   path breaks. Runs on every platform so the probe itself is
   exercised by the local battery too. */

/* Read the pending connect error off the socket: after a wait
   timeout the error is the only authority on whether the
   connect is merely slow or already failed, and with what
   code. On Windows the value is a Winsock code (10061 =
   refused, 10060 = timed out, 10054 = reset); elsewhere it
   is an errno. */
static void print_so_error(sock_t fd) {
    int soerr = 0;
#ifdef _WIN32
    int olen = sizeof(soerr);
#else
    socklen_t olen = sizeof(soerr);
#endif
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&soerr, &olen) != 0) {
#ifdef _WIN32
        printf("nb connect: getsockopt SO_ERROR failed, WSA error %d\n",
               WSAGetLastError());
#else
        printf("nb connect: getsockopt SO_ERROR failed, errno %d (%s)\n",
               errno, strerror(errno));
#endif
        return;
    }
#ifdef _WIN32
    printf("nb connect: SO_ERROR=%d\n", soerr);
#else
    printf("nb connect: SO_ERROR=%d (%s)\n", soerr, strerror(soerr));
#endif
    fflush(stdout);
}

static void probe_nb_connect(void) {
    sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (!sock_valid(fd)) {
#ifdef _WIN32
        printf("nb connect: socket failed, WSA error %d\n", WSAGetLastError());
#else
        printf("nb connect: socket failed, errno %d (%s)\n",
               errno, strerror(errno));
#endif
        return;
    }
#ifdef _WIN32
    /* The WSAPoll event constants are the Winsock numbering, not
       the BSD poll(2) numbering: print what this binary was built
       with so the revents decode below is self-evidencing and no
       reader has to guess which numbering applies. */
    printf("nb connect: WSAPoll constants: POLLIN=0x%x POLLPRI=0x%x "
           "POLLOUT=0x%x POLLERR=0x%x POLLHUP=0x%x POLLNVAL=0x%x\n",
           (unsigned)POLLIN, (unsigned)POLLPRI, (unsigned)POLLOUT,
           (unsigned)POLLERR, (unsigned)POLLHUP, (unsigned)POLLNVAL);
    fflush(stdout);
#endif
#ifdef _WIN32
    u_long nb = 1;
    if (ioctlsocket(fd, FIONBIO, &nb) != 0) {
        printf("nb connect: ioctlsocket failed, WSA error %d\n",
               WSAGetLastError());
        sock_close(fd);
        return;
    }
#else
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0 || fcntl(fd, F_SETFL, fl | O_NONBLOCK) != 0) {
        printf("nb connect: fcntl O_NONBLOCK failed, errno %d (%s)\n",
               errno, strerror(errno));
        sock_close(fd);
        return;
    }
#endif
    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons((unsigned short)base_port);
    int rc = connect(fd, (struct sockaddr *)&a, sizeof(a));
    if (rc == 0) {
        printf("nb connect: connected immediately\n");
        sock_close(fd);
        return;
    }
#ifdef _WIN32
    int cerr = WSAGetLastError();
    if (cerr != WSAEWOULDBLOCK) {
        printf("nb connect: connect failed at once, WSA error %d\n", cerr);
        sock_close(fd);
        return;
    }
#else
    if (errno != EINPROGRESS) {
        printf("nb connect: connect failed at once, errno %d (%s)\n",
               errno, strerror(errno));
        sock_close(fd);
        return;
    }
#endif
#ifdef _WIN32
    printf("nb connect: connect in progress (WSAEWOULDBLOCK), t=%llu ms\n",
           now_ms());
#else
    printf("nb connect: connect in progress (EINPROGRESS), t=%llu ms\n",
           now_ms());
#endif
    fflush(stdout);
    /* Wait for writability with select; on a timeout the
       pending connect error (print_so_error) is the authority
       on whether the connect is merely slow or already
       failed. */
    fd_set wfds;
    struct timeval tv = { 5, 0 };
    FD_ZERO(&wfds);
    FD_SET(fd, &wfds);
#ifdef _WIN32
    /* Winsock select reads the socket handles out of the
       fd_set: this pre-select fd_count is the authority on
       whether the macro registered the socket. select()
       rewrites the sets in place and empties them on a
       timeout, so a fd_count read after it returns is 0 by
       design and says nothing about FD_SET. */
    printf("nb connect: socket=%lld fd_count=%u t=%llu ms\n",
           (long long)fd, wfds.fd_count, now_ms());
    fflush(stdout);
#endif
    rc = select((int)fd + 1, NULL, &wfds, NULL, &tv);
    if (rc == 0) {
#ifdef _WIN32
        printf("nb connect: select timed out after 5s (not writable), "
               "WSA error %d, FD_ISSET=%d, t=%llu ms\n",
               WSAGetLastError(), FD_ISSET(fd, &wfds) != 0, now_ms());
        /* Same socket, other wait primitive: if WSAPoll reports
           it writable, select itself is the broken step; if it
           also times out, the non-blocking connect never
           completes and the fault is below select. The revents
           bits are decoded with the Winsock values (not the
           BSD poll(2) numbering): POLLERR 0x0001, POLLHUP
           0x0002, POLLOUT 0x0010, POLLIN 0x0300, POLLPRI
           0x0400, POLLNVAL 0x0004 -- so 0x13 is
           POLLOUT|POLLERR|POLLHUP, a refused connect, not a
           readable one. */
        {
            WSAPOLLFD pfd;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            int prc = WSAPoll(&pfd, 1, 5000);
            printf("nb connect: WSAPoll on same socket: rc=%d revents=0x%x, "
                   "WSA error %d, t=%llu ms\n", prc, (unsigned)pfd.revents,
                   WSAGetLastError(), now_ms());
            printf("nb connect: WSAPoll revents 0x%x:",
                   (unsigned)pfd.revents);
            if (pfd.revents & POLLIN) printf(" POLLIN");
            if (pfd.revents & POLLPRI) printf(" POLLPRI");
            if (pfd.revents & POLLOUT) printf(" POLLOUT");
            if (pfd.revents & POLLERR) printf(" POLLERR");
            if (pfd.revents & POLLHUP) printf(" POLLHUP");
            if (pfd.revents & POLLNVAL) printf(" POLLNVAL");
            printf("\n");
        }
        fflush(stdout);
        /* The wait primitives disagree (select: nothing in 5 s;
           WSAPoll: events pending), so the pending error is the
           authority: it says whether the connect failed, and
           with what code, or is still in progress (SO_ERROR 0). */
        print_so_error(fd);
#else
        printf("nb connect: select timed out after 5s (not writable)\n");
        print_so_error(fd);
#endif
        sock_close(fd);
        return;
    }
    if (rc < 0) {
#ifdef _WIN32
        printf("nb connect: select failed, WSA error %d\n", WSAGetLastError());
#else
        printf("nb connect: select failed, errno %d (%s)\n",
               errno, strerror(errno));
#endif
        sock_close(fd);
        return;
    }
#ifdef _WIN32
    printf("nb connect: select rc=%d FD_ISSET=%d\n",
           rc, FD_ISSET(fd, &wfds) != 0);
    fflush(stdout);
#endif
    int soerr = 0;
#ifdef _WIN32
    int olen = sizeof(soerr);
#else
    socklen_t olen = sizeof(soerr);
#endif
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&soerr, &olen) != 0) {
#ifdef _WIN32
        printf("nb connect: getsockopt SO_ERROR failed, WSA error %d\n",
               WSAGetLastError());
#else
        printf("nb connect: getsockopt SO_ERROR failed, errno %d (%s)\n",
               errno, strerror(errno));
#endif
        sock_close(fd);
        return;
    }
    if (soerr != 0) {
#ifdef _WIN32
        printf("nb connect: writable but SO_ERROR=%d\n", soerr);
#else
        printf("nb connect: writable but SO_ERROR=%d (%s)\n",
               soerr, strerror(soerr));
#endif
    } else {
        printf("nb connect: ok, non-blocking connect + select + SO_ERROR clean\n");
    }
    sock_close(fd);
}

/* The ws/wss handlers are referenced only from the static
   protocols table in url.c, so a link that pulls archive
   members only on direct reference drops them and the ws
   scheme is reported unsupported even though libcurl.a
   contains them (seen on the mac-host link). Reference them
   directly so the archive pull sees them on every platform;
   the linker resolves the names, the object types do not
   matter here. */
extern const char Curl_handler_ws[];
extern const char Curl_handler_wss[];
static const void *tinyc_ws_handler_refs[] = {
    (const void *)Curl_handler_ws,
    (const void *)Curl_handler_wss,
};

/* Probe whether the ws scheme is recognised at all, before any
   server is involved: a closed port must fail with "couldn't
   connect" (7); "unsupported protocol" (1) means the protocol
   table lost the ws handler, which is a different defect from
   any network failure. */
static int check_ws_scheme(void) {
    CURL *c = curl_easy_init();
    if (!c) { printf("curl: init failed\n"); return 1; }
    curl_easy_setopt(c, CURLOPT_URL, "ws://127.0.0.1:1/");
    curl_easy_setopt(c, CURLOPT_CONNECT_ONLY, 2L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT_MS, 2000L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT_MS, 5000L);
    CURLcode rc = curl_easy_perform(c);
    curl_easy_cleanup(c);
    if (rc == CURLE_UNSUPPORTED_PROTOCOL) {
        printf("curl: ws scheme NOT supported (protocol table)\n");
        return 1;
    }
    printf("curl: ws scheme recognised (probe rc=%d)\n", (int)rc);
    return 0;
}

int main(int argc, char **argv) {
    base_port = argc > 1 ? atoi(argv[1]) : 18231;
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa)) { printf("WSAStartup failed\n"); return 1; }
#endif
    (void)tinyc_ws_handler_refs;
    if (check_ws_scheme()) return 1;
    /* The CA the HTTPS check verifies against, as a file for
       CURLOPT_CAINFO. */
    const char *cafile = "default_libs_ca.pem";
    FILE *cf = fopen(cafile, "wb");
    if (!cf) { perror("cafile"); return 1; }
    fwrite(TEST_CA_PEM, 1, strlen(TEST_CA_PEM), cf);
    fclose(cf);

    thread_t th_http, th_ws;
    thread_create(&th_http, http_server);
    thread_create(&th_ws, ws_server);
    if (!wait_ready(&ready_http, "http", base_port)) return 1;
    if (check_http()) { probe_raw_connect(); probe_nb_connect(); return 1; }
    if (!wait_ready(&ready_ws, "ws", base_port + 2)) return 1;
    if (check_ws()) return 1;
    thread_join(th_http);
    thread_join(th_ws);

    /* The HTTPS check connects twice (verified, then no-CA);
       the server thread serves both sequentially. */
    thread_t th_tls;
    thread_create(&th_tls, https_server);
    if (!wait_ready(&ready_tls, "tls", base_port + 1)) return 1;
    if (check_https(cafile)) return 1;
    thread_join(th_tls);
    remove(cafile);
    printf("default libs battery 2 ok\n");
    return 0;
}
