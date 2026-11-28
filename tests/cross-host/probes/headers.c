/* Cross-host differential probe: the hosted header surface.
 * An ordinary hosted program includes the standard headers
 * and uses their constants and types; the baked values
 * fingerprint the header set the compiler actually searched
 * (constants, type sizes, struct layouts fixed by headers).
 * If a header is missing on one host, that side's compile
 * fails -- the failure itself is the differential result.
 * sizeof(FILE *) stands in for sizeof(FILE): FILE is an
 * incomplete type in the bionic and Darwin header sets (it
 * is complete in glibc's), so its size is not part of the
 * portable surface this probe compares. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <float.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <signal.h>
#include <assert.h>

static const long hdr_vals[] = {
    EOF, EXIT_SUCCESS, EXIT_FAILURE, RAND_MAX,
    CHAR_BIT, CHAR_MAX, SCHAR_MAX, SHRT_MAX, INT_MAX, LONG_MAX,
    UCHAR_MAX, USHRT_MAX, UINT_MAX,
    (long)sizeof(size_t), (long)sizeof(ptrdiff_t),
    (long)sizeof(FILE *), (long)sizeof(div_t), (long)sizeof(ldiv_t),
    (long)sizeof(struct tm), (long)sizeof(clock_t), (long)sizeof(time_t),
    (long)sizeof(uint64_t), (long)sizeof(intmax_t),
    (long)sizeof(wchar_t), (long)sizeof(sig_atomic_t),
    FLT_RADIX, FLT_DIG, DBL_DIG, LDBL_DIG,
    FLT_MANT_DIG, DBL_MANT_DIG, LDBL_MANT_DIG,
    (long)sizeof(errno) > 0 ? 1 : 0,
    SIGINT, SIGTERM, SIGSEGV
};

static const double hdr_dvals[] = {
    FLT_EPSILON, DBL_EPSILON, LDBL_EPSILON
};

int probe_headers_anchor(void)
{
    /* HUGE_VAL is checked here, not in the static table: in
     * the MSVC-style headers the Windows target ships it is an
     * extern variable (_HUGE), not a constant expression. */
    return (int)hdr_vals[0] + (int)hdr_dvals[0] +
        (HUGE_VAL > 1e308 ? 1 : 0);
}
