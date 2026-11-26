/* sched.h - execution scheduling, Darwin.
 *
 * Written for this package from the POSIX <sched.h>
 * requirements.  Darwin policy numbers: SCHED_OTHER is 1,
 * SCHED_FIFO is 4, SCHED_RR is 2 (not the glibc 1/2).
 */

#ifndef _TINYC_DARWIN_SCHED_H
#define _TINYC_DARWIN_SCHED_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

struct sched_param {
	int sched_priority;
	char __reserved[4];
};

#define SCHED_OTHER	1
#define SCHED_FIFO	4
#define SCHED_RR	2

int sched_yield(void);
int sched_get_priority_max(int policy);
int sched_get_priority_min(int policy);
int sched_getparam(pid_t pid, struct sched_param *param);
int sched_setparam(pid_t pid, const struct sched_param *param);
int sched_getscheduler(pid_t pid);
int sched_setscheduler(pid_t pid, int policy,
		       const struct sched_param *param);
int sched_rr_get_interval(pid_t pid, struct timespec *interval);

#endif
