/* Cross-host differential probe: type layout. sizeof,
 * offsetof and _Alignof are compile-time constants fixed by
 * the target ABI as the compiler understands it; baking them
 * into arrays fingerprints that understanding (padding,
 * alignment, bitfield allocation order and unit, enum size,
 * long double size/alignment). */
#include <stddef.h>

struct mixed {
    char a;
    double b;
    char c;
    int d;
    short e;
    long long f;
    char g[3];
    void *h;
    long double i;
};

struct bits {
    unsigned a : 3;
    unsigned b : 5;
    unsigned c : 10;
    unsigned long long d : 40;
    unsigned : 0;
    unsigned e : 7;
    signed f : 13;
};

union uni {
    char c[16];
    double d;
    long long ll;
    long double ld;
    void *p;
};

struct nested {
    struct mixed m;
    union uni u;
    struct bits b;
    char tail;
};

enum big {
    BIG_A = 0,
    BIG_B = 2147483647,
    BIG_C = -2147483647 - 1
};

enum small {
    SMALL_A = 1,
    SMALL_B = 2
};

static const unsigned long layout_vals[] = {
    sizeof(char), sizeof(short), sizeof(int), sizeof(long),
    sizeof(long long), sizeof(float), sizeof(double),
    sizeof(long double), sizeof(void *), sizeof(void (*)(void)),
    sizeof(struct mixed), _Alignof(struct mixed),
    offsetof(struct mixed, b), offsetof(struct mixed, d),
    offsetof(struct mixed, f), offsetof(struct mixed, h),
    offsetof(struct mixed, i),
    sizeof(struct bits), _Alignof(struct bits),
    sizeof(union uni), _Alignof(union uni),
    sizeof(struct nested), _Alignof(struct nested),
    offsetof(struct nested, u), offsetof(struct nested, b),
    offsetof(struct nested, tail),
    sizeof(enum big), sizeof(enum small),
    sizeof(size_t), sizeof(ptrdiff_t),
    sizeof("hello") + sizeof(L"wide"),
    _Alignof(long double), _Alignof(void *)
};

static const unsigned long bit_offsets[] = {
    offsetof(struct bits, e) * 8,
    sizeof(struct bits) * 8
};

unsigned long probe_layout_anchor(void)
{
    return layout_vals[0] + bit_offsets[0];
}
