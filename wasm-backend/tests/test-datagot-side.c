/* test-datagot-side.c - dylink side module: a static data
   initializer points at an imported function, and that
   initializer is the module's only reference to the function.
   The reference must route through GOT.func (generated from the
   data relocation alone), and the loader must bind it eagerly:
   the synthesized init copies the GOT value into the slot, so
   after load the slot holds the function's canonical table
   slot — the value a runtime address materialisation or dlsym
   produces — not the addend 0 the slot starts with. */
extern int imp_fn(int x);
int (*imp_fp)(int) = imp_fn;
int call_imp_fp(int x) { return imp_fp(x); }
int imp_fp_is(int (*p)(int)) { return imp_fp == p; }
