/* Cross-host differential probe: string and character
 * literals. The stored bytes fingerprint literal parsing
 * (escape handling, hex/octal escapes, wide and Unicode
 * literal encodings, concatenation, character constant
 * values) in the compiler's preprocessor and code generator.
 * The C11 prefixed literals (u8/u/U) live in their own probe
 * (unicode_literals.c): this compiler's tokenizer does not
 * recognize them, and the probe set keeps that fact separate
 * from the literal forms that do compile. */
#include <stddef.h>

static const char s_plain[] = "hello, world";
static const char s_esc[] = "a\tb\nc\rd\\e\"f\a\b\f\v";
static const char s_hex[] = "\x41\x42\x7f\x80\xff";
static const char s_oct[] = "\101\102\0\07";
static const char s_cat[] = "foo" "bar" "baz";
static const char s_empty[] = "";
static const char s_long[] =
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

static const wchar_t s_wide[] = L"wide \x41\101";

static const int char_vals[] = {
    'a', 'Z', '0',
    '\n', '\t', '\\', '\'', '\"', '\0',
    '\x41', '\101',
    'ab', 'abc',
    L'A',
    sizeof(s_plain), sizeof(s_wide),
    sizeof(s_hex), sizeof(s_oct)
};

const char *probe_strings_anchor(void)
{
    return s_cat;
}
