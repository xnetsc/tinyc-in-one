/* runetype.h - rune locale tables, Darwin.
 *
 * Written for this package from Darwin's published ABI facts.
 * The C library keeps the current rune locale in the
 * _DefaultRuneLocale pointer, which the platform declares in
 * this header (ctype.h reaches it through the platform's
 * ctype/_ctype/runetype include chain).  Only the pointer
 * itself is declared here: bcheck.c takes its address and its
 * size, never the table behind it, so the locale struct stays
 * an opaque forward declaration.
 */

#ifndef _TINYC_DARWIN_RUNETYPE_H
#define _TINYC_DARWIN_RUNETYPE_H

struct _RuneLocale;
typedef struct _RuneLocale _RuneLocale;

extern _RuneLocale *_DefaultRuneLocale;

#endif
