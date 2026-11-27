/* Windows feature smoke: name lookup and a stream exchange
 * over the Winsock surface the win32 target ships
 * (winsock2.h / ws2tcpip.h, linked against ws2_32), compiled
 * with the packaged compiler and run on the Windows CI
 * runner. getaddrinfo is exercised on a numeric address and
 * on the localhost name; success prints FEATURE-SOCKET-OK
 * and returns 0, each failure a distinct exit code. */
#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

int main(void)
{
    WSADATA wsa;
    struct addrinfo hints, *res = NULL, *p;
    int ok_num = 0, ok_name = 0;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;
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
        return 2;
    if (getaddrinfo("localhost", NULL, &hints, &res) == 0 && res) {
        for (p = res; p; p = p->ai_next) {
            struct sockaddr_in *sin = (struct sockaddr_in *)p->ai_addr;
            if (p->ai_family == AF_INET &&
                sin->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
                ok_name = 1;
        }
        freeaddrinfo(res);
        res = NULL;
    }
    if (!ok_name)
        return 3;
    {
        SOCKET ls = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in a;
        int alen = sizeof a;
        int ok = 0;
        memset(&a, 0, sizeof a);
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = 0;
        if (ls != INVALID_SOCKET &&
            bind(ls, (struct sockaddr *)&a, sizeof a) == 0 &&
            listen(ls, 4) == 0 &&
            getsockname(ls, (struct sockaddr *)&a, &alen) == 0) {
            SOCKET cs = socket(AF_INET, SOCK_STREAM, 0);
            if (cs != INVALID_SOCKET &&
                connect(cs, (struct sockaddr *)&a, sizeof a) == 0) {
                SOCKET as = accept(ls, NULL, NULL);
                if (as != INVALID_SOCKET) {
                    char buf[8] = {0};
                    ok = send(cs, "ping", 4, 0) == 4 &&
                         recv(as, buf, 4, 0) == 4 &&
                         memcmp(buf, "ping", 4) == 0 &&
                         send(as, "pong", 4, 0) == 4 &&
                         recv(cs, buf, 4, 0) == 4 &&
                         memcmp(buf, "pong", 4) == 0;
                    closesocket(as);
                }
                closesocket(cs);
            }
            closesocket(ls);
        }
        if (!ok)
            return 4;
    }
    WSACleanup();
    printf("FEATURE-SOCKET-OK\n");
    return 0;
}
