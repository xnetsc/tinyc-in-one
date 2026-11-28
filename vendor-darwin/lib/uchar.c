/*
 * uchar.c - the <uchar.h> conversion functions for the
 * darwin targets.  libSystem has no mbrtoc16/c16rtomb/
 * mbrtoc32/c32rtomb (the staged .tbd stubs carry no such
 * symbols), so the conversions are implemented here
 * directly: UTF-8 <-> UTF-16/UTF-32 with the C11 restartable
 * state semantics.
 *
 * The conversion state lives in a single 32-bit word at the
 * front of the caller's mbstate_t (the darwin mbstate_t is a
 * 128-byte union, so there is room; the packed form also
 * fits the smallest mbstate_t among the targets).  The zero
 * word is the initial state, as the standard requires of a
 * zeroed mbstate_t.  The state is private to these
 * functions: an mbstate_t shared with the wchar.h conversion
 * functions does not carry a partially converted character
 * between the two families.
 *
 *   bits  0-20  accumulated code point (decode), or the
 *               completed astral code point while a low
 *               surrogate is owed, or (encode side) the
 *               held high surrogate minus 0xD800
 *   bits 21-22  continuation bytes still expected
 *   bits 23-24  overlong guard: index of the smallest legal
 *               code point for the sequence length
 *   bit  25     a low surrogate is owed (mbrtoc16)
 *   bit  26     a high surrogate is held (c16rtomb)
 */
#include <uchar.h>
#include <errno.h>
#include <string.h>

struct tinyc_uchar_state {
	unsigned int v;
};

#define ACC(v)		((v) & 0x1FFFFFu)
#define NCONT(v)	(((v) >> 21) & 3u)
#define MINCP(v)	(((v) >> 23) & 3u)
#define OWES(v)		(((v) >> 25) & 1u)
#define HELD(v)		(((v) >> 26) & 1u)
#define PACK(acc, ncont, mincp, owes, held) \
	((acc) | ((ncont) << 21) | ((mincp) << 23) | \
	 ((owes) << 25) | ((held) << 26))

static const unsigned int tinyc_mincp_tab[4] = { 0, 0x80, 0x800, 0x10000 };

static struct tinyc_uchar_state *tinyc_uchar_ps(mbstate_t *ps,
						mbstate_t *internal)
{
	if (!ps)
		ps = internal;
	return (struct tinyc_uchar_state *)ps;
}

static void tinyc_uchar_reset(struct tinyc_uchar_state *st)
{
	st->v = 0;
}

/*
 * Feed bytes to the decoder.  Returns 1 and stores the code
 * point when a character completes, 0 when more bytes are
 * needed, -1 on an invalid sequence (state reset).
 */
static int tinyc_utf8_feed(struct tinyc_uchar_state *st,
			   unsigned char c, unsigned int *cp)
{
	unsigned int acc = ACC(st->v);
	unsigned int ncont = NCONT(st->v);
	unsigned int mincp = MINCP(st->v);

	if (ncont == 0) {
		if (c < 0x80) {
			*cp = c;
			return 1;
		}
		if (c >= 0xC2 && c <= 0xDF) {
			st->v = PACK(c & 0x1F, 1, 1, 0, 0);
			return 0;
		}
		if (c >= 0xE0 && c <= 0xEF) {
			st->v = PACK(c & 0x0F, 2, 2, 0, 0);
			return 0;
		}
		if (c >= 0xF0 && c <= 0xF4) {
			st->v = PACK(c & 0x07, 3, 3, 0, 0);
			return 0;
		}
		tinyc_uchar_reset(st);
		return -1;
	}
	if (c < 0x80 || c > 0xBF) {
		tinyc_uchar_reset(st);
		return -1;
	}
	acc = (acc << 6) | (c & 0x3F);
	ncont--;
	if (ncont > 0) {
		st->v = PACK(acc, ncont, mincp, 0, 0);
		return 0;
	}
	tinyc_uchar_reset(st);
	if (acc < tinyc_mincp_tab[mincp] || acc > 0x10FFFF ||
	    (acc >= 0xD800 && acc <= 0xDFFF))
		return -1;
	*cp = acc;
	return 1;
}

/*
 * Decode from s[0..n).  Returns the number of bytes consumed
 * when a character completes, 0 for a null character, -2
 * when the input ends mid-character, -1 on an invalid
 * sequence.
 */
static size_t tinyc_utf8_decode(struct tinyc_uchar_state *st,
				const char *s, size_t n,
				unsigned int *cp)
{
	size_t i;

	if (st->v == 0 && n > 0 && s[0] == '\0') {
		*cp = 0;
		return 0;
	}
	for (i = 0; i < n; i++) {
		int r = tinyc_utf8_feed(st, (unsigned char)s[i], cp);
		if (r == 1)
			return *cp == 0 ? 0 : i + 1;
		if (r < 0)
			return (size_t)-1;
	}
	return (size_t)-2;
}

/* Encode one code point; returns the byte count or -1. */
static size_t tinyc_utf8_encode(char *s, unsigned int cp)
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

size_t mbrtoc32(char32_t *pc32, const char *s, size_t n, mbstate_t *ps)
{
	static mbstate_t internal;
	struct tinyc_uchar_state *st = tinyc_uchar_ps(ps, &internal);
	unsigned int cp = 0;
	size_t r;

	if (!s) {
		int pending = st->v != 0;
		tinyc_uchar_reset(st);
		return pending;
	}
	r = tinyc_utf8_decode(st, s, n, &cp);
	if (r == (size_t)-1)
		errno = EILSEQ;
	else if (r != (size_t)-2 && pc32)
		*pc32 = (char32_t)cp;
	return r;
}

size_t c32rtomb(char *s, char32_t c32, mbstate_t *ps)
{
	char buf[4];
	size_t r;

	(void)ps;		/* the encode side keeps no state */
	if (!s)
		s = buf;
	r = tinyc_utf8_encode(s, (unsigned int)c32);
	if (r == (size_t)-1)
		errno = EILSEQ;
	return r;
}

size_t mbrtoc16(char16_t *pc16, const char *s, size_t n, mbstate_t *ps)
{
	static mbstate_t internal;
	struct tinyc_uchar_state *st = tinyc_uchar_ps(ps, &internal);
	unsigned int cp = 0;
	size_t r;

	if (!s) {
		int pending = st->v != 0;
		tinyc_uchar_reset(st);
		return pending;
	}
	if (OWES(st->v)) {
		/* the second half of a surrogate pair is owed */
		if (pc16)
			*pc16 = (char16_t)(0xDC00 + (ACC(st->v) & 0x3FF));
		tinyc_uchar_reset(st);
		return (size_t)-3;
	}
	r = tinyc_utf8_decode(st, s, n, &cp);
	if (r == (size_t)-1) {
		errno = EILSEQ;
		return r;
	}
	if (r == (size_t)-2)
		return r;
	if (cp >= 0x10000) {
		st->v = PACK(cp, 0, 0, 1, 0);
		if (pc16)
			*pc16 = (char16_t)(0xD800 + ((cp - 0x10000) >> 10));
	} else if (pc16) {
		*pc16 = (char16_t)cp;
	}
	return r;
}

size_t c16rtomb(char *s, char16_t c16, mbstate_t *ps)
{
	static mbstate_t internal;
	struct tinyc_uchar_state *st = tinyc_uchar_ps(ps, &internal);
	char buf[4];
	unsigned int cp;
	size_t r;

	if (!s)
		s = buf;
	if (HELD(st->v)) {
		if (c16 < 0xDC00 || c16 > 0xDFFF) {
			tinyc_uchar_reset(st);
			errno = EILSEQ;
			return (size_t)-1;
		}
		cp = 0x10000 + (ACC(st->v) << 10) + (c16 - 0xDC00);
		tinyc_uchar_reset(st);
	} else if (c16 >= 0xD800 && c16 <= 0xDBFF) {
		st->v = PACK(c16 - 0xD800, 0, 0, 0, 1);
		return 0;
	} else if (c16 >= 0xDC00 && c16 <= 0xDFFF) {
		errno = EILSEQ;
		return (size_t)-1;
	} else {
		cp = c16;
	}
	r = tinyc_utf8_encode(s, cp);
	if (r == (size_t)-1)
		errno = EILSEQ;
	return r;
}
