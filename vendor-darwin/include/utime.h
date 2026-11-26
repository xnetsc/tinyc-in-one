/* utime.h - file times, Darwin.
 *
 * Written for this package from the POSIX <utime.h> requirements.
 */

#ifndef _TINYC_DARWIN_UTIME_H
#define _TINYC_DARWIN_UTIME_H

#include <sys/types.h>

struct utimbuf {
	time_t actime;
	time_t modtime;
};

int utime(const char *path, const struct utimbuf *times);

#endif
