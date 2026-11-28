#if defined(__wasm__) || defined(__wasm32__)

#ifndef _STDLIB_H
#define _STDLIB_H

/* stdlib.h - general utilities for the tinyc-web freestanding
   sysroot.
 *
 * The functions below are provided by the host shim layer as
 * env imports.  Allocation is served by the layer's allocator
 * inside the guest's linear memory (or forwarded to the
 * module's own allocator when the module exports one).  exit
 * unwinds the guest without terminating the host and runs
 * atexit handlers; abort unwinds the same way with the
 * conventional failure code.  getenv reads the environment the
 * process was created with and returns NULL for a name it does
 * not carry.  There is no command processor — system(NULL)
 * returns 0 (none available) and any command returns -1 with
 * errno set to ENOSYS.  long is 32-bit and long double is
 * double in this target's ABI, so the strto* surface ends at
 * the forms below.  The div family returns its struct by
 * value: div_t and ldiv_t are 8 bytes and travel as two i32
 * results, lldiv_t is 16 bytes and travels through a hidden
 * first-argument pointer — the compiler's wasm calling
 * convention for struct returns, which the host layer's
 * implementations follow. */

#include <stddef.h>

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
/* rand's range: the generator returns 15-bit values. */
#define RAND_MAX	32767

/* Allocation */
void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

/* Process control */
void exit(int status);
void abort(void);
int atexit(void (*func)(void));
char *getenv(const char *name);
int system(const char *command);

/* String conversion */
int atoi(const char *nptr);
long atol(const char *nptr);
long long atoll(const char *nptr);
double atof(const char *nptr);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);
long long strtoll(const char *nptr, char **endptr, int base);
unsigned long long strtoull(const char *nptr, char **endptr, int base);
double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);

/* Arithmetic */
int abs(int j);
long labs(long j);
long long llabs(long long j);

typedef struct { int quot; int rem; } div_t;
typedef struct { long quot; long rem; } ldiv_t;
typedef struct { long long quot; long long rem; } lldiv_t;
div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);
lldiv_t lldiv(long long numer, long long denom);

/* Searching and sorting.  The comparator is called through the
   module's indirect function table, as in the host layer. */
void *bsearch(const void *key, const void *base, size_t nmemb,
	      size_t size, int (*compar)(const void *, const void *));
void qsort(void *base, size_t nmemb, size_t size,
	   int (*compar)(const void *, const void *));

/* Pseudo-random numbers */
int rand(void);
void srand(unsigned int seed);

#endif /* _STDLIB_H */

#else

/* This header ships in the wasm target's include set.  For any
   other target the system's own stdlib.h is the right one. */
#include_next <stdlib.h>

#endif
