#if defined(__wasm__) || defined(__wasm32__)

#ifndef _CTYPE_H
#define _CTYPE_H

/* ctype.h - character handling for the tinyc-web freestanding
   sysroot.
 *
 * The functions below are provided by the host shim layer as
 * env imports, table-driven over the C locale's 256 byte
 * values; there is no other locale in the process model. */

int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);

#endif /* _CTYPE_H */

#else

/* This header ships in the wasm target's include set.  For any
   other target the system's own ctype.h is the right one. */
#include_next <ctype.h>

#endif
