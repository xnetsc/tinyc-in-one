/* test-datareloc-main.c - dylink main module: dlopen the side
   module from test-datareloc-side.c and run its checks. */
#include <dlfcn.h>
int puts(const char *s);
typedef int (*intfn)(void);
typedef char *(*strfn)(void);
int main(void)
{
    void *h = dlopen("libdata.wasm", RTLD_NOW | RTLD_LOCAL);
    intfn check_data;
    strfn get_s;
    char *sv;
    if (!h) { puts("FAIL: dlopen libdata"); return 1; }
    check_data = (intfn)dlsym(h, "check_data");
    get_s = (strfn)dlsym(h, "get_s");
    if (!check_data || !get_s) { puts("FAIL: dlsym"); return 2; }
    if (check_data() != 0) { puts("FAIL: check_data mask"); return 3; }
    sv = get_s();
    if (!sv || sv[0] != 'h' || sv[1] != 'i') { puts("FAIL: get_s view"); return 4; }
    puts("DYDATA-RESULT: PASS");
    return 0;
}
