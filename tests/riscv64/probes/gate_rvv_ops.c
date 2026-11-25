/* riscv64 gating probe: a raw vector operation with no header.
   Without the V feature the backend must refuse it with the named
   error; with -march=rv64gcv it compiles. */
typedef int v4 __attribute__((vector_size(16)));
v4 f(v4 a, v4 b) { return a + b; }
