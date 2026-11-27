/* netinet/tcp.h - TCP socket options, Darwin.
 *
 * Written for this package from the platform's <netinet/tcp.h>:
 * the option numbers are the Darwin assignments (TCP_KEEPALIVE
 * is the idle-time option; there is no separate TCP_KEEPIDLE).
 * The macOS CI probe compares the values against the platform
 * headers.
 */

#ifndef _TINYC_DARWIN_NETINET_TCP_H
#define _TINYC_DARWIN_NETINET_TCP_H

#include <sys/types.h>

#define TCP_NODELAY		0x01
#define TCP_MAXSEG		0x02
#define TCP_NOPUSH		0x04
#define TCP_NOOPT		0x08
#define TCP_KEEPALIVE		0x10
#define TCP_CONNECTIONTIMEOUT	0x20
#define TCP_RXT_CONNDROPTIME	0x80
#define TCP_RXT_FINDROP		0x0100
#define TCP_KEEPINTVL		0x101
#define TCP_KEEPCNT		0x102
#define TCP_FASTOPEN		0x105

#define TCPOLEN_MAXSEG		4
#define TCPOLEN_WINDOW		3
#define TCPOLEN_TIMESTAMP	10

#endif
