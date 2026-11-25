/* F3: masked forms, merges, carry forms, compress, mask logical
   and mask inspection forms.  Lane counts derive from
   __riscv_vlenb() so the probe runs at every VLEN. */
#include <riscv_vector.h>
#include <stdio.h>

#define L32 ((int)(__riscv_vlenb() / 4))

static int ia[64], ib[64], io[64];
static float fa[64], fb[64], fo[64];

static void pi(const char *tag, vint32m1_t v)
{
    int out[64], i;
    __riscv_vse32_v_i32m1(out, v, L32);
    printf("%s", tag);
    for (i = 0; i < L32; i++) printf(" %d", out[i]);
    printf("\n");
}
static void pu(const char *tag, vuint32m1_t v)
{
    unsigned int out[64];
    int i;
    __riscv_vse32_v_u32m1(out, v, L32);
    printf("%s", tag);
    for (i = 0; i < L32; i++) printf(" %u", out[i]);
    printf("\n");
}
static void pf(const char *tag, vfloat32m1_t v)
{
    float out[64];
    int i;
    __riscv_vse32_v_f32m1(out, v, L32);
    printf("%s", tag);
    for (i = 0; i < L32; i++) printf(" %g", (double)out[i]);
    printf("\n");
}
static void pm(const char *tag, vbool32_t m)
{
    unsigned char mb[8] = {0};
    unsigned int bits;
    __riscv_vsm_v_b32(mb, m, L32);
    bits = (unsigned int)mb[0] | ((unsigned int)mb[1] << 8);
    printf("%s %u\n", tag, bits & ((L32 >= 16) ? 0xffffu : ((1u << L32) - 1u)));
}

int main(void)
{
    int i;
    for (i = 0; i < 64; i++) {
        ia[i] = i * 13 - 20;
        ib[i] = (i % 5) - 2;
        io[i] = 900 + i;
        fa[i] = i * 0.75f - 2.0f;
        fb[i] = (i % 4) * 0.5f - 0.25f;
        fo[i] = 50.0f + i;
    }
    {
        vint32m1_t a = __riscv_vle32_v_i32m1(ia, L32);
        vint32m1_t b = __riscv_vle32_v_i32m1(ib, L32);
        vint32m1_t off = __riscv_vle32_v_i32m1(io, L32);
        /* mask: lanes where a < 0, plus lane 1 forced on */
        vbool32_t m = __riscv_vmslt_vx_i32m1_b32(a, 0, L32);
        unsigned char mb[8] = {0};
        vbool32_t m2;
        __riscv_vsm_v_b32(mb, m, L32);
        mb[0] |= 2;
        m2 = __riscv_vlm_v_b32(mb, L32);
        pm("mask", m2);
        pi("add_m", __riscv_vadd_vv_i32m1_mu(m2, off, a, b, L32));
        pi("addx_m", __riscv_vadd_vx_i32m1_mu(m2, off, a, -5, L32));
        pi("mul_m", __riscv_vmul_vv_i32m1_mu(m2, off, a, b, L32));
        pi("sll_m", __riscv_vsll_vx_i32m1_mu(m2, off, a, 1, L32));
        pi("neg_m", __riscv_vneg_v_i32m1_mu(m2, off, a, L32));
        {
            /* the _m form leaves masked-off lanes unspecified;
               overwrite them with a sentinel before printing */
            int out[64];
            unsigned char mb2[8] = {0};
            __riscv_vse32_v_i32m1(out, __riscv_vadd_vv_i32m1_m(m2, a, b, L32), L32);
            __riscv_vsm_v_b32(mb2, m2, L32);
            printf("add_m0");
            for (i = 0; i < L32; i++)
                printf(" %d", (mb2[i >> 3] & (1u << (i & 7))) ? out[i] : -999);
            printf("\n");
        }
        pi("not_m", __riscv_vnot_v_i32m1_mu(m2, off, a, L32));
        pm("slt_m", __riscv_vmslt_vv_i32m1_b32_mu(m2, m, a, b, L32));
        pm("seq_m", __riscv_vmseq_vx_i32m1_b32_mu(m2, m, a, 0, L32));
        pi("madd_m", __riscv_vmadd_vv_i32m1_mu(m2, off, a, b, L32));
        pi("maddx_m", __riscv_vmadd_vx_i32m1_mu(m2, off, -3, b, L32));
        pi("merge", __riscv_vmerge_vvm_i32m1(a, b, m2, L32));
        pi("mergex", __riscv_vmerge_vxm_i32m1(a, 777, m2, L32));
        pi("adc", __riscv_vadc_vvm_i32m1(a, b, m2, L32));
        pi("sbc", __riscv_vsbc_vvm_i32m1(a, b, m2, L32));
        pi("adcx", __riscv_vadc_vxm_i32m1(a, -3, m2, L32));
        pi("sbcx", __riscv_vsbc_vxm_i32m1(a, 5, m2, L32));
        pi("rsub_m", __riscv_vrsub_vx_i32m1_mu(m2, off, a, 100, L32));
        {
            /* lanes past the mask popcount are unspecified */
            int out[64];
            int np = (int)__riscv_vcpop_m_b32(m2, L32), j;
            __riscv_vse32_v_i32m1(out, __riscv_vcompress_vm_i32m1(a, m2, L32), L32);
            printf("compress");
            for (j = 0; j < np; j++) printf(" %d", out[j]);
            printf("\n");
        }
        {
            int out[64];
            __riscv_vse32_v_i32m1(out, __riscv_vmv_s_x_i32m1(4242, L32), L32);
            printf("mv_s_x %d", out[0]);
            for (i = 1; i < L32; i++) printf(" -999");
            printf("\n");
        }
        /* mask logical family on m2 and m */
        pm("mand", __riscv_vmand_mm_b32(m2, m, L32));
        pm("mnand", __riscv_vmnand_mm_b32(m2, m, L32));
        pm("mandn", __riscv_vmandn_mm_b32(m2, m, L32));
        pm("mor", __riscv_vmor_mm_b32(m2, m, L32));
        pm("morn", __riscv_vmorn_mm_b32(m2, m, L32));
        pm("mxor", __riscv_vmxor_mm_b32(m2, m, L32));
        pm("mxnor", __riscv_vmxnor_mm_b32(m2, m, L32));
        pm("mnor", __riscv_vmnor_mm_b32(m2, m, L32));
        pm("mnot", __riscv_vmnot_m_b32(m2, L32));
        pm("msbf", __riscv_vmsbf_m_b32(m2, L32));
        pm("msif", __riscv_vmsif_m_b32(m2, L32));
        pm("msof", __riscv_vmsof_m_b32(m2, L32));
        printf("cpop %ld\n", __riscv_vcpop_m_b32(m2, L32));
        printf("first %ld\n", __riscv_vfirst_m_b32(m2, L32));
        printf("first0 %ld\n", __riscv_vfirst_m_b32(
            __riscv_vmxor_mm_b32(m2, m2, L32), L32));
        pu("iota", __riscv_viota_m_u32m1(m2, L32));
        pu("id", __riscv_vid_v_u32m1(L32));
    }
    {
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
        vfloat32m1_t y = __riscv_vle32_v_f32m1(fb, L32);
        vfloat32m1_t off = __riscv_vle32_v_f32m1(fo, L32);
        vbool32_t m = __riscv_vmflt_vv_f32m1_b32(x, y, L32);
        pf("fadd_m", __riscv_vfadd_vv_f32m1_mu(m, off, x, y, L32));
        pf("fmul_m", __riscv_vfmul_vf_f32m1_mu(m, off, x, 2.0f, L32));
        pf("fabs_m", __riscv_vfabs_v_f32m1_mu(m, off, x, L32));
        pf("fmacc_m", __riscv_vfmacc_vv_f32m1_mu(m, off, x, y, L32));
        pf("fmerge", __riscv_vmerge_vvm_f32m1(x, y, m, L32));
        pf("fmergef", __riscv_vfmerge_vfm_f32m1(x, 3.25f, m, L32));
        pm("fne_m", __riscv_vmfne_vv_f32m1_b32_mu(m, m, x, y, L32));
        {
            float out[64];
            __riscv_vse32_v_f32m1(out, __riscv_vfmv_s_f_f32m1(9.5f, L32), L32);
            printf("fmv_s_f %g", (double)out[0]);
            for (i = 1; i < L32; i++) printf(" -999");
            printf("\n");
        }
        pm("flt_m", __riscv_vmflt_vv_f32m1_b32_mu(m, m, x, y, L32));
    }
    {
        unsigned int uu[64], uw[64];
        for (i = 0; i < 64; i++) {
            uu[i] = (unsigned)(i * 13 + 5);
            uw[i] = (unsigned)(i % 7);
        }
        {
            vuint32m1_t ua = __riscv_vle32_v_u32m1(uu, L32);
            vuint32m1_t ub = __riscv_vle32_v_u32m1(uw, L32);
            vbool32_t um = __riscv_vmsltu_vx_u32m1_b32(ua, 40, L32);
            pu("rsubu", __riscv_vrsub_vx_u32m1(ua, 7, L32));
            pu("negu", __riscv_vneg_v_u32m1(ua, L32));
            pu("notu", __riscv_vnot_v_u32m1(ua, L32));
            pm("sequ", __riscv_vmseq_vx_u32m1_b32(ua, 26, L32));
            pm("sneu_m", __riscv_vmsne_vv_u32m1_b32_mu(um, um, ua, ub, L32));
        }
    }
    {
        /* small-ratio masks: b4 (i8m2), b2 (i8m4), b1 (i8m8) */
        signed char ca[256];
        int L8 = (int)__riscv_vlenb();
        for (i = 0; i < 256; i++) ca[i] = (signed char)(i % 5 - 2);
        {
            vint8m2_t c2 = __riscv_vle8_v_i8m2(ca, 2 * L8);
            vbool4_t m4 = __riscv_vmseq_vx_i8m2_b4(c2, 1, 2 * L8);
            vint8m4_t c4 = __riscv_vle8_v_i8m4(ca, 4 * L8);
            vbool2_t m2b = __riscv_vmseq_vx_i8m4_b2(c4, -1, 4 * L8);
            vint8m8_t c8 = __riscv_vle8_v_i8m8(ca, 8 * L8);
            vbool1_t m1b = __riscv_vmseq_vx_i8m8_b1(c8, 0, 8 * L8);
            unsigned char mb[32] = {0};
            printf("cpop4 %ld\n", __riscv_vcpop_m_b4(m4, 2 * L8));
            printf("first4 %ld\n", __riscv_vfirst_m_b4(m4, 2 * L8));
            printf("cpop2 %ld\n", __riscv_vcpop_m_b2(m2b, 4 * L8));
            printf("first2 %ld\n", __riscv_vfirst_m_b2(m2b, 4 * L8));
            printf("cpop1 %ld\n", __riscv_vcpop_m_b1(m1b, 8 * L8));
            printf("first1 %ld\n", __riscv_vfirst_m_b1(m1b, 8 * L8));
            __riscv_vsm_v_b4(mb, __riscv_vmnor_mm_b4(m4, m4, 2 * L8),
                             2 * L8);
            printf("mnor4 %u %u\n", (unsigned)mb[0], (unsigned)mb[1]);
        }
    }
    return 0;
}
