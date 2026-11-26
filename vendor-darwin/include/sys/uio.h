/* sys/uio.h - scatter/gather input and output, Darwin.
 *
 * Written for this package from the POSIX <sys/uio.h>
 * requirements.  Darwin provides readv and writev; the preadv /
 * pwritev pair is not part of the platform surface and is not
 * declared.
 */

#ifndef _TINYC_DARWIN_SYS_UIO_H
#define _TINYC_DARWIN_SYS_UIO_H

#include <stddef.h>
#include <sys/types.h>

struct iovec {
	void *	iov_base;
	size_t	iov_len;
};

#define UIO_MAXIOV	1024
#define IOV_MAX		UIO_MAXIOV

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#endif
