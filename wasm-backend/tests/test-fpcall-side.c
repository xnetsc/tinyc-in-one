/* test-fpcall-side.c - dylink side module: calls through global
   function-pointer variables in the two flavours the loader sees.
   own_fp is statically initialised to this module's own function;
   imp_fp is bound at runtime (bind_imp) to an imported function -
   one this module also calls directly (call_imp_direct), so the
   import itself is an ordinary function import.  The call sites
   own_fp(x) / imp_fp(x) must load the cell and go indirect: as
   direct calls they would target whatever function bears the
   variable's name in the process. */
extern int imp_fn(int x);
static int seven_up(int x) { return x + 7; }
int (*own_fp)(int) = seven_up;
int (*imp_fp)(int);
void bind_imp(void) { imp_fp = imp_fn; }
int call_own_fp(int x) { return own_fp(x); }
int call_imp_fp(int x) { return imp_fp(x); }
int call_imp_direct(int x) { return imp_fn(x); }
