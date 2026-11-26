/* test-fpcall-main.c - dylink main module: provides the side
   module's imported function, dlopens it, and checks that its
   calls through global function-pointer variables reach the
   pointees (own function 8, imported function 1001). */
#include <dlfcn.h>
int puts(const char *s);
typedef int (*intfn)(int);
typedef void (*voidfn)(void);
int imp_fn(int x) { return x + 1000; }
int main(void)
{
    void *h = dlopen("libfp.wasm", RTLD_NOW | RTLD_LOCAL);
    intfn own, imp, direct;
    voidfn bind;
    if (!h) { puts("FAIL: dlopen libfp"); return 1; }
    own = (intfn)dlsym(h, "call_own_fp");
    imp = (intfn)dlsym(h, "call_imp_fp");
    direct = (intfn)dlsym(h, "call_imp_direct");
    bind = (voidfn)dlsym(h, "bind_imp");
    if (!own || !imp || !direct || !bind) { puts("FAIL: dlsym"); return 2; }
    bind();
    if (own(1) != 8) { puts("FAIL: own_fp"); return 3; }
    if (imp(1) != 1001) { puts("FAIL: imp_fp"); return 4; }
    if (direct(1) != 1001) { puts("FAIL: imp direct"); return 5; }
    puts("DYFP-RESULT: PASS");
    return 0;
}
