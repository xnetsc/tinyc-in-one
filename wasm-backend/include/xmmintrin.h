/* xmmintrin.h - SSE intrinsic subset for the TCC wasm backend.
 *
 * Only the operations whose SSE -> wasm SIMD128 mapping is direct
 * (ASSESSMENT section 8.3) are provided: packed/single float
 * arithmetic, min/max, sqrt, bitwise ops, comparisons, immediate
 * shuffles (i8x16.shuffle), register shuffles (i8x16.swizzle is in
 * emmintrin.h as _mm_shuffle_epi8), movemask (f32x4 bitmask via the
 * i32x4 form), load/store and the int<->float conversions that map
 * to the wasm convert/trunc_sat family.
 * Not provided: reciprocal / reciprocal-sqrt approximations (no wasm
 * instruction), prefetch and fence intrinsics (no-ops with no wasm
 * counterpart), and the SSE integer forms that live in emmintrin.h.
 * _mm_cvtss_si32 rounds via f32x4.nearest + trunc_sat; on overflow
 * it saturates where x86 produces the indefinite value 0x80000000.
 * Shuffle intrinsics take a compile-time constant selector and are
 * macros.
 */
#ifndef _XMMINTRIN_H
#define _XMMINTRIN_H

typedef float __m128 __attribute__((vector_size(16)));

typedef int          __xmm_i32 __attribute__((vector_size(16)));
typedef signed char  __xmm_b8  __attribute__((vector_size(16)));

#define _MM_SHUFFLE(z, y, x, w) (((z) << 6) | ((y) << 4) | ((x) << 2) | (w))

/* ---- load / store / set --------------------------------------------- */

static inline __m128 _mm_load_ps(const float *p) { return *(const __m128 *)p; }
static inline __m128 _mm_loadu_ps(const float *p) { return *(const __m128 *)p; }
static inline void _mm_store_ps(float *p, __m128 v) { *(__m128 *)p = v; }
static inline void _mm_storeu_ps(float *p, __m128 v) { *(__m128 *)p = v; }

static inline __m128 _mm_set_ps(float a, float b, float c, float d)
{
    __m128 v = { d, c, b, a };
    return v;
}
static inline __m128 _mm_setr_ps(float a, float b, float c, float d)
{
    __m128 v = { a, b, c, d };
    return v;
}
static inline __m128 _mm_set1_ps(float a)
{
    return __builtin_wasm_splat(19, a, __m128);
}
static inline __m128 _mm_set_ss(float a)
{
    __m128 v = { a, 0.0f, 0.0f, 0.0f };
    return v;
}
static inline __m128 _mm_setzero_ps(void)
{
    __m128 v = { 0.0f, 0.0f, 0.0f, 0.0f };
    return v;
}
static inline __m128 _mm_load1_ps(const float *p)
{
    return __builtin_wasm_splat(19, *p, __m128);
}
static inline __m128 _mm_load_ss(const float *p)
{
    return __builtin_wasm_replace(32, _mm_setzero_ps(), 0, *p);
}

/* ---- arithmetic -------------------------------------------------------- */

#define __MM_BIN(name, op) \
    static inline __m128 name(__m128 a, __m128 b) { return a op b; }

__MM_BIN(_mm_add_ps, +)
__MM_BIN(_mm_sub_ps, -)
__MM_BIN(_mm_mul_ps, *)
__MM_BIN(_mm_div_ps, /)
#undef __MM_BIN

/* the packed bitwise ops go through the integer carrier: the C
   operators & | ^ are not defined on float vectors in the frontend */
static inline __m128 _mm_and_ps(__m128 a, __m128 b)
{
    return (__m128)((__xmm_i32)a & (__xmm_i32)b);
}
static inline __m128 _mm_andnot_ps(__m128 a, __m128 b)
{
    return (__m128)(~(__xmm_i32)a & (__xmm_i32)b);
}
static inline __m128 _mm_or_ps(__m128 a, __m128 b)
{
    return (__m128)((__xmm_i32)a | (__xmm_i32)b);
}
static inline __m128 _mm_xor_ps(__m128 a, __m128 b)
{
    return (__m128)((__xmm_i32)a ^ (__xmm_i32)b);
}

static inline __m128 _mm_sqrt_ps(__m128 a)
{
    return __builtin_wasm_vop1(227, a, __m128);
}
static inline __m128 _mm_min_ps(__m128 a, __m128 b)
{
    return __builtin_wasm_vop2(232, a, b, __m128);
}
static inline __m128 _mm_max_ps(__m128 a, __m128 b)
{
    return __builtin_wasm_vop2(233, a, b, __m128);
}

/* scalar (low-lane) forms: full-width op, then the high three lanes
   are taken back from a with one shuffle */
#define __MM_SS_SHUF(t, a) \
    ((__m128)__builtin_wasm_shuffle((__xmm_b8)(t), (__xmm_b8)(a), \
        0,1,2,3, 20,21,22,23, 24,25,26,27, 28,29,30,31))

static inline __m128 _mm_add_ss(__m128 a, __m128 b)
{
    return __MM_SS_SHUF(a + b, a);
}
static inline __m128 _mm_sub_ss(__m128 a, __m128 b)
{
    return __MM_SS_SHUF(a - b, a);
}
static inline __m128 _mm_mul_ss(__m128 a, __m128 b)
{
    return __MM_SS_SHUF(a * b, a);
}
static inline __m128 _mm_div_ss(__m128 a, __m128 b)
{
    return __MM_SS_SHUF(a / b, a);
}
static inline __m128 _mm_sqrt_ss(__m128 a)
{
    return __MM_SS_SHUF(_mm_sqrt_ps(a), a);
}
static inline __m128 _mm_min_ss(__m128 a, __m128 b)
{
    return __MM_SS_SHUF(_mm_min_ps(a, b), a);
}
static inline __m128 _mm_max_ss(__m128 a, __m128 b)
{
    return __MM_SS_SHUF(_mm_max_ps(a, b), a);
}

/* ---- comparisons ---------------------------------------------------------- */

#define __MM_CMP(name, op) \
    static inline __m128 name(__m128 a, __m128 b) \
    { return (__m128)(a op b); }

__MM_CMP(_mm_cmpeq_ps, ==)
__MM_CMP(_mm_cmplt_ps, <)
__MM_CMP(_mm_cmpgt_ps, >)
__MM_CMP(_mm_cmple_ps, <=)
__MM_CMP(_mm_cmpge_ps, >=)
__MM_CMP(_mm_cmpneq_ps, !=)

#define __MM_CMP_SS(name, psname) \
    static inline __m128 name(__m128 a, __m128 b) \
    { return __MM_SS_SHUF(psname(a, b), a); }

__MM_CMP_SS(_mm_cmpeq_ss, _mm_cmpeq_ps)
__MM_CMP_SS(_mm_cmplt_ss, _mm_cmplt_ps)
__MM_CMP_SS(_mm_cmpgt_ss, _mm_cmpgt_ps)
__MM_CMP_SS(_mm_cmple_ss, _mm_cmple_ps)
__MM_CMP_SS(_mm_cmpge_ss, _mm_cmpge_ps)
__MM_CMP_SS(_mm_cmpneq_ss, _mm_cmpneq_ps)

/* scalar compare-and-branch forms: the mask lane is all-ones or 0 */
#define __MM_COMI(name, op) \
    static inline int name(__m128 a, __m128 b) \
    { return __builtin_wasm_extract(27, (__xmm_i32)(a op b), 0, int) & 1; }

__MM_COMI(_mm_comieq_ss, ==)
__MM_COMI(_mm_comilt_ss, <)
__MM_COMI(_mm_comigt_ss, >)
__MM_COMI(_mm_comile_ss, <=)
__MM_COMI(_mm_comige_ss, >=)
__MM_COMI(_mm_comineq_ss, !=)

static inline int _mm_movemask_ps(__m128 a)
{
    return __builtin_wasm_vop1(164, (__xmm_i32)a, int);
}

/* ---- shuffles (selector is a compile-time constant) ------------------------ */

#define _mm_shuffle_ps(a, b, imm) \
    ((__m128)__builtin_wasm_shuffle((__xmm_b8)(a), (__xmm_b8)(b), \
        ((imm) & 3) * 4, ((imm) & 3) * 4 + 1, \
        ((imm) & 3) * 4 + 2, ((imm) & 3) * 4 + 3, \
        (((imm) >> 2) & 3) * 4, (((imm) >> 2) & 3) * 4 + 1, \
        (((imm) >> 2) & 3) * 4 + 2, (((imm) >> 2) & 3) * 4 + 3, \
        16 + (((imm) >> 4) & 3) * 4, 16 + (((imm) >> 4) & 3) * 4 + 1, \
        16 + (((imm) >> 4) & 3) * 4 + 2, 16 + (((imm) >> 4) & 3) * 4 + 3, \
        16 + (((imm) >> 6) & 3) * 4, 16 + (((imm) >> 6) & 3) * 4 + 1, \
        16 + (((imm) >> 6) & 3) * 4 + 2, 16 + (((imm) >> 6) & 3) * 4 + 3))

#define _mm_movelh_ps(a, b) \
    ((__m128)__builtin_wasm_shuffle((__xmm_b8)(a), (__xmm_b8)(b), \
        0,1,2,3, 4,5,6,7, 16,17,18,19, 20,21,22,23))
#define _mm_movehl_ps(a, b) \
    ((__m128)__builtin_wasm_shuffle((__xmm_b8)(a), (__xmm_b8)(b), \
        24,25,26,27, 28,29,30,31, 8,9,10,11, 12,13,14,15))
#define _mm_unpacklo_ps(a, b) \
    ((__m128)__builtin_wasm_shuffle((__xmm_b8)(a), (__xmm_b8)(b), \
        0,1,2,3, 16,17,18,19, 4,5,6,7, 20,21,22,23))
#define _mm_unpackhi_ps(a, b) \
    ((__m128)__builtin_wasm_shuffle((__xmm_b8)(a), (__xmm_b8)(b), \
        8,9,10,11, 24,25,26,27, 12,13,14,15, 28,29,30,31))

/* ---- conversions -------------------------------------------------------------- */

static inline float _mm_cvtss_f32(__m128 a)
{
    return __builtin_wasm_extract(31, a, 0, float);
}
static inline __m128 _mm_cvtsi32_ss(__m128 a, int i)
{
    __xmm_i32 vi = __builtin_wasm_splat(17, i, __xmm_i32);
    __m128 vf = __builtin_wasm_vop1(250, vi, __m128);
    return __builtin_wasm_replace(32, a, 0,
                                  __builtin_wasm_extract(31, vf, 0, float));
}
static inline int _mm_cvtss_si32(__m128 a)
{
    __m128 n = __builtin_wasm_vop1(106, a, __m128);       /* nearest */
    __xmm_i32 r = __builtin_wasm_vop1(248, n, __xmm_i32); /* trunc_sat */
    return __builtin_wasm_extract(27, r, 0, int);
}
static inline int _mm_cvttss_si32(__m128 a)
{
    __xmm_i32 r = __builtin_wasm_vop1(248, a, __xmm_i32);
    return __builtin_wasm_extract(27, r, 0, int);
}

#undef __MM_SS_SHUF
#undef __MM_CMP
#undef __MM_CMP_SS
#undef __MM_COMI

#endif /* _XMMINTRIN_H */
