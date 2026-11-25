/* F1: multiply-accumulate, multiply-high, saturating, averaging,
   widening and narrowing forms.  Lane counts come from sizeof so
   the probe runs at every VLEN configuration. */
#include <riscv_vector.h>
#include <stdio.h>

#define L32 ((int)(__riscv_vlenb() / 4))
#define L16 ((int)(__riscv_vlenb() / 2))

static int ia[64], ib[64], id0[64];
static short sa[64], sb[64];

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
    unsigned int out[64], i;
    __riscv_vse32_v_u32m1(out, v, L32);
    printf("%s", tag);
    for (i = 0; i < L32; i++) printf(" %u", out[i]);
    printf("\n");
}
static void pi16(const char *tag, vint16m1_t v)
{
    short out[64];
    int i;
    __riscv_vse16_v_i16m1(out, v, L16);
    printf("%s", tag);
    for (i = 0; i < L16; i++) printf(" %d", out[i]);
    printf("\n");
}
static void pu16(const char *tag, vuint16m1_t v)
{
    unsigned short out[64];
    int i;
    __riscv_vse16_v_u16m1(out, v, L16);
    printf("%s", tag);
    for (i = 0; i < L16; i++) printf(" %u", out[i]);
    printf("\n");
}

int main(void)
{
    int i;
    for (i = 0; i < 64; i++) {
        ia[i] = i * 37 - 100;
        ib[i] = i * 11 + 3;
        id0[i] = 1000 - i * 7;
        sa[i] = (short)(i * 53 - 200);
        sb[i] = (short)(i * 29 + 5);
    }
    ia[0] = 2147483647; ia[1] = -2147483647 - 1; ia[2] = 65536; ia[3] = -65536;
    {
        vint32m1_t a = __riscv_vle32_v_i32m1(ia, L32);
        vint32m1_t b = __riscv_vle32_v_i32m1(ib, L32);
        vint32m1_t d = __riscv_vle32_v_i32m1(id0, L32);
        vuint32m1_t ua = __riscv_vle32_v_u32m1((const unsigned int *)ia, L32);
        vuint32m1_t ub = __riscv_vle32_v_u32m1((const unsigned int *)ib, L32);
        vuint32m1_t ud = __riscv_vle32_v_u32m1((const unsigned int *)id0, L32);
        pi("macc_vv", __riscv_vmacc_vv_i32m1(d, a, b, L32));
        pi("macc_vx", __riscv_vmacc_vx_i32m1(d, 3, b, L32));
        pi("madd_vv", __riscv_vmadd_vv_i32m1(d, a, b, L32));
        pi("madd_vx", __riscv_vmadd_vx_i32m1(d, -2, b, L32));
        pi("nmsac_vv", __riscv_vnmsac_vv_i32m1(d, a, b, L32));
        pi("nmsac_vx", __riscv_vnmsac_vx_i32m1(d, 5, b, L32));
        pi("nmsub_vv", __riscv_vnmsub_vv_i32m1(d, a, b, L32));
        pi("nmsub_vx", __riscv_vnmsub_vx_i32m1(d, -7, b, L32));
        pu("maccu_vv", __riscv_vmacc_vv_u32m1(ud, ua, ub, L32));
        pi("mulh", __riscv_vmulh_vv_i32m1(a, b, L32));
        pi("mulhx", __riscv_vmulh_vx_i32m1(a, -3, L32));
        pu("mulhu", __riscv_vmulhu_vv_u32m1(ua, ub, L32));
        pi("mulhsu", __riscv_vmulhsu_vv_i32m1(a, ub, L32));
        pi("sadd", __riscv_vsadd_vv_i32m1(a, b, L32));
        pi("saddx", __riscv_vsadd_vx_i32m1(a, 2147483647, L32));
        pi("ssub", __riscv_vssub_vv_i32m1(a, b, L32));
        pu("saddu", __riscv_vsaddu_vv_u32m1(ua, ub, L32));
        pu("ssubu", __riscv_vssubu_vv_u32m1(ua, ub, L32));
        pi("aadd", __riscv_vaadd_vv_i32m1(a, b, L32));
        pi("aaddx", __riscv_vaadd_vx_i32m1(a, 7, L32));
        pi("asub", __riscv_vasub_vv_i32m1(a, b, L32));
        pu("aaddu", __riscv_vaaddu_vv_u32m1(ua, ub, L32));
        pu("asubu", __riscv_vasubu_vv_u32m1(ua, ub, L32));
    }
    {
        /* widening: i16mf2 -> i32m1, i16m1 -> i32m2 */
        vint16mf2_t a = __riscv_vle16_v_i16mf2(sa, L32);
        vint16mf2_t b = __riscv_vle16_v_i16mf2(sb, L32);
        vint32m1_t w = __riscv_vle32_v_i32m1(id0, L32);
        vuint16mf2_t ua = __riscv_vle16_v_u16mf2((const unsigned short *)sa, L32);
        vuint16mf2_t ub = __riscv_vle16_v_u16mf2((const unsigned short *)sb, L32);
        pi("wadd_vv", __riscv_vwadd_vv_i32m1(a, b, L32));
        pi("wadd_wv", __riscv_vwadd_wv_i32m1(w, a, L32));
        pi("wadd_vx", __riscv_vwadd_vx_i32m1(a, -5, L32));
        pi("wadd_wx", __riscv_vwadd_wx_i32m1(w, 9, L32));
        pi("wsub_vv", __riscv_vwsub_vv_i32m1(a, b, L32));
        pi("wsub_wv", __riscv_vwsub_wv_i32m1(w, a, L32));
        pi("wmul", __riscv_vwmul_vv_i32m1(a, b, L32));
        pi("wmulsu", __riscv_vwmulsu_vv_i32m1(a, ub, L32));
        pu("waddu_vv", __riscv_vwaddu_vv_u32m1(ua, ub, L32));
        pu("wmulu", __riscv_vwmulu_vv_u32m1(ua, ub, L32));
        {
            vint16m1_t a2 = __riscv_vle16_v_i16m1(sa, L16);
            vint16m1_t b2 = __riscv_vle16_v_i16m1(sb, L16);
            int out[64];
            __riscv_vse32_v_i32m2(out, __riscv_vwadd_vv_i32m2(a2, b2, L16), L16);
            printf("wadd2");
            for (i = 0; i < L16; i++) printf(" %d", out[i]);
            printf("\n");
        }
    }
    {
        /* narrowing: i32m2 -> i16m1, u32m2 -> u16m1 */
        vint32m2_t a = __riscv_vle32_v_i32m2(ia, L16);
        vuint16m1_t sh = __riscv_vle16_v_u16m1((const unsigned short *)sb, L16);
        vuint32m2_t ua = __riscv_vle32_v_u32m2((const unsigned int *)ia, L16);
        pi16("nsra_wv", __riscv_vnsra_wv_i16m1(a, sh, L16));
        pi16("nsra_wx", __riscv_vnsra_wx_i16m1(a, 3, L16));
        pi16("nclip_wv", __riscv_vnclip_wv_i16m1(a, sh, L16));
        pi16("nclip_wx", __riscv_vnclip_wx_i16m1(a, 1, L16));
        pu16("nsrl_wv", __riscv_vnsrl_wv_u16m1(ua, sh, L16));
        pu16("nsrl_wx", __riscv_vnsrl_wx_u16m1(ua, 5, L16));
        pu16("nclipu_wv", __riscv_vnclipu_wv_u16m1(ua, sh, L16));
    }
    {
        float fa[64], fb[64], fd[64];
        for (i = 0; i < 64; i++) {
            fa[i] = i * 0.5f - 3.0f;
            fb[i] = i * 0.25f + 0.5f;
            fd[i] = 10.0f - i;
        }
        {
            vfloat32m1_t x = __riscv_vle32_v_f32m1(fa, L32);
            vfloat32m1_t y = __riscv_vle32_v_f32m1(fb, L32);
            vfloat32m1_t d = __riscv_vle32_v_f32m1(fd, L32);
            float out[64];
            __riscv_vse32_v_f32m1(out, __riscv_vfmacc_vv_f32m1(d, x, y, L32), L32);
            printf("fmacc"); for (i = 0; i < L32; i++) printf(" %g", (double)out[i]); printf("\n");
            __riscv_vse32_v_f32m1(out, __riscv_vfmacc_vf_f32m1(d, 1.5f, y, L32), L32);
            printf("fmaccf"); for (i = 0; i < L32; i++) printf(" %g", (double)out[i]); printf("\n");
            __riscv_vse32_v_f32m1(out, __riscv_vfmadd_vv_f32m1(d, x, y, L32), L32);
            printf("fmadd"); for (i = 0; i < L32; i++) printf(" %g", (double)out[i]); printf("\n");
            __riscv_vse32_v_f32m1(out, __riscv_vfnmsac_vv_f32m1(d, x, y, L32), L32);
            printf("fnmsac"); for (i = 0; i < L32; i++) printf(" %g", (double)out[i]); printf("\n");
            __riscv_vse32_v_f32m1(out, __riscv_vfnmsub_vf_f32m1(d, -0.5f, y, L32), L32);
            printf("fnmsubf"); for (i = 0; i < L32; i++) printf(" %g", (double)out[i]); printf("\n");
        }
    }
    return 0;
}
