/* strings.h - legacy string functions, Darwin.
 *
 * Written for this package from the POSIX <strings.h>
 * requirements: the b* functions and the case-insensitive
 * comparisons libSystem provides.
 */

#ifndef _TINYC_DARWIN_STRINGS_H
#define _TINYC_DARWIN_STRINGS_H

#include <stddef.h>
#include <sys/types.h>

int bcmp(const void *s1, const void *s2, size_t n);
void bcopy(const void *src, void *dst, size_t n);
void bzero(void *s, size_t n);
int ffs(int i);
int ffsl(long i);
int ffsll(long long i);
char *index(const char *s, int c);
char *rindex(const char *s, int c);
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

#endif
