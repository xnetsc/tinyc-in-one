/* probe_wasm_cstd.c - the wasm32 C-standard fills: <assert.h>,
 * <locale.h>, <fenv.h>, tmpnam and timespec_get, used the
 * ordinary way.  (The <uchar.h> surface is covered by
 * probe_uchar.c, compiled for wasm32 as well.) */
#include <assert.h>
#include <locale.h>
#include <fenv.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int fails;
#define CHECK(cond) do { if (!(cond)) { \
	printf("wasm cstd probe: FAIL line %d: %s\n", __LINE__, #cond); \
	fails++; } } while (0)

int main(void)
{
	/* assert: a true assertion passes control through */
	assert(1 == 1);
	CHECK(1);

	/* locale: the single "C" locale */
	CHECK(strcmp(setlocale(LC_ALL, NULL), "C") == 0);
	CHECK(strcmp(setlocale(LC_ALL, "C"), "C") == 0);
	CHECK(strcmp(setlocale(LC_NUMERIC, "POSIX"), "C") == 0);
	CHECK(setlocale(LC_ALL, "xx_YY.UTF-8") == NULL);
	CHECK(strcmp(localeconv()->decimal_point, ".") == 0);
	CHECK(localeconv()->frac_digits == 127);

	/* fenv: the fixed environment */
	CHECK(fegetround() == FE_TONEAREST);
	CHECK(fesetround(FE_TONEAREST) == 0);
	CHECK(fesetround(12345) != 0);
	CHECK(fetestexcept(FE_ALL_EXCEPT) == 0);
	CHECK(feclearexcept(FE_ALL_EXCEPT) == 0);
	CHECK(feraiseexcept(0) == 0);
	CHECK(feraiseexcept(FE_INEXACT) != 0);
	{
		fenv_t env;
		fexcept_t flags = 77;
		CHECK(fegetenv(&env) == 0);
		CHECK(fesetenv(&env) == 0);
		CHECK(fesetenv(FE_DFL_ENV) == 0);
		CHECK(fegetexceptflag(&flags, FE_ALL_EXCEPT) == 0);
		CHECK(flags == 0);
	}

	/* tmpnam: distinct names, usable as file names */
	{
		char buf[L_tmpnam];
		char *a = tmpnam(NULL);
		char *b;
		CHECK(a != NULL && a[0] == '/');
		b = tmpnam(buf);
		CHECK(b == buf);
		CHECK(strcmp(a, b) != 0);
		{
			FILE *f = fopen(a, "w");
			CHECK(f != NULL);
			if (f) {
				fputs("x", f);
				fclose(f);
				CHECK(remove(a) == 0);
			}
		}
	}

	/* timespec_get: TIME_UTC is the wall clock */
	{
		struct timespec ts;
		CHECK(timespec_get(&ts, TIME_UTC) == TIME_UTC);
		CHECK(ts.tv_sec > 1700000000);	/* after 2023-11 */
		CHECK(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000);
		CHECK(timespec_get(&ts, 999) == 0);
		CHECK(difftime(5, 3) == 2.0);
	}

	if (fails)
		return 1;
	printf("wasm cstd probe: OK\n");
	return 0;
}
