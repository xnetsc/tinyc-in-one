/* uchar.h — Unicode characters for the wasm32 sysroot.
 *
 * The four conversion functions are a direct UTF-8 codec
 * implemented in this header: the multibyte encoding on this
 * target is UTF-8 (the shim layer's strings are UTF-8), and
 * the conversions are pure functions of the input and the
 * caller-supplied state, so no runtime surface is needed
 * behind them.  The all-zero mbstate_t is the initial state.
 *
 * Return conventions follow C11 7.28: (size_t)-1 on an
 * invalid sequence (this target has no errno; see the errno
 * entry in the sysroot's header inventory), (size_t)-2 when the input ends in
 * the middle of a multibyte character, and (size_t)-3 from
 * mbrtoc16 when it outputs the second half of a surrogate
 * pair without consuming input.
 */
#ifndef _TINYC_UCHAR_H
#define _TINYC_UCHAR_H

#include <stddef.h>
#include <stdint.h>

typedef uint_least16_t char16_t;
typedef uint_least32_t char32_t;

typedef struct {
	uint32_t __acc;		/* partial UTF-8 decode accumulator */
	uint32_t __mincp;	/* smallest value the open sequence may decode to */
	uint16_t __pend;	/* pending low surrogate (mbrtoc16) */
	uint16_t __hi;		/* pending high surrogate (c16rtomb) */
	uint8_t __ncont;	/* continuation bytes still expected */
	uint8_t __has_pend;
	uint8_t __has_hi;
	uint8_t __pad;
} mbstate_t;

static void __tinyc_uchar_reset(mbstate_t *st)
{
	st->__acc = 0;
	st->__mincp = 0;
	st->__pend = 0;
	st->__hi = 0;
	st->__ncont = 0;
	st->__has_pend = 0;
	st->__has_hi = 0;
	st->__pad = 0;
}

/* Feed one byte to the UTF-8 decoder in *st.  Returns 1 and
 * sets *cp when a code point completes, 0 when more bytes are
 * needed, -1 on an invalid byte. */
static int __tinyc_utf8_feed(mbstate_t *st, unsigned char b, uint32_t *cp)
{
	if (st->__ncont == 0) {
		if (b < 0x80) {
			*cp = b;
			return 1;
		}
		if (b >= 0xC2 && b <= 0xDF) {
			st->__acc = b & 0x1F;
			st->__ncont = 1;
			st->__mincp = 0x80;
			return 0;
		}
		if (b >= 0xE0 && b <= 0xEF) {
			st->__acc = b & 0x0F;
			st->__ncont = 2;
			st->__mincp = 0x800;
			return 0;
		}
		if (b >= 0xF0 && b <= 0xF4) {
			st->__acc = b & 0x07;
			st->__ncont = 3;
			st->__mincp = 0x10000;
			return 0;
		}
		return -1;
	}
	if ((b & 0xC0) != 0x80)
		return -1;
	st->__acc = (st->__acc << 6) | (b & 0x3F);
	if (--st->__ncont > 0)
		return 0;
	*cp = st->__acc;
	st->__acc = 0;
	/* Reject overlong forms, surrogates and values past
	 * U+10FFFF, which the lead-byte ranges alone allow. */
	if (*cp < st->__mincp || (*cp >= 0xD800 && *cp <= 0xDFFF) ||
	    *cp > 0x10FFFF)
		return -1;
	st->__mincp = 0;
	return 1;
}

/* Decode one code point from s[0..n).  Returns the number of
 * bytes consumed (>0), 0 for a completed null character,
 * (size_t)-1 on error, (size_t)-2 on incomplete input. */
static size_t __tinyc_utf8_decode(mbstate_t *st, const char *s,
				  size_t n, uint32_t *cp)
{
	size_t i;
	if (st->__ncont == 0 && n > 0 && s[0] == '\0') {
		*cp = 0;
		__tinyc_uchar_reset(st);
		return 0;
	}
	for (i = 0; i < n; i++) {
		int r = __tinyc_utf8_feed(st, (unsigned char)s[i], cp);
		if (r < 0) {
			__tinyc_uchar_reset(st);
			return (size_t)-1;
		}
		if (r == 1) {
			if (*cp == 0) {
				__tinyc_uchar_reset(st);
				return 0;
			}
			return i + 1;
		}
	}
	return (size_t)-2;
}

/* Encode one code point as UTF-8 into s (at most 4 bytes).
 * Returns the byte count, or (size_t)-1 for a value that is
 * not a valid scalar value. */
static size_t __tinyc_utf8_encode(char *s, uint32_t cp)
{
	if (cp < 0x80) {
		s[0] = (char)cp;
		return 1;
	}
	if (cp < 0x800) {
		s[0] = (char)(0xC0 | (cp >> 6));
		s[1] = (char)(0x80 | (cp & 0x3F));
		return 2;
	}
	if (cp >= 0xD800 && cp <= 0xDFFF)
		return (size_t)-1;
	if (cp < 0x10000) {
		s[0] = (char)(0xE0 | (cp >> 12));
		s[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		s[2] = (char)(0x80 | (cp & 0x3F));
		return 3;
	}
	if (cp <= 0x10FFFF) {
		s[0] = (char)(0xF0 | (cp >> 18));
		s[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		s[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		s[3] = (char)(0x80 | (cp & 0x3F));
		return 4;
	}
	return (size_t)-1;
}

static size_t mbrtoc32(char32_t *pc32, const char *s, size_t n,
		       mbstate_t *ps)
{
	static mbstate_t internal;
	uint32_t cp = 0;
	size_t r;
	if (!ps)
		ps = &internal;
	if (!s) {
		int pending = ps->__acc != 0 || ps->__ncont != 0 ||
			ps->__has_pend || ps->__has_hi;
		__tinyc_uchar_reset(ps);
		return pending;
	}
	r = __tinyc_utf8_decode(ps, s, n, &cp);
	if (r != (size_t)-1 && r != (size_t)-2 && pc32)
		*pc32 = (char32_t)cp;
	return r;
}

static size_t mbrtoc16(char16_t *pc16, const char *s, size_t n,
		       mbstate_t *ps)
{
	static mbstate_t internal;
	uint32_t cp = 0;
	size_t r;
	if (!ps)
		ps = &internal;
	if (!s) {
		int pending = ps->__acc != 0 || ps->__ncont != 0 ||
			ps->__has_pend || ps->__has_hi;
		__tinyc_uchar_reset(ps);
		return pending;
	}
	if (ps->__has_pend) {
		if (pc16)
			*pc16 = (char16_t)ps->__pend;
		ps->__pend = 0;
		ps->__has_pend = 0;
		return (size_t)-3;
	}
	r = __tinyc_utf8_decode(ps, s, n, &cp);
	if (r == (size_t)-1 || r == (size_t)-2)
		return r;
	if (cp >= 0x10000) {
		uint32_t v = cp - 0x10000;
		ps->__pend = (uint16_t)(0xDC00 + (v & 0x3FF));
		ps->__has_pend = 1;
		if (pc16)
			*pc16 = (char16_t)(0xD800 + (v >> 10));
	} else if (pc16) {
		*pc16 = (char16_t)cp;
	}
	return r;
}

static size_t c32rtomb(char *s, char32_t c32, mbstate_t *ps)
{
	char buf[4];
	size_t r;
	(void)ps;		/* the encode side keeps no state */
	if (!s)
		s = buf;
	r = __tinyc_utf8_encode(s, (uint32_t)c32);
	return r;
}

static size_t c16rtomb(char *s, char16_t c16, mbstate_t *ps)
{
	static mbstate_t internal;
	char buf[4];
	uint32_t cp;
	size_t r;
	if (!ps)
		ps = &internal;
	if (!s)
		s = buf;
	if (ps->__has_hi) {
		if (c16 < 0xDC00 || c16 > 0xDFFF) {
			__tinyc_uchar_reset(ps);
			return (size_t)-1;
		}
		cp = 0x10000 + (((uint32_t)ps->__hi - 0xD800) << 10) +
			((uint32_t)c16 - 0xDC00);
		ps->__hi = 0;
		ps->__has_hi = 0;
	} else if (c16 >= 0xD800 && c16 <= 0xDBFF) {
		ps->__hi = c16;
		ps->__has_hi = 1;
		return 0;
	} else if (c16 >= 0xDC00 && c16 <= 0xDFFF) {
		return (size_t)-1;
	} else {
		cp = c16;
	}
	r = __tinyc_utf8_encode(s, cp);
	return r;
}

#endif /* _TINYC_UCHAR_H */
