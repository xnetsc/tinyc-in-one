typedef float __m128 __attribute__((__vector_size__(16)));
#include <stdio.h>

/* The fixture consumer's computations written directly against the
   builtins, with no wrapper layers: the outputs must agree. */
static void pr(__m128 r)
{
    printf("%.0f %.0f %.0f %.0f\n", r[0], r[1], r[2], r[3]);
}

int main(void)
{
    __m128 a = { 1, 2, 3, 4 };
    __m128 b = { 10, 20, 30, 40 };
    pr(__builtin_ia32_vop2i(49, a, a, 0x1b, __m128));
    pr(__builtin_ia32_vop2i(49, a, a, 0x06, __m128));
    pr(__builtin_ia32_vop2i(49, a, a, 3 + 0xe1, __m128));
    pr(__builtin_ia32_vop2i(49, a, b, 0x1b, __m128));
    pr(a + b);
    return 0;
}
