/* test-simd128.c - exercises include/wasm_simd128.h; every function
   returns a scalar so tests/run-simd128.mjs can assert the value. */
#include "wasm_simd128.h"

int h_load_store(void)
{
    int in[4] = { 1, 2, 3, 4 };
    int out[4];
    v128_t a = wasm_v128_load(in);
    v128_t b = wasm_i32x4_add(a, wasm_i32x4_splat(10));
    wasm_v128_store(out, b);
    return out[0] * 1000 + out[1] * 100 + out[2] * 10 + out[3];
}

int h_const(void)
{
    v128_t a = wasm_i32x4_const(1, 2, 3, 4);
    return wasm_i32x4_extract_lane(a, 0) * 1000
         + wasm_i32x4_extract_lane(a, 1) * 100
         + wasm_i32x4_extract_lane(a, 2) * 10
         + wasm_i32x4_extract_lane(a, 3);
}

int h_sat(void)
{
    v128_t a = wasm_i16x8_const(30000, -30000, 0, 0, 0, 0, 0, 0);
    v128_t b = wasm_i16x8_const(10000, -10000, 0, 0, 0, 0, 0, 0);
    v128_t c = wasm_i16x8_add_sat_s(a, b);
    return wasm_i16x8_extract_lane_s(c, 0) + wasm_i16x8_extract_lane_s(c, 1);
}

int h_minmax(void)
{
    v128_t a = wasm_i32x4_const(5, -3, 100, 0);
    v128_t b = wasm_i32x4_const(7, -9, 50, 1);
    v128_t lo = wasm_i32x4_min_s(a, b);
    v128_t hi = wasm_i32x4_max_s(a, b);
    return wasm_i32x4_extract_lane(lo, 2) + wasm_i32x4_extract_lane(hi, 1)
         + wasm_i32x4_extract_lane(hi, 3);
}

float h_farith(void)
{
    v128_t a = wasm_f32x4_const(1.5f, 2.0f, 3.0f, 4.0f);
    v128_t b = wasm_f32x4_const(2.0f, 2.0f, 2.0f, 2.0f);
    v128_t c = wasm_f32x4_mul(a, b);
    v128_t d = wasm_f32x4_add(c, wasm_f32x4_splat(1.0f));
    return wasm_f32x4_extract_lane(d, 2);
}

int h_cmp_bitmask(void)
{
    v128_t a = wasm_i32x4_const(10, 3, 10, 3);
    v128_t b = wasm_i32x4_const(5, 5, 5, 5);
    v128_t m = wasm_i32x4_gt_s(a, b);
    return wasm_i32x4_bitmask(m);
}

int h_shifts(void)
{
    v128_t a = wasm_i32x4_const(-16, 3, 0, 0);
    v128_t s = wasm_i32x4_shr_s(a, 2);
    v128_t u = wasm_i32x4_shr_u(a, 2);
    v128_t l = wasm_i32x4_shl(wasm_i32x4_const(3, 0, 0, 0), 4);
    return wasm_i32x4_extract_lane(s, 0)
         + (wasm_i32x4_extract_lane(u, 0) == 1073741820 ? 100 : 0)
         + wasm_i32x4_extract_lane(l, 0);
}

int h_shuffle(void)
{
    v128_t a = wasm_i8x16_const(0, 1, 2, 3, 4, 5, 6, 7,
                                8, 9, 10, 11, 12, 13, 14, 15);
    v128_t r = wasm_i8x16_shuffle(a, a, 15, 14, 13, 12, 11, 10, 9, 8,
                                  7, 6, 5, 4, 3, 2, 1, 0);
    v128_t w = wasm_i8x16_swizzle(a, r);
    return wasm_i8x16_extract_lane_u(r, 0) * 16
         + wasm_i8x16_extract_lane_u(r, 15)
         + wasm_i8x16_extract_lane_u(w, 0);
}

int h_convert(void)
{
    v128_t a = wasm_i32x4_const(3, -2, 0, 0);
    v128_t f = wasm_f32x4_convert_i32x4_s(a);
    v128_t r = wasm_i32x4_trunc_sat_f32x4_s(f);
    return wasm_i32x4_extract_lane(r, 0) * 10 + wasm_i32x4_extract_lane(r, 1);
}

int h_extend_narrow(void)
{
    v128_t a = wasm_i8x16_const(-1, 2, 0, 0, 0, 0, 0, 0,
                                100, 0, 0, 0, 0, 0, 0, 0);
    v128_t w = wasm_i16x8_extend_low_i8x16_s(a);
    v128_t n = wasm_i8x16_narrow_i16x8_s(
        wasm_i16x8_const(1000, -1000, 5, 0, 0, 0, 0, 0),
        wasm_i16x8_const(7, 0, 0, 0, 0, 0, 0, 0));
    return wasm_i16x8_extract_lane_s(w, 0) * 1000
         + wasm_i16x8_extract_lane_s(w, 1)
         + wasm_i8x16_extract_lane_s(n, 0)
         + wasm_i8x16_extract_lane_s(n, 8);
}

int h_dot_extmul(void)
{
    v128_t a = wasm_i16x8_const(1, 2, 3, 4, 0, 0, 0, 0);
    v128_t b = wasm_i16x8_const(5, 6, 7, 8, 0, 0, 0, 0);
    v128_t d = wasm_i32x4_dot_i16x8_s(a, b);
    v128_t e = wasm_i16x8_extmul_low_i8x16_s(
        wasm_i8x16_const(3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        wasm_i8x16_const(5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    return wasm_i32x4_extract_lane(d, 0) * 1000
         + wasm_i32x4_extract_lane(d, 1)
         + wasm_i16x8_extract_lane_s(e, 1);
}

int h_avgr_popcnt(void)
{
    v128_t a = wasm_i8x16_splat(100);
    v128_t b = wasm_i8x16_splat(51);
    v128_t r = wasm_i8x16_avgr_u(a, b);
    v128_t p = wasm_i8x16_popcnt(wasm_i8x16_const(
        0x7f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    return wasm_i8x16_extract_lane_u(r, 3) * 10
         + wasm_i8x16_extract_lane_u(p, 0);
}

int h_reduce(void)
{
    v128_t nz = wasm_i32x4_const(1, 0, 0, 0);
    v128_t all = wasm_i32x4_const(-1, -1, -1, -1);
    return wasm_v128_any_true(nz) * 4
         + wasm_i32x4_all_true(all) * 2
         + wasm_v128_any_true(wasm_v128_zero());
}

double h_f64(void)
{
    v128_t a = wasm_f64x2_splat(9.0);
    v128_t r = wasm_f64x2_sqrt(a);
    v128_t s = wasm_f64x2_add(r, wasm_f64x2_splat(1.5));
    return wasm_f64x2_extract_lane(s, 1);
}

int h_replace(void)
{
    v128_t a = wasm_i32x4_const(1, 2, 3, 4);
    v128_t b = wasm_i32x4_replace_lane(a, 2, 99);
    return wasm_i32x4_extract_lane(b, 2) + wasm_i32x4_extract_lane(b, 0);
}

int h_load_splat(void)
{
    int x = 7;
    v128_t a = wasm_v128_load_splat_i32(&x);
    return wasm_i32x4_extract_lane(a, 3);
}

int h_bitops(void)
{
    v128_t a = wasm_i32x4_const(0x0f0f, 0, 0, 0);
    v128_t b = wasm_i32x4_const(0x3333, 0, 0, 0);
    v128_t c = wasm_v128_andnot(wasm_v128_or(a, b), wasm_v128_and(a, b));
    return wasm_i32x4_extract_lane(c, 0);
}
