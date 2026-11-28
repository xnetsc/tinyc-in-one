/* time.h — the time types for the wasm32 sysroot.
 *
 * This header carries types only: time_t, clock_t, struct tm and
 * struct timespec. It declares no functions — the sysroot's shim
 * layer supplies no time calls, and a header here declares nothing
 * the runtime does not supply. The header exists because other
 * shipped headers name these types (curl/curl.h includes <time.h>
 * unconditionally; mbedtls/platform_time.h likewise), and because
 * struct timespec is shared with <pthread.h> under the same guard
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

#endif /* _TINYC_TIME_H */
