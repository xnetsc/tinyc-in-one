#include <stdio.h>

static int bump(int v) { return v + 100; }

static int classify(int c)
{
    switch (c) {
    case 9:  return 1;
    case 10: return 2;
    case 35: return 3;
    case 65: return 4;
    default: return 5;
    }
}

int main(void)
{
    /* a scalar local read and written across statements */
    int x = 3;
    x = x * 7 + 1;
    x = x - 4;
    printf("seq %d\n", x + x / 2);

    /* a call between a write and a read of the same local */
    int y = 5;
    int z = bump(y);
    printf("call %d %d\n", y, z);

    /* address taken: writes through the pointer stay visible */
    int w = 1;
    int *pw = &w;
    *pw = 41;
    w = w + 1;
    printf("addr %d\n", w);

    /* short-circuit: the skipped side's assignment must not happen,
       the evaluated side's write-back must survive */
    int a = 1;
    int r1 = a || (a = 99);
    int r2 = (a = 0) || (a = 42);
    int r3 = (a = 7) && (a = 8);
    printf("sc %d %d %d %d\n", r1, r2, r3, a);

    /* switch on a local assigned just before the dispatch, in a loop */
    int s = 0;
    int i;
    for (i = 0; i < 5; i++) {
        int c = 9 + i * 14;
        s += classify(c % 70);
    }
    printf("sw %d\n", s);

    /* a 64-bit local first read through a 32-bit cast, then in full */
    unsigned long long u = 0x123456789abcdef0ULL;
    unsigned lo = (unsigned)u;
    unsigned long long hi = u >> 32;
    printf("wide %x %llx\n", lo, hi);

    return 0;
}
