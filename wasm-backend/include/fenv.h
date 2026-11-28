/* fenv.h — floating-point environment for the wasm32 sysroot.
 *
 * The wasm floating-point environment is fixed by the
 * instruction set: arithmetic is always round-to-nearest, no
 * exception flags are ever raised, and there are no traps.
 * This header describes exactly that fixed environment rather
 * than simulating a variable one: fegetround always reports
 * FE_TONEAREST, the exception flags always read as clear, and
 * an operation that would change the environment (a different
 * rounding mode, raising or installing exception flags)
 * reports failure instead of pretending to succeed.
 */
#ifndef _TINYC_FENV_H
#define _TINYC_FENV_H

typedef unsigned int fexcept_t;

typedef struct {
	unsigned int __fixed;	/* the environment has one state */
} fenv_t;

#define FE_INVALID	0x01
#define FE_DIVBYZERO	0x02
#define FE_OVERFLOW	0x04
#define FE_UNDERFLOW	0x08
#define FE_INEXACT	0x10
#define FE_ALL_EXCEPT	0x1F

#define FE_TONEAREST	0

/* The default environment is the only environment; the
 * accessors accept any env pointer, this one included, and
 * report the fixed state. */
#define FE_DFL_ENV	((const fenv_t *)0)

static inline int feclearexcept(int excepts)
{
	(void)excepts;		/* the flags are always clear */
	return 0;
}

static inline int fegetexceptflag(fexcept_t *flagp, int excepts)
{
	(void)excepts;
	*flagp = 0;
	return 0;
}

static inline int feraiseexcept(int excepts)
{
	/* No flag can be raised in this environment. */
	return excepts == 0 ? 0 : 1;
}

static inline int fesetexceptflag(const fexcept_t *flagp, int excepts)
{
	(void)excepts;
	return *flagp == 0 ? 0 : 1;
}

static inline int fetestexcept(int excepts)
{
	(void)excepts;
	return 0;
}

static inline int fegetround(void)
{
	return FE_TONEAREST;
}

static inline int fesetround(int round)
{
	return round == FE_TONEAREST ? 0 : 1;
}

static inline int fegetenv(fenv_t *envp)
{
	envp->__fixed = 0;
	return 0;
}

static inline int feholdexcept(fenv_t *envp)
{
	envp->__fixed = 0;	/* already non-stop, flags clear */
	return 0;
}

static inline int fesetenv(const fenv_t *envp)
{
	(void)envp;		/* one environment: already in effect */
	return 0;
}

static inline int feupdateenv(const fenv_t *envp)
{
	(void)envp;
	return 0;
}

#endif /* _TINYC_FENV_H */
