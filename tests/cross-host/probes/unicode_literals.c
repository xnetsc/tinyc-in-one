/* Cross-host differential probe: C11 prefixed literals.
 * u8, u and U string and character literals are standard C11;
 * a compiler whose tokenizer recognizes them compiles this
 * probe, and the stored bytes fingerprint the encodings.
 * A compiler that does not recognize them fails -- on both
 * hosts alike, which the harness records as a consistent
 * failure (a functional-surface gap, not a cross-host
 * difference). */
#include <stddef.h>
#include <uchar.h>

static const char s_u8[] = u8"u8 A";
static const char16_t s_u16[] = u"u16 A";
static const char32_t s_u32[] = U"u32 A";

static const int u_vals[] = {
    u'A', U'A',
    sizeof(s_u8), sizeof(s_u16), sizeof(s_u32)
};

int probe_unicode_anchor(void)
{
    return u_vals[0];
}
