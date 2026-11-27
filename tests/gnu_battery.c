/* Hosted battery for the linux-gnu targets: one translation
 * unit that exercises the bundled glibc header set's basic
 * facilities end to end (stdio file IO and the scanf family,
 * pthread, the socket endpoint API, and netdb name lookup),
 * so the matrix smoke test compiles it with each triple's
 * packaged compiler against the bundled headers and crt
 * objects, then runs the product: natively for the host
 * triple, under qemu-user for the other two.  Every section
 * prints "ok <name>" or "FAIL <name>"; the program exits with
 * the number of failed sections and prints "BATTERY OK" when
 * all pass. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static int fails;

#define SECTION(name, cond) do { \
    if (cond) { printf("ok %s\n", name); } \
    else { printf("FAIL %s\n", name); fails++; } \
} while (0)

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int counter;

static void *worker(void *arg)
{
    int i;
    (void)arg;
    for (i = 0; i < 25000; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(void)
{
    /* stdio: a file round trip through fopen/fprintf/fscanf */
    {
        char path[128];
        FILE *f;
        int v = 0, ok = 0;
        char word[16] = {0};
        snprintf(path, sizeof path, "/tmp/tinyc-gnu-battery-%ld.txt",
                 (long)getpid());
        f = fopen(path, "w");
        if (f) {
            fprintf(f, "answer %d\n", 42);
            fclose(f);
            f = fopen(path, "r");
            if (f) {
                ok = fscanf(f, "%15s %d", word, &v) == 2 &&
                     strcmp(word, "answer") == 0 && v == 42;
                fclose(f);
            }
            unlink(path);
        }
        SECTION("stdio.file", ok);
        SECTION("stdio.sscanf", sscanf("3 14", "%d %d", &v, &ok) == 2 &&
                v == 3 && ok == 14);
    }
    /* pthread: four workers on one mutex-protected counter */
    {
        pthread_t th[4];
        int i, made = 0;
        for (i = 0; i < 4; i++)
            if (pthread_create(&th[i], NULL, worker, NULL) == 0)
                made++;
        for (i = 0; i < made; i++)
            pthread_join(th[i], NULL);
        SECTION("pthread.counter", made == 4 && counter == 100000);
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
                    char buf[8] = {0};
                    ok = send(cs, "ping", 4, 0) == 4 &&
                         recv(as, buf, 4, MSG_WAITALL) == 4 &&
                         memcmp(buf, "ping", 4) == 0 &&
                         send(as, "pong", 4, 0) == 4 &&
                         recv(cs, buf, 4, MSG_WAITALL) == 4 &&
                         memcmp(buf, "pong", 4) == 0;
                    close(as);
                }
                close(cs);
            }
            close(ls);
        }
        SECTION("socket.loopback", ok);
    }
    /* netdb: numeric lookup, the localhost name, numeric reverse */
    {
        struct addrinfo hints, *res = NULL, *p;
        int ok_num = 0, ok_name = 0, ok_rev = 0;
        char host[NI_MAXHOST];
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
        {
            struct sockaddr_in a;
            memset(&a, 0, sizeof a);
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            a.sin_port = htons(80);
            ok_rev = getnameinfo((struct sockaddr *)&a, sizeof a,
                                 host, sizeof host, NULL, 0,
                                 NI_NUMERICHOST) == 0 &&
                     strcmp(host, "127.0.0.1") == 0;
        }
        SECTION("netdb.numeric", ok_num);
        SECTION("netdb.localhost", ok_name);
        SECTION("netdb.reverse", ok_rev);
    }
    if (fails == 0)
        printf("BATTERY OK\n");
    return fails;
}
