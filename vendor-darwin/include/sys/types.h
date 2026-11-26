/* sys/types.h - basic system types for the bundled Darwin headers.
 *
 * Written for this package from the POSIX type requirements and
 * the Darwin LP64 ABI (the widths the kernel and libSystem use on
 * both arm64 and x86_64): dev_t and blksize_t are 32-bit, ino_t is
 * 64-bit, mode_t and nlink_t are 16-bit, off_t and blkcnt_t are
 * 64-bit, time_t is 64-bit, suseconds_t is 32-bit.  This header is
 * the single place these typedefs live; the other bundled headers
 * include it instead of repeating them, so no name is ever
 * typedef'd twice in one translation unit.
 */

#ifndef _TINYC_DARWIN_SYS_TYPES_H
#define _TINYC_DARWIN_SYS_TYPES_H

#include <stddef.h>
#include <stdint.h>

typedef int32_t		dev_t;
typedef uint64_t	ino_t;
typedef uint16_t	mode_t;
typedef uint16_t	nlink_t;
typedef int32_t		pid_t;
typedef uint32_t	uid_t;
typedef uint32_t	gid_t;
typedef uint32_t	id_t;
typedef int32_t		key_t;
typedef int64_t		off_t;
typedef int64_t		blkcnt_t;
typedef int32_t		blksize_t;
typedef int64_t		time_t;
typedef uint32_t	useconds_t;
typedef int32_t		suseconds_t;
typedef unsigned long	clock_t;
typedef long		ssize_t;

/* BSD spellings of the fixed-width types. */
typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;
typedef uint8_t		u_int8_t;
typedef uint16_t	u_int16_t;
typedef uint32_t	u_int32_t;
typedef uint64_t	u_int64_t;
typedef int64_t		quad_t;
typedef uint64_t	u_quad_t;
typedef char *		caddr_t;

/* fd_set for select(): 1024 bits as 32 int32 words, the Darwin
 * layout.  The macros follow the standard bit-array definitions. */
#define FD_SETSIZE	1024

typedef struct fd_set {
	int32_t fds_bits[32];
} fd_set;

#define FD_SET(n, p)	((p)->fds_bits[(n) >> 5] |= (int32_t)(1 << ((n) & 31)))
#define FD_CLR(n, p)	((p)->fds_bits[(n) >> 5] &= (int32_t)~(1 << ((n) & 31)))
#define FD_ISSET(n, p)	(((p)->fds_bits[(n) >> 5] & (int32_t)(1 << ((n) & 31))) != 0)
#define FD_ZERO(p)	do { \
		fd_set *_p = (p); \
		int _i; \
		for (_i = 0; _i < 32; _i++) _p->fds_bits[_i] = 0; \
	} while (0)

#endif
