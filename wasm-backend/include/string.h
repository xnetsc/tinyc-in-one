#if defined(__wasm__) || defined(__wasm32__)

#ifndef _STRING_H
#define _STRING_H

/* string.h - string handling for the tinyc-web freestanding
   sysroot.
 *
 * The functions below are provided by the host shim layer as
 * env imports and operate directly on the guest's linear
 * memory.  There is no locale in the process model: strcoll
 * compares as strcmp does, and strxfrm copies as the C locale
 * prescribes. */

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memchr(const void *s, int c, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
char *strdup(const char *s);

size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strtok(char *str, const char *delim);
char *strerror(int errnum);
int strcoll(const char *s1, const char *s2);
size_t strxfrm(char *dest, const char *src, size_t n);

#endif /* _STRING_H */

#else

/* This header ships in the wasm target's include set.  For any
   other target the system's own string.h is the right one. */
#include_next <string.h>

#endif
