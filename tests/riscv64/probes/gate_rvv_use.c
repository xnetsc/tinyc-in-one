/* riscv64 gating probe: a shipped intrinsic through the header.
   Compiles at -march=rv64gcv_zvl128b; without a -march the header
   itself refuses (its types are sized by __riscv_v_min_vlen). */
#include <stddef.h>
#include <riscv_vector.h>
vint32m1_t f(vint32m1_t a, vint32m1_t b, size_t vl)
{ return __riscv_vadd_vv_i32m1(a, b, vl); }
