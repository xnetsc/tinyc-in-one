/* string.h - string handling, Darwin.
 *
 * Written for this package from the C99 <string.h> requirements
 * plus the POSIX and BSD functions libSystem provides (strlcpy
 * and strlcat are BSD functions that Darwin has always carried).
 * Darwin's strerror_r is the XSI form returning int.
 */

#ifndef _TINYC_DARWIN_STRING_H
#define _TINYC_DARWIN_STRING_H

#include <stddef.h>
#include <sys/types.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
int strcmp(const char *s1, const char *s2);
int strcoll(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strxfrm(char *dst, const char *src, size_t n);
void *memchr(const void *s, int c, size_t n);
char *strchr(const char *s, int c);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strrchr(const char *s, int c);
size_t strspn(const char *s, const char *accept);
char *strstr(const char *haystack, const char *needle);
char *strtok(char *s, const char *delim);
char *strtok_r(char *s, const char *delim, char **savep);
void *memset(void *s, int c, size_t n);
char *strerror(int errnum);
int strerror_r(int errnum, char *buf, size_t buflen);
size_t strlen(const char *s);

/* POSIX and BSD additions provided by libSystem. */
void *memccpy(void *dst, const void *src, int c, size_t n);
char *stpcpy(char *dst, const char *src);
char *stpncpy(char *dst, const char *src, size_t n);
char *strdup(const char *s);
char *strndup(const char *s, size_t n);
size_t strnlen(const char *s, size_t maxlen);
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
char *strnstr(const char *haystack, const char *needle, size_t len);
void *memmem(const void *haystack, size_t hlen,
	     const void *needle, size_t nlen);
char *strsignal(int sig);
void *memset_pattern4(void *s, const void *pattern4, size_t n);
void *memset_pattern8(void *s, const void *pattern8, size_t n);
void *memset_pattern16(void *s, const void *pattern16, size_t n);

#endif
