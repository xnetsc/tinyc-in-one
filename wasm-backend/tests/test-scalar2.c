/* scalar suite 2: floating point, control flow, conversions and
   64-bit integers - the parts of the backend that were stubs in
   the skeleton (gen_opf, gen_cvt_*, VT_CMP/gjmp dispatch, 64-bit
   constant loads).  Every function is exported and checked against
   an exact expected value by run-scalar2.mjs. */

typedef int v4si __attribute__((vector_size(16)));
typedef unsigned int v4su __attribute__((vector_size(16)));
typedef long long v2di __attribute__((vector_size(16)));

/* ---- floating-point arithmetic ---------------------------------- */

double d_add(double a, double b) { return a + b; }
double d_sub(double a, double b) { return a - b; }
double d_mul(double a, double b) { return a * b; }
double d_div(double a, double b) { return a / b; }
float f_add(float a, float b) { return a + b; }
float f_mul(float a, float b) { return a * b; }
float f_div(float a, float b) { return a / b; }
double d_neg(double x) { return -x; }
float f_neg(float x) { return -x; }

/* a float value widened to double keeps the f32 rounding */
double d_from_f(float f) { return f; }
/* double -> float conversion rounds to f32 precision */
double d_roundtrip(double d) { float f = (float)d; return f; }

/* ---- comparisons as values -------------------------------------- */

int cmp_vals(int a, int b)
{
    return (a < b) + 2 * (a == b) + 4 * (a > b) + 8 * (a <= b) + 16 * (a != b);
}

int fcmp_vals(double a, double b)
{
    return (a < b) + 2 * (a == b) + 4 * (a >= b);
}

int cmp_and(int a, int b, int c, int d) { return (a < b) && (c < d); }
int cmp_or(int a, int b, int c, int d) { return (a < b) || (c < d); }
int cmp_not(int a, int b) { return !(a < b); }
int cmp_chain(int a, int b) { return (a < b) == (b < a + 10); }

/* ---- comparisons as conditions ---------------------------------- */

int if_else(int x) { if (x > 3) return 1; else return 2; }
int if_float(double f) { if (f >= 2.5) return 1; return 0; }
int fcond_and(double a, double b, double c, double d)
{
    if (a < b && c < d) return 1;
    return 0;
}

int while_sum(int n)
{
    int i = 1, s = 0;
    while (i <= n) { s += i; i++; }
    return s;
}

int while_float(void)
{
    double x = 0.0;
    int n = 0;
    while (x < 1.0) { x += 0.25; n++; }
    return n;
}

int for_loop(void)
{
    int i, s = 0;
    for (i = 0; i < 7; i++) s += i;
    return s;
}

int do_while(void)
{
    int n = 0;
    do { n++; } while (n < 3);
    return n;
}

int nested_bc(void)
{
    int i, s = 0;
    for (i = 1; ; i++) {
        if (i % 2 == 0) continue;
        if (i > 15) break;
        s += i;
    }
    return s; /* 1+3+5+7+9+11+13+15 = 64 */
}

int sw(int x)
{
    int r = 0;
    switch (x) {
    case 1: r = 10; break;
    case 2: r = 20; /* fallthrough */
    case 3: r += 1; break;
    default: r = 99;
    }
    return r;
}

int tern(int x) { return x > 0 ? 111 : 222; }

int go_fwd(int x)
{
    int r = 1;
    if (x) goto end;
    r = 2;
end:
    return r + 10;
}

int go_loop(int n)
{
    int i = 0, s = 0;
top:
    s += i;
    i++;
    if (i < n) goto top;
    return s;
}

double loop_float_mix(void)
{
    double s = 0.0;
    int i;
    for (i = 0; i < 10; i++) {
        if (i % 2) s += 0.5;
        else s += 1.5;
    }
    return s; /* 5*0.5 + 5*1.5 = 10 */
}

/* ---- conversions ------------------------------------------------- */

int f2i(float f) { return (int)f; }
int d2i(double d) { return (int)d; }
int f2u(float f) { return (int)(unsigned int)f; }
double i2d(int i) { return (double)i; }
double u2d(unsigned int u) { return (double)u; }
double i2f_round(int i) { return (double)(float)i; } /* 16777217 -> 16777216 */
long long f2ll(float f) { return (long long)f; }
long long d2ll(double d) { return (long long)d; }
unsigned long long d2ull(double d) { return (unsigned long long)d; }
double ll2d(long long x) { return (double)x; }
double ll2f(long long x) { return (double)(float)x; }
double ull2d(unsigned long long x) { return (double)x; }

/* ---- mixed int/float expressions --------------------------------- */

double mix1(int i, double d) { return i + d; }
double mix2(int i) { return i * 3.5 - 1; }
int mix3(void) { return (int)(2.5 * 4) + 1; }
int mix_cmp(int i, double d) { return i < d; }

/* ---- 64-bit integers ---------------------------------------------- */

long long ll_const(void) { return 5000000000LL; }
long long ll_const_neg(void) { return -5000000000LL; }
int ll_hi(void) { long long x = 5000000000LL; return (int)(x >> 32); }
int ll_lo(void) { long long x = 5000000000LL; return (int)x; }
long long ll_id(long long x) { return x; }
long long ll_add(long long a, long long b) { return a + b; }
long long ll_sub(long long a, long long b) { return a - b; }
long long ll_mul(long long a, long long b) { return a * b; }
long long ll_shr(long long x) { return x >> 32; }
long long ll_shl(int i) { return (long long)i << 40; }
int ll_cmp(long long a, long long b) { return a > b; }
double ll_mix(void) { long long x = 5000000000LL; return (double)(x & 4294967295LL); }

/* ---- i64 vector brace initializers -------------------------------- */

long long v64_a(void) { v2di v = { 5000000000LL, 3 }; return v[0]; }
long long v64_b(void) { v2di v = { 5000000000LL, 3 }; return v[1]; }

static v2di gv64 = { 9000000000LL, -2 };
long long gv64_a(void) { return gv64[0]; }
long long gv64_b(void) { return gv64[1]; }

/* ---- vector frontend cases ---------------------------------------- */

static v4si mkv(void)
{
    v4si a = { 10, 20, 30, 40 };
    return a;
}

/* subscript of a vector rvalue (call result / operation result) */
int vec_rvalue_sub(void) { return mkv()[0] + mkv()[2]; } /* 10 + 30 */
int vec_op_sub(void)
{
    v4si a = { 1, 2, 3, 4 }, b = { 10, 20, 30, 40 };
    return (a + b)[1]; /* 22 */
}

/* a comparison result initializing an unsigned-element vector */
int vec_ucmp_init(void)
{
    v4si a = { 1, 9, 3, 4 }, b = { 4, 3, 5, 1 };
    v4su m = (a < b);
    return (int)m[0] + (int)m[1] + (int)m[2] + (int)m[3]; /* -1+0+-1+0 */
}
