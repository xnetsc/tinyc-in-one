/* arpa/inet.h - Internet address conversion, Darwin.
 *
 * Written for this package from the POSIX <arpa/inet.h>
 * requirements: the numeric conversion functions and the
 * byte-order functions, with the types from <netinet/in.h>.
 */

#ifndef _TINYC_DARWIN_ARPA_INET_H
#define _TINYC_DARWIN_ARPA_INET_H

#include <sys/types.h>
#include <netinet/in.h>

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

in_addr_t inet_addr(const char *cp);
int inet_aton(const char *cp, struct in_addr *addr);
char *inet_ntoa(struct in_addr addr);
in_addr_t inet_network(const char *cp);
struct in_addr inet_makeaddr(in_addr_t net, in_addr_t host);
in_addr_t inet_lnaof(struct in_addr addr);
in_addr_t inet_netof(struct in_addr addr);
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
int inet_pton(int af, const char *src, void *dst);

#endif
