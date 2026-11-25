/* Smoke probe for the RVV fixed-length surface: the core integer
   and floating-point forms on the m1/m2/mf2 shapes, C operator
   forms, mask results and scalar extraction.  Compiled by both
   legs; vl arguments are the lane counts at VLEN 128.
   riscv_vector.h comes first: the cross compiler does not
   predefine __GNUC__, after which the C library headers would
   define __attribute__ away. */
#include <riscv_vector.h>
#include <stdio.h>
#include <string.h>

static int ia[8] = {10, -3, 7, 100, -200, 55, 0, 9};
static int ib[8] = {3, 5, -2, 4, 6, -7, 11, 13};

static void pi(const char *tag, vint32m1_t v)
{
    int out[4];
    __riscv_vse32_v_i32m1(out, v, 4);
    printf("%s %d %d %d %d\n", tag, out[0], out[1], out[2], out[3]);
}

static void pu(const char *tag, vuint32m1_t v)
{
    unsigned int out[4];
    __riscv_vse32_v_u32m1(out, v, 4);
    printf("%s %u %u %u %u\n", tag, out[0], out[1], out[2], out[3]);
}

int main(void)
{
    vint32m1_t a = __riscv_vle32_v_i32m1(ia, 4);
    vint32m1_t b = __riscv_vle32_v_i32m1(ib, 4);
    pi("add", __riscv_vadd_vv_i32m1(a, b, 4));
    pi("sub", __riscv_vsub_vv_i32m1(a, b, 4));
    pi("rsubx", __riscv_vrsub_vx_i32m1(a, 100, 4));
    pi("mul", __riscv_vmul_vv_i32m1(a, b, 4));
    pi("addx", __riscv_vadd_vx_i32m1(a, 1000, 4));
    pi("addi", __riscv_vadd_vx_i32m1(a, -3, 4));
    pi("div", __riscv_vdiv_vv_i32m1(a, b, 4));
    pi("rem", __riscv_vrem_vv_i32m1(a, b, 4));
    pi("and", __riscv_vand_vv_i32m1(a, b, 4));
    pi("andi", __riscv_vand_vx_i32m1(a, 12, 4));
    pi("sll", __riscv_vsll_vx_i32m1(a, 2, 4));
    pu("srlx", __riscv_vsrl_vx_u32m1(__riscv_vle32_v_u32m1((const unsigned int *)ia, 4), 1, 4));
    pi("srai", __riscv_vsra_vx_i32m1(a, 2, 4));
    pi("neg", __riscv_vneg_v_i32m1(a, 4));
    pi("not", __riscv_vnot_v_i32m1(a, 4));
    pi("min", __riscv_vmin_vv_i32m1(a, b, 4));
    pi("max", __riscv_vmax_vv_i32m1(a, b, 4));
    {
        vbool32_t m = __riscv_vmseq_vv_i32m1_b32(a, b, 4);
        unsigned char mb[1] = {0};
        __riscv_vsm_v_b32(mb, m, 4);
        printf("mseq %u\n", (unsigned)(mb[0] & 0xf));
        m = __riscv_vmslt_vv_i32m1_b32(a, b, 4);
        __riscv_vsm_v_b32(mb, m, 4);
        printf("mslt %u\n", (unsigned)(mb[0] & 0xf));
        m = __riscv_vmseq_vx_i32m1_b32(a, 7, 4);
        __riscv_vsm_v_b32(mb, m, 4);
        printf("mseqi %u\n", (unsigned)(mb[0] & 0xf));
    }
    printf("mvx %d\n", __riscv_vmv_x_s_i32m1_i32(a));
    pi("splatn", __riscv_vmv_v_x_i32m1(-7, 4));
    pi("splatx", __riscv_vmv_v_x_i32m1(42, 4));
    pi("move", __riscv_vmv_v_v_i32m1(b, 4));
    {
        float fa[4] = {1.5f, -2.25f, 3.0f, -4.5f};
        float fb[4] = {0.5f, 2.0f, -1.5f, 4.25f};
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, 4);
        vfloat32m1_t y = __riscv_vle32_v_f32m1(fb, 4);
        float out[4];
        unsigned char mb[1] = {0};
        vbool32_t m;
        __riscv_vse32_v_f32m1(out, __riscv_vfadd_vv_f32m1(x, y, 4), 4);
        printf("fadd %g %g %g %g\n", out[0], out[1], out[2], out[3]);
        __riscv_vse32_v_f32m1(out, __riscv_vfmul_vf_f32m1(x, 2.5f, 4), 4);
        printf("fmulx %g %g %g %g\n", out[0], out[1], out[2], out[3]);
        __riscv_vse32_v_f32m1(out, __riscv_vfabs_v_f32m1(x, 4), 4);
        printf("fabs %g %g %g %g\n", out[0], out[1], out[2], out[3]);
        __riscv_vse32_v_f32m1(out, __riscv_vfneg_v_f32m1(x, 4), 4);
        printf("fneg %g %g %g %g\n", out[0], out[1], out[2], out[3]);
        __riscv_vse32_v_f32m1(out, __riscv_vfsqrt_v_f32m1(
            __riscv_vfabs_v_f32m1(x, 4), 4), 4);
        printf("fsqrt %g %g %g %g\n", out[0], out[1], out[2], out[3]);
        printf("fmvx %g\n", (double)__riscv_vfmv_f_s_f32m1_f32(y));
        m = __riscv_vmflt_vv_f32m1_b32(x, y, 4);
        __riscv_vsm_v_b32(mb, m, 4);
        printf("mflt %u\n", (unsigned)(mb[0] & 0xf));
        {
            vuint32m1_t cls = __riscv_vfclass_v_u32m1(x, 4);
            unsigned int co[4];
            __riscv_vse32_v_u32m1(co, cls, 4);
            printf("fclass %u %u %u %u\n", co[0], co[1], co[2], co[3]);
        }
    }
    {
        signed char ca[16], cb[16];
        signed char out[16];
        int i;
        vint8m1_t x, y;
        for (i = 0; i < 16; i++) {
            ca[i] = (signed char)(i * 3 - 20);
            cb[i] = (signed char)(5 - i);
        }
        x = __riscv_vle8_v_i8m1(ca, 16);
        y = __riscv_vle8_v_i8m1(cb, 16);
        __riscv_vse8_v_i8m1(out, __riscv_vadd_vv_i8m1(x, y, 16), 16);
        printf("i8add");
        for (i = 0; i < 16; i++) printf(" %d", out[i]);
        printf("\n");
    }
    {
        long long la[2] = {10000000000LL, -5};
        long long lb[2] = {7, 3};
        vint64m1_t x = __riscv_vle64_v_i64m1(la, 2);
        vint64m1_t y = __riscv_vle64_v_i64m1(lb, 2);
        long long out[2];
        __riscv_vse64_v_i64m1(out, __riscv_vmul_vv_i64m1(x, y, 2), 2);
        printf("i64mul %lld %lld\n", out[0], out[1]);
    }
    {
        vint32m2_t x = __riscv_vle32_v_i32m2(ia, 8);
        vint32m2_t y = __riscv_vle32_v_i32m2(ib, 8);
        int out[8];
        int i;
        __riscv_vse32_v_i32m2(out, __riscv_vadd_vv_i32m2(x, y, 8), 8);
        printf("m2add");
        for (i = 0; i < 8; i++) printf(" %d", out[i]);
        printf("\n");
    }
    {
        vint32mf2_t p = __riscv_vle32_v_i32mf2(ia, 2);
        vint32mf2_t q = __riscv_vle32_v_i32mf2(ib, 2);
        int out2[2];
        __riscv_vse32_v_i32mf2(out2, __riscv_vadd_vv_i32mf2(p, q, 2), 2);
        printf("mf2add %d %d\n", out2[0], out2[1]);
    }
    return 0;
}
