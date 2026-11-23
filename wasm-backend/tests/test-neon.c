/* test-neon.c - exercises include/arm_neon.h; every function
   returns a scalar so tests/run-neon.mjs can assert the value. */
#include "arm_neon.h"

int n_add(void)
{
    int32x4_t a = { 1, 2, 3, 4 };
    int32x4_t b = { 10, 20, 30, 40 };
    int32x4_t c = vaddq_s32(a, b);
    return c[0] * 1000 + c[1] * 100 + c[2] * 10 + c[3];
}

int n_sat(void)
{
    int16x8_t a = vdupq_n_s16(30000);
    int16x8_t b = vdupq_n_s16(10000);
    int16x8_t c = vqaddq_s16(a, b);
    return vgetq_lane_s16(c, 5);
}

int n_minmax(void)
{
    int32x4_t a = { 5, -3, 100, 0 };
    int32x4_t b = { 7, -9, 50, 1 };
    int32x4_t mx = vmaxq_s32(a, b);
    int32x4_t mn = vminq_s32(a, b);
    return mx[1] + mn[2];
}

int n_cmp(void)
{
    int32x4_t a = { 10, 3, 10, 3 };
    int32x4_t b = { 5, 5, 5, 5 };
    uint32x4_t m = vcgtq_s32(a, b);
    uint32x4_t e = vceqq_s32(a, b);
    return (int)vgetq_lane_u32(m, 0) + (int)vgetq_lane_u32(m, 1)
         + (int)vgetq_lane_u32(e, 0);
}

int n_absneg(void)
{
    int32x4_t a = { -5, 3, 0, -100 };
    int32x4_t ab = vabsq_s32(a);
    int32x4_t ng = vnegq_s32(a);
    return ab[3] + ng[0];
}

int n_zip(void)
{
    int32x4_t a = { 1, 2, 3, 4 };
    int32x4_t b = { 5, 6, 7, 8 };
    int32x4_t z = vzip1q_s32(a, b);
    int32x4_t u = vuzp1q_s32(a, b);
    int32x4_t t = vtrn1q_s32(a, b);
    return z[1] * 1000 + u[3] * 100 + t[2] * 10 + z[3];
}

int n_zip8(void)
{
    int8x16_t a = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    int8x16_t b = { 16, 17, 18, 19, 20, 21, 22, 23,
                    24, 25, 26, 27, 28, 29, 30, 31 };
    int8x16_t z = vzip1q_s8(a, b);
    return z[0] * 100 + z[1] * 10 + z[3];
}

int n_tbl(void)
{
    uint8x16_t a = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    uint8x16_t idx = { 15, 14, 13, 12, 11, 10, 9, 8,
                       7, 6, 5, 4, 3, 2, 1, 0 };
    uint8x16_t r = vtbl1q_u8(a, idx);
    return r[0] * 16 + r[15];
}

int n_duplane(void)
{
    int32x4_t a = { 1, 2, 3, 4 };
    int32x4_t d = vdupq_laneq_s32(a, 2);
    return d[0] * 10 + d[3];
}

int n_shift(void)
{
    int32x4_t a = { -16, 3, 0, 0 };
    int32x4_t sr = vshrq_n_s32(a, 2);
    int32x4_t sl = vshlq_n_s32(a, 1);
    return sr[0] + sl[1];
}

int n_ldst(void)
{
    int in[4] = { 9, 8, 7, 6 };
    int out[4];
    int32x4_t v = vld1q_s32(in);
    vst1q_s32(out, vaddq_s32(v, vdupq_n_s32(1)));
    return out[0] * 1000 + out[1] * 100 + out[2] * 10 + out[3];
}

float n_f32(void)
{
    float32x4_t a = { 1.5f, 2.0f, 3.0f, 4.0f };
    float32x4_t b = vdupq_n_f32(2.0f);
    float32x4_t c = vmulq_f32(a, b);
    return vgetq_lane_f32(c, 2);
}

int n_setlane(void)
{
    int32x4_t a = { 1, 2, 3, 4 };
    int32x4_t b = vsetq_lane_s32(99, a, 1);
    return b[1] * 10 + b[0];
}

static const long long n_i64a[2] = { 1000000, 5 };
static const long long n_i64b[2] = { 3, 7 };
static long long n_i64r[2];

long long *n_mul64(void)
{
    int64x2_t a = vld1q_s64(n_i64a);
    int64x2_t b = vld1q_s64(n_i64b);
    vst1q_s64(n_i64r, vmulq_s64(a, b));
    return n_i64r;
}

int n_reinterpret(void)
{
    uint32x4_t u = { 5, 5, 5, 5 };
    int32x4_t s = vreinterpretq_s32_u32(u);
    uint32x4_t m = vceqq_s32(s, vdupq_n_s32(5));
    return (int)vgetq_lane_u32(m, 2) + (int)vgetq_lane_u32(m, 0);
}
