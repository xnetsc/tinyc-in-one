/* uchar.h - C11 Unicode characters, Darwin.
 *
 * Written for this package from the C11 <uchar.h> requirements.
 * libSystem provides none of these four functions, so they are
 * implemented in lib/uchar.c (shipped inside libtcc1.a) as a
 * direct UTF-8 codec: on this platform the multibyte encoding
 * these functions convert from and to is UTF-8.  The conversion
 * state lives in the first bytes of mbstate_t, with the all-zero
 * state as the initial state.  One boundary follows from that:
 * an mbstate_t used with these functions does not share its
 * intermediate state with the <wchar.h> conversion functions,
 * whose state format is libSystem's own and unpublished.
 */

#ifndef _TINYC_DARWIN_UCHAR_H
#define _TINYC_DARWIN_UCHAR_H

#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;

size_t mbrtoc16(char16_t *pc16, const char *s, size_t n, mbstate_t *ps);
size_t c16rtomb(char *s, char16_t c16, mbstate_t *ps);
size_t mbrtoc32(char32_t *pc32, const char *s, size_t n, mbstate_t *ps);
size_t c32rtomb(char *s, char32_t c32, mbstate_t *ps);

#endif
