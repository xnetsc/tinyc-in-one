#include "neon_style.h"

/* Compile-level consumer for the aarch64 cross compiler: the
   immediate-forwarding layers must compile, which the entity path
   cannot do at all. */
int neon_consumer(int32x4_t v)
{
    int32x4_t r = tc_add(tc_shl2(v), v);
    return tc_lane2(r) + tc_get_lane_fwd(r, 0) + tc_get_lane(r, 1);
}
