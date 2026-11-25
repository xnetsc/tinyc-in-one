/* TCC-only probe: C operator forms on the fixed-length vector
   types (the reference compiler's sizeless types have no C
   operators).  Every case is checked against a scalar reference
   computed in plain C; a disagreement prints MISMATCH and the
   program exits nonzero. */
#include <riscv_vector.h>
#include <stdio.h>

static int fails;
static int ia[4] = {10, -3, 7, 100};
static int ib[4] = {3, 5, -2, 4};

static void chk(const char *tag, vint32m1_t v, const int *ref)
{
    int out[4], i;
    __riscv_vse32_v_i32m1(out, v, 4);
    for (i = 0; i < 4; i++)
        if (out[i] != ref[i]) {
            printf("MISMATCH %s lane %d: got %d want %d\n", tag, i, out[i], ref[i]);
            fails++;
        }
}

static vint32m1_t passret(vint32m1_t v) { return v; }

int main(void)
{
    vint32m1_t a = __riscv_vle32_v_i32m1(ia, 4);
    vint32m1_t b = __riscv_vle32_v_i32m1(ib, 4);
    int r[4], i;
    for (i = 0; i < 4; i++) r[i] = ia[i] + ib[i];
    chk("add", a + b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] - ib[i];
    chk("sub", a - b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] * ib[i];
    chk("mul", a * b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] / ib[i];
    chk("div", a / b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] % ib[i];
    chk("mod", a % b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] & ib[i];
    chk("and", a & b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] | ib[i];
    chk("or", a | b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] ^ ib[i];
    chk("xor", a ^ b, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] << 3;
    chk("shl", a << 3, r);
    for (i = 0; i < 4; i++) r[i] = ib[i] >> 1;
    chk("shr", b >> 1, r);
    for (i = 0; i < 4; i++) r[i] = ia[i] << (i + 1);
    {
        int sh[4] = {1, 2, 3, 4};
        chk("shlv", a << __riscv_vle32_v_i32m1(sh, 4), r);
    }
    for (i = 0; i < 4; i++) r[i] = -(ia[i] == ib[i]);
    chk("eq", a == b, r);
    for (i = 0; i < 4; i++) r[i] = -(ia[i] != ib[i]);
    chk("ne", a != b, r);
    for (i = 0; i < 4; i++) r[i] = -(ia[i] < ib[i]);
    chk("lt", a < b, r);
    for (i = 0; i < 4; i++) r[i] = -(ia[i] <= ib[i]);
    chk("le", a <= b, r);
    for (i = 0; i < 4; i++) r[i] = -(ia[i] > ib[i]);
    chk("gt", a > b, r);
    for (i = 0; i < 4; i++) r[i] = -(ia[i] >= ib[i]);
    chk("ge", a >= b, r);
    for (i = 0; i < 4; i++) r[i] = -ia[i];
    chk("neg", -a, r);
    for (i = 0; i < 4; i++) r[i] = ~ia[i];
    chk("not", ~a, r);
    {
        vint32m1_t c = a;
        c += b;
        for (i = 0; i < 4; i++) r[i] = ia[i] + ib[i];
        chk("pluseq", c, r);
        c = passret(c);
        chk("passret", c, r);
    }
    {
        /* subscript read/write on a vector value */
        vint32m1_t c = a;
        int x = c[2];
        if (x != ia[2]) { printf("MISMATCH subscript read: %d\n", x); fails++; }
        c[1] = 777;
        if (c[1] != 777 || c[0] != ia[0]) { printf("MISMATCH subscript write\n"); fails++; }
    }
    {
        float fa[4] = {1.5f, -2.25f, 3.0f, -4.5f};
        float fb[4] = {0.5f, 2.0f, -1.5f, 4.25f};
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, 4);
        vfloat32m1_t y = __riscv_vle32_v_f32m1(fb, 4);
        float out[4];
        __riscv_vse32_v_f32m1(out, x + y, 4);
        for (i = 0; i < 4; i++)
            if (out[i] != fa[i] + fb[i]) { printf("MISMATCH fadd %d\n", i); fails++; }
        __riscv_vse32_v_f32m1(out, x / y, 4);
        for (i = 0; i < 4; i++)
            if (out[i] != fa[i] / fb[i]) { printf("MISMATCH fdiv %d\n", i); fails++; }
        __riscv_vse32_v_f32m1(out, -x, 4);
        for (i = 0; i < 4; i++)
            if (out[i] != -fa[i]) { printf("MISMATCH fneg %d\n", i); fails++; }
        {
            vint32m1_t cmp = (vint32m1_t)(x < y);
            int co[4];
            __riscv_vse32_v_i32m1(co, cmp, 4);
            for (i = 0; i < 4; i++)
                if (co[i] != -(fa[i] < fb[i])) { printf("MISMATCH fcmp %d\n", i); fails++; }
        }
    }
    if (!fails)
        printf("ops: all ok\n");
    return fails != 0;
}
