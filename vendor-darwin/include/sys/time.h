/* sys/time.h - time types and interval timers, Darwin.
 *
 * Written for this package from the POSIX <sys/time.h>
 * requirements.  Darwin facts: struct timeval is a 64-bit
 * time_t plus a 32-bit suseconds_t (16 bytes with padding), and
 * select() is declared here with the fd_set from <sys/types.h>.
 */

#ifndef _TINYC_DARWIN_SYS_TIME_H
#define _TINYC_DARWIN_SYS_TIME_H

#include <stddef.h>
#include <sys/types.h>
#include <time.h>

struct timeval {
	time_t		tv_sec;
	suseconds_t	tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

#define ITIMER_REAL	0
#define ITIMER_VIRTUAL	1
#define ITIMER_PROF	2

struct itimerval {
	struct timeval it_interval;
	struct timeval it_value;
};

int gettimeofday(struct timeval *tp, void *tzp);
int settimeofday(const struct timeval *tp, const struct timezone *tzp);
int getitimer(int which, struct itimerval *value);
int setitimer(int which, const struct itimerval *value,
	      struct itimerval *ovalue);
int utimes(const char *path, const struct timeval times[2]);
int lutimes(const char *path, const struct timeval times[2]);
int futimes(int fd, const struct timeval times[2]);
int adjtime(const struct timeval *delta, struct timeval *olddelta);
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds,
	   struct timeval *timeout);

/* The BSD convenience macros, in their standard form. */
#define timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define timerclear(tvp)		do { \
		(tvp)->tv_sec = 0; \
		(tvp)->tv_usec = 0; \
	} while (0)
#define timercmp(tvp, uvp, cmp) \
	(((tvp)->tv_sec == (uvp)->tv_sec) \
	 ? ((tvp)->tv_usec cmp (uvp)->tv_usec) \
	 : ((tvp)->tv_sec cmp (uvp)->tv_sec))
#define timeradd(tvp, uvp, vvp) do { \
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec; \
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec; \
		if ((vvp)->tv_usec >= 1000000) { \
			(vvp)->tv_sec++; \
			(vvp)->tv_usec -= 1000000; \
		} \
	} while (0)
#define timersub(tvp, uvp, vvp) do { \
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec; \
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec; \
		if ((vvp)->tv_usec < 0) { \
			(vvp)->tv_sec--; \
			(vvp)->tv_usec += 1000000; \
		} \
	} while (0)

#endif
