/* test-two-states.c - libtcc API host for the registry-isolation
 * regression.  Drives several TCCStates of the wasm32 target in one
 * process and writes one module per scenario; run-two-states.mjs
 * instantiates the modules and asserts their export sets.
 *
 * Scenarios:
 *   m1   state A compiles fa, outputs.
 *   m1b  state A outputs the same program again (no compile in
 *        between): the module must be re-emitted identically.
 *   m3   state A then compiles a different program (fc) and outputs:
 *        the new module must not contain fa.
 *   m2   state B compiles fb and outputs: no fa, no fc.
 *   multi state C compiles a two-file program (ts-f1.c + ts-f2.c)
 *        and outputs: both files' functions are in the module.
 *   m5   state D compiles fz but never outputs and is deleted;
 *        state E then compiles fy and outputs: no fz.
 *
 * Build (mirrors the wasm32-tcc object flags in build.log):
 *   gcc -o tests/wasm32-libtcc.o -c build-src/libtcc.c \
 *       -DTCC_TARGET_WASM32 -DCONFIG_TCC_CROSSPREFIX='"wasm32-"' \
 *       -Ibuild-src -Wall -O2
 *   gcc -o tests/test-two-states tests/test-two-states.c \
 *       tests/wasm32-libtcc.o -Ibuild-src -lm -ldl -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include "libtcc.h"

static int failures;

static TCCState *new_state(void)
{
    TCCState *s = tcc_new();
    if (!s) { printf("FAIL: tcc_new\n"); exit(1); }
    tcc_set_options(s, "-nostdlib");
    if (tcc_set_output_type(s, TCC_OUTPUT_EXE) != 0) {
        printf("FAIL: tcc_set_output_type\n"); exit(1);
    }
    return s;
}

static void compile_str(TCCState *s, const char *src, const char *what)
{
    if (tcc_compile_string(s, src) != 0) {
        printf("FAIL: compile_string (%s)\n", what);
        failures++;
    }
}

static void output(TCCState *s, const char *path, const char *what)
{
    if (tcc_output_file(s, path) != 0) {
        printf("FAIL: tcc_output_file (%s)\n", what);
        failures++;
    } else {
        printf("wrote %s\n", path);
    }
}

int main(void)
{
    TCCState *a, *b, *c, *d, *e;

    a = new_state();
    compile_str(a, "int fa(void) { return 11; }", "m1");
    output(a, "tests/ts-m1.wasm", "m1");
    output(a, "tests/ts-m1b.wasm", "m1b");
    compile_str(a, "int fc(void) { return 33; }", "m3");
    output(a, "tests/ts-m3.wasm", "m3");
    tcc_delete(a);

    b = new_state();
    compile_str(b, "int fb(void) { return 22; }", "m2");
    output(b, "tests/ts-m2.wasm", "m2");
    tcc_delete(b);

    c = new_state();
    if (tcc_add_file(c, "tests/ts-f1.c") != 0 ||
        tcc_add_file(c, "tests/ts-f2.c") != 0) {
        printf("FAIL: tcc_add_file (multi)\n");
        failures++;
    }
    output(c, "tests/ts-multi.wasm", "multi");
    tcc_delete(c);

    d = new_state();
    compile_str(d, "int fz(void) { return 99; }", "abandoned");
    tcc_delete(d); /* no output: fz must not leak into the next module */

    e = new_state();
    compile_str(e, "int fy(void) { return 55; }", "m5");
    output(e, "tests/ts-m5.wasm", "m5");
    tcc_delete(e);

    if (failures) {
        printf("test-two-states: %d failure(s)\n", failures);
        return 1;
    }
    printf("test-two-states: all modules written\n");
    return 0;
}
