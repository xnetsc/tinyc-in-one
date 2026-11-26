/* test-addronly-main.c - dylink main module: provides imp_fn
   and loads the addronly side module, whose only reference to
   imp_fn is a runtime address-take.  dlopen must succeed (the
   reference is a function reference, not a variable reference),
   and the bound pointer must be the canonical one. */
#include <dlfcn.h>
int puts(const char *s);
typedef int (*intfn)(int);
typedef int (*isfn)(int (*)(int));
typedef void (*vfn)(void);
int imp_fn(int x) { return x + 1000; }
int main(void)
{
    void *h = dlopen("libaddronly.wasm", RTLD_NOW | RTLD_LOCAL);
    intfn call;
    isfn is;
    vfn bind;
    void *dv;
    if (!h) { puts(dlerror()); puts("FAIL: dlopen addronly"); return 1; }
    call = (intfn)dlsym(h, "call_imp_fp");
    is = (isfn)dlsym(h, "imp_fp_is");
    bind = (vfn)dlsym(h, "bind_imp");
    if (!call || !is || !bind) { puts("FAIL: dlsym"); return 2; }
    bind();
    if (call(1) != 1001) { puts("FAIL: call through bound fp"); return 3; }
    if (!is(imp_fn)) { puts("FAIL: bound != main materialisation"); return 4; }
    dv = dlsym(0, "imp_fn");
    if (!dv || !is((intfn)dv)) { puts("FAIL: bound != dlsym value"); return 5; }
    puts("DYADDR-RESULT: PASS");
    return 0;
}
