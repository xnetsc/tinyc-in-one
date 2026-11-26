/* test-datagot-main.c - dylink main module: provides imp_fn,
   loads the datagot side modules, and checks that a function
   pointer installed by a static data initializer is callable
   and identical to the pointer every other route produces —
   this module's own materialisation, dlsym, the raw slot
   contents, and a runtime address-take inside the side module.
   Loading the bad side must fail with an error naming its
   missing function. */
#include <dlfcn.h>
int puts(const char *s);
typedef int (*intfn)(int);
typedef int (*isfn)(int (*)(int));
typedef int (*checkfn)(void);
typedef int *(*getpfn)(void);
int imp_fn(int x) { return x + 1000; }
int ext_var = 4242;

static int contains(const char *hay, const char *needle)
{
    int i, j;
    for (i = 0; hay[i]; i++) {
        for (j = 0; needle[j]; j++)
            if (hay[i + j] != needle[j]) break;
        if (!needle[j]) return 1;
    }
    return 0;
}

int main(void)
{
    void *h, *h2, *h3;
    intfn call;
    isfn is;
    checkfn take;
    int *cell;
    void *dv;
    const char *err;

    h = dlopen("libdatagot.wasm", RTLD_NOW | RTLD_LOCAL);
    if (!h) { puts(dlerror()); puts("FAIL: dlopen datagot"); return 1; }
    call = (intfn)dlsym(h, "call_imp_fp");
    is = (isfn)dlsym(h, "imp_fp_is");
    cell = (int *)dlsym(h, "imp_fp");
    if (!call || !is || !cell) { puts("FAIL: dlsym datagot"); return 2; }
    if (call(1) != 1001) { puts("FAIL: call through data-init fp"); return 3; }
    if (!is(imp_fn)) { puts("FAIL: slot != main materialisation"); return 4; }
    dv = dlsym(0, "imp_fn");
    if (!dv) { puts("FAIL: dlsym imp_fn"); return 5; }
    if (!is((intfn)dv)) { puts("FAIL: slot != dlsym value"); return 6; }
    if (*cell != (int)dv) { puts("FAIL: raw slot != dlsym value"); return 7; }

    h2 = dlopen("libdatagot2.wasm", RTLD_NOW | RTLD_LOCAL);
    if (!h2) { puts(dlerror()); puts("FAIL: dlopen datagot2"); return 8; }
    take = (checkfn)dlsym(h2, "take_matches");
    call = (intfn)dlsym(h2, "call_imp_fp");
    if (!take || !call) { puts("FAIL: dlsym datagot2"); return 9; }
    if (!take()) { puts("FAIL: slot != runtime GOT take"); return 10; }
    if (call(2) != 1002) { puts("FAIL: call through datagot2 fp"); return 11; }

    h2 = dlopen("libdatagotvar.wasm", RTLD_NOW | RTLD_LOCAL);
    if (!h2) { puts(dlerror()); puts("FAIL: dlopen datagotvar"); return 12; }
    {
        checkfn rd = (checkfn)dlsym(h2, "read_p");
        getpfn gp = (getpfn)dlsym(h2, "get_p");
        void *vv = dlsym(0, "ext_var");
        if (!rd || !gp || !vv) { puts("FAIL: dlsym datagotvar"); return 13; }
        if (rd() != 4242) { puts("FAIL: *p through data-init var ptr"); return 14; }
        if (gp() != (int *)vv) { puts("FAIL: var slot != dlsym value"); return 15; }
    }

    h3 = dlopen("libbadgot.wasm", RTLD_NOW | RTLD_LOCAL);
    if (h3) { puts("FAIL: dlopen badgot succeeded"); return 16; }
    err = dlerror();
    if (!err || !contains(err, "missing_fn")) {
        puts("FAIL: dlerror does not name missing_fn");
        return 17;
    }
    puts("DYGOT-RESULT: PASS");
    return 0;
}
