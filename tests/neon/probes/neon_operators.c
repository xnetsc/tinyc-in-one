/* Hand-written probes for C operators on vector values, which
   the generic vector layer lowers to NEON instructions: integer
   and float arithmetic, unary operators, shifts by scalars and
   by vectors, comparisons and compound assignment. */
#include "neon_probe.h"

int main(void)
{
    int32x4_t a = { 100, -200, 300, -400 };
    int32x4_t b = { 7, -3, 1000, 5 };
    int32x4_t add = a + b, sub = a - b, mul = a * b;
    int32x4_t neg = -a, notv = ~a;
    int32x4_t shl = a << 3, shr = a >> 2;
    uint32x4_t ua = { 0x80000000u, 3, 0xfffffff0u, 123456 };
    uint32x4_t ushr = ua >> 4, ushl = ua << 9;
    int32x4_t vsh = a << (int32x4_t){ 1, 2, 3, 4 };
    int32x4_t vshr = a >> (int32x4_t){ 0, 1, 2, 3 };
    float32x4_t fa = { 1.5f, -2.5f, 3.25f, -4.75f };
    float32x4_t fb = { 0.5f, 2.0f, -1.25f, 4.0f };
    float32x4_t fadd = fa + fb, fsub = fa - fb, fmul = fa * fb;
    float32x4_t fdiv = fa / fb, fneg = -fa;
    uint32x4_t eq = (uint32x4_t)(a == b);
    uint32x4_t gt = (uint32x4_t)(a > b);
    uint32x4_t le = (uint32x4_t)(a <= b);
    uint32x4_t feq = (uint32x4_t)(fa == fb);
    int32x4_t c = a;
    c += b;
    c <<= 2;
    c -= b;
    int64x2_t la = { 5000000000LL, -6000000000LL };
    int64x2_t lb = { 7LL, -9LL };
    int64x2_t ladd = la + lb, lshl = la << 4;
    print_hex(1, &add, sizeof add);
    print_hex(2, &sub, sizeof sub);
    print_hex(3, &mul, sizeof mul);
    print_hex(4, &neg, sizeof neg);
    print_hex(5, &notv, sizeof notv);
    print_hex(6, &shl, sizeof shl);
    print_hex(7, &shr, sizeof shr);
    print_hex(8, &ushr, sizeof ushr);
    print_hex(9, &ushl, sizeof ushl);
    print_hex(10, &vsh, sizeof vsh);
    print_hex(11, &vshr, sizeof vshr);
    print_hex(12, &fadd, sizeof fadd);
    print_hex(13, &fsub, sizeof fsub);
    print_hex(14, &fmul, sizeof fmul);
    print_hex(15, &fdiv, sizeof fdiv);
    print_hex(16, &fneg, sizeof fneg);
    print_hex(17, &eq, sizeof eq);
    print_hex(18, &gt, sizeof gt);
    print_hex(19, &le, sizeof le);
    print_hex(20, &feq, sizeof feq);
    print_hex(21, &c, sizeof c);
    print_hex(22, &ladd, sizeof ladd);
    print_hex(23, &lshl, sizeof lshl);
    {
        /* element select spelled with a comparison mask */
        uint32x4_t mask = (uint32x4_t)(a > (int32x4_t){ 0, 0, 0, 0 });
        int32x4_t sel = (int32x4_t)((mask & (uint32x4_t)a) |
                                    (~mask & (uint32x4_t)b));
        print_hex(24, &sel, sizeof sel);
    }
    return fails != 0;
}
