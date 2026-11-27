/* sched.h - scheduling parameters for the win32 target.
 *
 * Part of the pthread surface: the implementation lives in
 * win32/lib/pthread.c and ships inside libtcc1.a.  Windows has
 * no scheduler policies in the POSIX sense; the policy values
 * are accepted and reported, sched_yield gives up the rest of
 * the caller's time slice, and the priority bounds report the
 * Win32 thread-priority range. */
#ifndef _SCHED_H
#define _SCHED_H

#include <sys/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sched_param {
    int sched_priority;
};

#define SCHED_OTHER 0
#define SCHED_FIFO  1
#define SCHED_RR    2

int sched_yield(void);
int sched_get_priority_max(int policy);
int sched_get_priority_min(int policy);
int sched_rr_get_interval(pid_t pid, struct timespec *interval);

#ifdef __cplusplus
}
#endif

#endif /* _SCHED_H */
