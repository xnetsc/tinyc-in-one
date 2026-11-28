/* sys/time.h — struct timeval for the wasm32 sysroot.
 *
 * Types only. libcurl's headers name struct timeval (the multi
 * interface's timeout query); no time-of-day facility is declared
 * here, matching the sysroot's types-only time surface.
 */
#ifndef _TINYC_SYS_TIME_H
#define _TINYC_SYS_TIME_H

#include <sys/types.h>
#include <sys/select.h>

struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

#endif /* _TINYC_SYS_TIME_H */
