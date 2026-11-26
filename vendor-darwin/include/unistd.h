/* unistd.h - POSIX operating system interface, Darwin.
 *
 * Written for this package from the POSIX <unistd.h>
 * requirements: the file, process and user/group functions
 * libSystem provides, with the Darwin access-mode bits and the
 * small set of sysconf names whose numbers are stable platform
 * facts.  environ is declared as on Darwin; a program that needs
 * it in a form the linker cannot resolve can use _NSGetEnviron()
 * instead (not declared here).
 */

#ifndef _TINYC_DARWIN_UNISTD_H
#define _TINYC_DARWIN_UNISTD_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/uio.h>

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

#define F_OK		0
#define X_OK		1
#define W_OK		2
#define R_OK		4

#define _POSIX_VERSION		200112L
#define _POSIX2_VERSION		200112L
#define _XOPEN_VERSION		600

/* sysconf names (Darwin numbering, the stable core set). */
#define _SC_ARG_MAX		1
#define _SC_CHILD_MAX		2
#define _SC_CLK_TCK		3
#define _SC_NGROUPS_MAX		4
#define _SC_OPEN_MAX		5
#define _SC_JOB_CONTROL		6
#define _SC_SAVED_IDS		7
#define _SC_VERSION		8
#define _SC_PAGESIZE		29
#define _SC_PAGE_SIZE		_SC_PAGESIZE
#define _SC_NPROCESSORS_CONF	14
#define _SC_NPROCESSORS_ONLN	15

/* confstr names. */
#define _CS_PATH		1

extern char **environ;
extern char *optarg;
extern int optind, opterr, optopt;

void _exit(int status);
int access(const char *path, int mode);
unsigned alarm(unsigned seconds);
int chdir(const char *path);
int fchdir(int fd);
int chown(const char *path, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group);
int close(int fd);
size_t confstr(int name, char *buf, size_t len);
int dup(int fd);
int dup2(int oldfd, int newfd);
int execl(const char *path, const char *arg, ...);
int execle(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execv(const char *path, char * const *argv);
int execve(const char *path, char * const *argv, char * const *envp);
int execvp(const char *file, char * const *argv);
pid_t fork(void);
long fpathconf(int fd, int name);
long pathconf(const char *path, int name);
char *getcwd(char *buf, size_t size);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int gidsetsize, gid_t grouplist[]);
char *getlogin(void);
int getlogin_r(char *name, size_t len);
pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
int isatty(int fd);
int link(const char *oldname, const char *newname);
int linkat(int fd1, const char *oldname, int fd2, const char *newname,
	   int flag);
off_t lseek(int fd, off_t offset, int whence);
int nice(int incr);
int pause(void);
int pipe(int fildes[2]);
ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset);
ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t readlink(const char *path, char *buf, size_t bufsiz);
ssize_t readlinkat(int fd, const char *path, char *buf, size_t bufsiz);
int rmdir(const char *path);
int setegid(gid_t gid);
int seteuid(uid_t uid);
int setgid(gid_t gid);
int setpgid(pid_t pid, pid_t pgrp);
pid_t setsid(void);
int setregid(gid_t rgid, gid_t egid);
int setreuid(uid_t ruid, uid_t euid);
int setuid(uid_t uid);
unsigned sleep(unsigned seconds);
int swab(const void *src, void *dst, ssize_t nbytes);
int symlink(const char *oldname, const char *newname);
int symlinkat(const char *oldname, int fd, const char *newname);
void sync(void);
long sysconf(int name);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);
int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);
char *ttyname(int fd);
int ttyname_r(int fd, char *buf, size_t len);
int unlink(const char *path);
int unlinkat(int fd, const char *path, int flag);
ssize_t write(int fd, const void *buf, size_t nbyte);
int usleep(useconds_t useconds);
int chroot(const char *path);
int fsync(int fd);
int fdatasync(int fd);
int gethostname(char *name, size_t len);
long gethostid(void);
int getopt(int argc, char * const *argv, const char *optstring);

struct option {
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

#define no_argument		0
#define required_argument	1
#define optional_argument	2

int getopt_long(int argc, char * const *argv, const char *optstring,
		const struct option *longopts, int *longindex);
int initgroups(const char *name, gid_t basegid);
int setgroups(int ngroups, const gid_t *gidset);
pid_t getpgid(pid_t pid);
pid_t getsid(pid_t pid);
int lockf(int fd, int cmd, off_t len);
char *getpass(const char *prompt);
int brk(const void *addr);
void *sbrk(intptr_t incr);

#endif
