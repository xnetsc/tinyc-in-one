/* test-datagot-side2.c - companion to test-datagot-side.c: the
   same static initializer, plus a runtime address-take of the
   imported function inside the module, so the guest can check
   the data-initialised value against the GOT.func value a code
   materialisation reads. */
extern int imp_fn(int x);
int (*imp_fp)(int) = imp_fn;
int call_imp_fp(int x) { return imp_fp(x); }
int take_matches(void)
{
    int (*p)(int) = imp_fn;
    return p == imp_fp;
}
