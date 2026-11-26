/* fcntl.h - file control, Darwin.
 *
 * Written for this package from the POSIX <fcntl.h> requirements
 * with the Darwin flag values, which differ from Linux at most
 * positions (O_CREAT is 0x0200, O_APPEND is 0x0008, O_CLOEXEC is
 * 0x1000000).  struct flock is the Darwin layout: two 64-bit
 * offsets, the owner pid, then the two 16-bit type/whence fields.
 */

#ifndef _TINYC_DARWIN_FCNTL_H
#define _TINYC_DARWIN_FCNTL_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>

#define O_RDONLY	0x0000
#define O_WRONLY	0x0001
#define O_RDWR		0x0002
#define O_ACCMODE	0x0003
#define O_NONBLOCK	0x0004
#define O_APPEND	0x0008
#define O_SHLOCK	0x0010
#define O_EXLOCK	0x0020
#define O_ASYNC		0x0040
#define O_FSYNC		0x0080
#define O_SYNC		O_FSYNC
#define O_NOFOLLOW	0x0100
#define O_CREAT		0x0200
#define O_TRUNC		0x0400
#define O_EXCL		0x0800
#define O_EVTONLY	0x8000
#define O_NOCTTY	0x20000
#define O_DIRECTORY	0x100000
#define O_SYMLINK	0x200000
#define O_CLOEXEC	0x1000000
#define O_DSYNC		0x4000000

#define F_DUPFD		0
#define F_GETFD		1
#define F_SETFD		2
#define F_GETFL		3
#define F_SETFL		4
#define F_GETOWN	5
#define F_SETOWN	6
#define F_GETLK		7
#define F_SETLK		8
#define F_SETLKW	9
#define F_SETLKWTIMEOUT	10
#define F_FLUSH_DATA	40
#define F_CHKCLEAN	41
#define F_PREALLOCATE	42
#define F_SETSIZE		43
#define F_RDADVISE		44
#define F_RDAHEAD		45
#define F_READBOOTSTRAP	46
#define F_WRITEBOOTSTRAP	47
#define F_NOCACHE		48
#define F_LOG2PHYS		49
#define F_GETPATH		50
#define F_FULLFSYNC		51
#define F_PATHPKG_CHECK	52
#define F_FREEZE_FS		53
#define F_THAW_FS		54
#define F_GLOBAL_NOCACHE	55
#define F_ADD_SEALS		56
#define F_GET_SEALS		57
#define F_DUPFD_CLOEXEC	67

#define FD_CLOEXEC	1

#define F_RDLCK		1
#define F_UNLCK		2
#define F_WRLCK		3

struct flock {
	off_t l_start;
	off_t l_len;
	pid_t l_pid;
	short l_type;
	short l_whence;
};

#define AT_FDCWD	(-2)
#define AT_EACCESS		0x0010
#define AT_SYMLINK_NOFOLLOW	0x0020
#define AT_SYMLINK_FOLLOW	0x0040
#define AT_REMOVEDIR		0x0080

#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

int open(const char *path, int oflag, ...);
int openat(int fd, const char *path, int oflag, ...);
int creat(const char *path, mode_t mode);
int fcntl(int fd, int cmd, ...);
int lockf(int fd, int cmd, off_t len);

#endif
