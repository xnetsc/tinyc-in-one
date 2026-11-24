/* Shared support for the NEON differential probes.  The same
   probe source is compiled by TCC (with the vendor arm_neon.h)
   and by the cross gcc (with its own arm_neon.h); both binaries
   run under qemu-aarch64 and their outputs are diffed.  Cases
   print their result bits; where a probe carries a scalar
   reference, a disagreement prints MISMATCH and fails the run. */
#ifndef NEON_PROBE_H
#define NEON_PROBE_H

#include <arm_neon.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int fails;
static void mismatch(const char *name)
{
    printf("MISMATCH %s\n", name);
    fails++;
}

/* table lookup: result byte i is tab[idx[i]] when idx[i] is in
   range, else zero (TBL) or the default operand byte (TBX) */
static void tbl_check(const unsigned char *def,
                      const unsigned char *tab,
                      const unsigned char *idx,
                      const unsigned char *res, int n, int bound,
                      int keep, const char *what)
{
    int i;
    for (i = 0; i < n; i++) {
        unsigned char want = idx[i] < bound ? tab[idx[i]]
                                            : (keep ? def[i] : 0);
        if (res[i] != want)
            mismatch(what);
    }
}

static void print_hex(int idx, const void *p, int n)
{
    const unsigned char *b = (const unsigned char *)p;
    int i;
    printf("%d ", idx);
    for (i = 0; i < n; i++)
        printf("%02x", b[i]);
    printf("\n");
}

/* absolute value of a difference computed in long long, for
   the widening-free difference references */
static long long wide_abs(long long x) { return x < 0 ? -x : x; }

/* deterministic byte pattern, identical for both compilers */
static void fill_bytes(void *p, int n, int seed)
{
    unsigned char *b = (unsigned char *)p;
    int i;
    for (i = 0; i < n; i++)
        b[i] = (unsigned char)(i * 37 + seed * 11 + 5);
}

/* tame float patterns: finite, no NaN, no negative zero, so
   scalar references and cross-implementation results agree */
static const float ftab[8] = {
    0.0f, 0.5f, -0.5f, 1.5f, -1.5f, 2.25f, -2.25f, 3.75f
};
static const double dtab[8] = {
    0.0, 0.5, -0.5, 1.5, -1.5, 2.25, -2.25, 3.75
};

/* saturating arithmetic references, one per element type */
#define DEF_SAT(T, NAME, MN, MX)                                       \
    static T sat_add_##NAME(T a, T b)                                  \
    {                                                                  \
        long long r = (long long)a + (long long)b;                     \
        return r > (long long)(MX) ? (T)(MX) :                         \
               r < (long long)(MN) ? (T)(MN) : (T)r;                   \
    }                                                                  \
    static T sat_sub_##NAME(T a, T b)                                  \
    {                                                                  \
        long long r = (long long)a - (long long)b;                     \
        return r > (long long)(MX) ? (T)(MX) :                         \
               r < (long long)(MN) ? (T)(MN) : (T)r;                   \
    }                                                                  \
    static T sat_abs_##NAME(T a)                                       \
    {                                                                  \
        return a == (T)(MN) ? (T)(MX) : a < 0 ? (T)-a : a;             \
    }                                                                  \
    static T sat_neg_##NAME(T a)                                       \
    {                                                                  \
        return a == (T)(MN) ? (T)(MX) : (T)-a;                        \
    }                                                                  \
    static T abs_ref_##NAME(T a) { return a < 0 ? (T)-a : a; }

DEF_SAT(signed char, s8, -128, 127)
DEF_SAT(short, s16, -32768, 32767)
DEF_SAT(int, s32, -2147483647 - 1, 2147483647)
/* the 64-bit saturating pair goes through unsigned arithmetic:
   the wrapped sum in unsigned long long is well defined, and
   the sign combination of the operands tells whether the true
   result overflowed and in which direction */
static long long sat_add_s64(long long a, long long b)
{
    long long r = (long long)((unsigned long long)a +
                              (unsigned long long)b);
    if (a > 0 && b > 0 && r < 0)
        return 9223372036854775807LL;
    if (a < 0 && b < 0 && r >= 0)
        return -9223372036854775807LL - 1;
    return r;
}
static long long sat_sub_s64(long long a, long long b)
{
    long long r = (long long)((unsigned long long)a -
                              (unsigned long long)b);
    if (a > 0 && b < 0 && r < 0)
        return 9223372036854775807LL;
    if (a < 0 && b > 0 && r >= 0)
        return -9223372036854775807LL - 1;
    return r;
}
static long long sat_abs_s64(long long a)
{
    return a == (-9223372036854775807LL - 1) ?
        9223372036854775807LL : a < 0 ? -a : a;
}
static long long sat_neg_s64(long long a)
{
    return a == (-9223372036854775807LL - 1) ?
        9223372036854775807LL : -a;
}
static long long abs_ref_s64(long long a)
{
    return a < 0 ? (long long)(0ULL - (unsigned long long)a) : a;
}
#define DEF_SATU(T, NAME, MX)                                          \
    static T sat_add_##NAME(T a, T b)                                  \
    {                                                                  \
        if ((unsigned long long)a >                                    \
            (unsigned long long)(MX) - (unsigned long long)b)          \
            return (T)(MX);                                            \
        return (T)(a + b);                                             \
    }                                                                  \
    static T sat_sub_##NAME(T a, T b)                                  \
    {                                                                  \
        return a < b ? (T)0 : (T)(a - b);                              \
    }                                                                  \
    static T abs_ref_##NAME(T a) { return a; }
DEF_SATU(unsigned char, u8, 255)
DEF_SATU(unsigned short, u16, 65535)
DEF_SATU(unsigned int, u32, 4294967295u)
DEF_SATU(unsigned long long, u64, 18446744073709551615ULL)

#define fabs_ref(x) ((x) >= 0 ? (x) : -(x))
#define fmax_ref(a, b) ((a) > (b) ? (a) : (b))
#define fmin_ref(a, b) ((a) < (b) ? (a) : (b))

/* polynomial (carryless) multiply of two bytes */
static unsigned short pmul_ref(unsigned a, unsigned b)
{
    unsigned r = 0;
    while (b) {
        if (b & 1)
            r ^= a;
        a <<= 1;
        b >>= 1;
    }
    return (unsigned short)r;
}

#define DEF_CLS(NAME, T, BITS)                                       \
    static int cls_ref_##NAME(T a)                                   \
    {                                                                \
        unsigned x = (unsigned)(a);                                  \
        unsigned sign = (x >> (BITS - 1)) & 1;                       \
        int n = 0, i;                                                \
        for (i = BITS - 2; i >= 0; i--) {                            \
            if (((x >> i) & 1) != sign)                              \
                break;                                               \
            n++;                                                     \
        }                                                            \
        return n;                                                    \
    }

/* mixed-sign saturating adds: usqadd is an unsigned accumulator
   plus a signed addend, uqadd the mirror image */
#define DEF_USQADD(UT, ST, NAME, MX)                                   \
    static UT usqadd_ref_##NAME(UT a, ST b)                            \
    {                                                                  \
        if (b >= 0) {                                                  \
            unsigned long long r =                                     \
                (unsigned long long)a + (unsigned long long)b;         \
            return r > (unsigned long long)(MX) ? (UT)(MX) : (UT)r;   \
        }                                                              \
        {                                                              \
            unsigned long long m =                                     \
                (unsigned long long)(-(b + 1)) + 1;                    \
            return (unsigned long long)a < m ? (UT)0 : (UT)(a - m);   \
        }                                                              \
    }
DEF_USQADD(unsigned char, signed char, u8, 255)
DEF_USQADD(unsigned short, short, u16, 65535)
DEF_USQADD(unsigned int, int, u32, 4294967295u)
static unsigned long long usqadd_ref_u64(unsigned long long a,
                                         long long b)
{
    if (b >= 0) {
        unsigned long long r = a + (unsigned long long)b;
        return r < a ? ~0ULL : r;
    }
    {
        unsigned long long m = (unsigned long long)(-(b + 1)) + 1;
        return a < m ? 0 : a - m;
    }
}
#define DEF_UQADD(ST, UT, NAME, MX)                                    \
    static ST uqadd_ref_##NAME(ST a, UT b)                             \
    {                                                                  \
        if (a >= 0) {                                                  \
            unsigned long long r =                                     \
                (unsigned long long)a + (unsigned long long)b;         \
            return r > (unsigned long long)(MX) ? (ST)(MX) : (ST)r;   \
        }                                                              \
        {                                                              \
            unsigned long long m =                                     \
                (unsigned long long)(-(a + 1)) + 1;                    \
            if ((unsigned long long)b >= m) {                          \
                unsigned long long r = (unsigned long long)b - m;      \
                return r > (unsigned long long)(MX) ? (ST)(MX)        \
                                                    : (ST)r;           \
            }                                                          \
            return (ST)(-(long long)(m - (unsigned long long)b));      \
        }                                                              \
    }
DEF_UQADD(signed char, unsigned char, s8, 127)
DEF_UQADD(short, unsigned short, s16, 32767)
DEF_UQADD(int, unsigned int, s32, 2147483647)
static long long uqadd_ref_s64(long long a, unsigned long long b)
{
    if (a >= 0) {
        if (b > ~0ULL - (unsigned long long)a)
            return 9223372036854775807LL;
        {
            unsigned long long r = (unsigned long long)a + b;
            return r > 9223372036854775807ULL
                       ? 9223372036854775807LL
                       : (long long)r;
        }
    }
    {
        unsigned long long m = (unsigned long long)(-(a + 1)) + 1;
        if (b >= m) {
            unsigned long long r = b - m;
            return r > 9223372036854775807ULL ? 9223372036854775807LL
                                              : (long long)r;
        }
        return -(long long)(m - b);
    }
}

DEF_CLS(s8, signed char, 8)
DEF_CLS(s16, short, 16)
DEF_CLS(s32, int, 32)
DEF_CLS(u8, unsigned char, 8)
DEF_CLS(u16, unsigned short, 16)
DEF_CLS(u32, unsigned int, 32)

static int popcount_ref(unsigned a)
{
    int n = 0;
    a &= 0xff;
    while (a) {
        n += a & 1;
        a >>= 1;
    }
    return n;
}

#endif
