/* errno.h — error numbers for the wasm32 sysroot.
 *
 * errno is a macro over the shim layer's accessor function, as
 * the standard permits (errno expands to a modifiable lvalue of
 * type int; it need not be a variable). A data symbol cannot be
 * supplied by the shim layer across the import boundary, so the
 * number itself lives in a cell of the process's own memory: the
 * accessor allocates it on first use and returns its address.
 * Failed shim calls record their code there; guest code reads
 * and writes it exactly like a hosted errno, and perror reports
 * the same cell. Each thread of a threaded process calls the
 * accessor of its own shim set and gets its own cell, so the
 * value is per-thread as on a hosted system.
 *
 * The numbers are the Linux values, the numbering the shim
 * layer's own codes (ENOENT 2, EBADF 9, ENOMEM 12, EEXIST 17,
 * EINVAL 22) already follow. Every macro is guarded so headers
 * that publish a few of them first (pthread.h) coexist in any
 * include order.
 */
#ifndef _TINYC_ERRNO_H
#define _TINYC_ERRNO_H

int *__tinyc_errno(void);
#define errno (*__tinyc_errno())

#ifndef EPERM
#define EPERM		1
#endif
#ifndef ENOENT
#define ENOENT		2
#endif
#ifndef ESRCH
#define ESRCH		3
#endif
#ifndef EINTR
#define EINTR		4
#endif
#ifndef EIO
#define EIO		5
#endif
#ifndef ENXIO
#define ENXIO		6
#endif
#ifndef E2BIG
#define E2BIG		7
#endif
#ifndef ENOEXEC
#define ENOEXEC		8
#endif
#ifndef EBADF
#define EBADF		9
#endif
#ifndef ECHILD
#define ECHILD		10
#endif
#ifndef EAGAIN
#define EAGAIN		11
#endif
#ifndef ENOMEM
#define ENOMEM		12
#endif
#ifndef EACCES
#define EACCES		13
#endif
#ifndef EFAULT
#define EFAULT		14
#endif
#ifndef ENOTBLK
#define ENOTBLK		15
#endif
#ifndef EBUSY
#define EBUSY		16
#endif
#ifndef EEXIST
#define EEXIST		17
#endif
#ifndef EXDEV
#define EXDEV		18
#endif
#ifndef ENODEV
#define ENODEV		19
#endif
#ifndef ENOTDIR
#define ENOTDIR		20
#endif
#ifndef EISDIR
#define EISDIR		21
#endif
#ifndef EINVAL
#define EINVAL		22
#endif
#ifndef ENFILE
#define ENFILE		23
#endif
#ifndef EMFILE
#define EMFILE		24
#endif
#ifndef ENOTTY
#define ENOTTY		25
#endif
#ifndef ETXTBSY
#define ETXTBSY		26
#endif
#ifndef EFBIG
#define EFBIG		27
#endif
#ifndef ENOSPC
#define ENOSPC		28
#endif
#ifndef ESPIPE
#define ESPIPE		29
#endif
#ifndef EROFS
#define EROFS		30
#endif
#ifndef EMLINK
#define EMLINK		31
#endif
#ifndef EPIPE
#define EPIPE		32
#endif
#ifndef EDOM
#define EDOM		33
#endif
#ifndef ERANGE
#define ERANGE		34
#endif
#ifndef EDEADLK
#define EDEADLK		35
#endif
#ifndef ENAMETOOLONG
#define ENAMETOOLONG	36
#endif
#ifndef ENOLCK
#define ENOLCK		37
#endif
#ifndef ENOSYS
#define ENOSYS		38
#endif
#ifndef ENOTEMPTY
#define ENOTEMPTY	39
#endif
#ifndef ELOOP
#define ELOOP		40
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK	EAGAIN
#endif
#ifndef ENOMSG
#define ENOMSG		42
#endif
#ifndef EIDRM
#define EIDRM		43
#endif
#ifndef EPROTO
#define EPROTO		71
#endif
#ifndef EOVERFLOW
#define EOVERFLOW	75
#endif
#ifndef EILSEQ
#define EILSEQ		84
#endif
#ifndef ENOTSOCK
#define ENOTSOCK	88
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP	95
#endif
#ifndef ECONNRESET
#define ECONNRESET	104
#endif
#ifndef ETIMEDOUT
#define ETIMEDOUT	110
#endif

#endif /* _TINYC_ERRNO_H */
