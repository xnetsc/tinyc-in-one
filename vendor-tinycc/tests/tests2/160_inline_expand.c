#if defined(__x86_64__) || defined(__aarch64__)
/* before the system headers: glibc defines __attribute__ away,
   which would strip the vector_size */
typedef float v4sf __attribute__((__vector_size__(16)));
#endif
#include <stdio.h>

/* Parse-level expansion of single-return static inline wrappers:
   every result in this file must be identical whether a call is
   expanded in place or goes through the emitted entity. */

static inline int add2(int a, int b) { return a + b; }
static inline int twice(int x) { return x + x; }
static inline int inc_ret(int x) { return ++x; }
static inline int addr_ret(int x) { return *(&x) + 1; }
static inline int addr_wr(int x) { return ((*(&x) = 77), x); }
static inline int comma_w(int a, int b) { return (a += 2, a * b); }
static inline int *idp(int *p) { return p; }
static inline long lsel(long a, long b, int c) { return c ? a : b; }
static inline long long llinc(long long x) { return x + 1; }
static inline double dmul(double a, double b) { return a * b; }
static inline float fadd(float a, float b) { return a + b; }
static inline short sh_wrap(int x) { return (short)(x + 1); }
static inline signed char ch_wrap(int x) { return (signed char)x; }
static inline _Bool bool_wrap(int x) { return x > 3; }
static inline int seven(void) { return 7; }
static inline int clamp0(int x) { return x < 0 ? 0 : x; }
static inline int both(int a, int b) { return a && b; }

/* multi-level nesting: wrappers calling wrappers */
static inline int lvl1(int x) { return x * 3; }
static inline int lvl2(int x) { return lvl1(x) + lvl1(x + 1); }
static inline int lvl3(int x) { return lvl2(x) - lvl1(2); }

/* a wrapper calling a plain (non-inline) function */
static int plain_double(int x) { return 2 * x; }
static inline int via_plain(int x) { return plain_double(x) + 1; }

/* hygiene: names in the body resolve at the definition point */
static int g = 41;
static inline int read_g(int d) { return g + d; }
static const int tab[3] = { 10, 20, 30 };
static inline int tabi(int i) { return tab[i]; }
enum Color { RED = 3, GREEN = 5 };
static inline int col(void) { return GREEN + RED; }
typedef int myint;
static inline int castw(double d) { return (myint)d; }
struct P { int a, b; };
static inline int psum(struct P *p) { return p->a + p->b; }

/* recursion: the inner call cannot expand and uses the entity */
static inline int rsum(int n) { return n <= 0 ? 0 : n + rsum(n - 1); }

/* mutual recursion */
static inline int is_even_n(int n);
static inline int is_odd_n(int n) { return n == 0 ? 0 : is_even_n(n - 1); }
static inline int is_even_n(int n) { return n == 0 ? 1 : is_odd_n(n - 1); }

/* address taken below: calls before and after must agree */
static inline int at_wrap(int x) { return x * 7; }

/* the parameter is a copy: writes through an aliasing pointer
   argument must not change what a later read of it returns */
static inline int alias_read(int x, int *p) { *p = 9; return x; }

#if defined(__x86_64__) || defined(__aarch64__)
static inline v4sf vaddw(v4sf a, v4sf b) { return a + b; }
#endif

int main(void)
{
    int v = 5, c = 0, a = 3;
    int ir, ar;
    struct P p = { 30, 12 };
    int (*fp)(int);
    int myint = 5;

    printf("%d %d %d\n", add2(1, 2), add2(v, 10), twice(21));
    printf("%d %d\n", add2(add2(1, 2), add2(3, 4)), 1 + add2(2, 3) * 4);
    printf("%d\n", twice(c++));
    printf("%d %d\n", c, add2(c, 100));
    ir = inc_ret(v);
    printf("%d %d\n", ir, v);
    printf("%d %d\n", addr_ret(10), addr_wr(1));
    printf("%d\n", comma_w(3, 4));
    printf("%d %ld %lld\n", *idp(&v), lsel(10000000000L, 7L, 1),
           llinc(9000000000LL));
    printf("%.1f %.1f\n", dmul(1.5, 4.0), (double)fadd(0.5f, 0.25f));
    printf("%d %d %d %d\n", sh_wrap(41), ch_wrap(200), bool_wrap(5),
           bool_wrap(2));
    printf("%d %d %d %d\n", seven(), clamp0(-3) + clamp0(9),
           both(1, 2), both(1, 0));
    printf("%d %d %d\n", lvl1(2), lvl2(2), lvl3(2));
    printf("%d\n", via_plain(20));
    printf("%d %d %d\n", tabi(0) + tabi(2), col(), psum(&p));
    printf("%d %d\n", castw(3.7), myint);
    printf("%d %d\n", rsum(5), rsum(0));
    printf("%d %d %d %d\n", is_even_n(10), is_odd_n(10), is_even_n(7),
           is_odd_n(7));
    printf("%d ", at_wrap(3));
    fp = at_wrap;
    printf("%d %d %d\n", fp(3), at_wrap(4), (add2)(100, 23));
    ar = alias_read(a, &a);
    printf("%d %d\n", ar, a);
    {
        int g = 1000;
        printf("%d %d\n", read_g(1), g);
    }
#if defined(__x86_64__) || defined(__aarch64__)
    {
        v4sf x = { 1, 2, 3, 4 };
        v4sf y = { 10, 20, 30, 40 };
        v4sf r = vaddw(x, y);
        printf("%.0f %.0f %.0f %.0f\n", r[0], r[1], r[2], r[3]);
    }
#else
    printf("11 22 33 44\n");
#endif
    return 0;
}
