/* sys/stat.h - file status, Darwin.
 *
 * Written for this package from the POSIX <sys/stat.h>
 * requirements with the Darwin LP64 struct stat layout, which is
 * the same on arm64 and x86_64: 32-bit dev_t, 16-bit mode and
 * link count, 64-bit inode, the four timespecs (access, modify,
 * change, birth) at offsets 32/48/64/80, size at 96, blocks at
 * 104, for a total of 144 bytes.  This is the layout the kernel
 * and libSystem have used since the 64-bit-inode transition; the
 * macOS CI probe compares these offsets against the platform
 * headers field by field.
 */

#ifndef _TINYC_DARWIN_SYS_STAT_H
#define _TINYC_DARWIN_SYS_STAT_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

struct stat {
	dev_t		st_dev;
	mode_t		st_mode;
	nlink_t		st_nlink;
	ino_t		st_ino;
	uid_t		st_uid;
	gid_t		st_gid;
	dev_t		st_rdev;
	struct timespec	st_atimespec;
	struct timespec	st_mtimespec;
	struct timespec	st_ctimespec;
	struct timespec	st_birthtimespec;
	off_t		st_size;
	blkcnt_t	st_blocks;
	blksize_t	st_blksize;
	uint32_t	st_flags;
	uint32_t	st_gen;
	int32_t		st_lspare;
	int64_t		st_qspare[2];
};

#define st_atime	st_atimespec.tv_sec
#define st_mtime	st_mtimespec.tv_sec
#define st_ctime	st_ctimespec.tv_sec
#define st_birthtime	st_birthtimespec.tv_sec

#define S_IFMT		0170000
#define S_IFIFO		0010000
#define S_IFCHR		0020000
#define S_IFDIR		0040000
#define S_IFBLK		0060000
#define S_IFREG		0100000
#define S_IFLNK		0120000
#define S_IFSOCK	0140000
#define S_IFWHT		0160000

#define S_ISUID		0004000
#define S_ISGID		0002000
#define S_ISVTX		0001000
#define S_IRWXU		0000700
#define S_IRUSR		0000400
#define S_IWUSR		0000200
#define S_IXUSR		0000100
#define S_IRWXG		0000070
#define S_IRGRP		0000040
#define S_IWGRP		0000020
#define S_IXGRP		0000010
#define S_IRWXO		0000007
#define S_IROTH		0000004
#define S_IWOTH		0000002
#define S_IXOTH		0000001

#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define UTIME_NOW	((1 << 30) - 1)
#define UTIME_OMIT	((1 << 30) - 2)

int stat(const char *path, struct stat *sb) __DARWIN_INODE64(stat);
int fstat(int fd, struct stat *sb) __DARWIN_INODE64(fstat);
int lstat(const char *path, struct stat *sb) __DARWIN_INODE64(lstat);
int chmod(const char *path, mode_t mode);
int fchmod(int fd, mode_t mode);
int lchmod(const char *path, mode_t mode);
mode_t umask(mode_t mask);
int mkdir(const char *path, mode_t mode);
int mkfifo(const char *path, mode_t mode);
int mknod(const char *path, mode_t mode, dev_t dev);
int fstatat(int fd, const char *path, struct stat *sb, int flag) __DARWIN_INODE64(fstatat);
int fchmodat(int fd, const char *path, mode_t mode, int flag);
int mkdirat(int fd, const char *path, mode_t mode);
int mkfifoat(int fd, const char *path, mode_t mode);
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2],
	      int flag);

#endif
