/* Shared support for the x86 SIMD differential probes.  The
   same probe source is compiled by TCC (with the vendor
   intrinsic headers) and by gcc (with its own headers); both
   binaries run natively and their outputs are diffed.  Cases
   print their result bits; where a probe carries a scalar
   reference, a disagreement prints MISMATCH and fails the run. */
#ifndef X86_PROBE_H
#define X86_PROBE_H

#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fenv.h>

static int fails;
static void mismatch(const char *name)
{
    printf("MISMATCH %s\n", name);
    fails++;
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

/* deterministic byte pattern, identical for both compilers */
static void fill_bytes(void *p, int n, int seed)
{
    unsigned char *b = (unsigned char *)p;
    int i;
    for (i = 0; i < n; i++)
        b[i] = (unsigned char)(i * 37 + seed * 11 + 5);
}

/* tame float patterns: finite, no NaN, so scalar references
   and cross-implementation results agree bit for bit */
static const float ftab[8] = {
    0.5f, -0.5f, 1.5f, -1.5f, 2.25f, -2.25f, 3.75f, 0.25f
};
static const double dtab[8] = {
    0.5, -0.5, 1.5, -1.5, 2.25, -2.25, 3.75, 0.25
};

static void fill_floats(float *p, int n, int seed)
{
    int i;
    for (i = 0; i < n; i++)
        p[i] = ftab[(i + seed) & 7] * (1 + ((seed + i) & 3));
}

static void fill_doubles(double *p, int n, int seed)
{
    int i;
    for (i = 0; i < n; i++)
        p[i] = dtab[(i + seed) & 7] * (1 + ((seed + i) & 3));
}

#endif

/* fused multiply-add at the operand's own precision */
#define MULADD(x, y, z) \
    (sizeof(x) == 4 ? fmaf((x), (y), (z)) : fma((x), (y), (z)))

static float half2float(unsigned short h)
{
    unsigned sign = (unsigned)(h & 0x8000) << 16;
    int exp = (h >> 10) & 0x1f;
    unsigned man = h & 0x3ff;
    unsigned u;
    float f;
    if (exp == 0) {
        if (!man)
            u = sign;
        else {
            int e = -14;
            while (!(man & 0x400)) {
                man <<= 1;
                e--;
            }
            man &= 0x3ff;
            u = sign | ((unsigned)(e + 127) << 23) | (man << 13);
        }
    } else if (exp == 31)
        u = sign | 0x7f800000u | (man << 13) |
            (man ? 0x400000u : 0);
    else
        u = sign | ((unsigned)(exp + 112) << 23) | (man << 13);
    memcpy(&f, &u, sizeof f);
    return f;
}

/* float -> half; mode is the vcvtps2ph rounding selector:
   0 nearest-even, 1 toward -inf, 2 toward +inf, 3 toward zero */
static unsigned short float2half(float f, int mode)
{
    unsigned u, sign, man, bits;
    int exp, sticky, halfway;
    memcpy(&u, &f, sizeof u);
    sign = (u >> 16) & 0x8000u;
    exp = (int)((u >> 23) & 0xff);
    man = u & 0x7fffffu;
    if (exp == 0xff)
        return (unsigned short)(sign | 0x7c00u | (man ? 0x200u : 0));
    exp -= 112;
    if (exp >= 31) {
        if (mode == 3 || (mode == 1 && sign) || (mode == 2 && !sign))
            return (unsigned short)(sign | 0x7bffu);
        return (unsigned short)(sign | 0x7c00u);
    }
    if (exp <= 0) {
        unsigned m = man | 0x800000u;
        int sh = 14 - exp;
        if (sh >= 25) {
            if (m && ((mode == 1 && sign) || (mode == 2 && !sign)))
                return (unsigned short)(sign | 1u);
            return (unsigned short)sign;
        }
        bits = m >> sh;
        sticky = (m & ((1u << sh) - 1)) != 0;
        halfway = sticky && (m & ((1u << sh) - 1)) == (1u << (sh - 1));
    } else {
        bits = ((unsigned)exp << 10) | (man >> 13);
        sticky = (man & 0x1fffu) != 0;
        halfway = sticky && (man & 0x1fffu) == 0x1000u;
    }
    if (mode == 3)
        return (unsigned short)(sign | bits);
    if (mode == 1)
        return (unsigned short)(sign |
                                (bits + ((sticky && sign) ? 1 : 0)));
    if (mode == 2)
        return (unsigned short)(sign |
                                (bits + ((sticky && !sign) ? 1 : 0)));
    if (sticky && (!halfway || (bits & 1)))
        bits++;
    return (unsigned short)(sign | bits);
}
