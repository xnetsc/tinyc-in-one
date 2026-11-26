/* libgen.h - filename components, Darwin.
 *
 * Written for this package from the POSIX <libgen.h>
 * requirements.  The Darwin prototypes take const char * (the
 * platform implementations do not modify their argument).
 */

#ifndef _TINYC_DARWIN_LIBGEN_H
#define _TINYC_DARWIN_LIBGEN_H

char *basename(const char *path);
char *dirname(const char *path);

#endif
