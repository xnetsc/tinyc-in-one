/* test-sse.c - exercises include/xmmintrin.h and include/emmintrin.h;
   every function returns a scalar so tests/run-sse.mjs can assert
   the value.  Float vector results are checked with the
   compare+movemask oracle where a lane value alone is not enough. */
#include "emmintrin.h"

float x_add_ps(void)
{
    __m128 a = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);   /* lanes 1,2,3,4 */
    __m128 b = _mm_add_ps(a, _mm_set1_ps(10.0f));
    return _mm_cvtss_f32(_mm_shuffle_ps(b, b, _MM_SHUFFLE(2, 2, 2, 2)));
}

float x_add_ss_hi(void)
{
    __m128 a = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    __m128 b = _mm_add_ss(a, _mm_set1_ps(10.0f));
    return _mm_cvtss_f32(_mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 3, 3, 3)));
}

float x_add_ss_lo(void)
{
    __m128 a = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    __m128 b = _mm_add_ss(a, _mm_set1_ps(10.0f));
    return _mm_cvtss_f32(b);
}

int x_movemask(void)
{
    __m128 a = _mm_set_ps(3.0f, 10.0f, 3.0f, 10.0f); /* lanes 10,3,10,3 */
    __m128 m = _mm_cmplt_ps(a, _mm_set1_ps(5.0f));   /* lanes 0,-1,0,-1 */
    return _mm_movemask_ps(m);
}

int x_comi(void)
{
    __m128 a = _mm_set_ss(2.0f);
    __m128 b = _mm_set_ss(2.0f);
    __m128 c = _mm_set_ss(3.0f);
    return _mm_comieq_ss(a, b) * 2 + _mm_comilt_ss(a, c);
}

int x_cvtss(void)
{
    return _mm_cvtss_si32(_mm_set_ss(3.6f)) * 10
         + _mm_cvttss_si32(_mm_set_ss(3.6f));
}

float x_cvtsi32(void)
{
    return _mm_cvtss_f32(_mm_cvtsi32_ss(_mm_setzero_ps(), 42));
}

int x_sqrt_minmax(void)
{
    __m128 a = _mm_sqrt_ps(_mm_set_ps(25.0f, 16.0f, 9.0f, 4.0f));
    __m128 m = _mm_cmpeq_ps(a, _mm_set_ps(5.0f, 4.0f, 3.0f, 2.0f));
    __m128 mx = _mm_max_ps(_mm_set_ps(1.0f, 9.0f, 1.0f, 1.0f),
                           _mm_set_ps(2.0f, 2.0f, 8.0f, 2.0f));
    __m128 e = _mm_cmpeq_ps(mx, _mm_set_ps(2.0f, 9.0f, 8.0f, 2.0f));
    return _mm_movemask_ps(m) + _mm_movemask_ps(e);
}

int x_unpack_movehl(void)
{
    __m128 a = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);
    __m128 b = _mm_set_ps(8.0f, 7.0f, 6.0f, 5.0f);
    __m128 u = _mm_unpacklo_ps(a, b);      /* 1,5,2,6 */
    __m128 e = _mm_cmpeq_ps(u, _mm_set_ps(6.0f, 2.0f, 5.0f, 1.0f));
    __m128 h = _mm_movehl_ps(a, b);        /* 7,8,3,4 */
    __m128 e2 = _mm_cmpeq_ps(h, _mm_set_ps(4.0f, 3.0f, 8.0f, 7.0f));
    return _mm_movemask_ps(e) + _mm_movemask_ps(e2);
}

int e_add_epi32(void)
{
    int out[4];
    __m128i a = _mm_setr_epi32(1, 2, 3, 4);
    __m128i b = _mm_add_epi32(a, _mm_set1_epi32(100));
    _mm_store_si128((__m128i *)out, b);
    return out[0] * 1000 + out[1] * 100 + out[2] * 10 + out[3];
}

int e_sat_epu8(void)
{
    unsigned char out[16];
    __m128i a = _mm_adds_epu8(_mm_set1_epi8(200), _mm_set1_epi8(100));
    _mm_store_si128((__m128i *)out, a);
    return out[3];
}

int e_madd(void)
{
    int out[4];
    __m128i x = _mm_set_epi16(0, 0, 0, 0, 4, 3, 2, 1);  /* lanes 1,2,3,4 */
    __m128i y = _mm_set_epi16(0, 0, 0, 0, 8, 7, 6, 5);  /* lanes 5,6,7,8 */
    _mm_store_si128((__m128i *)out, _mm_madd_epi16(x, y));
    return out[0] * 100 + out[1];
}

int e_packs(void)
{
    signed char out[16];
    /* lanes {1000,-1000,5,0,0,0,0,0} and {7,0,0,0,0,0,0,0} */
    __m128i big = _mm_set_epi16(0, 0, 0, 0, 0, 5, -1000, 1000);
    __m128i sml = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, 7);
    _mm_store_si128((__m128i *)out, _mm_packs_epi16(big, sml));
    return out[0] * 1000 + (out[1] + 128) * 10 + out[8];
}

int e_shuffle_epi32(void)
{
    int out[4];
    __m128i a = _mm_setr_epi32(1, 2, 3, 4);
    _mm_store_si128((__m128i *)out,
                    _mm_shuffle_epi32(a, _MM_SHUFFLE(0, 1, 2, 3)));
    return out[0] * 1000 + out[1] * 100 + out[2] * 10 + out[3];
}

int e_byte_shifts(void)
{
    unsigned char out[16];
    __m128i a = _mm_setr_epi32(0x04030201, 0, 0, 0);
    _mm_store_si128((__m128i *)out, _mm_slli_si128(a, 1));
    int hi = out[4];
    _mm_store_si128((__m128i *)out, _mm_srli_si128(a, 2));
    return hi * 100 + out[1];
}

int e_shuffle_epi8(void)
{
    unsigned char out[16];
    __m128i a = _mm_setr_epi32(0x04030201, 0, 0, 0);
    __m128i idx = _mm_setr_epi32(0x00010203, 0, 0, 0);
    _mm_store_si128((__m128i *)out, _mm_shuffle_epi8(a, idx));
    return out[0] * 10 + out[3];
}

int e_unpacklo_epi8(void)
{
    unsigned char out[16];
    __m128i a = _mm_setr_epi32(0x04030201, 0, 0, 0);
    __m128i b = _mm_setr_epi32(0x08070605, 0, 0, 0);
    _mm_store_si128((__m128i *)out, _mm_unpacklo_epi8(a, b));
    return out[0] * 1000 + out[1] * 100 + out[2] * 10 + out[3];
}

int e_cvtepi32_roundtrip(void)
{
    __m128 f = _mm_cvtepi32_ps(_mm_setr_epi32(3, -2, 0, 0));
    __m128i r = _mm_cvttps_epi32(f);
    int out[4];
    _mm_store_si128((__m128i *)out, r);
    return out[0] * 10 + out[1];
}

int e_srai(void)
{
    __m128i a = _mm_setr_epi32(-16, 3, 0, 0);
    int out[4];
    _mm_store_si128((__m128i *)out, _mm_srai_epi32(a, 2));
    return out[0];
}

int e_srli(void)
{
    __m128i a = _mm_setr_epi32(-16, 3, 0, 0);
    int out[4];
    _mm_store_si128((__m128i *)out, _mm_srli_epi32(a, 2));
    return out[0];
}

int e_movemask_epi8(void)
{
    __m128i a = _mm_setr_epi32(0x8001007f, 0, 0, 0);
    return _mm_movemask_epi8(a);
}

double e_add_pd(void)
{
    double out[2];
    __m128d a = _mm_set_pd(2.5, 1.5);      /* lanes 1.5, 2.5 */
    __m128d b = _mm_add_pd(a, _mm_set1_pd(0.5));
    _mm_store_pd(out, b);
    return out[1];
}

int e_cvtsd(void)
{
    __m128d a = _mm_set_pd(0.0, 7.8);       /* low lane 7.8 */
    return _mm_cvtsd_si32(a) * 10 + _mm_cvttsd_si32(a);
}
