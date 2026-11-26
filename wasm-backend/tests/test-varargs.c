/* test-varargs.c - variadic functions: definitions, calls, and
 * va_start/va_arg/va_copy/va_end across the scalar types.
 *
 * The wasm calling convention for a variadic function is its
 * fixed parameters plus a trailing pointer to a variadic
 * argument area the caller materialises in its frame, laid out
 * with the default argument promotions (float -> double,
 * sub-int -> int; i64/f64 in 8-byte slots aligned to 8, the
 * rest in 4-byte slots).  probe() returns a bitmask of
 * failures (0 = all good). */
#include <stdarg.h>

static int streq(const char *a, const char *b)
{
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static int sumi(int n, ...)
{
    va_list ap;
    int s = 0, i;
    va_start(ap, n);
    for (i = 0; i < n; i++) s += va_arg(ap, int);
    va_end(ap);
    return s;
}

static long long sumll(int n, ...)
{
    va_list ap;
    long long s = 0;
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++) s += va_arg(ap, long long);
    va_end(ap);
    return s;
}

static double sumd(int n, ...)
{
    va_list ap;
    double s = 0;
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++) s += va_arg(ap, double);
    va_end(ap);
    return s;
}

/* interleaved types: exercises slot alignment in the area */
static double mix(int n, ...)
{
    va_list ap;
    double r = 0;
    int i;
    va_start(ap, n);
    for (i = 0; i < n; i++) {
        if (i % 3 == 0) r += va_arg(ap, int);
        else if (i % 3 == 1) r += va_arg(ap, double);
        else r += va_arg(ap, long long);
    }
    va_end(ap);
    return r;
}

/* char/short/float arguments arrive promoted (int / double) */
static int promoted(int n, ...)
{
    va_list ap;
    int a, b;
    double c;
    va_start(ap, n);
    a = va_arg(ap, int);      /* passed a char */
    b = va_arg(ap, int);      /* passed a short */
    c = va_arg(ap, double);   /* passed a float */
    va_end(ap);
    return a * 10000 + b * 100 + (int)(c * 2);
}

/* va_copy: walk the same area twice */
static int twice(int n, ...)
{
    va_list ap, aq;
    int s1 = 0, s2 = 0, i;
    va_start(ap, n);
    va_copy(aq, ap);
    for (i = 0; i < n; i++) s1 += va_arg(ap, int);
    for (i = 0; i < n; i++) s2 += va_arg(aq, int);
    va_end(ap);
    va_end(aq);
    return s1 - s2;
}

/* a variadic function calling another variadic function */
static int wrap(int n, ...)
{
    va_list ap;
    int a, b;
    va_start(ap, n);
    a = va_arg(ap, int);
    b = va_arg(ap, int);
    va_end(ap);
    return sumi(3, a, b, 100);
}

/* recursion */
static int rec(int n, ...)
{
    va_list ap;
    int v, r;
    va_start(ap, n);
    v = va_arg(ap, int);
    va_end(ap);
    if (n <= 1) return v;
    r = rec(n - 1, v + 1);
    return r + v;
}

/* mini-printf: format into a buffer with %d %s %x %c %% %u
   and %llx (the decimal %lld path would need the __udivdi3
   helper, whose host-side convention this backend leaves to
   libtcc1; hex keeps this test self-contained) */
static void fmt_into(char *out, const char *fmt, ...)
{
    va_list ap;
    char *o = out;
    const char *f;
    va_start(ap, fmt);
    for (f = fmt; *f; f++) {
        if (*f != '%') { *o++ = *f; continue; }
        f++;
        if (*f == 'l' && f[1] == 'l' && f[2] == 'x') {
            unsigned long long v = va_arg(ap, unsigned long long);
            char tmp[16]; int n = 0;
            f += 2;
            do {
                unsigned d = (unsigned)(v & 15);
                tmp[n++] = (char)(d < 10 ? '0' + (int)d : 'a' + (int)d - 10);
                v >>= 4;
            } while (v);
            while (n) *o++ = tmp[--n];
        } else if (*f == 'd' || *f == 'i') {
            int v = va_arg(ap, int);
            char tmp[12]; int n = 0;
            unsigned u;
            if (v < 0) { *o++ = '-'; u = (unsigned)(-v); } else u = (unsigned)v;
            do { tmp[n++] = (char)('0' + (int)(u % 10)); u /= 10; } while (u);
            while (n) *o++ = tmp[--n];
        } else if (*f == 'u' || *f == 'x') {
            unsigned v = va_arg(ap, unsigned);
            char tmp[12]; int n = 0;
            unsigned base = *f == 'x' ? 16u : 10u;
            do {
                unsigned d = v % base;
                tmp[n++] = (char)(d < 10 ? '0' + (int)d : 'a' + (int)d - 10);
                v /= base;
            } while (v);
            if (n == 0) tmp[n++] = '0';
            while (n) *o++ = tmp[--n];
        } else if (*f == 's') {
            const char *s = va_arg(ap, const char *);
            while (*s) *o++ = *s++;
        } else if (*f == 'c') {
            *o++ = (char)va_arg(ap, int);
        } else if (*f == '%') {
            *o++ = '%';
        } else if (*f) {
            *o++ = *f;
        }
    }
    *o = 0;
    va_end(ap);
}

int probe(void)
{
    int bad = 0;
    char buf[128];
    if (sumi(3, 10, 20, 12) != 42) bad |= 1;
    if (sumi(0) != 0) bad |= 2;
    if (sumi(5, 1, -2, 3, -4, 5) != 3) bad |= 4;
    if (sumll(2, 5000000000LL, 7LL) != 5000000007LL) bad |= 8;
    if (sumd(2, 1.5, 2.25) != 3.75) bad |= 16;
    /* mix pattern per i%3: int, double, llong repeating */
    if (mix(6, 1, 2.5, 3LL, 4, 5.5, 6LL) != 22.0) bad |= 32;
    if (promoted(3, (char)7, (short)300, 1.5f) != 100003) bad |= 64;
    if (twice(3, 4, 5, 6) != 0) bad |= 128;
    if (wrap(2, 1, 2) != 103) bad |= 256;
    if (rec(3, 10) != 33) bad |= 512;
    fmt_into(buf, "%d %s %x %c %% %u %llx", -42, "hi", 255, 'z', 7u,
             0x123456789abLL);
    if (!streq(buf, "-42 hi ff z % 7 123456789ab")) bad |= 1024;
    fmt_into(buf, "%s=%d", "x", 0);
    if (!streq(buf, "x=0")) bad |= 2048;
    /* indirect call through a function pointer */
    {
        typedef int (*sumfn)(int, ...);
        sumfn f = sumi;
        if (f(3, 1, 2, 3) != 6) bad |= 4096;
    }
    return bad;
}
