/* probe_uchar.c - ordinary use of <uchar.h>: round-trips of
 * 1/2/3/4-byte UTF-8 through the char32 and char16 forms, the
 * surrogate-pair split across two mbrtoc16 calls, byte-at-a-
 * time decoding through the conversion state, and rejection
 * of an invalid sequence. */
#include <uchar.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>

static int fails;
#define CHECK(cond) do { if (!(cond)) { \
	printf("uchar probe: FAIL line %d: %s\n", __LINE__, #cond); \
	fails++; } } while (0)

int main(void)
{
	/* "A" U+0041, "é" U+00E9, "€" U+20AC, U+1F600 */
	static const char text[] = "A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80";
	static const char32_t want32[] = { 0x41, 0xE9, 0x20AC, 0x1F600 };
	mbstate_t st;
	const char *p;
	size_t r;
	int i;

	/* Where a locale system governs the multibyte encoding
	 * (glibc), ask it for UTF-8; the locale-free targets
	 * convert UTF-8 unconditionally and ignore this. */
	if (!setlocale(LC_ALL, "C.UTF-8"))
		setlocale(LC_ALL, "");

	/* char32 round trip */
	memset(&st, 0, sizeof st);
	p = text;
	for (i = 0; i < 4; i++) {
		char32_t c = 0;
		char out[8];
		size_t n;
		r = mbrtoc32(&c, p, 4, &st);
		CHECK(r != (size_t)-1 && r != (size_t)-2 && r > 0);
		CHECK(c == want32[i]);
		p += r;
		memset(out, 0, sizeof out);
		n = c32rtomb(out, want32[i], &st);
		CHECK(n == r);
		CHECK(memcmp(out, p - r, r) == 0);
	}
	CHECK(*p == '\0');
	r = mbrtoc32(NULL, NULL, 0, &st);	/* reset form */
	CHECK(r == 0);

	/* char16: the astral character splits into a pair */
	memset(&st, 0, sizeof st);
	p = text;
	{
		static const char16_t want16[] =
			{ 0x41, 0xE9, 0x20AC, 0xD83D, 0xDE00 };
		for (i = 0; i < 5; i++) {
			char16_t c = 0;
			r = mbrtoc16(&c, p, 4, &st);
			if (i < 4) {
				CHECK(r != (size_t)-1 && r != (size_t)-2);
				if (i < 3)
					CHECK(r > 0);
				p += r;
			} else {
				CHECK(r == (size_t)-3);
			}
			CHECK(c == want16[i]);
		}
		/* c16rtomb reassembles the pair */
		{
			char out[8];
			size_t n;
			memset(&st, 0, sizeof st);
			memset(out, 0, sizeof out);
			n = c16rtomb(out, 0xD83D, &st);
			CHECK(n == 0);
			n = c16rtomb(out, 0xDE00, &st);
			CHECK(n == 4);
			CHECK(memcmp(out, "\xF0\x9F\x98\x80", 4) == 0);
			n = c16rtomb(out, 0x20AC, &st);
			CHECK(n == 3);
		}
	}

	/* byte-at-a-time decode of the 3-byte character */
	memset(&st, 0, sizeof st);
	{
		const char *euro = "\xE2\x82\xAC";
		char32_t c = 0;
		r = mbrtoc32(&c, euro, 1, &st);
		CHECK(r == (size_t)-2);
		r = mbrtoc32(&c, euro + 1, 1, &st);
		CHECK(r == (size_t)-2);
		r = mbrtoc32(&c, euro + 2, 1, &st);
		CHECK(r == 1);
		CHECK(c == 0x20AC);
	}

	/* an invalid byte is rejected */
	memset(&st, 0, sizeof st);
	{
		char32_t c = 0;
		r = mbrtoc32(&c, "\xFF", 1, &st);
		CHECK(r == (size_t)-1);
	}

	if (fails)
		return 1;
	printf("uchar probe: OK\n");
	return 0;
}
