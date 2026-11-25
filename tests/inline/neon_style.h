/* clang's arm_neon.h shape: public intrinsics as static inline
   wrappers, layered over each other, forwarding lane and shift
   immediates as ordinary parameters into the raw builtins.  (This
   tree's own arm_neon.h spells the same surface as macros; this
   fixture keeps the wrapper form to exercise call expansion.) */
#ifndef TC_NEON_STYLE_H
#define TC_NEON_STYLE_H

typedef int int32x4_t __attribute__((__vector_size__(16)));

/* raw layer: lane extract and shift-by-immediate builtins */
static inline int tc_get_lane(int32x4_t a, int n)
{
    return __builtin_aarch64_extract(5, a, n, int);
}
static inline int32x4_t tc_shl_n(int32x4_t a, int n)
{
    return __builtin_aarch64_vop1(139269 + ((n) << 4), a, int32x4_t);
}

/* mid layer: forwards the immediate */
static inline int tc_get_lane_fwd(int32x4_t a, int n)
{
    return tc_get_lane(a, n);
}
static inline int32x4_t tc_shl_fwd(int32x4_t a, int n)
{
    return tc_shl_n(a, n);
}

/* public layer */
static inline int tc_lane2(int32x4_t a) { return tc_get_lane_fwd(a, 2); }
static inline int32x4_t tc_shl2(int32x4_t a) { return tc_shl_fwd(a, 2); }
static inline int32x4_t tc_add(int32x4_t a, int32x4_t b) { return a + b; }

#endif
