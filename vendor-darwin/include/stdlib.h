/* stdlib.h - general utilities, Darwin.
 *
 * Written for this package from the C99 <stdlib.h> requirements
 * plus the POSIX and BSD functions libSystem provides.  Darwin
 * facts: RAND_MAX is 2^31-1, MB_CUR_MAX reads the exported
 * __mb_cur_max, qsort_r takes the thunk argument first (the BSD
 * order, not the glibc order), and the arc4random family is part
 * of the system library.
 */

#ifndef _TINYC_DARWIN_STDLIB_H
#define _TINYC_DARWIN_STDLIB_H

#include <stddef.h>
#include <sys/types.h>

typedef struct {
	int quot;
	int rem;
} div_t;

typedef struct {
	long quot;
	long rem;
} ldiv_t;

typedef struct {
	long long quot;
	long long rem;
} lldiv_t;

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
#define RAND_MAX	2147483647

extern int __mb_cur_max;
#define MB_CUR_MAX	__mb_cur_max

/* Numeric conversion */
double atof(const char *s);
int atoi(const char *s);
long atol(const char *s);
long long atoll(const char *s);
double strtod(const char *s, char **endp);
float strtof(const char *s, char **endp);
long double strtold(const char *s, char **endp);
long strtol(const char *s, char **endp, int base);
long long strtoll(const char *s, char **endp, int base);
unsigned long strtoul(const char *s, char **endp, int base);
unsigned long long strtoull(const char *s, char **endp, int base);

/* Random numbers */
int rand(void);
void srand(unsigned seed);
long random(void);
void srandom(unsigned seed);
char *initstate(unsigned seed, char *state, size_t n);
char *setstate(char *state);
uint32_t arc4random(void);
void arc4random_buf(void *buf, size_t nbytes);
uint32_t arc4random_uniform(uint32_t upper_bound);
void arc4random_stir(void);
double drand48(void);
long lrand48(void);
long mrand48(void);
void srand48(long seed);
long jrand48(unsigned short xsubi[3]);
void lcong48(unsigned short param[7]);
double erand48(unsigned short xsubi[3]);
long nrand48(unsigned short xsubi[3]);
unsigned short *seed48(unsigned short seed16v[3]);

/* Memory */
void *calloc(size_t nmemb, size_t size);
void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void *valloc(size_t size);
int posix_memalign(void **memptr, size_t alignment, size_t size);
void *aligned_alloc(size_t alignment, size_t size);
void *alloca(size_t size);

/* Process control */
void abort(void);
int atexit(void (*func)(void));
int at_quick_exit(void (*func)(void));
void exit(int status);
void _Exit(int status);
void quick_exit(int status);
char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);
int putenv(char *string);
int system(const char *command);

/* Searching and sorting */
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
	      int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size,
	   int (*compar)(const void *, const void *));
void qsort_r(void *base, size_t nmemb, size_t size, void *thunk,
	     int (*compar)(void *, const void *, const void *));
int heapsort(void *base, size_t nmemb, size_t size,
	     int (*compar)(const void *, const void *));
int mergesort(void *base, size_t nmemb, size_t size,
	      int (*compar)(const void *, const void *));

/* Arithmetic */
int abs(int j);
long labs(long j);
long long llabs(long long j);
div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

/* Multibyte and wide conversion (single-byte state) */
int mblen(const char *s, size_t n);
int mbtowc(wchar_t *pwc, const char *s, size_t n);
int wctomb(char *s, wchar_t wc);
size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n);
size_t wcstombs(char *s, const wchar_t *pwcs, size_t n);

/* Files and directories */
char *realpath(const char *path, char *resolved);
int mkstemp(char *tmpl);
int mkstemps(char *tmpl, int suffixlen);
char *mktemp(char *tmpl);
char *mkdtemp(char *tmpl);
int getsubopt(char **optionp, char * const *tokens, char **valuep);
long a64l(const char *s);
char *l64a(long value);

#endif
