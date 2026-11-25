#include "lane_header.h"
#include <stdio.h>

/* A runtime (non-constant) argument to an immediate-forwarding
   wrapper must fail to compile: the parameter cannot satisfy the
   builtin's constant predicate in any form.  The error names the
   wrapper body in lane_header.h. */
int main(void)
{
    __m128 a = { 1, 2, 3, 4 };
    int n = 27;
    __m128 r = tc_fwd_n(a, n);
    printf("%.0f\n", r[0]);
    return 0;
}
