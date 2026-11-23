/*
 * win_host_test.c - Windows diagnostic host for the dual-form EXE.
 *
 * This is a body-level diagnostic, not a package test: it loads the
 * dual-form tinyc.exe with LoadLibrary, resolves the exported entry
 * points, and exercises both library-mode paths end to end:
 *   1. tinyc_init() after the load (an EXE gets no DllMain call, so
 *      initialization is the host's job - design doc 3.2);
 *   2. tinyc_main --version, then tinyc_main compiling a hello.c to
 *      a real .exe next to the dual-form file (its tcc dir resolves
 *      from its own location), which is then executed and its stdout
 *      checked;
 *   3. the tcc_* API compiling a self-contained program in memory
 *      and running it (expected exit code 42).
 *
 * Build (MinGW):  gcc -O2 -o win_host_test.exe win_host_test.c
 * Usage:          win_host_test.exe <path-to-dual-form-tinyc.exe>
 * Exit code 0 and a final "WIN HOST TEST: PASS" line on success.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef int (*init_fn)(void);
typedef int (*main_fn)(int, char **);
typedef void *(*new_fn)(void);
typedef void (*del_fn)(void *);
typedef int (*compile_fn)(void *, const char *);
typedef int (*outtype_fn)(void *, int);
typedef int (*reloc_fn)(void *, void *);
typedef int (*run_fn)(void *, int, char **);

static int failures = 0;
static void check(int ok, const char *what)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) failures++;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: win_host_test.exe <tinyc.exe>\n");
        return 2;
    }
    const char *exe = argv[1];

    HMODULE h = LoadLibraryA(exe);
    check(h != NULL, "LoadLibrary of the dual-form EXE");
    if (!h) {
        fprintf(stderr, "LoadLibrary error: %lu\n", (unsigned long)GetLastError());
        return 1;
    }

    init_fn p_init = (init_fn)(void *)GetProcAddress(h, "tinyc_init");
    main_fn p_main = (main_fn)(void *)GetProcAddress(h, "tinyc_main");
    new_fn p_new = (new_fn)(void *)GetProcAddress(h, "tcc_new");
    del_fn p_del = (del_fn)(void *)GetProcAddress(h, "tcc_delete");
    compile_fn p_compile = (compile_fn)(void *)GetProcAddress(h, "tcc_compile_string");
    outtype_fn p_outtype = (outtype_fn)(void *)GetProcAddress(h, "tcc_set_output_type");
    reloc_fn p_reloc = (reloc_fn)(void *)GetProcAddress(h, "tcc_relocate");
    run_fn p_run = (run_fn)(void *)GetProcAddress(h, "tcc_run");
    check(p_init && p_main && p_new && p_del && p_compile && p_outtype && p_reloc && p_run,
          "exports resolve: tinyc_init, tinyc_main, tcc_* API");
    if (failures) return 1;

    check(p_init() == 0, "tinyc_init() returns 0");

    {
        char a0[] = "tinyc", a1[] = "--version";
        char *args[] = { a0, a1, NULL };
        check(p_main(2, args) == 0, "tinyc_main --version returns 0");
    }

    /* tinyc_main compiles hello.c to an exe; run it and check stdout.
     * The output exe lands next to this test binary (cwd). */
    {
        const char *src =
            "#include <stdio.h>\n"
            "int main(void) { printf(\"hello from tinyc\\n\"); return 0; }\n";
        FILE *f = fopen("win_hello.c", "wb");
        fputs(src, f);
        fclose(f);
        char a0[] = "tinyc", a1[] = "win_hello.c", a2[] = "-o", a3[] = "win_hello.exe";
        char *args[] = { a0, a1, a2, a3, NULL };
        check(p_main(4, args) == 0, "tinyc_main compiles hello.c to win_hello.exe");
        int rc = system("win_hello.exe > win_hello.out 2>&1");
        check(rc == 0, "compiled hello.exe runs with exit code 0");
        char buf[128] = { 0 };
        f = fopen("win_hello.out", "rb");
        if (f) { size_t n = fread(buf, 1, sizeof buf - 1, f); buf[n] = 0; fclose(f); }
        check(strstr(buf, "hello from tinyc") != NULL,
              "compiled hello.exe prints the expected line");
    }

    /* tcc_* API: compile in memory and run; no external symbols, so
     * no library path is needed. TCC_OUTPUT_MEMORY == 0. */
    {
        void *s = p_new();
        check(s != NULL, "tcc_new returns a state");
        if (s) {
            p_outtype(s, 0);
            int rc = p_compile(s, "int main(void) { return 42; }");
            check(rc == 0, "tcc_compile_string succeeds");
            if (rc == 0) {
                check(p_reloc(s, (void *)(intptr_t)-1) == 0, "tcc_relocate succeeds");
                check(p_run(s, 0, NULL) == 42, "tcc_run returns the program's 42");
            }
            p_del(s);
        }
    }

    printf(failures ? "WIN HOST TEST: FAIL (%d)\n" : "WIN HOST TEST: PASS\n", failures);
    return failures ? 1 : 0;
}
