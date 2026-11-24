/* wasm_simd128.h - WebAssembly SIMD128 intrinsics for the TCC wasm
   backend.
 *
 * v128_t is a 128-bit vector value.  Every intrinsic below is a
 * macro: the compiler does not inline functions, so a function form
 * would cost a call per operation; the macro expands at the call
 * site to operations the backend emits directly - GCC vector
 * operators on vector_size(16) types, the __builtin_wasm_*
 * lane/splat/shuffle builtins, and the __builtin_wasm_vop1 /
 * __builtin_wasm_vop2 opcode builtins.  A C cast between two
 * vector types of the same size is a bit reinterpretation in this
 * backend; value conversions go through the convert / trunc_sat
 * intrinsics.
 *
 * Lane indices of extract/replace/shuffle are compile-time
 * constants (they are wasm immediates).
 * i64x2 splat and i64x2 lane extract/replace are not provided: the
 * backend has no 64-bit scalar register to carry the lane value.
 */
#ifndef _WASM_SIMD128_H
#define _WASM_SIMD128_H

typedef int v128_t __attribute__((vector_size(16)));

typedef signed char   __wasm_i8x16  __attribute__((vector_size(16)));
typedef short         __wasm_i16x8  __attribute__((vector_size(16)));
typedef int           __wasm_i32x4  __attribute__((vector_size(16)));
typedef long long     __wasm_i64x2  __attribute__((vector_size(16)));
typedef unsigned char __wasm_u8x16  __attribute__((vector_size(16)));
typedef unsigned short __wasm_u16x8 __attribute__((vector_size(16)));
typedef unsigned int  __wasm_u32x4  __attribute__((vector_size(16)));
typedef unsigned long long __wasm_u64x2 __attribute__((vector_size(16)));
typedef float         __wasm_f32x4  __attribute__((vector_size(16)));
typedef double        __wasm_f64x2  __attribute__((vector_size(16)));

/* ---- load / store ------------------------------------------------ */

#define wasm_v128_load(p) (*(const v128_t *)(p))
#define wasm_v128_store(p, v) (*(v128_t *)(p) = (v))
#define wasm_v128_zero() ((v128_t)__builtin_wasm_splat(17, 0, __wasm_i32x4))
#define wasm_v128_load_splat_i8(p) ((v128_t)__builtin_wasm_splat(15, *(const signed char *)(p), __wasm_i8x16))
#define wasm_v128_load_splat_i16(p) ((v128_t)__builtin_wasm_splat(16, *(const short *)(p), __wasm_i16x8))
#define wasm_v128_load_splat_i32(p) ((v128_t)__builtin_wasm_splat(17, *(const int *)(p), __wasm_i32x4))
#define wasm_v128_load_splat_f32(p) ((v128_t)__builtin_wasm_splat(19, *(const float *)(p), __wasm_f32x4))
#define wasm_v128_load_splat_f64(p) ((v128_t)__builtin_wasm_splat(20, *(const double *)(p), __wasm_f64x2))

/* ---- constants ---------------------------------------------------- */

#define wasm_i32x4_const(a, b, c, d) ((v128_t)(__wasm_i32x4){ (a), (b), (c), (d) })
#define wasm_f32x4_const(a, b, c, d) ((v128_t)(__wasm_f32x4){ (a), (b), (c), (d) })
#define wasm_i16x8_const(a, b, c, d, e, f, g, h) ((v128_t)(__wasm_i16x8){ (a), (b), (c), (d), (e), (f), (g), (h) })
#define wasm_i8x16_const(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p) ((v128_t)(__wasm_i8x16){ (a), (b), (c), (d), (e), (f), (g), (h), (i), (j), (k), (l), (m), (n), (o), (p) })

/* ---- splat -------------------------------------------------------- */

#define wasm_i8x16_splat(x) ((v128_t)__builtin_wasm_splat(15, (x), __wasm_i8x16))
#define wasm_i16x8_splat(x) ((v128_t)__builtin_wasm_splat(16, (x), __wasm_i16x8))
#define wasm_i32x4_splat(x) ((v128_t)__builtin_wasm_splat(17, (x), __wasm_i32x4))
#define wasm_f32x4_splat(x) ((v128_t)__builtin_wasm_splat(19, (x), __wasm_f32x4))
#define wasm_f64x2_splat(x) ((v128_t)__builtin_wasm_splat(20, (x), __wasm_f64x2))

/* ---- lane extract / replace (lane is a compile-time constant) ----- */

#define wasm_i8x16_extract_lane_s(v, lane) \
    ((signed char)__builtin_wasm_extract(21, (__wasm_i8x16)(v), lane, int))
#define wasm_i8x16_extract_lane_u(v, lane) \
    ((unsigned char)__builtin_wasm_extract(22, (__wasm_i8x16)(v), lane, int))
#define wasm_i16x8_extract_lane_s(v, lane) \
    ((short)__builtin_wasm_extract(24, (__wasm_i16x8)(v), lane, int))
#define wasm_i16x8_extract_lane_u(v, lane) \
    ((unsigned short)__builtin_wasm_extract(25, (__wasm_i16x8)(v), lane, int))
#define wasm_i32x4_extract_lane(v, lane) \
    __builtin_wasm_extract(27, (__wasm_i32x4)(v), lane, int)
#define wasm_f32x4_extract_lane(v, lane) \
    __builtin_wasm_extract(31, (__wasm_f32x4)(v), lane, float)
#define wasm_f64x2_extract_lane(v, lane) \
    __builtin_wasm_extract(33, (__wasm_f64x2)(v), lane, double)

#define wasm_i8x16_replace_lane(v, lane, x) \
    ((v128_t)__builtin_wasm_replace(23, (__wasm_i8x16)(v), lane, x))
#define wasm_i16x8_replace_lane(v, lane, x) \
    ((v128_t)__builtin_wasm_replace(26, (__wasm_i16x8)(v), lane, x))
#define wasm_i32x4_replace_lane(v, lane, x) \
    ((v128_t)__builtin_wasm_replace(28, (__wasm_i32x4)(v), lane, x))
#define wasm_f32x4_replace_lane(v, lane, x) \
    ((v128_t)__builtin_wasm_replace(32, (__wasm_f32x4)(v), lane, x))
#define wasm_f64x2_replace_lane(v, lane, x) \
    ((v128_t)__builtin_wasm_replace(34, (__wasm_f64x2)(v), lane, x))

/* ---- shuffle / swizzle --------------------------------------------- */

#define wasm_i8x16_shuffle(a, b, l0, l1, l2, l3, l4, l5, l6, l7, \
                           l8, l9, l10, l11, l12, l13, l14, l15) \
    ((v128_t)__builtin_wasm_shuffle((__wasm_i8x16)(a), (__wasm_i8x16)(b), \
        l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15))

#define wasm_i8x16_swizzle(a, idx) ((v128_t)__builtin_wasm_vop2(14, (__wasm_u8x16)(a), (__wasm_u8x16)(idx), __wasm_u8x16))

/* ---- bitwise ------------------------------------------------------- */

#define wasm_v128_and(a, b) ((a) & (b))
#define wasm_v128_or(a, b) ((a) | (b))
#define wasm_v128_xor(a, b) ((a) ^ (b))
#define wasm_v128_not(a) (~(a))
#define wasm_v128_andnot(a, b) ((a) & ~(b))

/* ---- integer arithmetic --------------------------------------------- */


#define wasm_i8x16_add(a, b) ((v128_t)((__wasm_i8x16)(a) + (__wasm_i8x16)(b)))
#define wasm_i8x16_sub(a, b) ((v128_t)((__wasm_i8x16)(a) - (__wasm_i8x16)(b)))
#define wasm_i8x16_mul(a, b) ((v128_t)((__wasm_i8x16)(a) * (__wasm_i8x16)(b)))
#define wasm_i16x8_add(a, b) ((v128_t)((__wasm_i16x8)(a) + (__wasm_i16x8)(b)))
#define wasm_i16x8_sub(a, b) ((v128_t)((__wasm_i16x8)(a) - (__wasm_i16x8)(b)))
#define wasm_i16x8_mul(a, b) ((v128_t)((__wasm_i16x8)(a) * (__wasm_i16x8)(b)))
#define wasm_i32x4_add(a, b) ((v128_t)((__wasm_i32x4)(a) + (__wasm_i32x4)(b)))
#define wasm_i32x4_sub(a, b) ((v128_t)((__wasm_i32x4)(a) - (__wasm_i32x4)(b)))
#define wasm_i32x4_mul(a, b) ((v128_t)((__wasm_i32x4)(a) * (__wasm_i32x4)(b)))
#define wasm_i64x2_add(a, b) ((v128_t)((__wasm_i64x2)(a) + (__wasm_i64x2)(b)))
#define wasm_i64x2_sub(a, b) ((v128_t)((__wasm_i64x2)(a) - (__wasm_i64x2)(b)))
#define wasm_i64x2_mul(a, b) ((v128_t)((__wasm_i64x2)(a) * (__wasm_i64x2)(b)))

#define wasm_i8x16_neg(a) ((v128_t)(-(__wasm_i8x16)(a)))
#define wasm_i16x8_neg(a) ((v128_t)(-(__wasm_i16x8)(a)))
#define wasm_i32x4_neg(a) ((v128_t)(-(__wasm_i32x4)(a)))
#define wasm_i64x2_neg(a) ((v128_t)(-(__wasm_i64x2)(a)))

/* saturating arithmetic: wasm has these only for 8/16-bit lanes */

#define wasm_i8x16_add_sat_s(a, b) ((v128_t)__builtin_wasm_vop2(111, (__wasm_i8x16)(a), (__wasm_i8x16)(b), __wasm_i8x16))
#define wasm_i8x16_add_sat_u(a, b) ((v128_t)__builtin_wasm_vop2(112, (__wasm_u8x16)(a), (__wasm_u8x16)(b), __wasm_u8x16))
#define wasm_i8x16_sub_sat_s(a, b) ((v128_t)__builtin_wasm_vop2(114, (__wasm_i8x16)(a), (__wasm_i8x16)(b), __wasm_i8x16))
#define wasm_i8x16_sub_sat_u(a, b) ((v128_t)__builtin_wasm_vop2(115, (__wasm_u8x16)(a), (__wasm_u8x16)(b), __wasm_u8x16))
#define wasm_i16x8_add_sat_s(a, b) ((v128_t)__builtin_wasm_vop2(143, (__wasm_i16x8)(a), (__wasm_i16x8)(b), __wasm_i16x8))
#define wasm_i16x8_add_sat_u(a, b) ((v128_t)__builtin_wasm_vop2(144, (__wasm_u16x8)(a), (__wasm_u16x8)(b), __wasm_u16x8))
#define wasm_i16x8_sub_sat_s(a, b) ((v128_t)__builtin_wasm_vop2(146, (__wasm_i16x8)(a), (__wasm_i16x8)(b), __wasm_i16x8))
#define wasm_i16x8_sub_sat_u(a, b) ((v128_t)__builtin_wasm_vop2(147, (__wasm_u16x8)(a), (__wasm_u16x8)(b), __wasm_u16x8))

/* min / max */
#define wasm_i8x16_min_s(a, b) ((v128_t)__builtin_wasm_vop2(118, (__wasm_i8x16)(a), (__wasm_i8x16)(b), __wasm_i8x16))
#define wasm_i8x16_min_u(a, b) ((v128_t)__builtin_wasm_vop2(119, (__wasm_u8x16)(a), (__wasm_u8x16)(b), __wasm_u8x16))
#define wasm_i8x16_max_s(a, b) ((v128_t)__builtin_wasm_vop2(120, (__wasm_i8x16)(a), (__wasm_i8x16)(b), __wasm_i8x16))
#define wasm_i8x16_max_u(a, b) ((v128_t)__builtin_wasm_vop2(121, (__wasm_u8x16)(a), (__wasm_u8x16)(b), __wasm_u8x16))
#define wasm_i16x8_min_s(a, b) ((v128_t)__builtin_wasm_vop2(150, (__wasm_i16x8)(a), (__wasm_i16x8)(b), __wasm_i16x8))
#define wasm_i16x8_min_u(a, b) ((v128_t)__builtin_wasm_vop2(151, (__wasm_u16x8)(a), (__wasm_u16x8)(b), __wasm_u16x8))
#define wasm_i16x8_max_s(a, b) ((v128_t)__builtin_wasm_vop2(152, (__wasm_i16x8)(a), (__wasm_i16x8)(b), __wasm_i16x8))
#define wasm_i16x8_max_u(a, b) ((v128_t)__builtin_wasm_vop2(153, (__wasm_u16x8)(a), (__wasm_u16x8)(b), __wasm_u16x8))
#define wasm_i32x4_min_s(a, b) ((v128_t)__builtin_wasm_vop2(182, (__wasm_i32x4)(a), (__wasm_i32x4)(b), __wasm_i32x4))
#define wasm_i32x4_min_u(a, b) ((v128_t)__builtin_wasm_vop2(183, (__wasm_u32x4)(a), (__wasm_u32x4)(b), __wasm_u32x4))
#define wasm_i32x4_max_s(a, b) ((v128_t)__builtin_wasm_vop2(184, (__wasm_i32x4)(a), (__wasm_i32x4)(b), __wasm_i32x4))
#define wasm_i32x4_max_u(a, b) ((v128_t)__builtin_wasm_vop2(185, (__wasm_u32x4)(a), (__wasm_u32x4)(b), __wasm_u32x4))
#define wasm_f32x4_min(a, b) ((v128_t)__builtin_wasm_vop2(232, (__wasm_f32x4)(a), (__wasm_f32x4)(b), __wasm_f32x4))
#define wasm_f32x4_max(a, b) ((v128_t)__builtin_wasm_vop2(233, (__wasm_f32x4)(a), (__wasm_f32x4)(b), __wasm_f32x4))
#define wasm_f64x2_min(a, b) ((v128_t)__builtin_wasm_vop2(244, (__wasm_f64x2)(a), (__wasm_f64x2)(b), __wasm_f64x2))
#define wasm_f64x2_max(a, b) ((v128_t)__builtin_wasm_vop2(245, (__wasm_f64x2)(a), (__wasm_f64x2)(b), __wasm_f64x2))
#define wasm_f32x4_pmin(a, b) ((v128_t)__builtin_wasm_vop2(234, (__wasm_f32x4)(a), (__wasm_f32x4)(b), __wasm_f32x4))
#define wasm_f32x4_pmax(a, b) ((v128_t)__builtin_wasm_vop2(235, (__wasm_f32x4)(a), (__wasm_f32x4)(b), __wasm_f32x4))
#define wasm_f64x2_pmin(a, b) ((v128_t)__builtin_wasm_vop2(246, (__wasm_f64x2)(a), (__wasm_f64x2)(b), __wasm_f64x2))
#define wasm_f64x2_pmax(a, b) ((v128_t)__builtin_wasm_vop2(247, (__wasm_f64x2)(a), (__wasm_f64x2)(b), __wasm_f64x2))

/* rounding average (unsigned) */
#define wasm_i8x16_avgr_u(a, b) ((v128_t)__builtin_wasm_vop2(123, (__wasm_u8x16)(a), (__wasm_u8x16)(b), __wasm_u8x16))
#define wasm_i16x8_avgr_u(a, b) ((v128_t)__builtin_wasm_vop2(155, (__wasm_u16x8)(a), (__wasm_u16x8)(b), __wasm_u16x8))

/* dot product of signed 16-bit lanes */
#define wasm_i32x4_dot_i16x8_s(a, b) ((v128_t)__builtin_wasm_vop2(186, (__wasm_i16x8)(a), (__wasm_i16x8)(b), __wasm_i32x4))

/* binary operations whose result lane shape differs from the
   operands' (extmul, narrow) */

/* extended multiply (i8x16 -> i16x8) */
#define wasm_i16x8_extmul_low_i8x16_s(a, b) ((v128_t)__builtin_wasm_vop2(156, (__wasm_i8x16)(a), (__wasm_i8x16)(b), __wasm_i16x8))
#define wasm_i16x8_extmul_high_i8x16_s(a, b) ((v128_t)__builtin_wasm_vop2(157, (__wasm_i8x16)(a), (__wasm_i8x16)(b), __wasm_i16x8))
#define wasm_i16x8_extmul_low_i8x16_u(a, b) ((v128_t)__builtin_wasm_vop2(158, (__wasm_u8x16)(a), (__wasm_u8x16)(b), __wasm_u16x8))
#define wasm_i16x8_extmul_high_i8x16_u(a, b) ((v128_t)__builtin_wasm_vop2(159, (__wasm_u8x16)(a), (__wasm_u8x16)(b), __wasm_u16x8))

/* ---- float arithmetic ------------------------------------------------ */

#define wasm_f32x4_add(a, b) ((v128_t)((__wasm_f32x4)(a) + (__wasm_f32x4)(b)))
#define wasm_f32x4_sub(a, b) ((v128_t)((__wasm_f32x4)(a) - (__wasm_f32x4)(b)))
#define wasm_f32x4_mul(a, b) ((v128_t)((__wasm_f32x4)(a) * (__wasm_f32x4)(b)))
#define wasm_f32x4_div(a, b) ((v128_t)((__wasm_f32x4)(a) / (__wasm_f32x4)(b)))
#define wasm_f64x2_add(a, b) ((v128_t)((__wasm_f64x2)(a) + (__wasm_f64x2)(b)))
#define wasm_f64x2_sub(a, b) ((v128_t)((__wasm_f64x2)(a) - (__wasm_f64x2)(b)))
#define wasm_f64x2_mul(a, b) ((v128_t)((__wasm_f64x2)(a) * (__wasm_f64x2)(b)))
#define wasm_f64x2_div(a, b) ((v128_t)((__wasm_f64x2)(a) / (__wasm_f64x2)(b)))

/* ---- unary operations (vop1) ------------------------------------------ */


#define wasm_i8x16_abs(a) ((v128_t)__builtin_wasm_vop1(96, (__wasm_i8x16)(a), __wasm_i8x16))
#define wasm_i16x8_abs(a) ((v128_t)__builtin_wasm_vop1(128, (__wasm_i16x8)(a), __wasm_i16x8))
#define wasm_i32x4_abs(a) ((v128_t)__builtin_wasm_vop1(160, (__wasm_i32x4)(a), __wasm_i32x4))
#define wasm_i64x2_abs(a) ((v128_t)__builtin_wasm_vop1(192, (__wasm_i64x2)(a), __wasm_i64x2))
#define wasm_f32x4_abs(a) ((v128_t)__builtin_wasm_vop1(224, (__wasm_f32x4)(a), __wasm_f32x4))
#define wasm_f64x2_abs(a) ((v128_t)__builtin_wasm_vop1(236, (__wasm_f64x2)(a), __wasm_f64x2))
#define wasm_f32x4_neg(a) ((v128_t)__builtin_wasm_vop1(225, (__wasm_f32x4)(a), __wasm_f32x4))
#define wasm_f64x2_neg(a) ((v128_t)__builtin_wasm_vop1(237, (__wasm_f64x2)(a), __wasm_f64x2))
#define wasm_f32x4_sqrt(a) ((v128_t)__builtin_wasm_vop1(227, (__wasm_f32x4)(a), __wasm_f32x4))
#define wasm_f64x2_sqrt(a) ((v128_t)__builtin_wasm_vop1(239, (__wasm_f64x2)(a), __wasm_f64x2))
#define wasm_f32x4_ceil(a) ((v128_t)__builtin_wasm_vop1(103, (__wasm_f32x4)(a), __wasm_f32x4))
#define wasm_f32x4_floor(a) ((v128_t)__builtin_wasm_vop1(104, (__wasm_f32x4)(a), __wasm_f32x4))
#define wasm_f32x4_trunc(a) ((v128_t)__builtin_wasm_vop1(105, (__wasm_f32x4)(a), __wasm_f32x4))
#define wasm_f32x4_nearest(a) ((v128_t)__builtin_wasm_vop1(106, (__wasm_f32x4)(a), __wasm_f32x4))
#define wasm_f64x2_ceil(a) ((v128_t)__builtin_wasm_vop1(116, (__wasm_f64x2)(a), __wasm_f64x2))
#define wasm_f64x2_floor(a) ((v128_t)__builtin_wasm_vop1(117, (__wasm_f64x2)(a), __wasm_f64x2))
#define wasm_f64x2_trunc(a) ((v128_t)__builtin_wasm_vop1(122, (__wasm_f64x2)(a), __wasm_f64x2))
#define wasm_f64x2_nearest(a) ((v128_t)__builtin_wasm_vop1(148, (__wasm_f64x2)(a), __wasm_f64x2))
#define wasm_i8x16_popcnt(a) ((v128_t)__builtin_wasm_vop1(98, (__wasm_i8x16)(a), __wasm_i8x16))

/* widening extends */
#define wasm_i16x8_extend_low_i8x16_s(a) ((v128_t)__builtin_wasm_vop1(135, (__wasm_i8x16)(a), __wasm_i16x8))
#define wasm_i16x8_extend_high_i8x16_s(a) ((v128_t)__builtin_wasm_vop1(136, (__wasm_i8x16)(a), __wasm_i16x8))
#define wasm_i16x8_extend_low_i8x16_u(a) ((v128_t)__builtin_wasm_vop1(137, (__wasm_i8x16)(a), __wasm_u16x8))
#define wasm_i16x8_extend_high_i8x16_u(a) ((v128_t)__builtin_wasm_vop1(138, (__wasm_i8x16)(a), __wasm_u16x8))
#define wasm_i32x4_extend_low_i16x8_s(a) ((v128_t)__builtin_wasm_vop1(167, (__wasm_i16x8)(a), __wasm_i32x4))
#define wasm_i32x4_extend_high_i16x8_s(a) ((v128_t)__builtin_wasm_vop1(168, (__wasm_i16x8)(a), __wasm_i32x4))
#define wasm_i32x4_extend_low_i16x8_u(a) ((v128_t)__builtin_wasm_vop1(169, (__wasm_i16x8)(a), __wasm_u32x4))
#define wasm_i32x4_extend_high_i16x8_u(a) ((v128_t)__builtin_wasm_vop1(170, (__wasm_i16x8)(a), __wasm_u32x4))
#define wasm_i64x2_extend_low_i32x4_s(a) ((v128_t)__builtin_wasm_vop1(199, (__wasm_i32x4)(a), __wasm_i64x2))
#define wasm_i64x2_extend_high_i32x4_s(a) ((v128_t)__builtin_wasm_vop1(200, (__wasm_i32x4)(a), __wasm_i64x2))
#define wasm_i64x2_extend_low_i32x4_u(a) ((v128_t)__builtin_wasm_vop1(201, (__wasm_i32x4)(a), __wasm_u64x2))
#define wasm_i64x2_extend_high_i32x4_u(a) ((v128_t)__builtin_wasm_vop1(202, (__wasm_i32x4)(a), __wasm_u64x2))

/* narrowing */
#define wasm_i8x16_narrow_i16x8_s(a, b) ((v128_t)__builtin_wasm_vop2(101, (__wasm_i16x8)(a), (__wasm_i16x8)(b), __wasm_i8x16))
#define wasm_i8x16_narrow_i16x8_u(a, b) ((v128_t)__builtin_wasm_vop2(102, (__wasm_i16x8)(a), (__wasm_i16x8)(b), __wasm_i8x16))
#define wasm_i16x8_narrow_i32x4_s(a, b) ((v128_t)__builtin_wasm_vop2(133, (__wasm_i32x4)(a), (__wasm_i32x4)(b), __wasm_i16x8))
#define wasm_i16x8_narrow_i32x4_u(a, b) ((v128_t)__builtin_wasm_vop2(134, (__wasm_i32x4)(a), (__wasm_i32x4)(b), __wasm_i16x8))

/* int <-> float conversions */
#define wasm_i32x4_trunc_sat_f32x4_s(a) ((v128_t)__builtin_wasm_vop1(248, (__wasm_f32x4)(a), __wasm_i32x4))
#define wasm_i32x4_trunc_sat_f32x4_u(a) ((v128_t)__builtin_wasm_vop1(249, (__wasm_f32x4)(a), __wasm_u32x4))
#define wasm_f32x4_convert_i32x4_s(a) ((v128_t)__builtin_wasm_vop1(250, (__wasm_i32x4)(a), __wasm_f32x4))
#define wasm_f32x4_convert_i32x4_u(a) ((v128_t)__builtin_wasm_vop1(251, (__wasm_u32x4)(a), __wasm_f32x4))
#define wasm_i32x4_trunc_sat_f64x2_s_zero(a) ((v128_t)__builtin_wasm_vop1(252, (__wasm_f64x2)(a), __wasm_i32x4))
#define wasm_i32x4_trunc_sat_f64x2_u_zero(a) ((v128_t)__builtin_wasm_vop1(253, (__wasm_f64x2)(a), __wasm_u32x4))
#define wasm_f64x2_convert_low_i32x4_s(a) ((v128_t)__builtin_wasm_vop1(254, (__wasm_i32x4)(a), __wasm_f64x2))
#define wasm_f64x2_convert_low_i32x4_u(a) ((v128_t)__builtin_wasm_vop1(255, (__wasm_u32x4)(a), __wasm_f64x2))
#define wasm_f32x4_demote_f64x2_zero(a) ((v128_t)__builtin_wasm_vop1(94, (__wasm_f64x2)(a), __wasm_f32x4))
#define wasm_f64x2_promote_low_f32x4(a) ((v128_t)__builtin_wasm_vop1(95, (__wasm_f32x4)(a), __wasm_f64x2))

/* ---- comparisons (result: all-ones / all-zero lanes) ------------------- */


#define wasm_i8x16_eq(a, b) ((v128_t)((__wasm_i8x16)(a) == (__wasm_i8x16)(b)))
#define wasm_i8x16_ne(a, b) ((v128_t)((__wasm_i8x16)(a) != (__wasm_i8x16)(b)))
#define wasm_i8x16_lt_s(a, b) ((v128_t)((__wasm_i8x16)(a) < (__wasm_i8x16)(b)))
#define wasm_i8x16_lt_u(a, b) ((v128_t)((__wasm_u8x16)(a) < (__wasm_u8x16)(b)))
#define wasm_i8x16_gt_s(a, b) ((v128_t)((__wasm_i8x16)(a) > (__wasm_i8x16)(b)))
#define wasm_i8x16_gt_u(a, b) ((v128_t)((__wasm_u8x16)(a) > (__wasm_u8x16)(b)))
#define wasm_i8x16_le_s(a, b) ((v128_t)((__wasm_i8x16)(a) <= (__wasm_i8x16)(b)))
#define wasm_i8x16_le_u(a, b) ((v128_t)((__wasm_u8x16)(a) <= (__wasm_u8x16)(b)))
#define wasm_i8x16_ge_s(a, b) ((v128_t)((__wasm_i8x16)(a) >= (__wasm_i8x16)(b)))
#define wasm_i8x16_ge_u(a, b) ((v128_t)((__wasm_u8x16)(a) >= (__wasm_u8x16)(b)))
#define wasm_i16x8_eq(a, b) ((v128_t)((__wasm_i16x8)(a) == (__wasm_i16x8)(b)))
#define wasm_i16x8_ne(a, b) ((v128_t)((__wasm_i16x8)(a) != (__wasm_i16x8)(b)))
#define wasm_i16x8_lt_s(a, b) ((v128_t)((__wasm_i16x8)(a) < (__wasm_i16x8)(b)))
#define wasm_i16x8_lt_u(a, b) ((v128_t)((__wasm_u16x8)(a) < (__wasm_u16x8)(b)))
#define wasm_i16x8_gt_s(a, b) ((v128_t)((__wasm_i16x8)(a) > (__wasm_i16x8)(b)))
#define wasm_i16x8_gt_u(a, b) ((v128_t)((__wasm_u16x8)(a) > (__wasm_u16x8)(b)))
#define wasm_i16x8_le_s(a, b) ((v128_t)((__wasm_i16x8)(a) <= (__wasm_i16x8)(b)))
#define wasm_i16x8_le_u(a, b) ((v128_t)((__wasm_u16x8)(a) <= (__wasm_u16x8)(b)))
#define wasm_i16x8_ge_s(a, b) ((v128_t)((__wasm_i16x8)(a) >= (__wasm_i16x8)(b)))
#define wasm_i16x8_ge_u(a, b) ((v128_t)((__wasm_u16x8)(a) >= (__wasm_u16x8)(b)))
#define wasm_i32x4_eq(a, b) ((v128_t)((__wasm_i32x4)(a) == (__wasm_i32x4)(b)))
#define wasm_i32x4_ne(a, b) ((v128_t)((__wasm_i32x4)(a) != (__wasm_i32x4)(b)))
#define wasm_i32x4_lt_s(a, b) ((v128_t)((__wasm_i32x4)(a) < (__wasm_i32x4)(b)))
#define wasm_i32x4_lt_u(a, b) ((v128_t)((__wasm_u32x4)(a) < (__wasm_u32x4)(b)))
#define wasm_i32x4_gt_s(a, b) ((v128_t)((__wasm_i32x4)(a) > (__wasm_i32x4)(b)))
#define wasm_i32x4_gt_u(a, b) ((v128_t)((__wasm_u32x4)(a) > (__wasm_u32x4)(b)))
#define wasm_i32x4_le_s(a, b) ((v128_t)((__wasm_i32x4)(a) <= (__wasm_i32x4)(b)))
#define wasm_i32x4_le_u(a, b) ((v128_t)((__wasm_u32x4)(a) <= (__wasm_u32x4)(b)))
#define wasm_i32x4_ge_s(a, b) ((v128_t)((__wasm_i32x4)(a) >= (__wasm_i32x4)(b)))
#define wasm_i32x4_ge_u(a, b) ((v128_t)((__wasm_u32x4)(a) >= (__wasm_u32x4)(b)))
#define wasm_i64x2_eq(a, b) ((v128_t)((__wasm_i64x2)(a) == (__wasm_i64x2)(b)))
#define wasm_i64x2_ne(a, b) ((v128_t)((__wasm_i64x2)(a) != (__wasm_i64x2)(b)))
#define wasm_i64x2_lt_s(a, b) ((v128_t)((__wasm_i64x2)(a) < (__wasm_i64x2)(b)))
#define wasm_i64x2_gt_s(a, b) ((v128_t)((__wasm_i64x2)(a) > (__wasm_i64x2)(b)))
#define wasm_i64x2_le_s(a, b) ((v128_t)((__wasm_i64x2)(a) <= (__wasm_i64x2)(b)))
#define wasm_i64x2_ge_s(a, b) ((v128_t)((__wasm_i64x2)(a) >= (__wasm_i64x2)(b)))
#define wasm_f32x4_eq(a, b) ((v128_t)((__wasm_f32x4)(a) == (__wasm_f32x4)(b)))
#define wasm_f32x4_ne(a, b) ((v128_t)((__wasm_f32x4)(a) != (__wasm_f32x4)(b)))
#define wasm_f32x4_lt(a, b) ((v128_t)((__wasm_f32x4)(a) < (__wasm_f32x4)(b)))
#define wasm_f32x4_gt(a, b) ((v128_t)((__wasm_f32x4)(a) > (__wasm_f32x4)(b)))
#define wasm_f32x4_le(a, b) ((v128_t)((__wasm_f32x4)(a) <= (__wasm_f32x4)(b)))
#define wasm_f32x4_ge(a, b) ((v128_t)((__wasm_f32x4)(a) >= (__wasm_f32x4)(b)))
#define wasm_f64x2_eq(a, b) ((v128_t)((__wasm_f64x2)(a) == (__wasm_f64x2)(b)))
#define wasm_f64x2_ne(a, b) ((v128_t)((__wasm_f64x2)(a) != (__wasm_f64x2)(b)))
#define wasm_f64x2_lt(a, b) ((v128_t)((__wasm_f64x2)(a) < (__wasm_f64x2)(b)))
#define wasm_f64x2_gt(a, b) ((v128_t)((__wasm_f64x2)(a) > (__wasm_f64x2)(b)))
#define wasm_f64x2_le(a, b) ((v128_t)((__wasm_f64x2)(a) <= (__wasm_f64x2)(b)))
#define wasm_f64x2_ge(a, b) ((v128_t)((__wasm_f64x2)(a) >= (__wasm_f64x2)(b)))

/* ---- shifts (uniform: one scalar count for all lanes) ------------------ */


#define wasm_i8x16_shl(a, n) ((v128_t)((__wasm_i8x16)(a) << (int)(n)))
#define wasm_i8x16_shr_s(a, n) ((v128_t)((__wasm_i8x16)(a) >> (int)(n)))
#define wasm_i8x16_shr_u(a, n) ((v128_t)((__wasm_u8x16)(a) >> (int)(n)))
#define wasm_i16x8_shl(a, n) ((v128_t)((__wasm_i16x8)(a) << (int)(n)))
#define wasm_i16x8_shr_s(a, n) ((v128_t)((__wasm_i16x8)(a) >> (int)(n)))
#define wasm_i16x8_shr_u(a, n) ((v128_t)((__wasm_u16x8)(a) >> (int)(n)))
#define wasm_i32x4_shl(a, n) ((v128_t)((__wasm_i32x4)(a) << (int)(n)))
#define wasm_i32x4_shr_s(a, n) ((v128_t)((__wasm_i32x4)(a) >> (int)(n)))
#define wasm_i32x4_shr_u(a, n) ((v128_t)((__wasm_u32x4)(a) >> (int)(n)))
#define wasm_i64x2_shl(a, n) ((v128_t)((__wasm_i64x2)(a) << (int)(n)))
#define wasm_i64x2_shr_s(a, n) ((v128_t)((__wasm_i64x2)(a) >> (int)(n)))
#define wasm_i64x2_shr_u(a, n) ((v128_t)((__wasm_u64x2)(a) >> (int)(n)))

/* ---- reductions to a scalar --------------------------------------------- */


#define wasm_i8x16_bitmask(a) __builtin_wasm_vop1(100, (__wasm_i8x16)(a), int)
#define wasm_i16x8_bitmask(a) __builtin_wasm_vop1(132, (__wasm_i16x8)(a), int)
#define wasm_i32x4_bitmask(a) __builtin_wasm_vop1(164, (__wasm_i32x4)(a), int)
#define wasm_i64x2_bitmask(a) __builtin_wasm_vop1(196, (__wasm_i64x2)(a), int)
#define wasm_i8x16_all_true(a) __builtin_wasm_vop1(99, (__wasm_i8x16)(a), int)
#define wasm_i16x8_all_true(a) __builtin_wasm_vop1(131, (__wasm_i16x8)(a), int)
#define wasm_i32x4_all_true(a) __builtin_wasm_vop1(163, (__wasm_i32x4)(a), int)
#define wasm_i64x2_all_true(a) __builtin_wasm_vop1(195, (__wasm_i64x2)(a), int)
#define wasm_v128_any_true(a) __builtin_wasm_vop1(83, (__wasm_i32x4)(a), int)


#endif /* _WASM_SIMD128_H */
