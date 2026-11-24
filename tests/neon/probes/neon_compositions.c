/* Hand-written probes for the composition surface: macros built
   from other intrinsics (struct transposes, pairwise-widening
   reductions, absolute comparisons, lane copies, multi-register
   loads and stores) across a spread of element types. */
#include "neon_probe.h"

static int8_t buf8[64];
static int16_t buf16[32];
static int32_t buf32[16];
static float bufF[16];
static double bufD[8];

static void fill_seq(void)
{
    int i;
    for (i = 0; i < 64; i++) buf8[i] = (int8_t)(i * 5 - 60);
    for (i = 0; i < 32; i++) buf16[i] = (int16_t)(i * 300 - 4000);
    for (i = 0; i < 16; i++) buf32[i] = i * 7000 - 50000;
    for (i = 0; i < 16; i++) bufF[i] = ftab[i & 7] * (i - 3);
    for (i = 0; i < 8; i++) bufD[i] = dtab[i & 7] * (i - 2);
}

static void case_struct_forms(void)
{
    int8x8x2_t t1 = vtrn_s8(vld1_s8(buf8), vld1_s8(buf8 + 8));
    int16x8x2_t t2 = vtrnq_s16(vld1q_s16(buf16), vld1q_s16(buf16 + 8));
    uint8x8x2_t z1 = vzip_u8(vld1_u8((const uint8_t *)buf8),
                             vld1_u8((const uint8_t *)buf8 + 16));
    int32x4x2_t z2 = vzipq_s32(vld1q_s32(buf32), vld1q_s32(buf32 + 4));
    int16x4x2_t u1 = vuzp_s16(vld1_s16(buf16), vld1_s16(buf16 + 4));
    float32x4x2_t u2 = vuzpq_f32(vld1q_f32(bufF), vld1q_f32(bufF + 4));
    print_hex(1, &t1, sizeof t1);
    print_hex(2, &t2, sizeof t2);
    print_hex(3, &z1, sizeof z1);
    print_hex(4, &z2, sizeof z2);
    print_hex(5, &u1, sizeof u1);
    print_hex(6, &u2, sizeof u2);
}

static void case_widen_reduce(void)
{
    printf("addlv %d %d %d %lld\n",
           (int)vaddlv_s8(vld1_s8(buf8)),
           (int)vaddlvq_u16(vld1q_u16((const uint16_t *)buf16)),
           (int)vaddlv_s32(vld1_s32(buf32)),
           (long long)vaddlvq_s32(vld1q_s32(buf32)));
    printf("addv %d %lld %g\n",
           (int)vaddv_s32(vld1_s32(buf32)),
           (long long)vaddvq_s64(vld1q_s64((const int64_t *)buf32)),
           (double)vaddvq_f32(vld1q_f32(bufF)));
    {
        int16x4_t p1 = vpaddl_s8(vld1_s8(buf8));
        uint32x4_t p2 = vpaddlq_u16(vld1q_u16((const uint16_t *)buf16));
        int16x4_t a1 = vpadal_s8(vdup_n_s16(3), vld1_s8(buf8));
        print_hex(7, &p1, sizeof p1);
        print_hex(8, &p2, sizeof p2);
        print_hex(9, &a1, sizeof a1);
    }
}

static void case_abs_cmp(void)
{
    float32x4_t a = vld1q_f32(bufF), b = vld1q_f32(bufF + 4);
    float64x2_t c = vld1q_f64(bufD), d = vld1q_f64(bufD + 2);
    uint32x4_t r1 = vcageq_f32(a, b);
    uint64x2_t r2 = vcagtq_f64(c, d);
    uint32x4_t r3 = vcaleq_f32(a, b);
    uint64x2_t r4 = vcaltq_f64(c, d);
    poly8x8_t p1 = vld1_p8((const poly8_t *)buf8);
    poly8x8_t p2 = vld1_p8((const poly8_t *)buf8 + 8);
    uint8x8_t e1 = vceq_p8(p1, p2);
    uint8x16_t e2 = vceqq_p8(vld1q_p8((const poly8_t *)buf8),
                             vld1q_p8((const poly8_t *)buf8 + 16));
    print_hex(10, &r1, sizeof r1);
    print_hex(11, &r2, sizeof r2);
    print_hex(12, &r3, sizeof r3);
    print_hex(13, &r4, sizeof r4);
    print_hex(14, &e1, sizeof e1);
    print_hex(15, &e2, sizeof e2);
}

static void case_lane_misc(void)
{
    int64_t bits = 0x0102030405060708LL;
    int64x1_t c1 = vcreate_s64(bits);
    float32x2_t c2 = vcreate_f32(0x3fc00000);
    int32x4_t cp1 = vcopyq_laneq_s32(vld1q_s32(buf32), 1,
                                     vld1q_s32(buf32 + 4), 3);
    uint8x8_t cp2 = vcopy_lane_u8(vld1_u8((const uint8_t *)buf8), 2,
                                  vld1_u8((const uint8_t *)buf8 + 8), 5);
    float64x2_t cp3 = vcopyq_lane_f64(vld1q_f64(bufD), 0,
                                      vld1_f64(bufD + 4), 0);
    int64x2_t dp1 = vdupq_lane_s64(vld1_s64((const int64_t *)buf32), 0);
    poly8x8_t dp2 = vdup_lane_p8(vld1_p8((const poly8_t *)buf8), 3);
    int16x8_t mv1 = vmovq_n_s16(-1234);
    float64x1_t mv2 = vmov_n_f64(2.5);
    poly16x4_t mv3 = vmov_n_p16(0xbeef);
    print_hex(16, &c1, sizeof c1);
    print_hex(17, &c2, sizeof c2);
    print_hex(18, &cp1, sizeof cp1);
    print_hex(19, &cp2, sizeof cp2);
    print_hex(20, &cp3, sizeof cp3);
    print_hex(21, &dp1, sizeof dp1);
    print_hex(22, &dp2, sizeof dp2);
    print_hex(23, &mv1, sizeof mv1);
    print_hex(24, &mv2, sizeof mv2);
    print_hex(25, &mv3, sizeof mv3);
}

static void case_multi_ldst(void)
{
    int8x8x3_t x3 = vld1_s8_x3(buf8);
    int16x8x4_t x4 = vld1q_s16_x4(buf16);
    int8x8x3_t l3 = vld3_lane_s8(buf8 + 32, x3, 2);
    int16x4x4_t l4 = vld4_lane_s16(buf16 + 16,
                                   vld4_s16(buf16), 1);
    print_hex(26, &x3, sizeof x3);
    print_hex(27, &x4, sizeof x4);
    print_hex(28, &l3, sizeof l3);
    print_hex(29, &l4, sizeof l4);
    vst1_s8_x3(buf8 + 40, x3);
    vst1q_s16_x4(buf16 + 8, x4);
    vst3_lane_s8(buf8 + 56, l3, 0);
    vst4_lane_s16(buf16 + 24, l4, 3);
    print_hex(30, buf8, 64);
    print_hex(31, buf16, sizeof buf16);
    {
        float32x2x2_t d2 = vld2_dup_f32(bufF);
        poly8x16x3_t d3 = vld3q_dup_p8((const poly8_t *)buf8);
        print_hex(32, &d2, sizeof d2);
        print_hex(33, &d3, sizeof d3);
    }
}

int main(void)
{
    fill_seq();
    case_struct_forms();
    fill_seq();
    case_widen_reduce();
    fill_seq();
    case_abs_cmp();
    fill_seq();
    case_lane_misc();
    fill_seq();
    case_multi_ldst();
    return fails != 0;
}
