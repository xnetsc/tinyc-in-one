/* riscv64 RVV differential probe: float estimates and
   conversions, slides, gathers, widening float arithmetic and
   reductions (families F2/F5).  Lane counts come from
   __riscv_vlenb(). */
#include <riscv_vector.h>
#include <stdio.h>
#include <string.h>

static int ia[64];
static unsigned int ua[64];
static float fa[64], fb[64];
static double da[64];
static long la[64];

static void pi(const char *tag, vint32m1_t v, int n)
{
    int out[64], i;
    __riscv_vse32_v_i32m1(out, v, n);
    printf("%s", tag);
    for (i = 0; i < n; i++) printf(" %d", out[i]);
    printf("\n");
}
static void pf(const char *tag, vfloat32m1_t v, int n)
{
    float out[64];
    int i;
    __riscv_vse32_v_f32m1(out, v, n);
    printf("%s", tag);
    for (i = 0; i < n; i++) printf(" %g", (double)out[i]);
    printf("\n");
}
static void pd(const char *tag, vfloat64m2_t v, int n)
{
    double out[64];
    int i;
    __riscv_vse64_v_f64m2(out, v, n);
    printf("%s", tag);
    for (i = 0; i < n; i++) printf(" %g", out[i]);
    printf("\n");
}

int main(void)
{
    int L32 = (int)(__riscv_vlenb() / 4);
    int i;

    for (i = 0; i < 64; i++) {
        ia[i] = i * 7 - 20;
        ua[i] = (unsigned)(i * 5 + 3);
        fa[i] = i * 0.75f - 2.0f;
        fb[i] = (i % 5) * 0.5f + 0.25f;
        da[i] = i * 1.5 - 4.0;
        la[i] = i * 11 - 30;
    }

    /* estimates (deterministic per the RVV specification) */
    {
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fb, L32);
        pf("rec7", __riscv_vfrec7_v_f32m1(x, L32), L32);
        pf("rsqrt7", __riscv_vfrsqrt7_v_f32m1(x, L32), L32);
    }
    /* same-width conversions */
    {
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
        pi("cvt_x", __riscv_vfcvt_x_f_v_i32m1(x, L32), L32);
        pi("cvt_rtz", __riscv_vfcvt_rtz_x_f_v_i32m1(x, L32), L32);
        {
            vuint32m1_t u = __riscv_vfcvt_xu_f_v_u32m1(x, L32);
            unsigned int out[64];
            __riscv_vse32_v_u32m1(out, u, L32);
            printf("cvt_xu");
            for (i = 0; i < L32; i++) printf(" %u", out[i]);
            printf("\n");
        }
        {
            vint32m1_t a = __riscv_vle32_v_i32m1(ia, L32);
            pf("cvt_f", __riscv_vfcvt_f_x_v_f32m1(a, L32), L32);
        }
    }
    /* widening and narrowing conversions */
    {
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
        vfloat64m2_t w = __riscv_vfwcvt_f_f_v_f64m2(x, L32);
        pd("wcvt_ff", w, L32);
        {
            vint64m2_t q = __riscv_vfwcvt_x_f_v_i64m2(x, L32);
            long out[64];
            __riscv_vse64_v_i64m2(out, q, L32);
            printf("wcvt_x");
            for (i = 0; i < L32; i++) printf(" %ld", out[i]);
            printf("\n");
        }
        pi("ncvt_x", __riscv_vfncvt_x_f_w_i32m1(w, L32), L32);
        pf("ncvt_ff", __riscv_vfncvt_f_f_w_f32m1(w, L32), L32);
        pf("ncvt_rod", __riscv_vfncvt_rod_f_f_w_f32m1(w, L32), L32);
        {
            vint64m2_t q = __riscv_vle64_v_i64m2(la, L32);
            pf("ncvt_fx", __riscv_vfncvt_f_x_w_f32m1(q, L32), L32);
        }
    }
    /* slides: lanes outside the shifted range are not printed
       where the architecture leaves them unspecified */
    {
        vint32m1_t a = __riscv_vle32_v_i32m1(ia, L32);
        vint32m1_t pt = __riscv_vmv_v_x_i32m1(-1, L32);
        pi("slideup", __riscv_vslideup_vx_i32m1(pt, a, 2, L32), L32);
        {
            vint32m1_t dn = __riscv_vslidedown_vx_i32m1(a, 1, L32);
            int out[64];
            __riscv_vse32_v_i32m1(out, dn, L32);
            printf("slidedn");
            for (i = 0; i < L32 - 1; i++) printf(" %d", out[i]);
            printf("\n");
        }
        pi("slide1up", __riscv_vslide1up_vx_i32m1(a, 777, L32), L32);
        {
            vint32m1_t d1 = __riscv_vslide1down_vx_i32m1(a, 888, L32);
            int out[64];
            __riscv_vse32_v_i32m1(out, d1, L32);
            printf("slide1dn");
            for (i = 0; i < L32 - 1; i++) printf(" %d", out[i]);
            printf("\n");
        }
        {
            vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
            pf("fslide1up", __riscv_vfslide1up_vf_f32m1(x, 9.5f, L32), L32);
            {
            vfloat32m1_t fd = __riscv_vfslide1down_vf_f32m1(x, 9.5f, L32);
            float out[64];
            __riscv_vse32_v_f32m1(out, fd, L32);
            printf("fslide1dn");
            for (i = 0; i < L32 - 1; i++) printf(" %g", (double)out[i]);
            printf("\n");
        }
        }
    }
    /* gathers */
    {
        vint32m1_t a = __riscv_vle32_v_i32m1(ia, L32);
        unsigned int gi[64];
        for (i = 0; i < L32; i++) gi[i] = (unsigned)(L32 - 1 - i);
        {
            vuint32m1_t idx = __riscv_vle32_v_u32m1(gi, L32);
            pi("gather", __riscv_vrgather_vv_i32m1(a, idx, L32), L32);
        }
        pi("gatherx", __riscv_vrgather_vx_i32m1(a, 1, L32), L32);
        {
            unsigned short hi[64];
            for (i = 0; i < L32; i++) hi[i] = (unsigned short)(L32 - 1 - i);
            {
                vuint16mf2_t idx = __riscv_vle16_v_u16mf2(hi, L32);
                pi("gather16", __riscv_vrgatherei16_vv_i32m1(a, idx, L32), L32);
            }
        }
    }
    /* widening float arithmetic and MAC */
    {
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
        vfloat32m1_t y = __riscv_vle32_v_f32m1(fb, L32);
        vfloat64m2_t acc = __riscv_vfwcvt_f_f_v_f64m2(x, L32);
        pd("fwadd", __riscv_vfwadd_vv_f64m2(x, y, L32), L32);
        pd("fwadd_wv", __riscv_vfwadd_wv_f64m2(acc, y, L32), L32);
        pd("fwadd_vf", __riscv_vfwadd_vf_f64m2(x, 0.5f, L32), L32);
        pd("fwsub", __riscv_vfwsub_vv_f64m2(x, y, L32), L32);
        pd("fwmul", __riscv_vfwmul_vv_f64m2(x, y, L32), L32);
        pd("fwmul_vf", __riscv_vfwmul_vf_f64m2(x, 2.0f, L32), L32);
        pd("fwmacc", __riscv_vfwmacc_vv_f64m2(acc, x, y, L32), L32);
        pd("fwmacc_vf", __riscv_vfwmacc_vf_f64m2(acc, 0.5f, x, L32), L32);
        pd("fwmsac", __riscv_vfwmsac_vv_f64m2(acc, x, y, L32), L32);
        pd("fwnmsac", __riscv_vfwnmsac_vv_f64m2(acc, x, y, L32), L32);
        pd("fwadd_wf", __riscv_vfwadd_wf_f64m2(acc, 0.25f, L32), L32);
        pd("fwsub_wf", __riscv_vfwsub_wf_f64m2(acc, 0.25f, L32), L32);
    }
    /* float MAC sign variants, reverse subtract, float compress
       and ei16 gather */
    {
        vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
        vfloat32m1_t y = __riscv_vle32_v_f32m1(fb, L32);
        vfloat32m1_t acc = __riscv_vfadd_vv_f32m1(x, y, L32);
        pf("fmsac", __riscv_vfmsac_vv_f32m1(acc, x, y, L32), L32);
        pf("fmsac_f", __riscv_vfmsac_vf_f32m1(acc, 0.5f, x, L32), L32);
        pf("fmsub", __riscv_vfmsub_vv_f32m1(acc, x, y, L32), L32);
        pf("fmsub_f", __riscv_vfmsub_vf_f32m1(acc, 0.5f, x, L32), L32);
        pf("fnmacc", __riscv_vfnmacc_vv_f32m1(acc, x, y, L32), L32);
        pf("fnmacc_f", __riscv_vfnmacc_vf_f32m1(acc, 0.5f, x, L32), L32);
        pf("fnmadd", __riscv_vfnmadd_vv_f32m1(acc, x, y, L32), L32);
        pf("fnmadd_f", __riscv_vfnmadd_vf_f32m1(acc, 0.5f, x, L32), L32);
        pf("frsub", __riscv_vfrsub_vf_f32m1(x, 10.0f, L32), L32);
        {
            /* lanes past the mask popcount are unspecified after a
               compress; print only the packed prefix */
            vbool32_t m = __riscv_vmflt_vv_f32m1_b32(x, y, L32);
            pf("fcompress", __riscv_vcompress_vm_f32m1(x, m, L32),
               (int)__riscv_vcpop_m_b32(m, L32));
        }
        {
            unsigned int uu[64];
            unsigned int out[64];
            vuint32m1_t uv;
            vbool32_t um;
            int np;
            for (i = 0; i < 64; i++) uu[i] = (unsigned)(i * 7 + 3);
            uv = __riscv_vle32_v_u32m1(uu, L32);
            um = __riscv_vmsltu_vx_u32m1_b32(uv, 20, L32);
            np = (int)__riscv_vcpop_m_b32(um, L32);
            __riscv_vse32_v_u32m1(out, __riscv_vcompress_vm_u32m1(uv, um, L32), L32);
            printf("ucompress");
            for (i = 0; i < np; i++) printf(" %u", out[i]);
            printf("\n");
        }
        {
            unsigned short hi[64];
            for (i = 0; i < L32; i++) hi[i] = (unsigned short)(L32 - 1 - i);
            {
                vuint16mf2_t idx = __riscv_vle16_v_u16mf2(hi, L32);
                pf("gather16f", __riscv_vrgatherei16_vv_f32m1(x, idx, L32), L32);
            }
        }
        pf("gatherxf", __riscv_vrgather_vx_f32m1(x, 1, L32), L32);
    }
    /* reductions: element 0 carries the result */
    {
        vint32m1_t a = __riscv_vle32_v_i32m1(ia, L32);
        vint32m1_t zero = __riscv_vmv_v_x_i32m1(0, L32);
        pi("redsum", __riscv_vredsum_vs_i32m1_i32m1(a, zero, L32), 1);
        pi("redmax", __riscv_vredmax_vs_i32m1_i32m1(a, zero, L32), 1);
        pi("redmin", __riscv_vredmin_vs_i32m1_i32m1(a, zero, L32), 1);
        pi("redxor", __riscv_vredxor_vs_i32m1_i32m1(a, zero, L32), 1);
        {
            vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
            vfloat32m1_t fz = __riscv_vfmv_v_f_f32m1(0.0f, L32);
            pf("fredusum", __riscv_vfredusum_vs_f32m1_f32m1(x, fz, L32), 1);
            pf("fredmax", __riscv_vfredmax_vs_f32m1_f32m1(x, fz, L32), 1);
            {
                vfloat64m1_t r = __riscv_vfwredusum_vs_f32m1_f64m1(x,
                    __riscv_vfmv_v_f_f64m1(0.0, L32), L32);
                double out[64];
                __riscv_vse64_v_f64m1(out, r, L32);
                printf("fwredusum %g\n", out[0]);
            }
        }
        {
            vint64m1_t s0 = __riscv_vmv_v_x_i64m1(0, L32);
            vint64m1_t r = __riscv_vwredsum_vs_i32m1_i64m1(a, s0, L32);
            long out[64];
            __riscv_vse64_v_i64m1(out, r, L32);
            printf("wredsum %ld\n", out[0]);
        }
    }
    return 0;
}
