/* sys/socket.h - the socket endpoint API, Darwin.
 *
 * Written for this package from the POSIX <sys/socket.h>
 * requirements with the Darwin values and layouts: the address
 * structures carry a length byte (sa_len, sin_len), the socket
 * option level is 0xffff and the option numbers are the BSD
 * assignments, and the message flags are the BSD bit positions.
 * The macOS CI probe compares the sizes, offsets and constant
 * values against the platform headers.
 */

#ifndef _TINYC_DARWIN_SYS_SOCKET_H
#define _TINYC_DARWIN_SYS_SOCKET_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/uio.h>

typedef unsigned char	sa_family_t;

struct sockaddr {
	uint8_t		sa_len;
	sa_family_t	sa_family;
	char		sa_data[14];
};

struct sockaddr_storage {
	uint8_t		ss_len;
	sa_family_t	ss_family;
	char		__ss_pad1[6];
	int64_t		__ss_align;
	char		__ss_pad2[112];
};

/* Address families. */
#define AF_UNSPEC	0
#define AF_UNIX		1
#define AF_LOCAL	1
#define AF_INET		2
#define AF_APPLETALK	16
#define AF_ROUTE	17
#define AF_LINK		18
#define AF_IPX		23
#define AF_NDRV		27
#define AF_INET6	30
#define PF_UNSPEC	AF_UNSPEC
#define PF_UNIX		AF_UNIX
#define PF_LOCAL	AF_LOCAL
#define PF_INET		AF_INET
#define PF_APPLETALK	AF_APPLETALK
#define PF_ROUTE	AF_ROUTE
#define PF_LINK		AF_LINK
#define PF_IPX		AF_IPX
#define PF_NDRV		AF_NDRV
#define PF_INET6	AF_INET6

/* Socket types. */
#define SOCK_STREAM	1
#define SOCK_DGRAM	2
#define SOCK_RAW	3
#define SOCK_RDM	4
#define SOCK_SEQPACKET	5

/* shutdown() directions. */
#define SHUT_RD		0
#define SHUT_WR		1
#define SHUT_RDWR	2

/* The option level for getsockopt/setsockopt, and the option
 * numbers at that level. */
#define SOL_SOCKET	0xffff
#define SO_DEBUG	0x0001
#define SO_ACCEPTCONN	0x0002
#define SO_REUSEADDR	0x0004
#define SO_KEEPALIVE	0x0008
#define SO_DONTROUTE	0x0010
#define SO_BROADCAST	0x0020
#define SO_USELOOPBACK	0x0040
#define SO_LINGER	0x1080
#define SO_OOBINLINE	0x0100
#define SO_REUSEPORT	0x0200
#define SO_SNDBUF	0x1001
#define SO_RCVBUF	0x1002
#define SO_SNDLOWAT	0x1003
#define SO_RCVLOWAT	0x1004
#define SO_SNDTIMEO	0x1005
#define SO_RCVTIMEO	0x1006
#define SO_ERROR	0x1007
#define SO_TYPE		0x1008
#define SO_NOSIGPIPE	0x1022

#define SOMAXCONN	128

struct linger {
	int	l_onoff;
	int	l_linger;
};

/* send/recv flags. */
#define MSG_OOB		0x1
#define MSG_PEEK	0x2
#define MSG_DONTROUTE	0x4
#define MSG_EOR		0x8
#define MSG_TRUNC	0x10
#define MSG_CTRUNC	0x20
#define MSG_WAITALL	0x40
#define MSG_DONTWAIT	0x80
#define MSG_EOF		0x100
#define MSG_WAITSTREAM	0x200
#define MSG_FLUSH	0x400
#define MSG_HOLD	0x800
#define MSG_SEND	0x1000
#define MSG_HAVEMORE	0x2000
#define MSG_RCVMORE	0x4000
#define MSG_COMPAT	0x8000
#define MSG_NOSIGNAL	0x80000

struct msghdr {
	void		*msg_name;
	socklen_t	msg_namelen;
	struct iovec	*msg_iov;
	int		msg_iovlen;
	void		*msg_control;
	socklen_t	msg_controllen;
	int		msg_flags;
};

struct cmsghdr {
	socklen_t	cmsg_len;
	int		cmsg_level;
	int		cmsg_type;
};

/* Ancillary data alignment and accessors: control data is
 * aligned to the machine word, the platform rule. */
#define _TINYC_CMSG_ALIGN(n) \
	(((n) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))
#define CMSG_DATA(cmsg) \
	((unsigned char *)(cmsg) + _TINYC_CMSG_ALIGN(sizeof(struct cmsghdr)))
#define CMSG_FIRSTHDR(mhdr) \
	((mhdr)->msg_controllen >= sizeof(struct cmsghdr) \
		? (struct cmsghdr *)(mhdr)->msg_control : (struct cmsghdr *)0)
#define CMSG_NXTHDR(mhdr, cmsg) \
	(((char *)(cmsg) + _TINYC_CMSG_ALIGN((cmsg)->cmsg_len) + \
	  sizeof(struct cmsghdr) > \
	  (char *)(mhdr)->msg_control + (mhdr)->msg_controllen) \
		? (struct cmsghdr *)0 \
		: (struct cmsghdr *)((char *)(cmsg) + \
			_TINYC_CMSG_ALIGN((cmsg)->cmsg_len)))
#define CMSG_SPACE(n) \
	(_TINYC_CMSG_ALIGN(sizeof(struct cmsghdr)) + _TINYC_CMSG_ALIGN(n))
#define CMSG_LEN(n) \
	(_TINYC_CMSG_ALIGN(sizeof(struct cmsghdr)) + (n))

#define SCM_RIGHTS	0x01
#define SCM_TIMESTAMP	0x02
#define SCM_CREDS	0x03

int accept(int fd, struct sockaddr *addr, socklen_t *len);
int bind(int fd, const struct sockaddr *addr, socklen_t len);
int connect(int fd, const struct sockaddr *addr, socklen_t len);
int getpeername(int fd, struct sockaddr *addr, socklen_t *len);
int getsockname(int fd, struct sockaddr *addr, socklen_t *len);
int getsockopt(int fd, int level, int name, void *value, socklen_t *len);
int listen(int fd, int backlog);
ssize_t recv(int fd, void *buf, size_t n, int flags);
ssize_t recvfrom(int fd, void *buf, size_t n, int flags,
	struct sockaddr *addr, socklen_t *len);
ssize_t recvmsg(int fd, struct msghdr *msg, int flags);
ssize_t send(int fd, const void *buf, size_t n, int flags);
ssize_t sendto(int fd, const void *buf, size_t n, int flags,
	const struct sockaddr *addr, socklen_t len);
ssize_t sendmsg(int fd, const struct msghdr *msg, int flags);
int setsockopt(int fd, int level, int name, const void *value, socklen_t len);
int shutdown(int fd, int how);
int socket(int domain, int type, int protocol);
int socketpair(int domain, int type, int protocol, int fds[2]);

#endif
