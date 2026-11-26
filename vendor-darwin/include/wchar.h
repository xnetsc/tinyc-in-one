/* wchar.h - wide character handling, Darwin.
 *
 * Written for this package from the C99 <wchar.h> requirements
 * plus the POSIX wide functions libSystem provides.  Darwin
 * facts: wint_t is int (so WEOF is -1 as an int constant),
 * mbstate_t is a 128-byte union the conversion functions keep
 * their state in, and there is no open_wmemstream.
 */

#ifndef _TINYC_DARWIN_WCHAR_H
#define _TINYC_DARWIN_WCHAR_H

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

typedef int wint_t;

typedef union {
	char __mbstate8[128];
	long long __mbstateL;
} mbstate_t;

#define WEOF	(-1)

/* Wide formatted input/output */
int fwprintf(FILE *stream, const wchar_t *format, ...);
int fwscanf(FILE *stream, const wchar_t *format, ...);
int swprintf(wchar_t *s, size_t n, const wchar_t *format, ...);
int swscanf(const wchar_t *s, const wchar_t *format, ...);
int vfwprintf(FILE *stream, const wchar_t *format, va_list ap);
int vfwscanf(FILE *stream, const wchar_t *format, va_list ap);
int vswprintf(wchar_t *s, size_t n, const wchar_t *format, va_list ap);
int vswscanf(const wchar_t *s, const wchar_t *format, va_list ap);
int vwprintf(const wchar_t *format, va_list ap);
int vwscanf(const wchar_t *format, va_list ap);
int wprintf(const wchar_t *format, ...);
int wscanf(const wchar_t *format, ...);

/* Wide character input/output */
wint_t fgetwc(FILE *stream);
wchar_t *fgetws(wchar_t *s, int n, FILE *stream);
wint_t fputwc(wchar_t c, FILE *stream);
int fputws(const wchar_t *s, FILE *stream);
wint_t getwc(FILE *stream);
wint_t getwchar(void);
wint_t putwc(wchar_t c, FILE *stream);
wint_t putwchar(wchar_t c);
wint_t ungetwc(wint_t c, FILE *stream);
int fwide(FILE *stream, int mode);

/* Wide string utilities */
double wcstod(const wchar_t *s, wchar_t **endp);
float wcstof(const wchar_t *s, wchar_t **endp);
long double wcstold(const wchar_t *s, wchar_t **endp);
long wcstol(const wchar_t *s, wchar_t **endp, int base);
long long wcstoll(const wchar_t *s, wchar_t **endp, int base);
unsigned long wcstoul(const wchar_t *s, wchar_t **endp, int base);
unsigned long long wcstoull(const wchar_t *s, wchar_t **endp, int base);
wchar_t *wcscpy(wchar_t *dst, const wchar_t *src);
wchar_t *wcsncpy(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wcscat(wchar_t *dst, const wchar_t *src);
wchar_t *wcsncat(wchar_t *dst, const wchar_t *src, size_t n);
int wcscmp(const wchar_t *s1, const wchar_t *s2);
int wcscoll(const wchar_t *s1, const wchar_t *s2);
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n);
size_t wcsxfrm(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wcschr(const wchar_t *s, wchar_t c);
size_t wcscspn(const wchar_t *s, const wchar_t *reject);
wchar_t *wcspbrk(const wchar_t *s, const wchar_t *accept);
wchar_t *wcsrchr(const wchar_t *s, wchar_t c);
size_t wcsspn(const wchar_t *s, const wchar_t *accept);
wchar_t *wcsstr(const wchar_t *haystack, const wchar_t *needle);
wchar_t *wcstok(wchar_t *s, const wchar_t *delim, wchar_t **savep);
size_t wcslen(const wchar_t *s);
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n);
int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n);
wchar_t *wmemcpy(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wmemmove(wchar_t *dst, const wchar_t *src, size_t n);
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n);
size_t wcsnlen(const wchar_t *s, size_t maxlen);
wchar_t *wcsdup(const wchar_t *s);

/* Conversion between multibyte and wide */
wint_t btowc(int c);
int wctob(wint_t c);
int mbsinit(const mbstate_t *ps);
size_t mbrlen(const char *s, size_t n, mbstate_t *ps);
size_t mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps);
size_t wcrtomb(char *s, wchar_t wc, mbstate_t *ps);
size_t mbsrtowcs(wchar_t *dst, const char **src, size_t len, mbstate_t *ps);
size_t mbsnrtowcs(wchar_t *dst, const char **src, size_t nmc, size_t len,
		  mbstate_t *ps);
size_t wcsrtombs(char *dst, const wchar_t **src, size_t len, mbstate_t *ps);
size_t wcsnrtombs(char *dst, const wchar_t **src, size_t nwc, size_t len,
		  mbstate_t *ps);

/* Wide time formatting */
size_t wcsftime(wchar_t *s, size_t maxsize, const wchar_t *format,
		const struct tm *timeptr);

#endif
