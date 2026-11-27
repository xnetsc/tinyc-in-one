/* regression suite for two wasm-backend code generation bugs:
 *
 * 1. long double is f64 on this target, but double -> long double
 *    conversions were emitted as a demote to f32 and back (losing
 *    precision, overflowing to Inf, flushing subnormals to 0),
 *    and a long double constant materialised into a register read
 *    the wrong half of the frontend's constant union.
 *
 * 2. gfunc_call staged every float argument through the single
 *    REG_FRET local, so an argument that already lived in
 *    REG_FRET - the result of the immediately preceding call -
 *    was overwritten as soon as an earlier argument was staged:
 *    combine(dval(7.5), dval(-2.25)) computed combine(7.5, 7.5).
 *
 * Every function is exported and checked against an exact
 * expected value by run-genfix.mjs. */

double fmod(double x, double y); /* imported; env.fmod in the runner */

/* ---- bug 1: long double conversions and literals ---------------- */

long double ld_from_d(double d) { long double x = d; return x; }
long double ld_cast(double d) { return (long double)d; }
long double ld_id(long double x) { return x; }
double call_ld_id(double d) { return (double)ld_id(d); }
long double ld_lit(void) { return 1.5L; }
long double ld_lit2(void) { return 0.1L; }
double d_lit(void) { long double x = 1.5L; return (double)x; }
long double ld_fold(void) { return 1.5L + 2.25L; }
long double g_ld = 1.5L;
double get_g(void) { return (double)g_ld; }
long double ld_from_f(float f) { long double x = f; return x; }
float f_from_ld(long double x) { return (float)x; }
long double ld_from_i(int i) { long double x = i; return x; }
int i_from_ld(long double x) { return (int)x; }

/* the literal forms that read the wrong constant half: a long
   double literal materialised as a call argument */
double lit_arg(void) { return (double)ld_id(1.5L); }
double lit_arg2(void) { return (double)ld_id(0.1L); }
static long double s_ld = 3.5L;
double get_s(void) { return (double)s_ld; }
double arr_ld(void) { long double a[2] = { 1.5L, 2.5L }; return (double)a[1]; }

/* ---- bug 2: call results as sibling arguments -------------------- */

double dval(double x) { return x; }
double combine(double a, double b) { return a * 1000.0 + b; }
double nested_dd(void) { return combine(dval(7.5), dval(-2.25)); }
double nested_cd(void) { return combine(3.5, dval(-2.25)); }
double nested_dc(void) { return combine(dval(7.5), -2.25); }
double tri(double a, double b, double c) { return a * 10000.0 + b * 100.0 + c; }
double nested3(void) { return tri(dval(1.5), dval(2.5), dval(3.5)); }
double nested_op(void) { return combine(dval(7.5) + 1.0, dval(-2.25)); }
double nested_deep(void) { return combine(combine(dval(1.0), dval(2.0)), dval(3.0)); }

/* the math battery's original shape: both arguments of an
   imported function produced by nested calls */
double dval2(double x) { return x * 1.0; }
double nested_import(void) { return fmod(dval2(-7.5), dval2(2.0)); }

int ival(int x) { return x; }
int icombine(int a, int b) { return a * 1000 + b; }
int nested_ii(void) { return icombine(ival(7), ival(-2)); }

long long llval(long long x) { return x; }
long long llcombine(long long a, long long b) { return a * 1000 + b; }
long long nested_ll(void) { return llcombine(llval(7), llval(-2)); }

float fval(float x) { return x; }
float fcombine(float a, float b) { return a * 1000.0f + b; }
float nested_ff(void) { return fcombine(fval(7.5f), fval(-2.25f)); }
