/* signal.h - signal handling, Darwin.
 *
 * Written for this package from the C99 <signal.h> requirements
 * plus the POSIX signal functions libSystem provides.  Darwin
 * facts: the signal numbers are the BSD ones (SIGBUS is 10,
 * SIGSYS is 12, SIGUSR1 is 30), sigset_t is a 32-bit mask,
 * struct sigaction wraps the handler in the __sigaction_u union
 * with sa_handler/sa_sigaction as member macros, and stack_t is
 * { sp, size, flags } in that order.
 */

#ifndef _TINYC_DARWIN_SIGNAL_H
#define _TINYC_DARWIN_SIGNAL_H

#include <stddef.h>
#include <sys/types.h>

typedef int sig_atomic_t;
typedef uint32_t sigset_t;

#define SIGHUP		1
#define SIGINT		2
#define SIGQUIT		3
#define SIGILL		4
#define SIGTRAP		5
#define SIGABRT		6
#define SIGEMT		7
#define SIGFPE		8
#define SIGKILL		9
#define SIGBUS		10
#define SIGSEGV		11
#define SIGSYS		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGURG		16
#define SIGSTOP		17
#define SIGTSTP		18
#define SIGCONT		19
#define SIGCHLD		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGIO		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGINFO		29
#define SIGUSR1		30
#define SIGUSR2		31
#define NSIG		32

#define SIG_DFL		((void (*)(int))0)
#define SIG_IGN		((void (*)(int))1)
#define SIG_ERR		((void (*)(int))-1)
#define SIG_HOLD	((void (*)(int))3)

union __sigaction_u {
	void (*__sa_handler)(int);
	void (*__sa_sigaction)(int, void *, void *);
};

struct sigaction {
	union __sigaction_u __sigaction_u;
	sigset_t sa_mask;
	int sa_flags;
};

#define sa_handler	__sigaction_u.__sa_handler
#define sa_sigaction	__sigaction_u.__sa_sigaction

#define SA_ONSTACK	0x0001
#define SA_RESTART	0x0002
#define SA_RESETHAND	0x0004
#define SA_NOCLDSTOP	0x0008
#define SA_NODEFER	0x0010
#define SA_NOCLDWAIT	0x0020
#define SA_SIGINFO	0x0040

#define SIG_BLOCK	1
#define SIG_UNBLOCK	2
#define SIG_SETMASK	3

typedef struct {
	void *ss_sp;
	size_t ss_size;
	int ss_flags;
} stack_t;

#define SS_ONSTACK	0x0001
#define SS_DISABLE	0x0004
#define MINSIGSTKSZ	32768
#define SIGSTKSZ	131072

void (*signal(int sig, void (*func)(int)))(int);
int raise(int sig);
int sigaction(int sig, const struct sigaction *act,
	      struct sigaction *oact);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);
int sigprocmask(int how, const sigset_t *set, sigset_t *oset);
int sigpending(sigset_t *set);
int sigsuspend(const sigset_t *sigmask);
int sigwait(const sigset_t *set, int *sig);
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
int kill(pid_t pid, int sig);
int killpg(pid_t pgrp, int sig);
int sigaltstack(const stack_t *ss, stack_t *oss);

#endif
