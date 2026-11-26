/* math.h - floating-point math, Darwin.
 *
 * Written for this package from the C99 <math.h> requirements:
 * the double, float and long double forms of the standard
 * functions, all of which libSystem provides (the math library is
 * part of libSystem on Darwin; there is no separate libm file at
 * run time).  float_t and double_t are float and double: the
 * targets evaluate float and double expressions in their own
 * precision (FLT_EVAL_METHOD == 0).  fpclassify dispatches to the
 * per-type __fpclassify functions libSystem exports, the same
 * mechanism the platform headers use.
 */

#ifndef _TINYC_DARWIN_MATH_H
#define _TINYC_DARWIN_MATH_H

typedef float		float_t;
typedef double		double_t;

#define HUGE_VAL	__builtin_huge_val()
#define HUGE_VALF	__builtin_huge_valf()
#define HUGE_VALL	__builtin_huge_vall()
#define INFINITY	__builtin_huge_valf()
#define NAN		__builtin_nanf("")

/* Classification values returned by __fpclassify{,f,l}: these are
 * Apple's enum values (FP_NAN = 1, FP_INFINITE = 2, FP_ZERO = 3,
 * FP_NORMAL = 4, FP_SUBNORMAL = 5), which are the ABI of the
 * libSystem classifiers, not the glibc numbering and not the
 * order the C standard lists the names in. */
#define FP_NAN		1
#define FP_INFINITE	2
#define FP_ZERO		3
#define FP_NORMAL	4
#define FP_SUBNORMAL	5
#define FP_ILOGB0	(-2147483647-1)
#define FP_ILOGBNAN	2147483647

#define MATH_ERRNO	1
#define MATH_ERREXCEPT	2
#define math_errhandling (MATH_ERRNO | MATH_ERREXCEPT)

int __fpclassifyf(float);
int __fpclassifyd(double);
int __fpclassifyl(long double);

#define fpclassify(x) \
	(sizeof(x) == sizeof(float) ? __fpclassifyf(x) \
	 : sizeof(x) == sizeof(double) ? __fpclassifyd(x) \
	 : __fpclassifyl(x))
#define isnan(x)	((x) != (x))
#define isinf(x)	((x) == __builtin_huge_val() || \
			 (x) == -__builtin_huge_val())
#define isfinite(x)	(!isnan(x) && !isinf(x))
#define isnormal(x)	(fpclassify(x) == FP_NORMAL)
#define signbit(x)	((x) < 0 || (1.0 / (x)) < 0)

#define isgreater(x, y)		((x) > (y))
#define isgreaterequal(x, y)	((x) >= (y))
#define isless(x, y)		((x) < (y))
#define islessequal(x, y)	((x) <= (y))
#define islessgreater(x, y)	((x) < (y) || (x) > (y))
#define isunordered(x, y)	(isnan(x) || isnan(y))

extern int signgam;

/* Trigonometric functions */
double acos(double);
float acosf(float);
long double acosl(long double);
double asin(double);
float asinf(float);
long double asinl(long double);
double atan(double);
float atanf(float);
long double atanl(long double);
double atan2(double, double);
float atan2f(float, float);
long double atan2l(long double, long double);
double cos(double);
float cosf(float);
long double cosl(long double);
double sin(double);
float sinf(float);
long double sinl(long double);
double tan(double);
float tanf(float);
long double tanl(long double);

/* Hyperbolic functions */
double acosh(double);
float acoshf(float);
long double acoshl(long double);
double asinh(double);
float asinhf(float);
long double asinhl(long double);
double atanh(double);
float atanhf(float);
long double atanhl(long double);
double cosh(double);
float coshf(float);
long double coshl(long double);
double sinh(double);
float sinhf(float);
long double sinhl(long double);
double tanh(double);
float tanhf(float);
long double tanhl(long double);

/* Exponential and logarithmic functions */
double exp(double);
float expf(float);
long double expl(long double);
double exp2(double);
float exp2f(float);
long double exp2l(long double);
double expm1(double);
float expm1f(float);
long double expm1l(long double);
double frexp(double, int *);
float frexpf(float, int *);
long double frexpl(long double, int *);
int ilogb(double);
int ilogbf(float);
int ilogbl(long double);
double ldexp(double, int);
float ldexpf(float, int);
long double ldexpl(long double, int);
double log(double);
float logf(float);
long double logl(long double);
double log10(double);
float log10f(float);
long double log10l(long double);
double log1p(double);
float log1pf(float);
long double log1pl(long double);
double log2(double);
float log2f(float);
long double log2l(long double);
double logb(double);
float logbf(float);
long double logbl(long double);
double modf(double, double *);
float modff(float, float *);
long double modfl(long double, long double *);
double scalbn(double, int);
float scalbnf(float, int);
long double scalbnl(long double, int);
double scalbln(double, long);
float scalblnf(float, long);
long double scalblnl(long double, long);

/* Power and absolute value */
double cbrt(double);
float cbrtf(float);
long double cbrtl(long double);
double fabs(double);
float fabsf(float);
long double fabsl(long double);
double hypot(double, double);
float hypotf(float, float);
long double hypotl(long double, long double);
double pow(double, double);
float powf(float, float);
long double powl(long double, long double);
double sqrt(double);
float sqrtf(float);
long double sqrtl(long double);

/* Error and gamma functions */
double erf(double);
float erff(float);
long double erfl(long double);
double erfc(double);
float erfcf(float);
long double erfcl(long double);
double lgamma(double);
float lgammaf(float);
long double lgammal(long double);
double tgamma(double);
float tgammaf(float);
long double tgammal(long double);

/* Nearest integer */
double ceil(double);
float ceilf(float);
long double ceill(long double);
double floor(double);
float floorf(float);
long double floorl(long double);
double nearbyint(double);
float nearbyintf(float);
long double nearbyintl(long double);
double rint(double);
float rintf(float);
long double rintl(long double);
long lrint(double);
long lrintf(float);
long lrintl(long double);
long long llrint(double);
long long llrintf(float);
long long llrintl(long double);
double round(double);
float roundf(float);
long double roundl(long double);
long lround(double);
long lroundf(float);
long lroundl(long double);
long long llround(double);
long long llroundf(float);
long long llroundl(long double);
double trunc(double);
float truncf(float);
long double truncl(long double);

/* Remainder */
double fmod(double, double);
float fmodf(float, float);
long double fmodl(long double, long double);
double remainder(double, double);
float remainderf(float, float);
long double remainderl(long double, long double);
double remquo(double, double, int *);
float remquof(float, float, int *);
long double remquol(long double, long double, int *);

/* Manipulation */
double copysign(double, double);
float copysignf(float, float);
long double copysignl(long double, long double);
double nan(const char *);
float nanf(const char *);
long double nanl(const char *);
double nextafter(double, double);
float nextafterf(float, float);
long double nextafterl(long double, long double);
double nexttoward(double, long double);
float nexttowardf(float, long double);
long double nexttowardl(long double, long double);

/* Maximum, minimum, positive difference */
double fdim(double, double);
float fdimf(float, float);
long double fdiml(long double, long double);
double fmax(double, double);
float fmaxf(float, float);
long double fmaxl(long double, long double);
double fmin(double, double);
float fminf(float, float);
long double fminl(long double, long double);

/* Fused multiply-add */
double fma(double, double, double);
float fmaf(float, float, float);
long double fmal(long double, long double, long double);

/* Bessel functions (BSD additions provided by libSystem) */
double j0(double);
double j1(double);
double jn(int, double);
double y0(double);
double y1(double);
double yn(int, double);

#endif
