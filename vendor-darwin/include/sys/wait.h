/* sys/wait.h - process waiting, Darwin.
 *
 * Written for this package from the POSIX <sys/wait.h>
 * requirements.  The status macros use the classic BSD encoding
 * Darwin keeps: the low 7 bits are the terminating signal (0 when
 * the process exited, 0177 when it stopped), the exit status is
 * the next byte up.
 */

#ifndef _TINYC_DARWIN_SYS_WAIT_H
#define _TINYC_DARWIN_SYS_WAIT_H

#include <stddef.h>
#include <sys/types.h>

struct rusage;

#define WIFEXITED(x)	(((x) & 0377) == 0)
#define WEXITSTATUS(x)	(((x) >> 8) & 0xff)
#define WIFSIGNALED(x)	(((x) & 0377) > 1 && ((x) & 0377) != 0177)
#define WTERMSIG(x)	((x) & 0177)
#define WIFSTOPPED(x)	(((x) & 0377) == 0177)
#define WSTOPSIG(x)	(((x) >> 8) & 0xff)
#define WIFCONTINUED(x)	((x) == 0x13)
#define WCOREFLAG	0200

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002
#define WCONTINUED	0x00000010
#define WSTOPPED	0x00000004
#define WEXITED		0x00000008
#define WNOWAIT		0x00000020

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
pid_t wait3(int *status, int options, struct rusage *rusage);
pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage);

#endif
