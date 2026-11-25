/* riscv64 gating probe: a vector type far wider than one register
   group at the -march minimum VLEN must fail with the named
   too-wide error, not a crash. */
typedef int big_t __attribute__((vector_size(4096)));
big_t f(big_t a, big_t b) { return a + b; }
