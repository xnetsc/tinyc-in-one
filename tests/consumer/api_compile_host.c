/* Host for the relocated-dist test (tests/test_relocated.sh).
 *
 * dlopen()s a dual-form file from a copied dist and compiles with
 * it WITHOUT setting the library path: no -B for tinyc_main, no
 * tcc_set_lib_path for the API route. The compiler must resolve
 * its own lib/tcc from the loaded file's location; the test moves
 * the original dist aside first, so a fallback to the baked
 * build-machine path fails instead of masking the result.
 *
 * Usage: api_compile_host <dual-form-file> <hello.c> <workdir>
 * Both produced programs are run and their output checked.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

typedef struct TCCState TCCState;
#define TCC_OUTPUT_EXE 2

static void *must_sym(void *h, const char *name)
{
    void *p = dlsym(h, name);
    if (!p) { printf("HOST-FAIL: no symbol %s\n", name); exit(1); }
    return p;
}

static int run_and_check(const char *path, const char *expect)
{
    char cmd[1200], buf[256];
    snprintf(cmd, sizeof cmd, "%s > %s.out 2>&1", path, path);
    if (system(cmd) != 0) { printf("HOST-FAIL: %s did not run\n", path); return 1; }
    snprintf(cmd, sizeof cmd, "%s.out", path);
    FILE *f = fopen(cmd, "rb");
    if (!f) { printf("HOST-FAIL: no output from %s\n", path); return 1; }
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = 0;
    printf("ran %s: %s", path, buf);
    if (!strstr(buf, expect)) {
        printf("HOST-FAIL: expected '%s' in output of %s\n", expect, path);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <dual-form-file> <hello.c> <workdir>\n", argv[0]);
        return 2;
    }
    const char *dual = argv[1], *hello = argv[2], *work = argv[3];
    void *h = dlopen(dual, RTLD_NOW | RTLD_LOCAL);
    if (!h) { printf("HOST-FAIL: dlopen %s: %s\n", dual, dlerror()); return 1; }
    printf("dlopen ok: %s\n", dual);

    typedef int (*tinyc_main_fn)(int, char **);
    tinyc_main_fn p_main = (tinyc_main_fn)must_sym(h, "tinyc_main");

    /* 1. tinyc_main compiles hello.c - no -B anywhere */
    char out1[1024], out2[1024];
    snprintf(out1, sizeof out1, "%s/hello_via_main", work);
    snprintf(out2, sizeof out2, "%s/hello_via_api", work);
    char *args1[] = { (char *)"tinyc", (char *)hello,
                      (char *)"-o", out1, NULL };
    int rc = p_main(4, args1);
    printf("tinyc_main(compile hello.c, no -B) rc=%d\n", rc);
    if (rc != 0) { printf("HOST-FAIL: compile via tinyc_main\n"); return 1; }

    /* 2. fine-grained API from a source string - no set_lib_path */
    TCCState *(*p_tcc_new)(void) = (void *)must_sym(h, "tcc_new");
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
    if (p_tcc_set_output_type(s, TCC_OUTPUT_EXE) != 0) {
        printf("HOST-FAIL: tcc_set_output_type\n"); return 1;
    }
    rc = p_tcc_compile_string(s, src);
    printf("tcc_compile_string (no set_lib_path) rc=%d\n", rc);
    if (rc != 0) { printf("HOST-FAIL: tcc_compile_string\n"); return 1; }
    rc = p_tcc_output_file(s, out2);
    printf("tcc_output_file rc=%d\n", rc);
    p_tcc_delete(s);
    if (rc != 0) { printf("HOST-FAIL: tcc_output_file\n"); return 1; }

    if (run_and_check(out1, "hello from tinyc")) return 1;
    if (run_and_check(out2, "hello via api: 42")) return 1;
    printf("HOST-ALIVE\n");
    return 0;
}
