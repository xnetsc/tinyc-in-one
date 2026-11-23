/* arm_neon.h - NEON intrinsic subset for the TCC wasm backend.
 *
 * Only the operations whose NEON -> wasm SIMD128 mapping is direct
 * (ASSESSMENT section 8.2) are provided:
 *   - integer/float add, sub, mul, div
 *   - saturating add/sub for 8/16-bit lanes (wasm has no 32/64-bit
 *     saturating arithmetic, so the s32/s64/u32/u64 forms are absent)
 *   - min/max (integer lanes up to 32 bits; wasm has no i64 min/max)
 *   - comparisons (mask result in the unsigned type of the shape)
 *   - abs / neg
 *   - fixed-pattern interleave (zip/uzp/trn) via i8x16.shuffle
 *   - table lookup (vtbl1q) via i8x16.swizzle
 *   - shifts by a uniform scalar count
 *   - lane read/write and broadcast (splat)
 *   - load/store
 * Not provided, by design: multiply-accumulate (vmlaq/vfmaq - no
 * FMA in wasm SIMD, a mul+add decomposition rounds twice), vector
 * (per-lane) shift counts and horizontal reductions (no wasm
 * instruction), dot product (vdotq), 64-bit (non-q) forms, i64 lane
 * extract/replace/splat (no 64-bit scalar register in the backend),
 * and the crypto extensions.
 * Lane indices of vgetq_lane/vsetq_lane/vdupq_laneq are compile-time
 * constants, so those are macros.
 */
#ifndef _ARM_NEON_H
#define _ARM_NEON_H

typedef signed char   int8x16_t   __attribute__((vector_size(16)));
typedef short         int16x8_t   __attribute__((vector_size(16)));
typedef int           int32x4_t   __attribute__((vector_size(16)));
typedef long long     int64x2_t   __attribute__((vector_size(16)));
typedef unsigned char uint8x16_t  __attribute__((vector_size(16)));
typedef unsigned short uint16x8_t __attribute__((vector_size(16)));
typedef unsigned int  uint32x4_t  __attribute__((vector_size(16)));
typedef unsigned long long uint64x2_t __attribute__((vector_size(16)));
typedef float         float32x4_t __attribute__((vector_size(16)));
typedef double        float64x2_t __attribute__((vector_size(16)));

/* ---- load / store -------------------------------------------------- */

#define __NEON_LD(name, type, elem) \
    static inline type name(const elem *p) { return *(const type *)p; }
#define __NEON_ST(name, type, elem) \
    static inline void name(elem *p, type v) { *(type *)p = v; }

__NEON_LD(vld1q_s8, int8x16_t, signed char)
__NEON_LD(vld1q_s16, int16x8_t, short)
__NEON_LD(vld1q_s32, int32x4_t, int)
__NEON_LD(vld1q_s64, int64x2_t, long long)
__NEON_LD(vld1q_u8, uint8x16_t, unsigned char)
__NEON_LD(vld1q_u16, uint16x8_t, unsigned short)
__NEON_LD(vld1q_u32, uint32x4_t, unsigned int)
__NEON_LD(vld1q_u64, uint64x2_t, unsigned long long)
__NEON_LD(vld1q_f32, float32x4_t, float)
__NEON_LD(vld1q_f64, float64x2_t, double)
__NEON_ST(vst1q_s8, int8x16_t, signed char)
__NEON_ST(vst1q_s16, int16x8_t, short)
__NEON_ST(vst1q_s32, int32x4_t, int)
__NEON_ST(vst1q_s64, int64x2_t, long long)
__NEON_ST(vst1q_u8, uint8x16_t, unsigned char)
__NEON_ST(vst1q_u16, uint16x8_t, unsigned short)
__NEON_ST(vst1q_u32, uint32x4_t, unsigned int)
__NEON_ST(vst1q_u64, uint64x2_t, unsigned long long)
__NEON_ST(vst1q_f32, float32x4_t, float)
__NEON_ST(vst1q_f64, float64x2_t, double)

/* ---- arithmetic ------------------------------------------------------ */

#define __NEON_BIN(name, type, op) \
    static inline type name(type a, type b) { return a op b; }

__NEON_BIN(vaddq_s8, int8x16_t, +)
__NEON_BIN(vaddq_s16, int16x8_t, +)
__NEON_BIN(vaddq_s32, int32x4_t, +)
__NEON_BIN(vaddq_s64, int64x2_t, +)
__NEON_BIN(vaddq_u8, uint8x16_t, +)
__NEON_BIN(vaddq_u16, uint16x8_t, +)
__NEON_BIN(vaddq_u32, uint32x4_t, +)
__NEON_BIN(vaddq_u64, uint64x2_t, +)
__NEON_BIN(vaddq_f32, float32x4_t, +)
__NEON_BIN(vaddq_f64, float64x2_t, +)
__NEON_BIN(vsubq_s8, int8x16_t, -)
__NEON_BIN(vsubq_s16, int16x8_t, -)
__NEON_BIN(vsubq_s32, int32x4_t, -)
__NEON_BIN(vsubq_s64, int64x2_t, -)
__NEON_BIN(vsubq_u8, uint8x16_t, -)
__NEON_BIN(vsubq_u16, uint16x8_t, -)
__NEON_BIN(vsubq_u32, uint32x4_t, -)
__NEON_BIN(vsubq_u64, uint64x2_t, -)
__NEON_BIN(vsubq_f32, float32x4_t, -)
__NEON_BIN(vsubq_f64, float64x2_t, -)
__NEON_BIN(vmulq_s8, int8x16_t, *)
__NEON_BIN(vmulq_s16, int16x8_t, *)
__NEON_BIN(vmulq_s32, int32x4_t, *)
__NEON_BIN(vmulq_s64, int64x2_t, *)
__NEON_BIN(vmulq_u8, uint8x16_t, *)
__NEON_BIN(vmulq_u16, uint16x8_t, *)
__NEON_BIN(vmulq_u32, uint32x4_t, *)
__NEON_BIN(vmulq_u64, uint64x2_t, *)
__NEON_BIN(vmulq_f32, float32x4_t, *)
__NEON_BIN(vmulq_f64, float64x2_t, *)
__NEON_BIN(vdivq_f32, float32x4_t, /)
__NEON_BIN(vdivq_f64, float64x2_t, /)

/* saturating arithmetic (8/16-bit lanes only) */
#define __NEON_SAT(name, opc, type) \
    static inline type name(type a, type b) \
    { return __builtin_wasm_vop2(opc, a, b, type); }

__NEON_SAT(vqaddq_s8, 111, int8x16_t)
__NEON_SAT(vqaddq_u8, 112, uint8x16_t)
__NEON_SAT(vqsubq_s8, 114, int8x16_t)
__NEON_SAT(vqsubq_u8, 115, uint8x16_t)
__NEON_SAT(vqaddq_s16, 143, int16x8_t)
__NEON_SAT(vqaddq_u16, 144, uint16x8_t)
__NEON_SAT(vqsubq_s16, 146, int16x8_t)
__NEON_SAT(vqsubq_u16, 147, uint16x8_t)

/* min / max */
__NEON_SAT(vminq_s8, 118, int8x16_t)
__NEON_SAT(vminq_u8, 119, uint8x16_t)
__NEON_SAT(vmaxq_s8, 120, int8x16_t)
__NEON_SAT(vmaxq_u8, 121, uint8x16_t)
__NEON_SAT(vminq_s16, 150, int16x8_t)
__NEON_SAT(vminq_u16, 151, uint16x8_t)
__NEON_SAT(vmaxq_s16, 152, int16x8_t)
__NEON_SAT(vmaxq_u16, 153, uint16x8_t)
__NEON_SAT(vminq_s32, 182, int32x4_t)
__NEON_SAT(vminq_u32, 183, uint32x4_t)
__NEON_SAT(vmaxq_s32, 184, int32x4_t)
__NEON_SAT(vmaxq_u32, 185, uint32x4_t)
__NEON_SAT(vminq_f32, 232, float32x4_t)
__NEON_SAT(vmaxq_f32, 233, float32x4_t)
__NEON_SAT(vminq_f64, 244, float64x2_t)
__NEON_SAT(vmaxq_f64, 245, float64x2_t)

/* ---- comparisons: result is the unsigned mask of the same shape ------- */

#define __NEON_CMP(name, type, restype, op) \
    static inline restype name(type a, type b) { return (restype)(a op b); }

__NEON_CMP(vceqq_s8, int8x16_t, uint8x16_t, ==)
__NEON_CMP(vceqq_s16, int16x8_t, uint16x8_t, ==)
__NEON_CMP(vceqq_s32, int32x4_t, uint32x4_t, ==)
__NEON_CMP(vceqq_u8, uint8x16_t, uint8x16_t, ==)
__NEON_CMP(vceqq_u16, uint16x8_t, uint16x8_t, ==)
__NEON_CMP(vceqq_u32, uint32x4_t, uint32x4_t, ==)
__NEON_CMP(vceqq_f32, float32x4_t, uint32x4_t, ==)
__NEON_CMP(vceqq_f64, float64x2_t, uint64x2_t, ==)
__NEON_CMP(vcgtq_s8, int8x16_t, uint8x16_t, >)
__NEON_CMP(vcgtq_s16, int16x8_t, uint16x8_t, >)
__NEON_CMP(vcgtq_s32, int32x4_t, uint32x4_t, >)
__NEON_CMP(vcgtq_u8, uint8x16_t, uint8x16_t, >)
__NEON_CMP(vcgtq_u16, uint16x8_t, uint16x8_t, >)
__NEON_CMP(vcgtq_u32, uint32x4_t, uint32x4_t, >)
__NEON_CMP(vcgtq_f32, float32x4_t, uint32x4_t, >)
__NEON_CMP(vcgtq_f64, float64x2_t, uint64x2_t, >)
__NEON_CMP(vcgeq_s8, int8x16_t, uint8x16_t, >=)
__NEON_CMP(vcgeq_s16, int16x8_t, uint16x8_t, >=)
__NEON_CMP(vcgeq_s32, int32x4_t, uint32x4_t, >=)
__NEON_CMP(vcgeq_u8, uint8x16_t, uint8x16_t, >=)
__NEON_CMP(vcgeq_u16, uint16x8_t, uint16x8_t, >=)
__NEON_CMP(vcgeq_u32, uint32x4_t, uint32x4_t, >=)
__NEON_CMP(vcgeq_f32, float32x4_t, uint32x4_t, >=)
__NEON_CMP(vcgeq_f64, float64x2_t, uint64x2_t, >=)
__NEON_CMP(vcltq_s8, int8x16_t, uint8x16_t, <)
__NEON_CMP(vcltq_s16, int16x8_t, uint16x8_t, <)
__NEON_CMP(vcltq_s32, int32x4_t, uint32x4_t, <)
__NEON_CMP(vcltq_u8, uint8x16_t, uint8x16_t, <)
__NEON_CMP(vcltq_u16, uint16x8_t, uint16x8_t, <)
__NEON_CMP(vcltq_u32, uint32x4_t, uint32x4_t, <)
__NEON_CMP(vcltq_f32, float32x4_t, uint32x4_t, <)
__NEON_CMP(vcltq_f64, float64x2_t, uint64x2_t, <)
__NEON_CMP(vcleq_s8, int8x16_t, uint8x16_t, <=)
__NEON_CMP(vcleq_s16, int16x8_t, uint16x8_t, <=)
__NEON_CMP(vcleq_s32, int32x4_t, uint32x4_t, <=)
__NEON_CMP(vcleq_u8, uint8x16_t, uint8x16_t, <=)
__NEON_CMP(vcleq_u16, uint16x8_t, uint16x8_t, <=)
__NEON_CMP(vcleq_u32, uint32x4_t, uint32x4_t, <=)
__NEON_CMP(vcleq_f32, float32x4_t, uint32x4_t, <=)
__NEON_CMP(vcleq_f64, float64x2_t, uint64x2_t, <=)

/* ---- abs / neg ---------------------------------------------------------- */

#define __NEON_ABS(name, opc, type) \
    static inline type name(type a) \
    { return __builtin_wasm_vop1(opc, a, type); }

__NEON_ABS(vabsq_s8, 96, int8x16_t)
__NEON_ABS(vabsq_s16, 128, int16x8_t)
__NEON_ABS(vabsq_s32, 160, int32x4_t)
__NEON_ABS(vabsq_s64, 192, int64x2_t)
__NEON_ABS(vabsq_f32, 224, float32x4_t)
__NEON_ABS(vabsq_f64, 236, float64x2_t)
__NEON_ABS(vnegq_f32, 225, float32x4_t)
__NEON_ABS(vnegq_f64, 237, float64x2_t)

#define __NEON_NEG(name, type) \
    static inline type name(type a) { return -a; }

__NEON_NEG(vnegq_s8, int8x16_t)
__NEON_NEG(vnegq_s16, int16x8_t)
__NEON_NEG(vnegq_s32, int32x4_t)
__NEON_NEG(vnegq_s64, int64x2_t)

/* ---- broadcast / lane access ---------------------------------------------- */

#define __NEON_DUP(name, opc, type, elem) \
    static inline type name(elem x) \
    { return __builtin_wasm_splat(opc, x, type); }

__NEON_DUP(vdupq_n_s8, 15, int8x16_t, signed char)
__NEON_DUP(vdupq_n_s16, 16, int16x8_t, short)
__NEON_DUP(vdupq_n_s32, 17, int32x4_t, int)
__NEON_DUP(vdupq_n_u8, 15, uint8x16_t, unsigned char)
__NEON_DUP(vdupq_n_u16, 16, uint16x8_t, unsigned short)
__NEON_DUP(vdupq_n_u32, 17, uint32x4_t, unsigned int)
__NEON_DUP(vdupq_n_f32, 19, float32x4_t, float)
__NEON_DUP(vdupq_n_f64, 20, float64x2_t, double)

#define vgetq_lane_s8(v, lane) \
    ((signed char)__builtin_wasm_extract(21, v, lane, int))
#define vgetq_lane_u8(v, lane) \
    ((unsigned char)__builtin_wasm_extract(22, v, lane, int))
#define vgetq_lane_s16(v, lane) \
    ((short)__builtin_wasm_extract(24, v, lane, int))
#define vgetq_lane_u16(v, lane) \
    ((unsigned short)__builtin_wasm_extract(25, v, lane, int))
#define vgetq_lane_s32(v, lane) __builtin_wasm_extract(27, v, lane, int)
#define vgetq_lane_u32(v, lane) \
    __builtin_wasm_extract(27, v, lane, unsigned int)
#define vgetq_lane_f32(v, lane) __builtin_wasm_extract(31, v, lane, float)
#define vgetq_lane_f64(v, lane) __builtin_wasm_extract(33, v, lane, double)

#define vsetq_lane_s8(x, v, lane) __builtin_wasm_replace(23, v, lane, x)
#define vsetq_lane_u8(x, v, lane) __builtin_wasm_replace(23, v, lane, x)
#define vsetq_lane_s16(x, v, lane) __builtin_wasm_replace(26, v, lane, x)
#define vsetq_lane_u16(x, v, lane) __builtin_wasm_replace(26, v, lane, x)
#define vsetq_lane_s32(x, v, lane) __builtin_wasm_replace(28, v, lane, x)
#define vsetq_lane_u32(x, v, lane) __builtin_wasm_replace(28, v, lane, x)
#define vsetq_lane_f32(x, v, lane) __builtin_wasm_replace(32, v, lane, x)
#define vsetq_lane_f64(x, v, lane) __builtin_wasm_replace(34, v, lane, x)

#define vdupq_laneq_s32(v, lane) \
    __builtin_wasm_splat(17, __builtin_wasm_extract(27, v, lane, int), int32x4_t)
#define vdupq_laneq_f32(v, lane) \
    __builtin_wasm_splat(19, __builtin_wasm_extract(31, v, lane, float), float32x4_t)

/* ---- fixed-pattern interleave (i8x16.shuffle with constant tables) -------- */

#define __NEON_SHUF(name, type) \
    static inline type name(type a, type b) \
    { return __builtin_wasm_shuffle((int8x16_t)a, (int8x16_t)b, \
        __NEON_TBL); }

#define __NEON_TBL 0,1,2,3, 16,17,18,19, 4,5,6,7, 20,21,22,23
__NEON_SHUF(vzip1q_s32, int32x4_t)
#undef __NEON_TBL
#define __NEON_TBL 8,9,10,11, 24,25,26,27, 12,13,14,15, 28,29,30,31
__NEON_SHUF(vzip2q_s32, int32x4_t)
#undef __NEON_TBL
#define __NEON_TBL 0,1,2,3, 8,9,10,11, 16,17,18,19, 24,25,26,27
__NEON_SHUF(vuzp1q_s32, int32x4_t)
#undef __NEON_TBL
#define __NEON_TBL 4,5,6,7, 12,13,14,15, 20,21,22,23, 28,29,30,31
__NEON_SHUF(vuzp2q_s32, int32x4_t)
#undef __NEON_TBL
#define __NEON_TBL 0,1,2,3, 16,17,18,19, 8,9,10,11, 24,25,26,27
__NEON_SHUF(vtrn1q_s32, int32x4_t)
#undef __NEON_TBL
#define __NEON_TBL 4,5,6,7, 20,21,22,23, 12,13,14,15, 28,29,30,31
__NEON_SHUF(vtrn2q_s32, int32x4_t)
#undef __NEON_TBL
#define __NEON_TBL 0,16,1,17,2,18,3,19,4,20,5,21,6,22,7,23
__NEON_SHUF(vzip1q_s8, int8x16_t)
#undef __NEON_TBL
#define __NEON_TBL 8,24,9,25,10,26,11,27,12,28,13,29,14,30,15,31
__NEON_SHUF(vzip2q_s8, int8x16_t)
#undef __NEON_TBL
#define __NEON_TBL 0,2,4,6,8,10,12,14, 16,18,20,22,24,26,28,30
__NEON_SHUF(vuzp1q_s8, int8x16_t)
#undef __NEON_TBL
#define __NEON_TBL 1,3,5,7,9,11,13,15, 17,19,21,23,25,27,29,31
__NEON_SHUF(vuzp2q_s8, int8x16_t)
#undef __NEON_TBL
#define __NEON_TBL 0,16,2,18,4,20,6,22, 8,24,10,26,12,28,14,30
__NEON_SHUF(vtrn1q_s8, int8x16_t)
#undef __NEON_TBL
#define __NEON_TBL 1,17,3,19,5,21,7,23, 9,25,11,27,13,29,15,31
__NEON_SHUF(vtrn2q_s8, int8x16_t)
#undef __NEON_TBL

static inline float32x4_t vzip1q_f32(float32x4_t a, float32x4_t b)
{
    return (float32x4_t)vzip1q_s32((int32x4_t)a, (int32x4_t)b);
}
static inline float32x4_t vuzp1q_f32(float32x4_t a, float32x4_t b)
{
    return (float32x4_t)vuzp1q_s32((int32x4_t)a, (int32x4_t)b);
}
static inline float32x4_t vtrn1q_f32(float32x4_t a, float32x4_t b)
{
    return (float32x4_t)vtrn1q_s32((int32x4_t)a, (int32x4_t)b);
}
static inline uint32x4_t vzip1q_u32(uint32x4_t a, uint32x4_t b)
{
    return (uint32x4_t)vzip1q_s32((int32x4_t)a, (int32x4_t)b);
}
static inline uint32x4_t vuzp1q_u32(uint32x4_t a, uint32x4_t b)
{
    return (uint32x4_t)vuzp1q_s32((int32x4_t)a, (int32x4_t)b);
}

/* ---- table lookup ---------------------------------------------------------- */

static inline uint8x16_t vtbl1q_u8(uint8x16_t a, uint8x16_t idx)
{
    return __builtin_wasm_vop2(14, a, idx, uint8x16_t);
}

/* ---- shifts by a uniform scalar count --------------------------------------- */

#define __NEON_SH(name, type, op) \
    static inline type name(type a, int n) { return a op n; }

__NEON_SH(vshlq_n_s32, int32x4_t, <<)
__NEON_SH(vshlq_n_u32, uint32x4_t, <<)
__NEON_SH(vshlq_n_s64, int64x2_t, <<)
__NEON_SH(vshrq_n_s32, int32x4_t, >>)
__NEON_SH(vshrq_n_u32, uint32x4_t, >>)
__NEON_SH(vshrq_n_s64, int64x2_t, >>)
__NEON_SH(vshrq_n_u64, uint64x2_t, >>)

/* ---- reinterpret (bitcast) ---------------------------------------------------- */

#define __NEON_REINT(name, from, to) \
    static inline to name(from a) { return (to)a; }

__NEON_REINT(vreinterpretq_s32_u32, uint32x4_t, int32x4_t)
__NEON_REINT(vreinterpretq_u32_s32, int32x4_t, uint32x4_t)
__NEON_REINT(vreinterpretq_f32_s32, int32x4_t, float32x4_t)
__NEON_REINT(vreinterpretq_s32_f32, float32x4_t, int32x4_t)
__NEON_REINT(vreinterpretq_f32_u32, uint32x4_t, float32x4_t)
__NEON_REINT(vreinterpretq_u32_f32, float32x4_t, uint32x4_t)
__NEON_REINT(vreinterpretq_s8_u8, uint8x16_t, int8x16_t)
__NEON_REINT(vreinterpretq_u8_s8, int8x16_t, uint8x16_t)
__NEON_REINT(vreinterpretq_s16_u16, uint16x8_t, int16x8_t)
__NEON_REINT(vreinterpretq_u16_s16, int16x8_t, uint16x8_t)
__NEON_REINT(vreinterpretq_f64_s64, int64x2_t, float64x2_t)
__NEON_REINT(vreinterpretq_s64_f64, float64x2_t, int64x2_t)
__NEON_REINT(vreinterpretq_s64_u64, uint64x2_t, int64x2_t)
__NEON_REINT(vreinterpretq_u64_s64, int64x2_t, uint64x2_t)

#undef __NEON_LD
#undef __NEON_ST
#undef __NEON_BIN
#undef __NEON_SAT
#undef __NEON_CMP
#undef __NEON_ABS
#undef __NEON_NEG
#undef __NEON_DUP
#undef __NEON_SHUF
#undef __NEON_SH
#undef __NEON_REINT

#endif /* _ARM_NEON_H */
