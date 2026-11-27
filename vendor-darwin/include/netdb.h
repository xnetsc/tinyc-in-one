/* netdb.h - name and address lookup, Darwin.
 *
 * Written for this package from the POSIX <netdb.h>
 * requirements with the Darwin values: the getaddrinfo error
 * codes are the positive BSD assignments (glibc uses negative
 * values), and the flag bits are the Darwin positions.  The
 * macOS CI probe compares the structure layouts and constant
 * values against the platform headers.
 */

#ifndef _TINYC_DARWIN_NETDB_H
#define _TINYC_DARWIN_NETDB_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct hostent {
	char	*h_name;
	char	**h_aliases;
	int	h_addrtype;
	int	h_length;
	char	**h_addr_list;
};
#define h_addr h_addr_list[0]

struct netent {
	char	*n_name;
	char	**n_aliases;
	int	n_addrtype;
	uint32_t n_net;
};

struct servent {
	char	*s_name;
	char	**s_aliases;
	int	s_port;
	char	*s_proto;
};

struct protoent {
	char	*p_name;
	char	**p_aliases;
	int	p_proto;
};

struct addrinfo {
	int		ai_flags;
	int		ai_family;
	int		ai_socktype;
	int		ai_protocol;
	socklen_t	ai_addrlen;
	char		*ai_canonname;
	struct sockaddr	*ai_addr;
	struct addrinfo	*ai_next;
};

/* getaddrinfo hints flags. */
#define AI_PASSIVE	0x00000001
#define AI_CANONNAME	0x00000002
#define AI_NUMERICHOST	0x00000004
#define AI_NUMERICSERV	0x00001000
#define AI_ALL		0x00000100
#define AI_V4MAPPED_CFG 0x00000200
#define AI_ADDRCONFIG	0x00000400
#define AI_V4MAPPED	0x00000800

/* getaddrinfo error codes: the Darwin assignments, in numeric
 * order (EAI_ADDRFAMILY 1 .. EAI_SYSTEM 11, EAI_OVERFLOW 14).
 * An earlier draft had these in the wrong order (1, 2, 3 for
 * BADFLAGS, NONAME, AGAIN); the macOS CI probe settled them
 * against the platform headers. */
#define EAI_ADDRFAMILY	1
#define EAI_AGAIN	2
#define EAI_BADFLAGS	3
#define EAI_FAIL	4
#define EAI_FAMILY	5
#define EAI_MEMORY	6
#define EAI_NODATA	7
#define EAI_NONAME	8
#define EAI_SERVICE	9
#define EAI_SOCKTYPE	10
#define EAI_SYSTEM	11
#define EAI_OVERFLOW	14

/* getnameinfo flags and buffer sizes. */
#define NI_MAXHOST	1025
#define NI_MAXSERV	32
#define NI_NOFQDN	0x00000001
#define NI_NUMERICHOST	0x00000002
#define NI_NAMEREQD	0x00000004
#define NI_NUMERICSERV	0x00000008
#define NI_DGRAM	0x00000010
#define NI_WITHSCOPEID	0x00000020
#define NI_NUMERICSCOPE	0x00000040

/* Database lookup result codes (h_errno values). */
#define NETDB_INTERNAL	-1
#define NETDB_SUCCESS	0
#define HOST_NOT_FOUND	1
#define TRY_AGAIN	2
#define NO_RECOVERY	3
#define NO_DATA		4
#define NO_ADDRESS	NO_DATA

int getaddrinfo(const char *node, const char *service,
	const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
	char *host, socklen_t hostlen, char *serv, socklen_t servlen,
	int flags);
const char *gai_strerror(int errcode);

struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyaddr(const void *addr, socklen_t len, int type);
struct hostent *gethostent(void);
void sethostent(int stayopen);
void endhostent(void);
struct netent *getnetbyname(const char *name);
struct netent *getnetbyaddr(uint32_t net, int type);
struct netent *getnetent(void);
void setnetent(int stayopen);
void endnetent(void);
struct servent *getservbyname(const char *name, const char *proto);
struct servent *getservbyport(int port, const char *proto);
struct servent *getservent(void);
void setservent(int stayopen);
void endservent(void);
struct protoent *getprotobyname(const char *name);
struct protoent *getprotobynumber(int proto);
struct protoent *getprotoent(void);
void setprotoent(int stayopen);
void endprotoent(void);
const char *hstrerror(int err);

#endif
