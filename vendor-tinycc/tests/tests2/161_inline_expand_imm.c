#if defined(__x86_64__) || defined(__i386__)
/* before the system headers: glibc defines __attribute__ away,
   which would strip the vector_size */
typedef float __m128 __attribute__((__vector_size__(16)));
#endif
#include <stdio.h>

#if defined(__x86_64__) || defined(__i386__)
/* Immediate penetration: the shuffle control reaches the builtin
   only as a compile-time constant.  Through the entity path these
   wrappers cannot be compiled at all (the parameter is not a
   constant inside the body); a call with a constant argument
   expands in place and the constant flows into the immediate. */
static inline __m128 shuf2(__m128 a, __m128 b, int i)
{
    return __builtin_ia32_vop2i(49, a, b, i, __m128);
}
static inline __m128 shuf1(__m128 a, int i) { return shuf2(a, a, i); }
enum { SHUF_ID = 0xe4 };

static void pr(__m128 r)
{
    printf("%.0f %.0f %.0f %.0f\n", r[0], r[1], r[2], r[3]);
}
#endif

int main(void)
{
#if defined(__x86_64__) || defined(__i386__)
    __m128 a = { 1, 2, 3, 4 };
    __m128 b = { 10, 20, 30, 40 };
    pr(shuf1(a, 0x1b));            /* literal */
    pr(shuf1(a, (1 << 2) | 2));    /* folded constant expression */
    pr(shuf1(a, SHUF_ID));         /* enum constant */
    pr(shuf2(a, b, 0x1b));         /* two-vector form */
#else
    printf("4 3 2 1\n3 2 1 1\n1 2 3 4\n4 3 20 10\n");
#endif
    return 0;
}
