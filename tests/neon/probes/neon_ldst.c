/* Hand-written probes for the load/store, lane and reinterpret
   surface: structured loads and stores, lane loads, splats,
   lane extract/insert/copy, combine and split. */
#include "neon_probe.h"

static int8_t buf8[64];
static int32_t buf32[16];
static float bufF[16];

static void fill_seq(void)
{
    int i;
    for (i = 0; i < 64; i++) buf8[i] = (int8_t)(i * 7 - 100);
    for (i = 0; i < 16; i++) buf32[i] = i * 1000 - 7000;
    for (i = 0; i < 16; i++) bufF[i] = ftab[i & 7] * (i + 1);
}

static void case_ldst_basic(void)
{
    int8x16_t a = vld1q_s8(buf8);
    int32x4_t b = vld1q_s32(buf32);
    float32x4_t c = vld1q_f32(bufF);
    int8x8_t d = vld1_s8(buf8 + 40);
    print_hex(1, &a, sizeof a);
    print_hex(2, &b, sizeof b);
    print_hex(3, &c, sizeof c);
    print_hex(4, &d, sizeof d);
    vst1q_s8(buf8 + 16, a);
    vst1q_s32(buf32 + 4, b);
    vst1q_f32(bufF + 4, c);
    vst1_s8(buf8 + 48, d);
    print_hex(5, buf8, 64);
    print_hex(6, buf32, sizeof buf32);
    print_hex(7, bufF, sizeof bufF);
}

static void case_ldst_struct(void)
{
    int8x8x2_t p2 = vld2_s8(buf8);
    int8x8x3_t p3 = vld3_s8(buf8);
    int8x8x4_t p4 = vld4_s8(buf8);
    int32x4x2_t q2 = vld2q_s32(buf32);
    print_hex(10, &p2, sizeof p2);
    print_hex(11, &p3, sizeof p3);
    print_hex(12, &p4, sizeof p4);
    print_hex(13, &q2, sizeof q2);
    vst2_s8(buf8 + 16, p2);
    vst3_s8(buf8 + 32, p3);
    vst4_s8(buf8 + 48, p4);
    print_hex(14, buf8, 64);
    {
        int8x16x2_t x2 = vld1q_s8_x2(buf8);
        int8x8x3_t x3 = vld1_s8_x3(buf8);
        print_hex(15, &x2, sizeof x2);
        print_hex(16, &x3, sizeof x3);
        vst1q_s8_x2(buf8 + 16, x2);
        vst1_s8_x3(buf8 + 40, x3);
        print_hex(17, buf8, 64);
    }
}

static void case_ldst_lane(void)
{
    int8x8_t a = vld1_s8(buf8);
    int8x8_t b = vld1_lane_s8(buf8 + 20, a, 3);
    int16x8_t c = vld1q_lane_s16((const int16_t *)(buf8 + 8),
                                 vld1q_s16((const int16_t *)buf8), 5);
    print_hex(20, &b, sizeof b);
    print_hex(21, &c, sizeof c);
    vst1_lane_s8(buf8 + 30, b, 6);
    print_hex(22, buf8 + 24, 16);
    {
        int8x8x2_t base = vld2_s8(buf8);
        int8x8x2_t r = vld2_lane_s8(buf8 + 33, base, 2);
        print_hex(23, &r, sizeof r);
    }
    {
        int8x8_t d = vld1_dup_s8(buf8 + 9);
        int32x2_t e = vld1_dup_s32(buf32 + 3);
        print_hex(24, &d, sizeof d);
        print_hex(25, &e, sizeof e);
    }
}

static void case_lanes(void)
{
    int32x4_t a = vld1q_s32(buf32);
    int32x2_t lo = vget_low_s32(a), hi = vget_high_s32(a);
    int32x4_t back = vcombine_s32(lo, hi);
    int s = vgetq_lane_s32(a, 2);
    int32x4_t b = vsetq_lane_s32(-777, a, 1);
    int32x2_t d = vdup_lane_s32(lo, 1);
    int32x4_t e = vdupq_lane_s32(hi, 0);
    int32x2_t f = vdup_laneq_s32(a, 3);
    int32x4_t g = vdupq_laneq_s32(a, 0);
    float32x4_t h = vdupq_n_f32(2.5f);
    int8x16_t m = vmovq_n_s8(-3);
    print_hex(30, &back, sizeof back);
    if (s != buf32[2]) mismatch("vgetq_lane_s32");
    print_hex(31, &b, sizeof b);
    print_hex(32, &d, sizeof d);
    print_hex(33, &e, sizeof e);
    print_hex(34, &f, sizeof f);
    print_hex(35, &g, sizeof g);
    print_hex(36, &h, sizeof h);
    print_hex(37, &m, sizeof m);
    {
        int16x4_t x = vld1_s16((const int16_t *)buf8);
        int16x4_t y = vcopy_lane_s16(x, 2, vget_high_s16(
            vld1q_s16((const int16_t *)(buf8 + 16))), 1);
        int16x8_t z = vcopyq_laneq_s16(vld1q_s16((const int16_t *)buf8),
                                       0, vld1q_s16((const int16_t *)(buf8 + 8)), 7);
        print_hex(38, &y, sizeof y);
        print_hex(39, &z, sizeof z);
    }
}

static void case_reinterpret_create(void)
{
    int8x16_t a = vld1q_s8(buf8);
    uint32x4_t u = vreinterpretq_u32_s8(a);
    float32x4_t f = vreinterpretq_f32_u32(u);
    int16x8_t h = vreinterpretq_s16_f32(f);
    int64x1_t c = vcreate_s64(0x0102030405060708LL);
    uint8x8_t z = vcreate_u8(0xffeeddccbbaa9988ULL);
    print_hex(40, &u, sizeof u);
    print_hex(41, &f, sizeof f);
    print_hex(42, &h, sizeof h);
    print_hex(43, &c, sizeof c);
    print_hex(44, &z, sizeof z);
    {
        int8x8_t d = vld1_s8(buf8);
        int32x2_t w = vreinterpret_s32_s8(d);
        uint64x1_t q = vreinterpret_u64_s32(w);
        print_hex(45, &w, sizeof w);
        print_hex(46, &q, sizeof q);
    }
}

static void case_lddup(void);

int main(void)
{
    fill_seq();
    case_ldst_basic();
    fill_seq();
    case_ldst_struct();
    fill_seq();
    case_ldst_lane();
    fill_seq();
    case_lanes();
    fill_seq();
    case_reinterpret_create();
    case_lddup();
    return fails != 0;
}

static void case_lddup(void)
{
    /* vldN_dup broadcasts a different source element into each
       member register */
    static const short src2[2] = { -7, 99 };
    static const short src3[3] = { -7, 99, 5 };
    static const short src4[4] = { -7, 99, 5, -300 };
    static const int isrc2[2] = { 123456, -77 };
    int16x4x2_t r2 = vld2_dup_s16(src2);
    int16x4x3_t r3 = vld3_dup_s16(src3);
    int16x8x4_t r4 = vld4q_dup_s16(src4);
    int32x4x2_t r5 = vld2q_dup_s32(isrc2);
    short out16[8];
    int out32[4];
    int i, j;
    for (j = 0; j < 2; j++) {
        vst1_s16(out16, r2.val[j]);
        for (i = 0; i < 4; i++)
            if (out16[i] != src2[j]) mismatch("vld2_dup_s16");
    }
    for (j = 0; j < 3; j++) {
        vst1_s16(out16, r3.val[j]);
        for (i = 0; i < 4; i++)
            if (out16[i] != src3[j]) mismatch("vld3_dup_s16");
    }
    for (j = 0; j < 4; j++) {
        vst1q_s16(out16, r4.val[j]);
        for (i = 0; i < 8; i++)
            if (out16[i] != src4[j]) mismatch("vld4q_dup_s16");
    }
    for (j = 0; j < 2; j++) {
        vst1q_s32(out32, r5.val[j]);
        for (i = 0; i < 4; i++)
            if (out32[i] != isrc2[j]) mismatch("vld2q_dup_s32");
    }
    print_hex(90, &r2, sizeof r2);
    print_hex(91, &r4, sizeof r4);
}
