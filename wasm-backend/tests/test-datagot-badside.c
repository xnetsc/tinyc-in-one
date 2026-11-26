/* test-datagot-badside.c - a static data initializer points at
   a function nobody provides.  Loading this module must fail,
   naming missing_fn: a GOT.func value baked into static data
   binds eagerly, exactly like GOT.mem — it may not be left as a
   stub slot or as the silent 0 the slot starts with. */
extern int missing_fn(int x);
int (*imp_fp)(int) = missing_fn;
int call_imp_fp(int x) { return imp_fp(x); }
