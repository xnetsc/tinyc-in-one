/* time.h — the time types for the wasm32 sysroot, plus the
 * one time function the shim layer supplies.
 *
 * The header carries the types time_t, clock_t, struct tm and
 * struct timespec, and declares the four time functions the
 * shim layer supplies: time, clock, difftime and timespec_get.
 * timespec_get reads the host wall clock (Date.now), which is
 * also the frame the pthread layer's CLOCK_REALTIME abstimes
 * use, so a program can form the abstimes the timed waits
 * take.  The calendar-conversion functions (gmtime, localtime,
 * mktime, strftime, asctime, ctime) are not supplied by the
 * layer and are not declared: a header here declares nothing
 * the runtime does not supply.
 * The header exists also because other shipped headers name
 * these types (curl/curl.h includes <time.h> unconditionally;
 * mbedtls/platform_time.h likewise), and because struct
 * timespec is shared with <pthread.h> under the same guard
 * that header already uses.
 */
#ifndef _TINYC_TIME_H
#define _TINYC_TIME_H

#include <stddef.h>

#ifndef _TINYC_TIME_T_DEFINED
#define _TINYC_TIME_T_DEFINED
typedef long time_t;
#endif

#ifndef _TINYC_CLOCK_T_DEFINED
#define _TINYC_CLOCK_T_DEFINED
typedef long clock_t;
#endif

#ifndef _TINYC_TIMESPEC_DEFINED
#define _TINYC_TIMESPEC_DEFINED
struct timespec {
	long tv_sec;
	long tv_nsec;
};
#endif

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

#define CLOCKS_PER_SEC 1000000

#define TIME_UTC	1

time_t time(time_t *timer);
clock_t clock(void);
double difftime(time_t time1, time_t time0);
int timespec_get(struct timespec *ts, int base);

#endif /* _TINYC_TIME_H */
