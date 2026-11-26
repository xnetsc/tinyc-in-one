/* test-addronly-side.c - dylink side module: the imported
   function is only ever address-taken (assigned to a function
   pointer at runtime), never called directly and never a data
   initializer.  Its address materialisation must still classify
   as a function — GOT.func — from the symbol's own type, not
   from whether a call ever interned it as a WFunc; classified
   as GOT.mem instead, the loader rejects the module with an
   "unresolved variable" error. */
extern int imp_fn(int x);
int (*imp_fp)(int);
void bind_imp(void) { imp_fp = imp_fn; }
int call_imp_fp(int x) { return imp_fp(x); }
int imp_fp_is(int (*p)(int)) { return imp_fp == p; }
