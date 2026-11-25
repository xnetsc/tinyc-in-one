/* Layered like a vendor SIMD header: public entry points are
   static inline wrappers over mid-layer wrappers over the raw
   immediate-taking builtin.  Include before any system header:
   glibc defines __attribute__ away, which would strip the
   vector_size from the typedef. */
#ifndef TC_LANE_HEADER_H
#define TC_LANE_HEADER_H

typedef float __m128 __attribute__((__vector_size__(16)));

/* raw layer */
static inline __m128 tc_shuffle(__m128 a, __m128 b, int i)
{
    return __builtin_ia32_vop2i(49, a, b, i, __m128);
}

/* mid layer: forwards the immediate */
static inline __m128 tc_dup_shuf(__m128 a, int i)
{
    return tc_shuffle(a, a, i);
}

/* public layer */
static inline __m128 tc_rev(__m128 a) { return tc_dup_shuf(a, 0x1b); }
static inline __m128 tc_fwd_n(__m128 a, int i) { return tc_dup_shuf(a, i); }
static inline __m128 tc_add(__m128 a, __m128 b) { return a + b; }

#endif
