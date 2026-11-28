/* sys/types.h — the POSIX base types for the wasm32 sysroot.
 *
 * Types only, no declarations: the fundamental typedefs other
 * shipped headers rely on (curl/curl.h includes this header
 * unconditionally for time_t; the socket and time headers below
 * build on it). size_t and ptrdiff_t come from <stddef.h>; the
 * typedef guards keep repeated definitions of time_t identical
 * across <time.h> and this header.
 */
#ifndef _TINYC_SYS_TYPES_H
#define _TINYC_SYS_TYPES_H

#include <stddef.h>

#ifndef _TINYC_TIME_T_DEFINED
#define _TINYC_TIME_T_DEFINED
typedef long time_t;
#endif

#ifndef _TINYC_CLOCK_T_DEFINED
#define _TINYC_CLOCK_T_DEFINED
typedef long clock_t;
#endif

typedef long off_t;
/* ssize_t comes from <stddef.h> in this tree (TCC's own header
 * defines it); it is not typedef'd here again. */
typedef int pid_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef unsigned int mode_t;
typedef long suseconds_t;
typedef unsigned long useconds_t;
typedef unsigned long dev_t;
typedef unsigned long ino_t;
typedef unsigned long nlink_t;
typedef long blksize_t;
typedef long blkcnt_t;

#endif /* _TINYC_SYS_TYPES_H */
