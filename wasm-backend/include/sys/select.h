/* sys/select.h — fd_set and its operations for the wasm32 sysroot.
 *
 * Types and macros only. libcurl's headers name fd_set (the multi
 * interface's fdset query); there is no select() facility behind
 * this in the browser environment, so the header provides the
 * data type and the standard bit operations and nothing else.
 */
#ifndef _TINYC_SYS_SELECT_H
#define _TINYC_SYS_SELECT_H

#include <sys/types.h>
#include <time.h>

#define FD_SETSIZE 1024

typedef struct {
	long __fds_bits[FD_SETSIZE / (8 * (int)sizeof(long))];
} fd_set;

#define __FD_BIT(fd) ((fd) / (8 * (int)sizeof(long)))
#define __FD_MASK(fd) (1L << ((fd) % (8 * (int)sizeof(long))))

#define FD_SET(fd, set) \
	((void)((set)->__fds_bits[__FD_BIT(fd)] |= __FD_MASK(fd)))
#define FD_CLR(fd, set) \
	((void)((set)->__fds_bits[__FD_BIT(fd)] &= ~__FD_MASK(fd)))
#define FD_ISSET(fd, set) \
	(((set)->__fds_bits[__FD_BIT(fd)] & __FD_MASK(fd)) != 0)
#define FD_ZERO(set) \
	do { \
		int __i; \
		for (__i = 0; __i < (int)(sizeof(fd_set) / sizeof(long)); __i++) \
			(set)->__fds_bits[__i] = 0; \
	} while (0)

#endif /* _TINYC_SYS_SELECT_H */
