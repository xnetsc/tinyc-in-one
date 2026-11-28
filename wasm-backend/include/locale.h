/* locale.h — localization for the wasm32 sysroot.
 *
 * This target has exactly one locale, "C" (also spelled
 * "POSIX"): the shim layer's conversions are locale-free, so
 * setlocale accepts that locale (and answers queries) and
 * reports failure for any other name, and localeconv returns
 * the "C" values.  That is a conforming single-locale
 * implementation, not a placeholder: the values below are the
 * ones the runtime actually observes.
 */
#ifndef _TINYC_LOCALE_H
#define _TINYC_LOCALE_H

#include <stddef.h>

#define LC_ALL		0
#define LC_COLLATE	1
#define LC_CTYPE	2
#define LC_MONETARY	3
#define LC_NUMERIC	4
#define LC_TIME		5

struct lconv {
	char *decimal_point;
	char *thousands_sep;
	char *grouping;
	char *int_curr_symbol;
	char *currency_symbol;
	char *mon_decimal_point;
	char *mon_thousands_sep;
	char *mon_grouping;
	char *positive_sign;
	char *negative_sign;
	char int_frac_digits;
	char frac_digits;
	char p_cs_precedes;
	char p_sep_by_space;
	char n_cs_precedes;
	char n_sep_by_space;
	char p_sign_posn;
	char n_sign_posn;
};

static inline int __tinyc_locale_eq(const char *a, const char *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}

static inline int __tinyc_locale_is_c(const char *locale)
{
	return __tinyc_locale_eq(locale, "C") ||
		__tinyc_locale_eq(locale, "POSIX");
}

static inline char *setlocale(int category, const char *locale)
{
	(void)category;
	if (locale == NULL)
		return (char *)"C";
	if (locale[0] == '\0' || __tinyc_locale_is_c(locale))
		return (char *)"C";
	return NULL;
}

static inline struct lconv *localeconv(void)
{
	static struct lconv conv;
	static int filled;
	if (!filled) {
		conv.decimal_point = (char *)".";
		conv.thousands_sep = (char *)"";
		conv.grouping = (char *)"";
		conv.int_curr_symbol = (char *)"";
		conv.currency_symbol = (char *)"";
		conv.mon_decimal_point = (char *)"";
		conv.mon_thousands_sep = (char *)"";
		conv.mon_grouping = (char *)"";
		conv.positive_sign = (char *)"";
		conv.negative_sign = (char *)"";
		conv.int_frac_digits = 127;
		conv.frac_digits = 127;
		conv.p_cs_precedes = 127;
		conv.p_sep_by_space = 127;
		conv.n_cs_precedes = 127;
		conv.n_sep_by_space = 127;
		conv.p_sign_posn = 127;
		conv.n_sign_posn = 127;
		filled = 1;
	}
	return &conv;
}

#endif /* _TINYC_LOCALE_H */
