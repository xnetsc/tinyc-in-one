/* Pipeline probe: the vector value pipeline itself -- plain C
   vector types and operators, argument passing, returns,
   variadic arguments, subscripts, globals -- plus a pass over
   the shipped header surface.  Compiled by TCC with the vendor
   headers and by gcc with its own; outputs must agree.  The
   plain-C typedefs come before the system headers: glibc
   defines __attribute__ away for compilers without __GNUC__,
   which would strip their vector_size. */
typedef float v4sf __attribute__((__vector_size__(16)));
typedef double v2df __attribute__((__vector_size__(16)));
typedef int v4si __attribute__((__vector_size__(16)));
typedef float v8sf __attribute__((__vector_size__(32)));
typedef float v16sf __attribute__((__vector_size__(64)));
typedef long long v8di __attribute__((__vector_size__(64)));

#include "x86_probe.h"
#include <stdarg.h>

static v4sf g_vec = { 1.5f, 2.5f, 3.5f, 4.5f };

static v4sf add_ret(v4sf a, v4sf b)
{
    return a + b;
}

static v8sf wide_mul(v8sf a, v8sf b)
{
    return a * b;
}

static v16sf widest_add(v16sf a, v16sf b)
{
    return a + b;
}

static v8di wide_int(v8di a, v8di b)
{
    return a ^ b;
}

/* six vector arguments: register pressure across the call */
static v4sf six_args(v4sf a, v4sf b, v4sf c, v4sf d, v4sf e, v4sf f)
{
    return a + b + c + d + e + f;
}

static float va_sum(int n, ...)
{
    va_list ap;
    float s = 0;
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++) {
        __m128 v = va_arg(ap, __m128);
        s += v[0] + v[1] + v[2] + v[3];
    }
    va_end(ap);
    return s;
}

static float va_wide(int n, ...)
{
    va_list ap;
    float s = 0;
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++) {
        __m256 v = va_arg(ap, __m256);
        s += v[0] + v[7];
    }
    va_end(ap);
    return s;
}

static void printv(int idx, const void *p, int n)
{
    print_hex(idx, p, n);
}

int main(void)
{
    int idx = 0;
    /* plain C operators on local typedefs */
    {
        v4sf a = { 1.0f, 2.0f, 3.0f, 4.0f };
        v4sf b = { 10.0f, 20.0f, 30.0f, 40.0f };
        v4sf r = add_ret(a, b);
        printv(idx++, &r, sizeof r);
        r = a * b - a / b;
        printv(idx++, &r, sizeof r);
        r = -a;
        printv(idx++, &r, sizeof r);
    }
    {
        v4si a = { 100, -200, 300, -400 };
        v4si b = { 3, 5, 7, 9 };
        v4si r = a + b;
        printv(idx++, &r, sizeof r);
        r = a << 2;
        printv(idx++, &r, sizeof r);
        r = a >> b;
        printv(idx++, &r, sizeof r);
        r = a > b;
        printv(idx++, &r, sizeof r);
        r = ~a;
        printv(idx++, &r, sizeof r);
        r = a == b;
        printv(idx++, &r, sizeof r);
    }
    {
        v2df a = { 1.25, -2.5 };
        v2df b = { 0.5, 4.0 };
        v2df r = a * b + a;
        printv(idx++, &r, sizeof r);
    }
    /* wide vectors: parameters, returns, operators */
    {
        v8sf a, b, r;
        int i;
        for (i = 0; i < 8; i++) { a[i] = i + 1; b[i] = 2 * i + 1; }
        r = wide_mul(a, b);
        printv(idx++, &r, sizeof r);
        r = a + b;
        printv(idx++, &r, sizeof r);
    }
    {
        v16sf a, b, r;
        int i;
        for (i = 0; i < 16; i++) { a[i] = i - 4; b[i] = 3; }
        r = widest_add(a, b);
        printv(idx++, &r, sizeof r);
    }
    {
        v8di a, b, r;
        int i;
        for (i = 0; i < 8; i++) { a[i] = 0x1234 + i; b[i] = 0xff << i; }
        r = wide_int(a, b);
        printv(idx++, &r, sizeof r);
    }
    /* globals and subscripts */
    {
        v4sf t = g_vec;
        t[1] = 9.5f;
        t[3] = t[0] + t[2];
        g_vec = t;
        printv(idx++, &g_vec, sizeof g_vec);
        printf("%d sub %g %g\n", idx++, (double)t[1], (double)t[3]);
    }
    /* many arguments */
    {
        v4sf one = { 1, 1, 1, 1 };
        v4sf r = six_args(one, one, one, one, one, one);
        printv(idx++, &r, sizeof r);
    }
    /* variadic vector arguments */
    {
        __m128 a = _mm_set_ps(4, 3, 2, 1);
        __m128 b = _mm_set_ps(40, 30, 20, 10);
        float s = va_sum(2, a, b);
        printf("%d vasum %g\n", idx++, (double)s);
        __m256 w = _mm256_set_ps(8, 7, 6, 5, 4, 3, 2, 1);
        float s2 = va_wide(1, w);
        printf("%d vawide %g\n", idx++, (double)s2);
    }
    /* header surface */
    {
        __m128 a = _mm_set_ps(16, 9, 4, 1);
        __m128 r = _mm_sqrt_ps(a);
        printv(idx++, &r, sizeof r);
        __m128i x = _mm_set1_epi32(100);
        __m128i y = _mm_set_epi32(4, 3, 2, 1);
        __m128i z = _mm_add_epi32(x, y);
        printv(idx++, &z, sizeof z);
        z = _mm_shuffle_epi32(z, 0x1b);
        printv(idx++, &z, sizeof z);
        printf("%d mask %d\n", idx++,
               _mm_movemask_epi8(_mm_cmpeq_epi32(z, y)));
        __m256 w = _mm256_set1_ps(2.5f);
        __m256 wr = _mm256_mul_ps(w, w);
        printv(idx++, &wr, sizeof wr);
        __m512 q = _mm512_set1_ps(1.5f);
        __m512 qr = _mm512_add_ps(q, q);
        printv(idx++, &qr, sizeof qr);
        __m128d d = _mm_set_pd(2.0, 1.0);
        __m128d dr = _mm_mul_pd(d, d);
        printv(idx++, &dr, sizeof dr);
        __m128 ld = _mm_load_ps((const float *)&a);
        printv(idx++, &ld, sizeof ld);
        float out[4];
        _mm_store_ps(out, r);
        printf("%d store %g %g %g %g\n", idx++, (double)out[0],
               (double)out[1], (double)out[2], (double)out[3]);
    }
    return fails != 0;
}
