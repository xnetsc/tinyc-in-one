/* sys/resource.h - resource limits and usage, Darwin.
 *
 * Written for this package from the POSIX <sys/resource.h>
 * requirements.  Darwin facts: rlim_t is 64-bit, struct rusage
 * is two timevals followed by fourteen longs (144 bytes), and
 * the RLIMIT numbers are the Darwin ones.
 */

#ifndef _TINYC_DARWIN_SYS_RESOURCE_H
#define _TINYC_DARWIN_SYS_RESOURCE_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/time.h>

typedef uint64_t rlim_t;

struct rlimit {
	rlim_t rlim_cur;
	rlim_t rlim_max;
};

#define RLIM_INFINITY	(((rlim_t)1 << 63) - 1)
#define RLIM_SAVED_MAX	RLIM_INFINITY
#define RLIM_SAVED_CUR	RLIM_INFINITY

#define RLIMIT_CPU	0
#define RLIMIT_FSIZE	1
#define RLIMIT_DATA	2
#define RLIMIT_STACK	3
#define RLIMIT_CORE	4
#define RLIMIT_AS	5
#define RLIMIT_MEMLOCK	6
#define RLIMIT_NPROC	7
#define RLIMIT_NOFILE	8
#define RLIM_NLIMITS	9

#define PRIO_PROCESS	0
#define PRIO_PGRP	1
#define PRIO_USER	2
#define PRIO_MIN	(-20)
#define PRIO_MAX	20

struct rusage {
	struct timeval ru_utime;
	struct timeval ru_stime;
	long ru_maxrss;
	long ru_ixrss;
	long ru_idrss;
	long ru_isrss;
	long ru_minflt;
	long ru_majflt;
	long ru_nswap;
	long ru_inblock;
	long ru_oublock;
	long ru_msgsnd;
	long ru_msgrcv;
	long ru_nsignals;
	long ru_nvcsw;
	long ru_nivcsw;
};

#define RUSAGE_SELF	0
#define RUSAGE_CHILDREN	(-1)
#define RUSAGE_THREAD	1

int getrlimit(int resource, struct rlimit *rlp);
int setrlimit(int resource, const struct rlimit *rlp);
int getrusage(int who, struct rusage *rusage);
int getpriority(int which, id_t who);
int setpriority(int which, id_t who, int prio);

#endif
