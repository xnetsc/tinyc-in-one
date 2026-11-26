/* dispatch/dispatch.h - Grand Central Dispatch semaphore surface, Darwin.
 *
 * Minimal subset written for this package from the libdispatch
 * interface (dispatch/semaphore.h, dispatch/object.h,
 * dispatch/time.h) as published in the Darwin manual pages.  Only
 * the semaphore surface the compiler's own build uses is declared:
 * tcc.h's TCCSem lock (libtcc) and bcheck.c's bounds-checking lock
 * (libtcc1) both use dispatch semaphores on Apple targets.  The
 * symbols resolve at run time through the system libSystem, which
 * re-exports libdispatch; the link stubs carry them via the
 * regenerated libc.tbd / libSystem.tbd.
 */

#ifndef _TINYC_DARWIN_DISPATCH_DISPATCH_H
#define _TINYC_DARWIN_DISPATCH_DISPATCH_H

#include <stdint.h>

typedef uint64_t dispatch_time_t;
typedef struct dispatch_object_s *dispatch_object_t;
typedef struct dispatch_semaphore_s *dispatch_semaphore_t;

#define DISPATCH_TIME_NOW	((dispatch_time_t)0)
#define DISPATCH_TIME_FOREVER	((dispatch_time_t)~0ull)

dispatch_semaphore_t dispatch_semaphore_create(long value);
long dispatch_semaphore_wait(dispatch_semaphore_t dsema, dispatch_time_t timeout);
long dispatch_semaphore_signal(dispatch_semaphore_t dsema);
void dispatch_release(dispatch_object_t object);

#endif
