/* test-late-dylink.c - libtcc API host for the late-dylink
 * regression.  Replays the browser's TinycState.run sequence
 * (MEMORY output type at compile time, flavor and EXE at run
 * time) against one minimal program, int answer() { return 42; },
 * in three modes; run-late-dylink.mjs checks the outcomes.
 *
 *   early      flavor set before compiling: output must succeed
 *              and be a valid dylink main module.
 *   late       flavor set after compiling: output must fail with
 *              the explicit ordering error, never an invalid
 *              module and never a crash.
 *   standalone no flavor at all: output must succeed and be a
 *              valid bare module.  (Covers the double
 *              tcc_set_output_type call on its own: MEMORY at
 *              compile time, EXE at output time.)
 *
 * Usage: test-late-dylink <mode> <outfile>
 *
 * Build (mirrors the wasm32-libtcc object flags in build.log):
 *   gcc -o tests/wasm32-libtcc.o -c build-src/libtcc.c \
 *       -DTCC_TARGET_WASM32 -DCONFIG_TCC_CROSSPREFIX='"wasm32-"' \
 *       -Ibuild-src -Wall -O2
 *   gcc -o tests/test-late-dylink tests/test-late-dylink.c \
 *       tests/wasm32-libtcc.o -Ibuild-src -lm -ldl -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libtcc.h"

static const char *SRC = "int answer() { return 42; }";

int main(int argc, char **argv)
{
    TCCState *s;
    int rc;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <early|late|standalone> <outfile>\n",
                argv[0]);
        return 2;
    }

    s = tcc_new();
    if (!s) { printf("FAIL: tcc_new\n"); return 1; }
    if (tcc_set_options(s, "-nostdlib") != 0) {
        printf("FAIL: -nostdlib\n"); return 1;
    }
    if (strcmp(argv[1], "early") == 0) {
        if (tcc_set_options(s, "-wasm-dylink=main") != 0) {
            printf("FAIL: -wasm-dylink=main\n"); return 1;
        }
    }
    if (tcc_set_output_type(s, TCC_OUTPUT_MEMORY) != 0) {
        printf("FAIL: output MEMORY\n"); return 1;
    }
    if (tcc_compile_string(s, SRC) != 0) {
        printf("FAIL: compile_string\n"); return 1;
    }
    if (strcmp(argv[1], "late") == 0) {
        if (tcc_set_options(s, "-wasm-dylink=main") != 0) {
            printf("FAIL: -wasm-dylink=main\n"); return 1;
        }
    }
    if (tcc_set_output_type(s, TCC_OUTPUT_EXE) != 0) {
        printf("FAIL: output EXE\n"); return 1;
    }
    rc = tcc_output_file(s, argv[2]);
    if (strcmp(argv[1], "late") == 0) {
        /* The late flavor must be refused loudly. */
        if (rc == 0) {
            printf("FAIL: late flavor unexpectedly produced output\n");
            return 1;
        }
        printf("late: output refused as expected (rc=%d)\n", rc);
        return 0;
    }
    if (rc != 0) {
        printf("FAIL: output_file\n"); return 1;
    }
    printf("%s: output ok\n", argv[1]);
    return 0;
}
