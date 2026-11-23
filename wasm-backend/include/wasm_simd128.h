/* wasm_simd128.h - WebAssembly SIMD128 intrinsics for the TCC wasm
   backend.
 *
 * v128_t is a 128-bit vector value.  Every function below maps to
 * operations the backend emits directly: GCC vector operators on
 * vector_size(16) types, the __builtin_wasm_* lane/splat/shuffle
 * builtins, and the __builtin_wasm_vop1/__builtin_wasm_vop2 opcode
 * builtins.  A C cast between two vector types of the same size is a
 * bit reinterpretation in this backend; value conversions go through
 * the convert / trunc_sat intrinsics.
 *
 * Lane indices of extract/replace/shuffle are compile-time constants
 * (they are wasm immediates), so those intrinsics are macros.
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

static inline v128_t wasm_v128_load(const void *p)
{
    return *(const v128_t *)p;
}
static inline void wasm_v128_store(void *p, v128_t v)
{
    *(v128_t *)p = v;
}
static inline v128_t wasm_v128_zero(void)
{
    v128_t v = { 0, 0, 0, 0 };
    return v;
}
static inline v128_t wasm_v128_load_splat_i8(const void *p)
{
    return (v128_t)__builtin_wasm_splat(15, *(const signed char *)p, __wasm_i8x16);
}
static inline v128_t wasm_v128_load_splat_i16(const void *p)
{
    return (v128_t)__builtin_wasm_splat(16, *(const short *)p, __wasm_i16x8);
}
static inline v128_t wasm_v128_load_splat_i32(const void *p)
{
    return (v128_t)__builtin_wasm_splat(17, *(const int *)p, __wasm_i32x4);
}
static inline v128_t wasm_v128_load_splat_f32(const void *p)
{
    return (v128_t)__builtin_wasm_splat(19, *(const float *)p, __wasm_f32x4);
}
static inline v128_t wasm_v128_load_splat_f64(const void *p)
{
    return (v128_t)__builtin_wasm_splat(20, *(const double *)p, __wasm_f64x2);
}

/* ---- constants ---------------------------------------------------- */

static inline v128_t wasm_i32x4_const(int a, int b, int c, int d)
{
    __wasm_i32x4 v = { a, b, c, d };
    return (v128_t)v;
}
static inline v128_t wasm_f32x4_const(float a, float b, float c, float d)
{
    __wasm_f32x4 v = { a, b, c, d };
    return (v128_t)v;
}
static inline v128_t wasm_i16x8_const(short a, short b, short c, short d,
                                      short e, short f, short g, short h)
{
    __wasm_i16x8 v = { a, b, c, d, e, f, g, h };
    return (v128_t)v;
}
static inline v128_t wasm_i8x16_const(signed char a, signed char b,
                                      signed char c, signed char d,
                                      signed char e, signed char f,
                                      signed char g, signed char h,
                                      signed char i, signed char j,
                                      signed char k, signed char l,
                                      signed char m, signed char n,
                                      signed char o, signed char p)
{
    __wasm_i8x16 v = { a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p };
    return (v128_t)v;
}

/* ---- splat -------------------------------------------------------- */

static inline v128_t wasm_i8x16_splat(signed char x)
{
    return (v128_t)__builtin_wasm_splat(15, x, __wasm_i8x16);
}
static inline v128_t wasm_i16x8_splat(short x)
{
    return (v128_t)__builtin_wasm_splat(16, x, __wasm_i16x8);
}
static inline v128_t wasm_i32x4_splat(int x)
{
    return (v128_t)__builtin_wasm_splat(17, x, __wasm_i32x4);
}
static inline v128_t wasm_f32x4_splat(float x)
{
    return (v128_t)__builtin_wasm_splat(19, x, __wasm_f32x4);
}
static inline v128_t wasm_f64x2_splat(double x)
{
    return (v128_t)__builtin_wasm_splat(20, x, __wasm_f64x2);
}

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

static inline v128_t wasm_i8x16_swizzle(v128_t a, v128_t idx)
{
    return (v128_t)__builtin_wasm_vop2(14, (__wasm_u8x16)a, (__wasm_u8x16)idx,
                                       __wasm_u8x16);
}

/* ---- bitwise ------------------------------------------------------- */

static inline v128_t wasm_v128_and(v128_t a, v128_t b) { return a & b; }
static inline v128_t wasm_v128_or(v128_t a, v128_t b)  { return a | b; }
static inline v128_t wasm_v128_xor(v128_t a, v128_t b) { return a ^ b; }
static inline v128_t wasm_v128_not(v128_t a)           { return ~a; }
static inline v128_t wasm_v128_andnot(v128_t a, v128_t b) { return a & ~b; }

/* ---- integer arithmetic --------------------------------------------- */

#define __WASM_INT_ARITH(name, shape, op) \
    static inline v128_t name(v128_t a, v128_t b) \
    { return (v128_t)((shape)a op (shape)b); }

__WASM_INT_ARITH(wasm_i8x16_add, __wasm_i8x16, +)
__WASM_INT_ARITH(wasm_i8x16_sub, __wasm_i8x16, -)
__WASM_INT_ARITH(wasm_i8x16_mul, __wasm_i8x16, *)
__WASM_INT_ARITH(wasm_i16x8_add, __wasm_i16x8, +)
__WASM_INT_ARITH(wasm_i16x8_sub, __wasm_i16x8, -)
__WASM_INT_ARITH(wasm_i16x8_mul, __wasm_i16x8, *)
__WASM_INT_ARITH(wasm_i32x4_add, __wasm_i32x4, +)
__WASM_INT_ARITH(wasm_i32x4_sub, __wasm_i32x4, -)
__WASM_INT_ARITH(wasm_i32x4_mul, __wasm_i32x4, *)
__WASM_INT_ARITH(wasm_i64x2_add, __wasm_i64x2, +)
__WASM_INT_ARITH(wasm_i64x2_sub, __wasm_i64x2, -)
__WASM_INT_ARITH(wasm_i64x2_mul, __wasm_i64x2, *)

static inline v128_t wasm_i8x16_neg(v128_t a)
{
    return (v128_t)(-(__wasm_i8x16)a);
}
static inline v128_t wasm_i16x8_neg(v128_t a)
{
    return (v128_t)(-(__wasm_i16x8)a);
}
static inline v128_t wasm_i32x4_neg(v128_t a)
{
    return (v128_t)(-(__wasm_i32x4)a);
}
static inline v128_t wasm_i64x2_neg(v128_t a)
{
    return (v128_t)(-(__wasm_i64x2)a);
}

/* saturating arithmetic: wasm has these only for 8/16-bit lanes */
#define __WASM_SAT(name, opc, shape) \
    static inline v128_t name(v128_t a, v128_t b) \
    { return (v128_t)__builtin_wasm_vop2(opc, (shape)a, (shape)b, shape); }

__WASM_SAT(wasm_i8x16_add_sat_s, 111, __wasm_i8x16)
__WASM_SAT(wasm_i8x16_add_sat_u, 112, __wasm_u8x16)
__WASM_SAT(wasm_i8x16_sub_sat_s, 114, __wasm_i8x16)
__WASM_SAT(wasm_i8x16_sub_sat_u, 115, __wasm_u8x16)
__WASM_SAT(wasm_i16x8_add_sat_s, 143, __wasm_i16x8)
__WASM_SAT(wasm_i16x8_add_sat_u, 144, __wasm_u16x8)
__WASM_SAT(wasm_i16x8_sub_sat_s, 146, __wasm_i16x8)
__WASM_SAT(wasm_i16x8_sub_sat_u, 147, __wasm_u16x8)

/* min / max */
__WASM_SAT(wasm_i8x16_min_s, 118, __wasm_i8x16)
__WASM_SAT(wasm_i8x16_min_u, 119, __wasm_u8x16)
__WASM_SAT(wasm_i8x16_max_s, 120, __wasm_i8x16)
__WASM_SAT(wasm_i8x16_max_u, 121, __wasm_u8x16)
__WASM_SAT(wasm_i16x8_min_s, 150, __wasm_i16x8)
__WASM_SAT(wasm_i16x8_min_u, 151, __wasm_u16x8)
__WASM_SAT(wasm_i16x8_max_s, 152, __wasm_i16x8)
__WASM_SAT(wasm_i16x8_max_u, 153, __wasm_u16x8)
__WASM_SAT(wasm_i32x4_min_s, 182, __wasm_i32x4)
__WASM_SAT(wasm_i32x4_min_u, 183, __wasm_u32x4)
__WASM_SAT(wasm_i32x4_max_s, 184, __wasm_i32x4)
__WASM_SAT(wasm_i32x4_max_u, 185, __wasm_u32x4)
__WASM_SAT(wasm_f32x4_min, 232, __wasm_f32x4)
__WASM_SAT(wasm_f32x4_max, 233, __wasm_f32x4)
__WASM_SAT(wasm_f64x2_min, 244, __wasm_f64x2)
__WASM_SAT(wasm_f64x2_max, 245, __wasm_f64x2)
__WASM_SAT(wasm_f32x4_pmin, 234, __wasm_f32x4)
__WASM_SAT(wasm_f32x4_pmax, 235, __wasm_f32x4)
__WASM_SAT(wasm_f64x2_pmin, 246, __wasm_f64x2)
__WASM_SAT(wasm_f64x2_pmax, 247, __wasm_f64x2)

/* rounding average (unsigned) */
__WASM_SAT(wasm_i8x16_avgr_u, 123, __wasm_u8x16)
__WASM_SAT(wasm_i16x8_avgr_u, 155, __wasm_u16x8)

/* dot product of signed 16-bit lanes */
static inline v128_t wasm_i32x4_dot_i16x8_s(v128_t a, v128_t b)
{
    return (v128_t)__builtin_wasm_vop2(186, (__wasm_i16x8)a, (__wasm_i16x8)b,
                                        __wasm_i32x4);
}

/* binary operations whose result lane shape differs from the
   operands' (extmul, narrow) */
#define __WASM_VOP2X(name, opc, inshape, outshape) \
    static inline v128_t name(v128_t a, v128_t b) \
    { return (v128_t)__builtin_wasm_vop2(opc, (inshape)a, (inshape)b, outshape); }

/* extended multiply (i8x16 -> i16x8) */
__WASM_VOP2X(wasm_i16x8_extmul_low_i8x16_s, 156, __wasm_i8x16, __wasm_i16x8)
__WASM_VOP2X(wasm_i16x8_extmul_high_i8x16_s, 157, __wasm_i8x16, __wasm_i16x8)
__WASM_VOP2X(wasm_i16x8_extmul_low_i8x16_u, 158, __wasm_u8x16, __wasm_u16x8)
__WASM_VOP2X(wasm_i16x8_extmul_high_i8x16_u, 159, __wasm_u8x16, __wasm_u16x8)

/* ---- float arithmetic ------------------------------------------------ */

__WASM_INT_ARITH(wasm_f32x4_add, __wasm_f32x4, +)
__WASM_INT_ARITH(wasm_f32x4_sub, __wasm_f32x4, -)
__WASM_INT_ARITH(wasm_f32x4_mul, __wasm_f32x4, *)
__WASM_INT_ARITH(wasm_f32x4_div, __wasm_f32x4, /)
__WASM_INT_ARITH(wasm_f64x2_add, __wasm_f64x2, +)
__WASM_INT_ARITH(wasm_f64x2_sub, __wasm_f64x2, -)
__WASM_INT_ARITH(wasm_f64x2_mul, __wasm_f64x2, *)
__WASM_INT_ARITH(wasm_f64x2_div, __wasm_f64x2, /)

/* ---- unary operations (vop1) ------------------------------------------ */

#define __WASM_VOP1(name, opc, inshape, outshape) \
    static inline v128_t name(v128_t a) \
    { return (v128_t)__builtin_wasm_vop1(opc, (inshape)a, outshape); }

__WASM_VOP1(wasm_i8x16_abs, 96, __wasm_i8x16, __wasm_i8x16)
__WASM_VOP1(wasm_i16x8_abs, 128, __wasm_i16x8, __wasm_i16x8)
__WASM_VOP1(wasm_i32x4_abs, 160, __wasm_i32x4, __wasm_i32x4)
__WASM_VOP1(wasm_i64x2_abs, 192, __wasm_i64x2, __wasm_i64x2)
__WASM_VOP1(wasm_f32x4_abs, 224, __wasm_f32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f64x2_abs, 236, __wasm_f64x2, __wasm_f64x2)
__WASM_VOP1(wasm_f32x4_neg, 225, __wasm_f32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f64x2_neg, 237, __wasm_f64x2, __wasm_f64x2)
__WASM_VOP1(wasm_f32x4_sqrt, 227, __wasm_f32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f64x2_sqrt, 239, __wasm_f64x2, __wasm_f64x2)
__WASM_VOP1(wasm_f32x4_ceil, 103, __wasm_f32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f32x4_floor, 104, __wasm_f32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f32x4_trunc, 105, __wasm_f32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f32x4_nearest, 106, __wasm_f32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f64x2_ceil, 116, __wasm_f64x2, __wasm_f64x2)
__WASM_VOP1(wasm_f64x2_floor, 117, __wasm_f64x2, __wasm_f64x2)
__WASM_VOP1(wasm_f64x2_trunc, 122, __wasm_f64x2, __wasm_f64x2)
__WASM_VOP1(wasm_f64x2_nearest, 148, __wasm_f64x2, __wasm_f64x2)
__WASM_VOP1(wasm_i8x16_popcnt, 98, __wasm_i8x16, __wasm_i8x16)

/* widening extends */
__WASM_VOP1(wasm_i16x8_extend_low_i8x16_s, 135, __wasm_i8x16, __wasm_i16x8)
__WASM_VOP1(wasm_i16x8_extend_high_i8x16_s, 136, __wasm_i8x16, __wasm_i16x8)
__WASM_VOP1(wasm_i16x8_extend_low_i8x16_u, 137, __wasm_i8x16, __wasm_u16x8)
__WASM_VOP1(wasm_i16x8_extend_high_i8x16_u, 138, __wasm_i8x16, __wasm_u16x8)
__WASM_VOP1(wasm_i32x4_extend_low_i16x8_s, 167, __wasm_i16x8, __wasm_i32x4)
__WASM_VOP1(wasm_i32x4_extend_high_i16x8_s, 168, __wasm_i16x8, __wasm_i32x4)
__WASM_VOP1(wasm_i32x4_extend_low_i16x8_u, 169, __wasm_i16x8, __wasm_u32x4)
__WASM_VOP1(wasm_i32x4_extend_high_i16x8_u, 170, __wasm_i16x8, __wasm_u32x4)
__WASM_VOP1(wasm_i64x2_extend_low_i32x4_s, 199, __wasm_i32x4, __wasm_i64x2)
__WASM_VOP1(wasm_i64x2_extend_high_i32x4_s, 200, __wasm_i32x4, __wasm_i64x2)
__WASM_VOP1(wasm_i64x2_extend_low_i32x4_u, 201, __wasm_i32x4, __wasm_u64x2)
__WASM_VOP1(wasm_i64x2_extend_high_i32x4_u, 202, __wasm_i32x4, __wasm_u64x2)

/* narrowing */
__WASM_VOP2X(wasm_i8x16_narrow_i16x8_s, 101, __wasm_i16x8, __wasm_i8x16)
__WASM_VOP2X(wasm_i8x16_narrow_i16x8_u, 102, __wasm_i16x8, __wasm_i8x16)
__WASM_VOP2X(wasm_i16x8_narrow_i32x4_s, 133, __wasm_i32x4, __wasm_i16x8)
__WASM_VOP2X(wasm_i16x8_narrow_i32x4_u, 134, __wasm_i32x4, __wasm_i16x8)

/* int <-> float conversions */
__WASM_VOP1(wasm_i32x4_trunc_sat_f32x4_s, 248, __wasm_f32x4, __wasm_i32x4)
__WASM_VOP1(wasm_i32x4_trunc_sat_f32x4_u, 249, __wasm_f32x4, __wasm_u32x4)
__WASM_VOP1(wasm_f32x4_convert_i32x4_s, 250, __wasm_i32x4, __wasm_f32x4)
__WASM_VOP1(wasm_f32x4_convert_i32x4_u, 251, __wasm_u32x4, __wasm_f32x4)
__WASM_VOP1(wasm_i32x4_trunc_sat_f64x2_s_zero, 252, __wasm_f64x2, __wasm_i32x4)
__WASM_VOP1(wasm_i32x4_trunc_sat_f64x2_u_zero, 253, __wasm_f64x2, __wasm_u32x4)
__WASM_VOP1(wasm_f64x2_convert_low_i32x4_s, 254, __wasm_i32x4, __wasm_f64x2)
__WASM_VOP1(wasm_f64x2_convert_low_i32x4_u, 255, __wasm_u32x4, __wasm_f64x2)
__WASM_VOP1(wasm_f32x4_demote_f64x2_zero, 94, __wasm_f64x2, __wasm_f32x4)
__WASM_VOP1(wasm_f64x2_promote_low_f32x4, 95, __wasm_f32x4, __wasm_f64x2)

/* ---- comparisons (result: all-ones / all-zero lanes) ------------------- */

#define __WASM_CMP(name, shape, op) \
    static inline v128_t name(v128_t a, v128_t b) \
    { return (v128_t)((shape)a op (shape)b); }

__WASM_CMP(wasm_i8x16_eq, __wasm_i8x16, ==)
__WASM_CMP(wasm_i8x16_ne, __wasm_i8x16, !=)
__WASM_CMP(wasm_i8x16_lt_s, __wasm_i8x16, <)
__WASM_CMP(wasm_i8x16_lt_u, __wasm_u8x16, <)
__WASM_CMP(wasm_i8x16_gt_s, __wasm_i8x16, >)
__WASM_CMP(wasm_i8x16_gt_u, __wasm_u8x16, >)
__WASM_CMP(wasm_i8x16_le_s, __wasm_i8x16, <=)
__WASM_CMP(wasm_i8x16_le_u, __wasm_u8x16, <=)
__WASM_CMP(wasm_i8x16_ge_s, __wasm_i8x16, >=)
__WASM_CMP(wasm_i8x16_ge_u, __wasm_u8x16, >=)
__WASM_CMP(wasm_i16x8_eq, __wasm_i16x8, ==)
__WASM_CMP(wasm_i16x8_ne, __wasm_i16x8, !=)
__WASM_CMP(wasm_i16x8_lt_s, __wasm_i16x8, <)
__WASM_CMP(wasm_i16x8_lt_u, __wasm_u16x8, <)
__WASM_CMP(wasm_i16x8_gt_s, __wasm_i16x8, >)
__WASM_CMP(wasm_i16x8_gt_u, __wasm_u16x8, >)
__WASM_CMP(wasm_i16x8_le_s, __wasm_i16x8, <=)
__WASM_CMP(wasm_i16x8_le_u, __wasm_u16x8, <=)
__WASM_CMP(wasm_i16x8_ge_s, __wasm_i16x8, >=)
__WASM_CMP(wasm_i16x8_ge_u, __wasm_u16x8, >=)
__WASM_CMP(wasm_i32x4_eq, __wasm_i32x4, ==)
__WASM_CMP(wasm_i32x4_ne, __wasm_i32x4, !=)
__WASM_CMP(wasm_i32x4_lt_s, __wasm_i32x4, <)
__WASM_CMP(wasm_i32x4_lt_u, __wasm_u32x4, <)
__WASM_CMP(wasm_i32x4_gt_s, __wasm_i32x4, >)
__WASM_CMP(wasm_i32x4_gt_u, __wasm_u32x4, >)
__WASM_CMP(wasm_i32x4_le_s, __wasm_i32x4, <=)
__WASM_CMP(wasm_i32x4_le_u, __wasm_u32x4, <=)
__WASM_CMP(wasm_i32x4_ge_s, __wasm_i32x4, >=)
__WASM_CMP(wasm_i32x4_ge_u, __wasm_u32x4, >=)
__WASM_CMP(wasm_i64x2_eq, __wasm_i64x2, ==)
__WASM_CMP(wasm_i64x2_ne, __wasm_i64x2, !=)
__WASM_CMP(wasm_i64x2_lt_s, __wasm_i64x2, <)
__WASM_CMP(wasm_i64x2_gt_s, __wasm_i64x2, >)
__WASM_CMP(wasm_i64x2_le_s, __wasm_i64x2, <=)
__WASM_CMP(wasm_i64x2_ge_s, __wasm_i64x2, >=)
__WASM_CMP(wasm_f32x4_eq, __wasm_f32x4, ==)
__WASM_CMP(wasm_f32x4_ne, __wasm_f32x4, !=)
__WASM_CMP(wasm_f32x4_lt, __wasm_f32x4, <)
__WASM_CMP(wasm_f32x4_gt, __wasm_f32x4, >)
__WASM_CMP(wasm_f32x4_le, __wasm_f32x4, <=)
__WASM_CMP(wasm_f32x4_ge, __wasm_f32x4, >=)
__WASM_CMP(wasm_f64x2_eq, __wasm_f64x2, ==)
__WASM_CMP(wasm_f64x2_ne, __wasm_f64x2, !=)
__WASM_CMP(wasm_f64x2_lt, __wasm_f64x2, <)
__WASM_CMP(wasm_f64x2_gt, __wasm_f64x2, >)
__WASM_CMP(wasm_f64x2_le, __wasm_f64x2, <=)
__WASM_CMP(wasm_f64x2_ge, __wasm_f64x2, >=)

/* ---- shifts (uniform: one scalar count for all lanes) ------------------ */

#define __WASM_SHIFT(name, shape, op) \
    static inline v128_t name(v128_t a, unsigned int n) \
    { return (v128_t)((shape)a op (int)n); }

__WASM_SHIFT(wasm_i8x16_shl, __wasm_i8x16, <<)
__WASM_SHIFT(wasm_i8x16_shr_s, __wasm_i8x16, >>)
__WASM_SHIFT(wasm_i8x16_shr_u, __wasm_u8x16, >>)
__WASM_SHIFT(wasm_i16x8_shl, __wasm_i16x8, <<)
__WASM_SHIFT(wasm_i16x8_shr_s, __wasm_i16x8, >>)
__WASM_SHIFT(wasm_i16x8_shr_u, __wasm_u16x8, >>)
__WASM_SHIFT(wasm_i32x4_shl, __wasm_i32x4, <<)
__WASM_SHIFT(wasm_i32x4_shr_s, __wasm_i32x4, >>)
__WASM_SHIFT(wasm_i32x4_shr_u, __wasm_u32x4, >>)
__WASM_SHIFT(wasm_i64x2_shl, __wasm_i64x2, <<)
__WASM_SHIFT(wasm_i64x2_shr_s, __wasm_i64x2, >>)
__WASM_SHIFT(wasm_i64x2_shr_u, __wasm_u64x2, >>)

/* ---- reductions to a scalar --------------------------------------------- */

#define __WASM_REDUCE(name, opc, shape) \
    static inline int name(v128_t a) \
    { return __builtin_wasm_vop1(opc, (shape)a, int); }

__WASM_REDUCE(wasm_i8x16_bitmask, 100, __wasm_i8x16)
__WASM_REDUCE(wasm_i16x8_bitmask, 132, __wasm_i16x8)
__WASM_REDUCE(wasm_i32x4_bitmask, 164, __wasm_i32x4)
__WASM_REDUCE(wasm_i64x2_bitmask, 196, __wasm_i64x2)
__WASM_REDUCE(wasm_i8x16_all_true, 99, __wasm_i8x16)
__WASM_REDUCE(wasm_i16x8_all_true, 131, __wasm_i16x8)
__WASM_REDUCE(wasm_i32x4_all_true, 163, __wasm_i32x4)
__WASM_REDUCE(wasm_i64x2_all_true, 195, __wasm_i64x2)
__WASM_REDUCE(wasm_v128_any_true, 83, __wasm_i32x4)

#undef __WASM_INT_ARITH
#undef __WASM_SAT
#undef __WASM_VOP2X
#undef __WASM_VOP1
#undef __WASM_CMP
#undef __WASM_SHIFT
#undef __WASM_REDUCE

#endif /* _WASM_SIMD128_H */
