/* riscv64 feature macros: compiles only when __riscv_vector is
   defined with __riscv_v_min_vlen == 256 */
#if defined(__riscv_vector) && __riscv_v_min_vlen == 256
int ok = 1;
#else
#error no v
#endif
