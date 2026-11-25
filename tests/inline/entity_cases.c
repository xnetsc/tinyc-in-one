#include <stdio.h>

/* Entity-path observables for the symbol checks in
   run_inline_tests.sh: wsum's calls all expand, so no entity is
   emitted; the other three must each have an entity. */
static inline int wsum(int a, int b) { return a + b; }
static inline int rsum(int n) { return n <= 0 ? 0 : n + rsum(n - 1); }
static inline int at_wrap(int x) { return x * 7; }
static inline int multi_stmt(int x) { int y = x + 1; return y * 2; }

int main(void)
{
    int (*fp)(int) = at_wrap;
    printf("%d %d %d %d %d\n", wsum(3, 4), rsum(5), fp(6), at_wrap(6),
           multi_stmt(10));
    return 0;
}
