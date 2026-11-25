#include "lane_header.h"
#include <stdio.h>

static void pr(__m128 r)
{
    printf("%.0f %.0f %.0f %.0f\n", r[0], r[1], r[2], r[3]);
}

int main(void)
{
    __m128 a = { 1, 2, 3, 4 };
    __m128 b = { 10, 20, 30, 40 };
    pr(tc_rev(a));              /* fixed immediate, two layers up */
    pr(tc_fwd_n(a, 0x06));      /* literal forwarded through a layer */
    pr(tc_fwd_n(a, 3 + 0xe1));  /* folded constant, same route */
    pr(tc_shuffle(a, b, 0x1b)); /* raw layer, two vectors */
    pr(tc_add(a, b));           /* plain vector wrapper */
    return 0;
}
