/* host_dlopen.c - dlopen-path test host.
 *
 * Loads the extracted dylib, then:
 *   1. calls tinyc_main to compile hello.c into an executable;
 *   2. calls tinyc_main with -dumpversion (the patched code path that
 *      used to end in exit(0)) - the host must survive it;
 *   3. uses the fine-grained tcc_* API (tcc_new / tcc_set_lib_path /
 *      tcc_compile_string / tcc_output_file) to build a second
 *      executable from a source string.
 * Prints HOST-ALIVE at the end: reaching it proves no call along the
 * way terminated the host process.
 *
 * Usage: host_dlopen <dylib> <tcc-lib-path> <hello.c> <workdir>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "libtcc.h"

typedef int (*tinyc_main_fn)(int, char **);

static void *must_sym(void *h, const char *name)
{
    void *p = dlsym(h, name);
    if (!p) {
        printf("HOST-FAIL: dlsym(%s): %s\n", name, dlerror());
        exit(1);
    }
    return p;
}

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: %s <dylib> <tcc-lib-path> <hello.c> <workdir>\n",
                argv[0]);
        return 2;
    }
    const char *dylib = argv[1];
    const char *tcclib = argv[2];
    const char *hello = argv[3];
    const char *work = argv[4];

    void *h = dlopen(dylib, RTLD_NOW | RTLD_GLOBAL);
    if (!h) {
        printf("HOST-FAIL: dlopen: %s\n", dlerror());
        return 1;
    }
    printf("dlopen ok: %s\n", dylib);

    tinyc_main_fn tinyc_main = (tinyc_main_fn)must_sym(h, "tinyc_main");

    /* 1. tinyc_main compiles hello.c */
    char out1[1024], out2[1024];
    snprintf(out1, sizeof out1, "%s/hello_via_main", work);
    snprintf(out2, sizeof out2, "%s/hello_via_api", work);
    char *args1[] = { (char *)"tinyc", (char *)"-B", (char *)tcclib,
                      (char *)"-o", out1, (char *)hello, NULL };
    int rc = tinyc_main(6, args1);
    printf("tinyc_main(compile hello.c) rc=%d\n", rc);
    if (rc != 0) { printf("HOST-FAIL: compile via tinyc_main\n"); return 1; }
    FILE *t = fopen(out1, "rb");
    if (!t) { printf("HOST-FAIL: %s not produced\n", out1); return 1; }
    fclose(t);

    /* 2. -dumpversion must return, not exit */
    char *args2[] = { (char *)"tinyc", (char *)"-dumpversion", NULL };
    rc = tinyc_main(2, args2);
    printf("tinyc_main(-dumpversion) rc=%d (host still alive)\n", rc);
    if (rc != 0) { printf("HOST-FAIL: -dumpversion rc\n"); return 1; }

    /* 3. fine-grained API from a source string */
    TCCState *(*p_tcc_new)(void) = (void *)must_sym(h, "tcc_new");
    void (*p_tcc_set_lib_path)(TCCState *, const char *) =
        (void *)must_sym(h, "tcc_set_lib_path");
    int (*p_tcc_set_output_type)(TCCState *, int) =
        (void *)must_sym(h, "tcc_set_output_type");
    int (*p_tcc_compile_string)(TCCState *, const char *) =
        (void *)must_sym(h, "tcc_compile_string");
    int (*p_tcc_output_file)(TCCState *, const char *) =
        (void *)must_sym(h, "tcc_output_file");
    void (*p_tcc_delete)(TCCState *) = (void *)must_sym(h, "tcc_delete");

    const char *src =
        "#include <stdio.h>\n"
        "int main(void) { printf(\"hello via api: %d\\n\", 6 * 7); return 0; }\n";
    TCCState *s = p_tcc_new();
    if (!s) { printf("HOST-FAIL: tcc_new\n"); return 1; }
    p_tcc_set_lib_path(s, tcclib);
    if (p_tcc_set_output_type(s, TCC_OUTPUT_EXE) != 0) {
        printf("HOST-FAIL: tcc_set_output_type\n"); return 1;
    }
    rc = p_tcc_compile_string(s, src);
    printf("tcc_compile_string rc=%d\n", rc);
    if (rc != 0) { printf("HOST-FAIL: tcc_compile_string\n"); return 1; }
    rc = p_tcc_output_file(s, out2);
    printf("tcc_output_file rc=%d\n", rc);
    p_tcc_delete(s);
    if (rc != 0) { printf("HOST-FAIL: tcc_output_file\n"); return 1; }
    t = fopen(out2, "rb");
    if (!t) { printf("HOST-FAIL: %s not produced\n", out2); return 1; }
    fclose(t);

    printf("HOST-ALIVE\n");
    return 0;
}
