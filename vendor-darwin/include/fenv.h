/* fenv.h - floating-point environment, Darwin.
 *
 * Written for this package from the C99 <fenv.h> requirements
 * and Darwin's published ABI facts.  The platform's fenv
 * implementation descends from the FreeBSD one: the exception
 * flags and rounding values below are the control-word
 * encodings the platform's fesetround writes, not the order
 * the C standard lists the names in.  Two Darwin differences
 * from the FreeBSD original: there is no FE_DENORMAL (the
 * platform man page lists only the five exception macros
 * below), and the default-environment object is _FE_DFL_ENV.
 * fenv_t is opaque to programs -- fegetenv/fesetenv pass it
 * to libSystem -- but its size is ABI, so the probe pins it.
 */

#ifndef _TINYC_DARWIN_FENV_H
#define _TINYC_DARWIN_FENV_H

#if defined(__x86_64__)
/* x87 status-word exception bits (FE_DENORMAL, 0x02, is not
 * defined on Darwin); the rounding values are the x87
 * control-word rounding-control field, bits 10-11, which the
 * platform's fesetround also writes, shifted, into the SSE
 * MXCSR. */
#define FE_INVALID	0x01
#define FE_DIVBYZERO	0x04
#define FE_OVERFLOW	0x08
#define FE_UNDERFLOW	0x10
#define FE_INEXACT	0x20
#define FE_ALL_EXCEPT	0x3f

#define FE_TONEAREST	0x000
#define FE_DOWNWARD	0x400
#define FE_UPWARD	0x800
#define FE_TOWARDZERO	0xc00

/* fenv_t is the x87 environment held as 32-bit fields plus
 * the SSE MXCSR: 32 bytes. */
typedef struct {
	struct {
		unsigned int	__control;
		unsigned int	__status;
		unsigned int	__tag;
		unsigned int	__others[4];
	} __x87;
	unsigned int	__mxcsr;
} fenv_t;

#elif defined(__aarch64__)
/* ARM FPSR exception-flag bits; the rounding values are the
 * FPCR rounding-mode field. */
#define FE_INVALID	0x01
#define FE_DIVBYZERO	0x02
#define FE_OVERFLOW	0x04
#define FE_UNDERFLOW	0x08
#define FE_INEXACT	0x10
#define FE_ALL_EXCEPT	0x1f

#define FE_TONEAREST	0
#define FE_UPWARD	1
#define FE_DOWNWARD	2
#define FE_TOWARDZERO	3

/* fenv_t is the ARM floating-point status and control
 * registers: 8 bytes. */
typedef struct {
	unsigned int	__fpsr;
	unsigned int	__fpcr;
} fenv_t;

#else
#error "fenv.h is only written for the two 64-bit Darwin targets"
#endif

typedef unsigned int fexcept_t;

extern const fenv_t _FE_DFL_ENV;
#define FE_DFL_ENV (&_FE_DFL_ENV)

int feclearexcept(int);
int fegetexceptflag(fexcept_t *, int);
int feraiseexcept(int);
int fesetexceptflag(const fexcept_t *, int);
int fetestexcept(int);
int fegetround(void);
int fesetround(int);
int fegetenv(fenv_t *);
int fesetenv(const fenv_t *);
int feholdexcept(fenv_t *);
int feupdateenv(const fenv_t *);

#endif
