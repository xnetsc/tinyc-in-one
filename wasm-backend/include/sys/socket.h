/* sys/socket.h — socket address types for the wasm32 sysroot.
 *
 * Types only, and deliberately so: the browser environment gives
 * guest code no socket facility (the standing wasm32 exception),
 * so this header declares no functions and no constants a program
 * could mistake for a working socket layer. It exists because
 * libcurl's headers include it unconditionally and name the
 * address types (curl's own socket handle type is its own, from
 * curl/system.h).
 */
#ifndef _TINYC_SYS_SOCKET_H
#define _TINYC_SYS_SOCKET_H

#include <sys/types.h>

typedef unsigned int socklen_t;
typedef unsigned short sa_family_t;

struct sockaddr {
	sa_family_t sa_family;
	char sa_data[14];
};

struct sockaddr_storage {
	sa_family_t ss_family;
	char __ss_padding[126];
};

#endif /* _TINYC_SYS_SOCKET_H */
