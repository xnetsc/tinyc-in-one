/* Hosted battery for the android targets: one translation unit
 * that pulls a representative facility from each part of the
 * bundled bionic header set (stdio, stdlib, string, math,
 * stdatomic, dlfcn, pthread, the socket endpoint API, netdb
 * lookup), so the matrix smoke test links it against the
 * bundled crt objects and link-time stubs and checks the
 * product's PT_INTERP and DT_NEEDED. The program itself runs
 * only on Android; here it is a link-level check. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdatomic.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static atomic_int counter;

static pthread_mutex_t plock = PTHREAD_MUTEX_INITIALIZER;
static int pshared;

static void *pworker(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < 25000; i++) {
        pthread_mutex_lock(&plock);
        pshared++;
        pthread_mutex_unlock(&plock);
    }
    return NULL;
}

int main(void)
{
    char buf[64];
    void *p;
    double v;
    void *h;

    strcpy(buf, "tinyc-android");
    if (strcmp(buf, "tinyc-android") != 0)
        return 1;
    p = malloc(128);
    if (!p)
        return 2;
    free(p);
    v = sqrt(144.0) + sin(0.0) + pow(2.0, 3.0);
    if (v < 19.9 || v > 20.1)
        return 3;
    atomic_fetch_add(&counter, 1);
    if (atomic_load(&counter) != 1)
        return 4;
    h = dlopen(NULL, RTLD_NOW);
    if (!h)
        return 5;
    snprintf(buf, sizeof buf, "%d", 42);
    printf("%s ok\n", buf);
    /* pthread: four workers on one mutex-protected counter */
    {
        pthread_t th[4];
        int i, made = 0;
        for (i = 0; i < 4; i++)
            if (pthread_create(&th[i], NULL, pworker, NULL) == 0)
                made++;
        for (i = 0; i < made; i++)
            pthread_join(th[i], NULL);
        if (made != 4 || pshared != 100000)
            return 6;
    }
    /* socket: a loopback stream pair, one exchange each way */
    {
        int ls = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in a;
        socklen_t alen = sizeof a;
        int ok = 0;
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = 0;
        if (ls >= 0 && bind(ls, (struct sockaddr *)&a, sizeof a) == 0 &&
            listen(ls, 4) == 0 &&
            getsockname(ls, (struct sockaddr *)&a, &alen) == 0) {
            int cs = socket(AF_INET, SOCK_STREAM, 0);
            if (cs >= 0 &&
                connect(cs, (struct sockaddr *)&a, sizeof a) == 0) {
                int as = accept(ls, NULL, NULL);
                if (as >= 0) {
                    char b[8] = {0};
                    ok = send(cs, "ping", 4, 0) == 4 &&
                         recv(as, b, 4, MSG_WAITALL) == 4 &&
                         memcmp(b, "ping", 4) == 0 &&
                         send(as, "pong", 4, 0) == 4 &&
                         recv(cs, b, 4, MSG_WAITALL) == 4 &&
                         memcmp(b, "pong", 4) == 0;
                    close(as);
                }
                close(cs);
            }
            close(ls);
        }
        if (!ok)
            return 7;
    }
    /* netdb: numeric lookup and the localhost name */
    {
        struct addrinfo hints, *res = NULL, *p;
        int ok_num = 0, ok_name = 0;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        if (getaddrinfo("127.0.0.1", "80", &hints, &res) == 0 && res) {
            struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
            ok_num = res->ai_family == AF_INET &&
                     sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK) &&
                     sin->sin_port == htons(80);
            freeaddrinfo(res);
            res = NULL;
        }
        if (!ok_num)
            return 8;
        if (getaddrinfo("localhost", NULL, &hints, &res) == 0 && res) {
            for (p = res; p; p = p->ai_next) {
                struct sockaddr_in *sin =
                    (struct sockaddr_in *)p->ai_addr;
                if (p->ai_family == AF_INET &&
                    sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
                    ok_name = 1;
            }
            freeaddrinfo(res);
            res = NULL;
        }
        if (!ok_name)
            return 9;
    }
    return 0;
}
