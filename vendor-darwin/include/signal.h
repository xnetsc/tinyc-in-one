/* signal.h - signal handling, Darwin.
 *
 * Written for this package from the C99 <signal.h> requirements
 * plus the POSIX signal functions libSystem provides.  Darwin
 * facts: the signal numbers are the BSD ones (SIGBUS is 10,
 * SIGSYS is 12, SIGUSR1 is 30), sigset_t is a 32-bit mask,
 * struct sigaction wraps the handler in the __sigaction_u union
 * with sa_handler/sa_sigaction as member macros, and stack_t is
 * { sp, size, flags } in that order.  siginfo_t is the 104-byte
 * LP64 form (signo, errno, code, pid, uid, status, addr,
 * value, band, padding) with the BSD si_code values.
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
#define SIG_HOLD	((void (*)(int))5)

union sigval {
	int sival_int;
	void *sival_ptr;
};

struct __siginfo {
	int si_signo;
	int si_errno;
	int si_code;
	pid_t si_pid;
	uid_t si_uid;
	int si_status;
	void *si_addr;
	union sigval si_value;
	long si_band;
	unsigned long __pad[7];
};
typedef struct __siginfo siginfo_t;

#define SI_USER		0x10001
#define SI_QUEUE	0x10002
#define SI_TIMER	0x10003
#define SI_ASYNCIO	0x10004
#define SI_MESGQ	0x10005

/* Codes for SIGILL */
#define ILL_ILLOPC	1
#define ILL_ILLTRP	2
#define ILL_PRVOPC	3
#define ILL_ILLOPN	4
#define ILL_ILLADR	5
#define ILL_PRVREG	6
#define ILL_COPROC	7
#define ILL_BADSTK	8

/* Codes for SIGFPE */
#define FPE_FLTDIV	1
#define FPE_FLTOVF	2
#define FPE_FLTUND	3
#define FPE_FLTRES	4
#define FPE_FLTINV	5
#define FPE_FLTSUB	6
#define FPE_INTDIV	7
#define FPE_INTOVF	8

/* Codes for SIGSEGV */
#define SEGV_MAPERR	1
#define SEGV_ACCERR	2

/* Codes for SIGBUS */
#define BUS_ADRALN	1
#define BUS_ADRERR	2
#define BUS_OBJERR	3

/* Codes for SIGTRAP */
#define TRAP_BRKPT	1
#define TRAP_TRACE	2

/* Codes for SIGCHLD */
#define CLD_EXITED	1
#define CLD_KILLED	2
#define CLD_DUMPED	3
#define CLD_TRAPPED	4
#define CLD_STOPPED	5
#define CLD_CONTINUED	6

/* Codes for SIGPOLL */
#define POLL_IN		1
#define POLL_OUT	2
#define POLL_MSG	3
#define POLL_ERR	4
#define POLL_PRI	5
#define POLL_HUP	6

union __sigaction_u {
	void (*__sa_handler)(int);
	void (*__sa_sigaction)(int, struct __siginfo *, void *);
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
