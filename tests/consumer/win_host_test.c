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
 *
 * Diagnostics: stdout is unbuffered, and before every step a marker
 * line is appended to win_host_test.markers in the working directory
 * (open/write/close per marker, so the file survives a crash of this
 * process). The last marker names the step that was in flight.
 *
 * Before tinyc_init, the test calls tinyc_ping - an exported
 * function whose whole body is one Win32 file append inside the
 * loaded image. If the ping dies, calling into the image is itself
 * the fault; if the ping returns and tinyc_init dies, the fault is
 * inside tinyc_init. A vectored exception handler is armed across
 * the ping + init window: on a fatal exception it appends the
 * exception code, the fault address, its RVA relative to the loaded
 * module, and (for an access violation) the access kind and data
 * address to win_host_test.markers, then exits with code 0xE1.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef int (*ping_fn)(void);
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

static void mark(const char *step_name)
{
    FILE *f = fopen("win_host_test.markers", "ab");
    if (f) {
        fprintf(f, "%s\n", step_name);
        fclose(f);
    }
}

/* Marker append with plain Win32 calls only (no CRT stdio): the
 * exception handler below runs while the process is in an unknown
 * state and uses this, never mark(). */
static void mark_line(const char *line)
{
    HANDLE h = CreateFileA("win_host_test.markers", FILE_APPEND_DATA,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written;
    int len = 0;
    if (h == INVALID_HANDLE_VALUE)
        return;
    while (line[len])
        len++;
    WriteFile(h, line, (DWORD)len, &written, NULL);
    WriteFile(h, "\n", 1, &written, NULL);
    CloseHandle(h);
}

struct linebuf { char buf[384]; int len; };

static void lb_puts(struct linebuf *lb, const char *s)
{
    while (*s && lb->len < (int)sizeof(lb->buf) - 1)
        lb->buf[lb->len++] = *s++;
    lb->buf[lb->len] = 0;
}

static void lb_hex(struct linebuf *lb, unsigned long long v)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[19];
    int i;
    tmp[0] = '0'; tmp[1] = 'x';
    for (i = 0; i < 16; i++)
        tmp[2 + i] = digits[(v >> (60 - 4 * i)) & 0xf];
    tmp[18] = 0;
    lb_puts(lb, tmp);
}

static void lb_code(struct linebuf *lb, unsigned long v)
{
    static const char digits[] = "0123456789abcdef";
    char tmp[11];
    int i;
    tmp[0] = '0'; tmp[1] = 'x';
    for (i = 0; i < 8; i++)
        tmp[2 + i] = digits[(v >> (28 - 4 * i)) & 0xf];
    tmp[10] = 0;
    lb_puts(lb, tmp);
}

static HMODULE g_target;           /* base of the loaded dual-form EXE */
static volatile LONG g_veh_armed;  /* log-and-exit only while nonzero */

/* RVA of an address inside the loaded module, or 0 when outside it
 * (module size from its in-memory PE headers). */
static unsigned long long module_rva(const void *addr)
{
    const unsigned char *base = (const unsigned char *)g_target;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    IMAGE_NT_HEADERS *nt;
    unsigned long long off;
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE)
        return 0;
    nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return 0;
    off = (unsigned long long)((const unsigned char *)addr - base);
    return off < nt->OptionalHeader.SizeOfImage ? off : 0;
}

static LONG WINAPI veh_handler(EXCEPTION_POINTERS *ep)
{
    EXCEPTION_RECORD *rec;
    struct linebuf lb;
    unsigned long long addr, rva;
    if (!g_veh_armed)
        return EXCEPTION_CONTINUE_SEARCH;
    rec = ep->ExceptionRecord;
    if (rec->ExceptionCode != EXCEPTION_ACCESS_VIOLATION
        && rec->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION
        && rec->ExceptionCode != EXCEPTION_PRIV_INSTRUCTION
        && rec->ExceptionCode != EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_CONTINUE_SEARCH;
    addr = (unsigned long long)(uintptr_t)rec->ExceptionAddress;
    rva = module_rva(rec->ExceptionAddress);
    lb.len = 0; lb.buf[0] = 0;
    lb_puts(&lb, "veh: exception code=");
    lb_code(&lb, (unsigned long)rec->ExceptionCode);
    lb_puts(&lb, " address=");
    lb_hex(&lb, addr);
    if (rva) {
        lb_puts(&lb, " rva=");
        lb_hex(&lb, rva);
    } else {
        lb_puts(&lb, " rva=outside-module");
    }
    mark_line(lb.buf);
    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
        && rec->NumberParameters >= 2) {
        unsigned long long data = (unsigned long long)rec->ExceptionInformation[1];
        unsigned long long drva = module_rva((const void *)(uintptr_t)data);
        lb.len = 0; lb.buf[0] = 0;
        lb_puts(&lb, "veh: access violation access=");
        lb_puts(&lb, rec->ExceptionInformation[0] == 0 ? "read"
                : rec->ExceptionInformation[0] == 1 ? "write" : "execute");
        lb_puts(&lb, " data=");
        lb_hex(&lb, data);
        if (drva) {
            lb_puts(&lb, " data_rva=");
            lb_hex(&lb, drva);
        }
        mark_line(lb.buf);
    }
    ExitProcess(0xE1);
    return EXCEPTION_EXECUTE_HANDLER; /* not reached */
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (argc < 2) {
        fprintf(stderr, "usage: win_host_test.exe <tinyc.exe>\n");
        return 2;
    }
    const char *exe = argv[1];

    AddVectoredExceptionHandler(1, veh_handler);

    mark("loadlibrary:begin");
    HMODULE h = LoadLibraryA(exe);
    mark("loadlibrary:done");
    check(h != NULL, "LoadLibrary of the dual-form EXE");
    if (!h) {
        fprintf(stderr, "LoadLibrary error: %lu\n", (unsigned long)GetLastError());
        return 1;
    }
    g_target = h;
    {
        struct linebuf lb;
        lb.len = 0; lb.buf[0] = 0;
        lb_puts(&lb, "module:base=");
        lb_hex(&lb, (unsigned long long)(uintptr_t)h);
        mark_line(lb.buf);
    }

    mark("getprocaddress:begin");
    ping_fn p_ping = (ping_fn)(void *)GetProcAddress(h, "tinyc_ping");
    init_fn p_init = (init_fn)(void *)GetProcAddress(h, "tinyc_init");
    main_fn p_main = (main_fn)(void *)GetProcAddress(h, "tinyc_main");
    new_fn p_new = (new_fn)(void *)GetProcAddress(h, "tcc_new");
    del_fn p_del = (del_fn)(void *)GetProcAddress(h, "tcc_delete");
    compile_fn p_compile = (compile_fn)(void *)GetProcAddress(h, "tcc_compile_string");
    outtype_fn p_outtype = (outtype_fn)(void *)GetProcAddress(h, "tcc_set_output_type");
    reloc_fn p_reloc = (reloc_fn)(void *)GetProcAddress(h, "tcc_relocate");
    run_fn p_run = (run_fn)(void *)GetProcAddress(h, "tcc_run");
    mark("getprocaddress:done");
    check(p_ping && p_init && p_main && p_new && p_del && p_compile && p_outtype && p_reloc && p_run,
          "exports resolve: tinyc_ping, tinyc_init, tinyc_main, tcc_* API");
    if (failures) return 1;

    /* The exception handler is armed only across the ping + init
     * window: those two calls are the ones under diagnosis, and
     * exceptions elsewhere keep their normal handling. */
    g_veh_armed = 1;
    mark("tinyc_ping:begin");
    check(p_ping() == 0, "tinyc_ping() returns 0");
    mark("tinyc_ping:done");

    mark("tinyc_init:begin");
    check(p_init() == 0, "tinyc_init() returns 0");
    g_veh_armed = 0;
    mark("tinyc_init:done");

    {
        char a0[] = "tinyc", a1[] = "--version";
        char *args[] = { a0, a1, NULL };
        mark("tinyc_main_version:begin");
        check(p_main(2, args) == 0, "tinyc_main --version returns 0");
        mark("tinyc_main_version:done");
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
        mark("tinyc_main_compile:begin");
        check(p_main(4, args) == 0, "tinyc_main compiles hello.c to win_hello.exe");
        mark("tinyc_main_compile:done");
        mark("hello_run:begin");
        int rc = system("win_hello.exe > win_hello.out 2>&1");
        mark("hello_run:done");
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
        mark("tcc_api:begin");
        void *s = p_new();
        check(s != NULL, "tcc_new returns a state");
        if (s) {
            p_outtype(s, 0);
            int rc = p_compile(s, "int main(void) { return 42; }");
            check(rc == 0, "tcc_compile_string succeeds");
            if (rc == 0) {
                mark("tcc_relocate:begin");
                check(p_reloc(s, (void *)(intptr_t)-1) == 0, "tcc_relocate succeeds");
                mark("tcc_relocate:done");
                mark("tcc_run:begin");
                check(p_run(s, 0, NULL) == 42, "tcc_run returns the program's 42");
                mark("tcc_run:done");
            }
            p_del(s);
        }
        mark("tcc_api:done");
    }

    mark("done");
    printf(failures ? "WIN HOST TEST: FAIL (%d)\n" : "WIN HOST TEST: PASS\n", failures);
    return failures ? 1 : 0;
}
