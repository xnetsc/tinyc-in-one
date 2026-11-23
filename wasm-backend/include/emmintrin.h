/* emmintrin.h - SSE2 intrinsic subset for the TCC wasm backend.
 *
 * Only the operations whose SSE2 -> wasm SIMD128 mapping is direct
 * (ASSESSMENT section 8.3) are provided: packed double and integer
 * arithmetic, saturating add/sub, integer min/max, rounding average,
 * comparisons, uniform shifts, immediate shuffles (i8x16.shuffle),
 * _mm_shuffle_epi8 (the SSSE3 register shuffle, i8x16.swizzle),
 * unpacks, movemask (i8x16.bitmask), packs (narrow), _mm_madd_epi16
 * (i32x4.dot_i16x8_s) and the conversions that map to the wasm
 * convert/trunc_sat family.
 * Not provided: _mm_mul_epu32 / _mm_mulhi_* (no widening 32-bit or
 * high-half multiply in wasm SIMD), _mm_sad_epu8 (needs a horizontal
 * sum), rounding-mode float->int conversions other than through
 * nearest+trunc_sat (out-of-range results saturate where x86
 * produces the indefinite value), 64-bit lane splat, and the cache /
 * fence intrinsics.
 * Shuffle intrinsics take a compile-time constant selector and are
 * macros.
 */
#ifndef _EMMINTRIN_H
#define _EMMINTRIN_H

#include "xmmintrin.h"

typedef int    __m128i __attribute__((vector_size(16)));
typedef double __m128d __attribute__((vector_size(16)));

typedef signed char    __emm_i8  __attribute__((vector_size(16)));
typedef short          __emm_i16 __attribute__((vector_size(16)));
typedef long long      __emm_i64 __attribute__((vector_size(16)));
typedef unsigned char  __emm_u8  __attribute__((vector_size(16)));
typedef unsigned short __emm_u16 __attribute__((vector_size(16)));
typedef unsigned int   __emm_u32 __attribute__((vector_size(16)));
typedef unsigned long long __emm_u64 __attribute__((vector_size(16)));

/* ---- packed double ------------------------------------------------------ */

#define __EMM_BINPD(name, op) \
    static inline __m128d name(__m128d a, __m128d b) { return a op b; }

__EMM_BINPD(_mm_add_pd, +)
__EMM_BINPD(_mm_sub_pd, -)
__EMM_BINPD(_mm_mul_pd, *)
__EMM_BINPD(_mm_div_pd, /)
#undef __EMM_BINPD

static inline __m128d _mm_min_pd(__m128d a, __m128d b)
{
    return __builtin_wasm_vop2(244, a, b, __m128d);
}
static inline __m128d _mm_max_pd(__m128d a, __m128d b)
{
    return __builtin_wasm_vop2(245, a, b, __m128d);
}
static inline __m128d _mm_sqrt_pd(__m128d a)
{
    return __builtin_wasm_vop1(239, a, __m128d);
}
static inline __m128d _mm_and_pd(__m128d a, __m128d b)
{
    return (__m128d)((__emm_i64)a & (__emm_i64)b);
}
static inline __m128d _mm_andnot_pd(__m128d a, __m128d b)
{
    return (__m128d)(~(__emm_i64)a & (__emm_i64)b);
}
static inline __m128d _mm_or_pd(__m128d a, __m128d b)
{
    return (__m128d)((__emm_i64)a | (__emm_i64)b);
}
static inline __m128d _mm_xor_pd(__m128d a, __m128d b)
{
    return (__m128d)((__emm_i64)a ^ (__emm_i64)b);
}

#define __EMM_CMPPD(name, op) \
    static inline __m128d name(__m128d a, __m128d b) \
    { return (__m128d)(a op b); }

__EMM_CMPPD(_mm_cmpeq_pd, ==)
__EMM_CMPPD(_mm_cmplt_pd, <)
__EMM_CMPPD(_mm_cmpgt_pd, >)
__EMM_CMPPD(_mm_cmple_pd, <=)
__EMM_CMPPD(_mm_cmpge_pd, >=)
__EMM_CMPPD(_mm_cmpneq_pd, !=)
#undef __EMM_CMPPD

static inline __m128d _mm_set_pd(double a, double b)
{
    __m128d v = { b, a };
    return v;
}
static inline __m128d _mm_setr_pd(double a, double b)
{
    __m128d v = { a, b };
    return v;
}
static inline __m128d _mm_set1_pd(double a)
{
    return __builtin_wasm_splat(20, a, __m128d);
}
static inline __m128d _mm_setzero_pd(void)
{
    __m128d v = { 0.0, 0.0 };
    return v;
}
static inline __m128d _mm_load_pd(const double *p)
{
    return *(const __m128d *)p;
}
static inline __m128d _mm_loadu_pd(const double *p)
{
    return *(const __m128d *)p;
}
static inline void _mm_store_pd(double *p, __m128d v)
{
    *(__m128d *)p = v;
}
static inline void _mm_storeu_pd(double *p, __m128d v)
{
    *(__m128d *)p = v;
}
static inline double _mm_cvtsd_f64(__m128d a)
{
    return __builtin_wasm_extract(33, a, 0, double);
}

/* ---- integer arithmetic ------------------------------------------------------ */

#define __EMM_BIN(name, type, op) \
    static inline __m128i name(__m128i a, __m128i b) \
    { return (__m128i)((type)a op (type)b); }

__EMM_BIN(_mm_add_epi8, __emm_i8, +)
__EMM_BIN(_mm_add_epi16, __emm_i16, +)
__EMM_BIN(_mm_add_epi32, __m128i, +)
__EMM_BIN(_mm_add_epi64, __emm_i64, +)
__EMM_BIN(_mm_sub_epi8, __emm_i8, -)
__EMM_BIN(_mm_sub_epi16, __emm_i16, -)
__EMM_BIN(_mm_sub_epi32, __m128i, -)
__EMM_BIN(_mm_sub_epi64, __emm_i64, -)
__EMM_BIN(_mm_mullo_epi16, __emm_i16, *)
#undef __EMM_BIN

static inline __m128i _mm_madd_epi16(__m128i a, __m128i b)
{
    return __builtin_wasm_vop2(186, (__emm_i16)a, (__emm_i16)b, __m128i);
}

#define __EMM_VOP2(name, opc, type) \
    static inline __m128i name(__m128i a, __m128i b) \
    { return (__m128i)__builtin_wasm_vop2(opc, (type)a, (type)b, type); }

__EMM_VOP2(_mm_adds_epi8, 111, __emm_i8)
__EMM_VOP2(_mm_adds_epu8, 112, __emm_u8)
__EMM_VOP2(_mm_subs_epi8, 114, __emm_i8)
__EMM_VOP2(_mm_subs_epu8, 115, __emm_u8)
__EMM_VOP2(_mm_adds_epi16, 143, __emm_i16)
__EMM_VOP2(_mm_adds_epu16, 144, __emm_u16)
__EMM_VOP2(_mm_subs_epi16, 146, __emm_i16)
__EMM_VOP2(_mm_subs_epu16, 147, __emm_u16)
__EMM_VOP2(_mm_min_epi16, 150, __emm_i16)
__EMM_VOP2(_mm_max_epi16, 152, __emm_i16)
__EMM_VOP2(_mm_min_epu8, 119, __emm_u8)
__EMM_VOP2(_mm_max_epu8, 121, __emm_u8)
__EMM_VOP2(_mm_avg_epu8, 123, __emm_u8)
__EMM_VOP2(_mm_avg_epu16, 155, __emm_u16)
#undef __EMM_VOP2

/* packs: wasm narrow saturates exactly like the x86 packs */
static inline __m128i _mm_packs_epi16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_wasm_vop2(101, (__emm_i16)a, (__emm_i16)b,
                                        __emm_i8);
}
static inline __m128i _mm_packus_epi16(__m128i a, __m128i b)
{
    return (__m128i)__builtin_wasm_vop2(102, (__emm_i16)a, (__emm_i16)b,
                                        __emm_i8);
}
static inline __m128i _mm_packs_epi32(__m128i a, __m128i b)
{
    return (__m128i)__builtin_wasm_vop2(133, (__m128i)a, (__m128i)b,
                                        __emm_i16);
}
static inline __m128i _mm_packus_epi32(__m128i a, __m128i b)
{
    return (__m128i)__builtin_wasm_vop2(134, (__m128i)a, (__m128i)b,
                                        __emm_i16);
}

/* ---- integer comparisons ------------------------------------------------------ */

#define __EMM_CMP(name, type, op) \
    static inline __m128i name(__m128i a, __m128i b) \
    { return (__m128i)((type)a op (type)b); }

__EMM_CMP(_mm_cmpeq_epi8, __emm_i8, ==)
__EMM_CMP(_mm_cmpeq_epi16, __emm_i16, ==)
__EMM_CMP(_mm_cmpeq_epi32, __m128i, ==)
__EMM_CMP(_mm_cmpgt_epi8, __emm_i8, >)
__EMM_CMP(_mm_cmpgt_epi16, __emm_i16, >)
__EMM_CMP(_mm_cmpgt_epi32, __m128i, >)
__EMM_CMP(_mm_cmplt_epi8, __emm_i8, <)
__EMM_CMP(_mm_cmplt_epi16, __emm_i16, <)
__EMM_CMP(_mm_cmplt_epi32, __m128i, <)
#undef __EMM_CMP

/* ---- integer bitwise ------------------------------------------------------------- */

static inline __m128i _mm_and_si128(__m128i a, __m128i b) { return a & b; }
static inline __m128i _mm_andnot_si128(__m128i a, __m128i b) { return ~a & b; }
static inline __m128i _mm_or_si128(__m128i a, __m128i b) { return a | b; }
static inline __m128i _mm_xor_si128(__m128i a, __m128i b) { return a ^ b; }

/* ---- shifts (uniform count) --------------------------------------------------------- */

#define __EMM_SH(name, type, op) \
    static inline __m128i name(__m128i a, int n) \
    { return (__m128i)((type)a op n); }

__EMM_SH(_mm_slli_epi16, __emm_i16, <<)
__EMM_SH(_mm_slli_epi32, __m128i, <<)
__EMM_SH(_mm_slli_epi64, __emm_i64, <<)
__EMM_SH(_mm_srli_epi16, __emm_u16, >>)
__EMM_SH(_mm_srli_epi32, __emm_u32, >>)
__EMM_SH(_mm_srli_epi64, __emm_u64, >>)
__EMM_SH(_mm_srai_epi16, __emm_i16, >>)
__EMM_SH(_mm_srai_epi32, __m128i, >>)
#undef __EMM_SH

/* byte shifts: a shuffle against a zero vector; the selector is a
   compile-time constant expression over the count */
#define _mm_slli_si128(a, n) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), \
        (__emm_i8)__builtin_wasm_splat(15, 0, __emm_i8), \
        (0 >= (n) ? 0 - (n) : 16), (1 >= (n) ? 1 - (n) : 16), \
        (2 >= (n) ? 2 - (n) : 16), (3 >= (n) ? 3 - (n) : 16), \
        (4 >= (n) ? 4 - (n) : 16), (5 >= (n) ? 5 - (n) : 16), \
        (6 >= (n) ? 6 - (n) : 16), (7 >= (n) ? 7 - (n) : 16), \
        (8 >= (n) ? 8 - (n) : 16), (9 >= (n) ? 9 - (n) : 16), \
        (10 >= (n) ? 10 - (n) : 16), (11 >= (n) ? 11 - (n) : 16), \
        (12 >= (n) ? 12 - (n) : 16), (13 >= (n) ? 13 - (n) : 16), \
        (14 >= (n) ? 14 - (n) : 16), (15 >= (n) ? 15 - (n) : 16)))
#define _mm_srli_si128(a, n) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), \
        (__emm_i8)__builtin_wasm_splat(15, 0, __emm_i8), \
        (0 + (n) < 16 ? 0 + (n) : 16), (1 + (n) < 16 ? 1 + (n) : 16), \
        (2 + (n) < 16 ? 2 + (n) : 16), (3 + (n) < 16 ? 3 + (n) : 16), \
        (4 + (n) < 16 ? 4 + (n) : 16), (5 + (n) < 16 ? 5 + (n) : 16), \
        (6 + (n) < 16 ? 6 + (n) : 16), (7 + (n) < 16 ? 7 + (n) : 16), \
        (8 + (n) < 16 ? 8 + (n) : 16), (9 + (n) < 16 ? 9 + (n) : 16), \
        (10 + (n) < 16 ? 10 + (n) : 16), (11 + (n) < 16 ? 11 + (n) : 16), \
        (12 + (n) < 16 ? 12 + (n) : 16), (13 + (n) < 16 ? 13 + (n) : 16), \
        (14 + (n) < 16 ? 14 + (n) : 16), (15 + (n) < 16 ? 15 + (n) : 16)))

/* ---- shuffles ------------------------------------------------------------------------ */

#define _mm_shuffle_epi32(a, imm) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(a), \
        ((imm) & 3) * 4, ((imm) & 3) * 4 + 1, \
        ((imm) & 3) * 4 + 2, ((imm) & 3) * 4 + 3, \
        (((imm) >> 2) & 3) * 4, (((imm) >> 2) & 3) * 4 + 1, \
        (((imm) >> 2) & 3) * 4 + 2, (((imm) >> 2) & 3) * 4 + 3, \
        (((imm) >> 4) & 3) * 4, (((imm) >> 4) & 3) * 4 + 1, \
        (((imm) >> 4) & 3) * 4 + 2, (((imm) >> 4) & 3) * 4 + 3, \
        (((imm) >> 6) & 3) * 4, (((imm) >> 6) & 3) * 4 + 1, \
        (((imm) >> 6) & 3) * 4 + 2, (((imm) >> 6) & 3) * 4 + 3))

#define _mm_shufflelo_epi16(a, imm) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(a), \
        ((imm) & 3) * 2, ((imm) & 3) * 2 + 1, \
        (((imm) >> 2) & 3) * 2, (((imm) >> 2) & 3) * 2 + 1, \
        (((imm) >> 4) & 3) * 2, (((imm) >> 4) & 3) * 2 + 1, \
        (((imm) >> 6) & 3) * 2, (((imm) >> 6) & 3) * 2 + 1, \
        8, 9, 10, 11, 12, 13, 14, 15))

#define _mm_shufflehi_epi16(a, imm) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(a), \
        0, 1, 2, 3, 4, 5, 6, 7, \
        8 + ((imm) & 3) * 2, 8 + ((imm) & 3) * 2 + 1, \
        8 + (((imm) >> 2) & 3) * 2, 8 + (((imm) >> 2) & 3) * 2 + 1, \
        8 + (((imm) >> 4) & 3) * 2, 8 + (((imm) >> 4) & 3) * 2 + 1, \
        8 + (((imm) >> 6) & 3) * 2, 8 + (((imm) >> 6) & 3) * 2 + 1))

/* SSSE3 byte shuffle: i8x16.swizzle, out-of-range indices give 0
   exactly like PSHUFB */
static inline __m128i _mm_shuffle_epi8(__m128i a, __m128i idx)
{
    return (__m128i)__builtin_wasm_vop2(14, (__emm_u8)a, (__emm_u8)idx,
                                        __emm_u8);
}

#define _mm_unpacklo_epi8(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        0,16,1,17,2,18,3,19,4,20,5,21,6,22,7,23))
#define _mm_unpackhi_epi8(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        8,24,9,25,10,26,11,27,12,28,13,29,14,30,15,31))
#define _mm_unpacklo_epi16(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        0,1,16,17, 2,3,18,19, 4,5,20,21, 6,7,22,23))
#define _mm_unpackhi_epi16(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        8,9,24,25, 10,11,26,27, 12,13,28,29, 14,15,30,31))
#define _mm_unpacklo_epi32(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        0,1,2,3, 16,17,18,19, 4,5,6,7, 20,21,22,23))
#define _mm_unpackhi_epi32(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        8,9,10,11, 24,25,26,27, 12,13,14,15, 28,29,30,31))
#define _mm_unpacklo_epi64(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        0,1,2,3,4,5,6,7, 16,17,18,19,20,21,22,23))
#define _mm_unpackhi_epi64(a, b) \
    ((__m128i)__builtin_wasm_shuffle((__emm_i8)(a), (__emm_i8)(b), \
        8,9,10,11,12,13,14,15, 24,25,26,27,28,29,30,31))

static inline int _mm_movemask_epi8(__m128i a)
{
    return __builtin_wasm_vop1(100, (__emm_i8)a, int);
}

/* ---- set / load / store ------------------------------------------------------------------ */

static inline __m128i _mm_set_epi32(int a, int b, int c, int d)
{
    __m128i v = { d, c, b, a };
    return v;
}
static inline __m128i _mm_setr_epi32(int a, int b, int c, int d)
{
    __m128i v = { a, b, c, d };
    return v;
}
static inline __m128i _mm_set_epi16(short a, short b, short c, short d,
                                    short e, short f, short g, short h)
{
    __emm_i16 v = { h, g, f, e, d, c, b, a };
    return (__m128i)v;
}
static inline __m128i _mm_set_epi8(signed char a, signed char b,
                                   signed char c, signed char d,
                                   signed char e, signed char f,
                                   signed char g, signed char h,
                                   signed char i, signed char j,
                                   signed char k, signed char l,
                                   signed char m, signed char n,
                                   signed char o, signed char p)
{
    __emm_i8 v = { p, o, n, m, l, k, j, i, h, g, f, e, d, c, b, a };
    return (__m128i)v;
}
static inline __m128i _mm_set1_epi32(int a)
{
    return __builtin_wasm_splat(17, a, __m128i);
}
static inline __m128i _mm_set1_epi16(short a)
{
    return (__m128i)__builtin_wasm_splat(16, a, __emm_i16);
}
static inline __m128i _mm_set1_epi8(signed char a)
{
    return (__m128i)__builtin_wasm_splat(15, a, __emm_i8);
}
static inline __m128i _mm_setzero_si128(void)
{
    __m128i v = { 0, 0, 0, 0 };
    return v;
}
static inline __m128i _mm_load_si128(const __m128i *p) { return *p; }
static inline __m128i _mm_loadu_si128(const __m128i *p) { return *p; }
static inline void _mm_store_si128(__m128i *p, __m128i v) { *p = v; }
static inline void _mm_storeu_si128(__m128i *p, __m128i v) { *p = v; }

/* ---- conversions ------------------------------------------------------------------------------ */

static inline __m128 _mm_cvtepi32_ps(__m128i a)
{
    return __builtin_wasm_vop1(250, a, __m128);
}
static inline __m128i _mm_cvttps_epi32(__m128 a)
{
    return __builtin_wasm_vop1(248, a, __m128i);
}
static inline __m128d _mm_cvtepi32_pd(__m128i a)
{
    return __builtin_wasm_vop1(254, a, __m128d);
}
static inline __m128i _mm_cvttpd_epi32(__m128d a)
{
    return __builtin_wasm_vop1(252, a, __m128i);
}
static inline __m128 _mm_cvtsd_ss(__m128 a, __m128d b)
{
    __m128 f = __builtin_wasm_vop1(94, b, __m128); /* demote, zero fill */
    return __builtin_wasm_replace(32, a, 0,
                                  __builtin_wasm_extract(31, f, 0, float));
}
static inline __m128d _mm_cvtsi32_sd(__m128d a, int i)
{
    __m128i vi = __builtin_wasm_splat(17, i, __m128i);
    __m128d vf = __builtin_wasm_vop1(254, vi, __m128d);
    return __builtin_wasm_replace(34, a, 0,
                                  __builtin_wasm_extract(33, vf, 0, double));
}
static inline int _mm_cvttsd_si32(__m128d a)
{
    /* trunc_sat_f64x2_s_zero converts both lanes and zeroes the
       high lanes of the result; lane 0 is the converted low lane */
    __m128i r = __builtin_wasm_vop1(252, a, __m128i);
    return __builtin_wasm_extract(27, r, 0, int);
}
static inline int _mm_cvtsd_si32(__m128d a)
{
    __m128d n = __builtin_wasm_vop1(148, a, __m128d);     /* nearest */
    __m128i r = __builtin_wasm_vop1(252, n, __m128i);     /* trunc_sat */
    return __builtin_wasm_extract(27, r, 0, int);
}

#endif /* _EMMINTRIN_H */
