/* ctype.h - character classification, Darwin.
 *
 * Written for this package from the C99 <ctype.h> requirements.
 * The classification functions are real functions in libSystem,
 * so this header only declares them; no table access is needed.
 */

#ifndef _TINYC_DARWIN_CTYPE_H
#define _TINYC_DARWIN_CTYPE_H

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

int digittoint(int c);
int ishexnumber(int c);
int isideogram(int c);
int isnumber(int c);
int isphonogram(int c);
int isrune(int c);
int isspecial(int c);

#define isascii(c)	((unsigned)(c) <= 0177)
#define toascii(c)	((c) & 0177)

#endif
