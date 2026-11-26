/* time.h - date and time, Darwin.
 *
 * Written for this package from the C99 <time.h> requirements
 * plus the POSIX clock and _r functions libSystem provides.
 * Darwin facts: struct tm carries the BSD tm_gmtoff and tm_zone
 * tail fields, clock_t is unsigned long, CLOCKS_PER_SEC is
 * 1000000, the clockid values are the Darwin ones (CLOCK_MONOTONIC
 * is 6), and "timezone" is a legacy function, not a variable.
 */

#ifndef _TINYC_DARWIN_TIME_H
#define _TINYC_DARWIN_TIME_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define CLOCKS_PER_SEC	1000000

typedef int clockid_t;

#define CLOCK_REALTIME			0
#define CLOCK_MONOTONIC_RAW		4
#define CLOCK_MONOTONIC_RAW_APPROX	5
#define CLOCK_MONOTONIC			6
#define CLOCK_UPTIME_RAW		8
#define CLOCK_UPTIME_RAW_APPROX		9
#define CLOCK_PROCESS_CPUTIME_ID	12
#define CLOCK_THREAD_CPUTIME_ID		16

#define TIME_UTC	1

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long tm_gmtoff;
	const char *tm_zone;
};

extern char *tzname[2];

clock_t clock(void);
double difftime(time_t time1, time_t time0);
time_t mktime(struct tm *timeptr);
time_t time(time_t *timer);
char *asctime(const struct tm *timeptr);
char *ctime(const time_t *timer);
struct tm *gmtime(const time_t *timer);
struct tm *localtime(const time_t *timer);
size_t strftime(char *s, size_t maxsize, const char *format,
		const struct tm *timeptr);

char *asctime_r(const struct tm *timeptr, char *buf);
char *ctime_r(const time_t *timer, char *buf);
struct tm *gmtime_r(const time_t *timer, struct tm *result);
struct tm *localtime_r(const time_t *timer, struct tm *result);

void tzset(void);
char *timezone(int zone, int dst);
int clock_getres(clockid_t clock_id, struct timespec *res);
int clock_gettime(clockid_t clock_id, struct timespec *tp);
uint64_t clock_gettime_nsec_np(clockid_t clock_id);
int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
int timespec_get(struct timespec *ts, int base);
time_t timegm(struct tm *timeptr);

#endif
