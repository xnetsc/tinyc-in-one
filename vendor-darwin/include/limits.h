/* limits.h - value limits, Darwin LP64.
 *
 * Written for this package from the C99 <limits.h> requirements
 * (long is 64-bit) plus the Darwin values of the POSIX runtime
 * limits (PATH_MAX is 1024 on Darwin, OPEN_MAX is 10240).
 */

#ifndef _TINYC_DARWIN_LIMITS_H
#define _TINYC_DARWIN_LIMITS_H

#define CHAR_BIT	8
#define SCHAR_MIN	(-128)
#define SCHAR_MAX	127
#define UCHAR_MAX	255
#ifdef __CHAR_UNSIGNED__
#  define CHAR_MIN	0
#  define CHAR_MAX	UCHAR_MAX
#else
#  define CHAR_MIN	SCHAR_MIN
#  define CHAR_MAX	SCHAR_MAX
#endif
#define SHRT_MIN	(-32768)
#define SHRT_MAX	32767
#define USHRT_MAX	65535
#define INT_MIN		(-2147483647-1)
#define INT_MAX		2147483647
#define UINT_MAX	4294967295U
#define LONG_MIN	(-9223372036854775807L-1)
#define LONG_MAX	9223372036854775807L
#define ULONG_MAX	18446744073709551615UL
#define LLONG_MIN	(-9223372036854775807LL-1)
#define LLONG_MAX	9223372036854775807LL
#define ULLONG_MAX	18446744073709551615ULL

#define MB_LEN_MAX	6

#define WORD_BIT	32
#define LONG_BIT	64

/* POSIX minimum values and the Darwin runtime limits. */
#define _POSIX_ARG_MAX		4096
#define _POSIX_CHILD_MAX	25
#define _POSIX_LINK_MAX		8
#define _POSIX_MAX_CANON	255
#define _POSIX_MAX_INPUT	255
#define _POSIX_NAME_MAX		14
#define _POSIX_NGROUPS_MAX	8
#define _POSIX_OPEN_MAX		20
#define _POSIX_PATH_MAX		256
#define _POSIX_PIPE_BUF		512
#define _POSIX_SSIZE_MAX	32767

#define ARG_MAX		262144
#define CHILD_MAX	266
#define LINK_MAX	32767
#define MAX_CANON	1024
#define MAX_INPUT	1024
#define NAME_MAX	255
#define NGROUPS_MAX	16
#define OPEN_MAX	10240
#define PATH_MAX	1024
#define PIPE_BUF	512
#define SSIZE_MAX	9223372036854775807L
#define BC_BASE_MAX	99
#define BC_DIM_MAX	2048
#define BC_SCALE_MAX	99
#define BC_STRING_MAX	1000
#define CHARCLASS_NAME_MAX	14
#define COLL_WEIGHTS_MAX	2
#define EXPR_NEST_MAX	32
#define LINE_MAX	2048
#define RE_DUP_MAX	255

#endif
