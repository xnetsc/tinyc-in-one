/* netinet/in.h - Internet address types and protocol constants,
 * Darwin.
 *
 * Written for this package from the POSIX <netinet/in.h>
 * requirements with the Darwin layouts: the address structures
 * carry a leading length byte, AF_INET6 is 30, and the IPv4
 * option numbers are the BSD assignments.  The macOS CI probe
 * compares the sizes, offsets and constant values against the
 * platform headers.
 */

#ifndef _TINYC_DARWIN_NETINET_IN_H
#define _TINYC_DARWIN_NETINET_IN_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef uint32_t	in_addr_t;
typedef uint16_t	in_port_t;

/* Protocol numbers. */
#define IPPROTO_IP	0
#define IPPROTO_ICMP	1
#define IPPROTO_IGMP	2
#define IPPROTO_GGP	3
#define IPPROTO_TCP	6
#define IPPROTO_PUP	12
#define IPPROTO_UDP	17
#define IPPROTO_IDP	22
#define IPPROTO_IPV6	41
#define IPPROTO_ROUTING	43
#define IPPROTO_FRAGMENT 44
#define IPPROTO_ESP	50
#define IPPROTO_AH	51
#define IPPROTO_ICMPV6	58
#define IPPROTO_NONE	59
#define IPPROTO_DSTOPTS	60
#define IPPROTO_ND	77
#define IPPROTO_RAW	255
#define IPPROTO_MAX	256

struct in_addr {
	in_addr_t	s_addr;
};

struct sockaddr_in {
	uint8_t		sin_len;
	sa_family_t	sin_family;
	in_port_t	sin_port;
	struct in_addr	sin_addr;
	char		sin_zero[8];
};

#define INADDR_ANY		((in_addr_t)0x00000000)
#define INADDR_BROADCAST	((in_addr_t)0xffffffff)
#define INADDR_NONE		((in_addr_t)0xffffffff)
#define INADDR_LOOPBACK		((in_addr_t)0x7f000001)
#define INADDR_UNSPEC_GROUP	((in_addr_t)0xe0000000)
#define INADDR_ALLHOSTS_GROUP	((in_addr_t)0xe0000001)
#define INADDR_ALLRTRS_GROUP	((in_addr_t)0xe0000002)
#define INADDR_MAX_LOCAL_GROUP	((in_addr_t)0xe00000ff)

#define INET_ADDRSTRLEN		16

/* IPv4 options at the IPPROTO_IP level. */
#define IP_OPTIONS	1
#define IP_HDRINCL	2
#define IP_TOS		3
#define IP_TTL		4
#define IP_RECVOPTS	5
#define IP_RECVRETOPTS	6
#define IP_RECVDSTADDR	7
#define IP_RETOPTS	8
#define IP_MULTICAST_IF	9
#define IP_MULTICAST_TTL 10
#define IP_MULTICAST_LOOP 11
#define IP_ADD_MEMBERSHIP 12
#define IP_DROP_MEMBERSHIP 13
#define IP_MULTICAST_VIF 14
#define IP_PORTRANGE	19
#define IP_RECVIF	20

struct ip_mreq {
	struct in_addr	imr_multiaddr;
	struct in_addr	imr_interface;
};

struct in6_addr {
	union {
		uint8_t		__u6_addr8[16];
		uint16_t	__u6_addr16[8];
		uint32_t	__u6_addr32[4];
	} __u6_addr;
};
#define s6_addr		__u6_addr.__u6_addr8
#define s6_addr16	__u6_addr.__u6_addr16
#define s6_addr32	__u6_addr.__u6_addr32

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

#define IN6ADDR_ANY_INIT \
	{ { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } }
#define IN6ADDR_LOOPBACK_INIT \
	{ { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } }

struct sockaddr_in6 {
	uint8_t		sin6_len;
	sa_family_t	sin6_family;
	in_port_t	sin6_port;
	uint32_t	sin6_flowinfo;
	struct in6_addr	sin6_addr;
	uint32_t	sin6_scope_id;
};

#define INET6_ADDRSTRLEN	46

/* IPv6 options at the IPPROTO_IPV6 level. */
#define IPV6_UNICAST_HOPS	4
#define IPV6_MULTICAST_IF	9
#define IPV6_MULTICAST_HOPS	10
#define IPV6_MULTICAST_LOOP	11
#define IPV6_JOIN_GROUP		12
#define IPV6_LEAVE_GROUP	13
#define IPV6_V6ONLY		27
#define IPV6_PORTRANGE		36
#define IPV6_RECVIF		51

struct ipv6_mreq {
	struct in6_addr	ipv6mr_multiaddr;
	unsigned int	ipv6mr_interface;
};

#endif
