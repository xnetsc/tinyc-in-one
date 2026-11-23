/*
 * tinyc_win.c - Windows entry-point dispatch and library-mode
 * initialization for the dual-form EXE.
 *
 * The Windows dual-form file is an EXE with an export table, and its
 * PE entry point is tinyc_win_entry below (the link selects it with
 * -Wl,-e,tinyc_win_entry), not the CRT's mainCRTStartup. The
 * distinction matters because the loader calls the entry point in
 * two incompatible situations:
 *
 *   - Process start: the image is the process's main module. The
 *     entry forwards to mainCRTStartup, which runs the full CRT
 *     startup (runtime pseudo-relocations, TLS init, global
 *     constructors), calls main, and tears the process down.
 *
 *   - LoadLibrary: the loader invokes the same entry point with the
 *     DllMain convention (DLL_PROCESS_ATTACH). mainCRTStartup is a
 *     process-entry routine: letting it run here executes the CLI
 *     on the *host's* command line inside the load and ends in the
 *     CRT's process teardown (_cexit / exit), corrupting or killing
 *     the host. So in this situation the entry returns TRUE at
 *     once, with no side effects; the module is mapped and its
 *     exports are usable, but its static CRT state is still
 *     uninitialized.
 *
 * The entry decides which situation it is in without calling
 * anything: it compares the process image base in the PEB against
 * this image's base. (The earlier form called GetModuleHandleA
 * through the import address table; see below for why no code in
 * this file may do that before tinyc_init has run.)
 *
 * Import binding is the second half of the problem. For an EXE
 * loaded with LoadLibrary, the loader's backfill of this image's
 * import address table is not reliable: on the Windows CI runner
 * the entry point's own GetModuleHandleA slot had been filled by
 * the time the entry ran, but the CreateFileA slot still held its
 * file value - the RVA of its hint/name entry - when tinyc_ping
 * first called through it, and the call jumped to that RVA as an
 * absolute address. So the library path cannot use the IAT at all
 * until it has bound it itself. tinyc_init therefore starts with a
 * self-binding pass that makes no external call:
 *
 *   1. Find kernel32's base by walking the loader's module list
 *      from the PEB (pure pointer arithmetic and the string
 *      compares below);
 *   2. Resolve LoadLibraryA and GetProcAddress from kernel32's
 *      export table (following export forwarders into kernelbase
 *      the same way, by hand);
 *   3. Walk this image's import directory and fill every first-
 *      thunk slot with the address those two functions return,
 *      exactly as the loader would. A slot the loader already
 *      filled keeps its value: it no longer equals its lookup-table
 *      entry, and the pass leaves it alone, so the pass is
 *      idempotent.
 *
 * Only after the binding does tinyc_init run the deferred CRT
 * initialization - the module-initialization sequence both CRT
 * entry paths run, in the same order (crtexe.c's
 * __tmainCRTStartup for an EXE, crtdll.c's _CRT_INIT for a DLL):
 *
 *   1. _pei386_runtime_relocator - fixes up the image's data
 *      imports from the CRT DLLs;
 *   2. _initterm over the C initializers in the .CRT$XI range
 *      (this image's range holds pre_c_init: app type,
 *      _fmode/_commode, _setargv, the user math error handler);
 *   3. _initterm over the C++ initializers in the .CRT$XC range;
 *   4. __main - the GCC global-constructor walk.
 *
 * (The UCRT entry paths walk the .CRT$XI range with _initterm_e;
 * this image links the msvcrt CRT, whose import library provides
 * _initterm only - the form the msvcrt-target startup code itself
 * uses for both ranges.)
 *
 * The process-entry-only steps around that sequence (command-line
 * and environment setup, argv duplication, the startup state
 * machine, process teardown) do not apply to a module being used
 * as a library and are not run. stdio and malloc need nothing:
 * they live in the CRT DLL (msvcrt.dll), which was initialized
 * when the host process started.
 *
 * Diagnostics: on the library path each step is bracketed by a
 * marker line appended to tinyc_init.markers in the process's
 * working directory. The markers are written through the manually
 * resolved CreateFileA / WriteFile / CloseHandle pointers from
 * step 2 above, never through the IAT, so they work from the
 * first line of tinyc_init - before the binding included. If the
 * process dies inside tinyc_init, the last marker in the file
 * names the step that was in flight; the first marker after the
 * (pointer-written) entry marker is bind:done, the point where
 * the image's imports are bound and ordinary calls become safe.
 *
 * Compiled only into the Windows host variant; a no-op translation
 * unit everywhere else.
 */
#include "tinyc_io.h"

#ifdef _WIN32
#include <windows.h>
#include <stdint.h>

/* Provided by the MinGW runtime: the process-entry CRT startup
 * (crt2.o), the runtime pseudo-relocation pass and the global
 * constructor walk (libmingw32). */
extern int mainCRTStartup(void);
extern void _pei386_runtime_relocator(void);
extern void __main(void);

/* The .CRT initializer ranges and the CRT's table walker, the
 * same declarations crtexe.c and crtdll.c use. _initterm is
 * exported by the CRT DLL. The .CRT$XI entries return int; the
 * msvcrt _initterm walks both ranges in the _PVFV form and the
 * return values are not consulted. */
typedef void (__cdecl *_PVFV)(void);
extern _PVFV __xi_a[], __xi_z[];
extern _PVFV __xc_a[], __xc_z[];
extern void __cdecl _initterm(_PVFV *first, _PVFV *last);

extern IMAGE_DOS_HEADER __ImageBase;

/* ------------------------------------------------------------------ */
/* Loader structures, read directly: the self-binding pass and the  */
/* entry point's main-module test run before any imported function  */
/* of this image may be called, so they work from the PEB and the   */
/* images' own headers only - no Win32 calls, no CRT calls.         */
/* ------------------------------------------------------------------ */

/* PEB field offsets (x64): ImageBaseAddress at 0x10, Ldr at 0x18.
 * PEB_LDR_DATA: the InLoadOrderModuleList head is at Ldr + 0x10,
 * and a load-order link pointer is the LDR_DATA_TABLE_ENTRY itself
 * (the links are the entry's first field). */
struct tinyc_unicode_string {
    unsigned short length;          /* in bytes */
    unsigned short maximum_length;
    wchar_t *buffer;
};

struct tinyc_ldr_entry {
    void *load_order_links[2];      /* 0x00 */
    void *memory_order_links[2];    /* 0x10 */
    void *init_order_links[2];      /* 0x20 */
    void *dll_base;                 /* 0x30 */
    void *entry_point;              /* 0x38 */
    unsigned long size_of_image;    /* 0x40 */
    unsigned long reserved;         /* 0x44 */
    struct tinyc_unicode_string full_name;  /* 0x48 */
    struct tinyc_unicode_string base_name;  /* 0x58 */
};

static unsigned char *tinyc_peb(void)
{
    unsigned char *peb;
    __asm__ volatile ("mov %%gs:0x60, %0" : "=r" (peb));
    return peb;
}

static int is_main_module(void)
{
    void *process_image = *(void **)(tinyc_peb() + 0x10);
    return process_image == (void *)&__ImageBase;
}

/* Own string helpers: the CRT's are on the far side of the IAT. */
static int tinyc_str_eq(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static unsigned long tinyc_str_len(const char *s)
{
    unsigned long n = 0;
    while (s[n])
        n++;
    return n;
}

static char tinyc_fold(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

/* Does the ASCII name want[0..want_len) name this module? The
 * entry's base name is UTF-16; the comparison is case-insensitive,
 * and a trailing ".dll" on either side is optional (a forwarder
 * string names its DLL with or without the extension). */
static int tinyc_name_matches(const wchar_t *buf, unsigned long n,
                              const char *want, unsigned long want_len)
{
    unsigned long i = 0;
    while (i < want_len) {
        char wc, ac;
        if (i >= n) {
            /* Module name exhausted: match only if the rest of
             * want is exactly ".dll". */
            return want_len - i == 4
                && tinyc_fold(want[i]) == '.'
                && tinyc_fold(want[i + 1]) == 'd'
                && tinyc_fold(want[i + 2]) == 'l'
                && tinyc_fold(want[i + 3]) == 'l';
        }
        if (buf[i] > 127)
            return 0;
        wc = tinyc_fold((char)buf[i]);
        ac = tinyc_fold(want[i]);
        if (wc != ac)
            return 0;
        i++;
    }
    if (i == n)
        return 1;
    /* want exhausted: match only if the rest of the name is ".dll". */
    if (n - i == 4) {
        const char *dll = ".dll";
        unsigned long j;
        for (j = 0; j < 4; j++) {
            if (buf[i + j] > 127 || tinyc_fold((char)buf[i + j]) != dll[j])
                return 0;
        }
        return 1;
    }
    return 0;
}

static void *tinyc_find_module_n(const char *name, unsigned long name_len)
{
    unsigned char *ldr = *(unsigned char **)(tinyc_peb() + 0x18);
    void **head = (void **)(ldr + 0x10);
    void **link = (void **)head[0];
    while (link != head) {
        struct tinyc_ldr_entry *e = (struct tinyc_ldr_entry *)link;
        if (tinyc_name_matches(e->base_name.buffer,
                               e->base_name.length / sizeof(wchar_t),
                               name, name_len))
            return e->dll_base;
        link = (void **)link[0];
    }
    return NULL;
}

static void *tinyc_find_module(const char *name)
{
    return tinyc_find_module_n(name, tinyc_str_len(name));
}

/* Export resolution within one loaded image, by name or ordinal.
 * An export whose RVA falls inside the export directory is a
 * forwarder string ("DLL.export" or "DLL.#ordinal"); it is
 * followed by hand, through the module list, with a depth cap. */
static IMAGE_NT_HEADERS *tinyc_nt_headers(const unsigned char *base)
{
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;
    return (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
}

static void *tinyc_export_addr(void *mod_base, const char *name, int depth);
static void *tinyc_export_addr_ordinal(void *mod_base,
                                       unsigned long ordinal, int depth);

static void *tinyc_resolve_rva(unsigned char *base, IMAGE_NT_HEADERS *nt,
                               DWORD rva, int depth)
{
    IMAGE_DATA_DIRECTORY *dir =
        &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    const char *fwd, *dot = NULL, *p;
    void *target;
    if (rva < dir->VirtualAddress || rva >= dir->VirtualAddress + dir->Size)
        return base + rva;
    if (depth > 8)
        return NULL;
    fwd = (const char *)(base + rva);
    for (p = fwd; *p; p++)
        if (*p == '.')
            dot = p;
    if (!dot || dot == fwd)
        return NULL;
    target = tinyc_find_module_n(fwd, (unsigned long)(dot - fwd));
    if (!target)
        return NULL;
    if (dot[1] == '#') {
        unsigned long ordinal = 0;
        for (p = dot + 2; *p >= '0' && *p <= '9'; p++)
            ordinal = ordinal * 10 + (unsigned long)(*p - '0');
        return tinyc_export_addr_ordinal(target, ordinal, depth + 1);
    }
    return tinyc_export_addr(target, dot + 1, depth + 1);
}

static void *tinyc_export_addr(void *mod_base, const char *name, int depth)
{
    unsigned char *base = (unsigned char *)mod_base;
    IMAGE_NT_HEADERS *nt = tinyc_nt_headers(base);
    IMAGE_EXPORT_DIRECTORY *exp;
    DWORD *names, *funcs;
    WORD *ords;
    DWORD i;
    if (!nt)
        return NULL;
    if (!nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
            .VirtualAddress)
        return NULL;
    exp = (IMAGE_EXPORT_DIRECTORY *)(base + nt->OptionalHeader
        .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    names = (DWORD *)(base + exp->AddressOfNames);
    ords = (WORD *)(base + exp->AddressOfNameOrdinals);
    funcs = (DWORD *)(base + exp->AddressOfFunctions);
    for (i = 0; i < exp->NumberOfNames; i++) {
        if (tinyc_str_eq((const char *)(base + names[i]), name))
            return tinyc_resolve_rva(base, nt, funcs[ords[i]], depth);
    }
    return NULL;
}

static void *tinyc_export_addr_ordinal(void *mod_base,
                                       unsigned long ordinal, int depth)
{
    unsigned char *base = (unsigned char *)mod_base;
    IMAGE_NT_HEADERS *nt = tinyc_nt_headers(base);
    IMAGE_EXPORT_DIRECTORY *exp;
    DWORD *funcs;
    unsigned long idx;
    if (!nt)
        return NULL;
    if (!nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
            .VirtualAddress)
        return NULL;
    exp = (IMAGE_EXPORT_DIRECTORY *)(base + nt->OptionalHeader
        .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    if (ordinal < exp->Base)
        return NULL;
    idx = ordinal - exp->Base;
    if (idx >= exp->NumberOfFunctions)
        return NULL;
    funcs = (DWORD *)(base + exp->AddressOfFunctions);
    return tinyc_resolve_rva(base, nt, funcs[idx], depth);
}

/* The kernel32 entry points this file needs, resolved by hand
 * above. Everything in this file calls these pointers, never the
 * IAT, until the binding pass has filled the IAT - and the marker
 * writer keeps using them afterwards, so the diagnostics do not
 * depend on the state of the binding at all. */
typedef HMODULE (WINAPI *tinyc_load_library_a_fn)(const char *);
typedef FARPROC (WINAPI *tinyc_get_proc_address_fn)(HMODULE, const char *);
typedef HANDLE (WINAPI *tinyc_create_file_a_fn)(const char *, DWORD, DWORD,
                                                SECURITY_ATTRIBUTES *,
                                                DWORD, DWORD, HANDLE);
typedef BOOL (WINAPI *tinyc_write_file_fn)(HANDLE, const void *, DWORD,
                                           DWORD *, OVERLAPPED *);
typedef BOOL (WINAPI *tinyc_close_handle_fn)(HANDLE);
typedef BOOL (WINAPI *tinyc_virtual_protect_fn)(void *, SIZE_T, DWORD,
                                                 DWORD *);

static tinyc_load_library_a_fn p_load_library_a;
static tinyc_get_proc_address_fn p_get_proc_address;
static tinyc_create_file_a_fn p_create_file_a;
static tinyc_write_file_fn p_write_file;
static tinyc_close_handle_fn p_close_handle;
static tinyc_virtual_protect_fn p_virtual_protect;

static int tinyc_resolve_core(void)
{
    void *k32;
    if (p_load_library_a && p_get_proc_address)
        return 1;
    k32 = tinyc_find_module("kernel32.dll");
    if (!k32)
        k32 = tinyc_find_module("kernelbase.dll");
    if (!k32)
        return 0;
    p_load_library_a = (tinyc_load_library_a_fn)
        tinyc_export_addr(k32, "LoadLibraryA", 0);
    p_get_proc_address = (tinyc_get_proc_address_fn)
        tinyc_export_addr(k32, "GetProcAddress", 0);
    p_create_file_a = (tinyc_create_file_a_fn)
        tinyc_export_addr(k32, "CreateFileA", 0);
    p_write_file = (tinyc_write_file_fn)
        tinyc_export_addr(k32, "WriteFile", 0);
    p_close_handle = (tinyc_close_handle_fn)
        tinyc_export_addr(k32, "CloseHandle", 0);
    p_virtual_protect = (tinyc_virtual_protect_fn)
        tinyc_export_addr(k32, "VirtualProtect", 0);
    return p_load_library_a && p_get_proc_address
        && p_create_file_a && p_write_file && p_close_handle;
}

/* Fill this image's import address table by hand: for each import
 * descriptor, load the DLL through the resolved LoadLibraryA and
 * write each first-thunk slot from the resolved GetProcAddress,
 * by name or by ordinal. Idempotent: a slot whose content no
 * longer equals its lookup-table entry was already filled (by
 * the loader) and is left untouched. The slot range of each
 * descriptor is made writable first - the .idata section is not
 * writable in this image - and its protection is restored after. */
static void tinyc_bind_imports(void)
{
    unsigned char *base = (unsigned char *)&__ImageBase;
    IMAGE_NT_HEADERS *nt = tinyc_nt_headers(base);
    IMAGE_DATA_DIRECTORY *dir;
    IMAGE_IMPORT_DESCRIPTOR *desc;
    if (!nt)
        return;
    dir = &nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir->VirtualAddress)
        return;
    for (desc = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir->VirtualAddress);
         desc->Name; desc++) {
        HMODULE dll = p_load_library_a((const char *)(base + desc->Name));
        IMAGE_THUNK_DATA *ilt, *iat;
        unsigned long count = 0, i;
        DWORD old_protect = 0;
        int unprotect = 0;
        if (!dll)
            continue;
        ilt = (IMAGE_THUNK_DATA *)(base + (desc->OriginalFirstThunk
                                          ? desc->OriginalFirstThunk
                                          : desc->FirstThunk));
        iat = (IMAGE_THUNK_DATA *)(base + desc->FirstThunk);
        while (ilt[count].u1.AddressOfData)
            count++;
        if (!count)
            continue;
        if (p_virtual_protect)
            unprotect = p_virtual_protect(
                &iat[0], count * sizeof(IMAGE_THUNK_DATA),
                PAGE_READWRITE, &old_protect) != 0;
        for (i = 0; i < count; i++) {
            FARPROC proc;
            if (iat[i].u1.Function != ilt[i].u1.AddressOfData)
                continue;   /* already filled: leave it alone */
            if (IMAGE_SNAP_BY_ORDINAL(ilt[i].u1.Ordinal)) {
                proc = p_get_proc_address(
                    dll, (const char *)(uintptr_t)
                        IMAGE_ORDINAL(ilt[i].u1.Ordinal));
            } else if (ilt[i].u1.AddressOfData
                       < nt->OptionalHeader.SizeOfImage) {
                IMAGE_IMPORT_BY_NAME *by_name = (IMAGE_IMPORT_BY_NAME *)
                    (base + ilt[i].u1.AddressOfData);
                proc = p_get_proc_address(dll,
                                          (const char *)by_name->Name);
            } else {
                continue;   /* not an RVA: cannot be an unfilled slot */
            }
            if (proc)
                iat[i].u1.Function = (ULONGLONG)(uintptr_t)proc;
        }
        if (unprotect) {
            DWORD dummy;
            p_virtual_protect(&iat[0], count * sizeof(IMAGE_THUNK_DATA),
                              old_protect, &dummy);
        }
    }
}

static void tinyc_init_mark(const char *step_name)
{
    HANDLE h;
    DWORD written;
    int len = 0;
    if (!p_create_file_a && !tinyc_resolve_core())
        return;
    h = p_create_file_a("tinyc_init.markers", FILE_APPEND_DATA,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    /* Volatile walk, not a plain loop: the compiler must not turn
     * this into a strlen call - strlen is an imported function and
     * this writer runs before the import binding. */
    while (((volatile char *)step_name)[len])
        len++;
    p_write_file(h, step_name, (DWORD)len, &written, NULL);
    p_write_file(h, "\n", 1, &written, NULL);
    p_close_handle(h);
}

/* Diagnostic export for the library path: the simplest function in
 * this module. It appends one line to tinyc_init.markers through
 * the manually resolved file pointers above and returns, touching
 * no CRT state and no globals. The Windows consumer diagnostic
 * (tests/consumer/win_host_test.c) calls it after tinyc_init: the
 * binding is done by then, and a ping that returns with its line
 * in the markers file proves the whole path - transfer into the
 * image, the resolved-pointer file write - end to end. */
int tinyc_ping(void)
{
    tinyc_init_mark("tinyc_ping");
    return 0;
}

/* Diagnostic export with an empty body: no calls, no globals, no
 * CRT. A host calls it before tinyc_init to split the call path
 * itself from everything a called function might do: if calling
 * tinyc_noop dies, the fault is in the transfer into this image
 * (the resolved address or the state of its code pages); if
 * tinyc_noop returns, the transfer works and any later fault is
 * inside the called function. */
int tinyc_noop(void)
{
    return 0;
}

int tinyc_win_entry(void *hinst, unsigned long reason, void *reserved)
{
    (void)hinst; (void)reason; (void)reserved;
    if (!is_main_module()) {
        /* LoadLibrary (DLL_PROCESS_ATTACH convention): succeed
         * without running anything. Initialization is deferred to
         * tinyc_init, which the host calls explicitly. */
        return 1;
    }
    /* Process entry: run the CRT startup; it calls main and does
     * not return until the process is on its way out. */
    return mainCRTStartup();
}

int tinyc_init(void)
{
    static int crt_ready;
    if (!is_main_module() && !crt_ready) {
        /* Self-binding first. Nothing above this point - and
         * nothing in the resolve or the binding itself - calls
         * through this image's IAT: the core pointers come from
         * walking the PEB and kernel32's export table by hand.
         * The entry marker is written through those pointers;
         * bind:done is the first marker past the binding, the
         * point where ordinary IAT calls become safe. */
        if (tinyc_resolve_core()) {
            tinyc_init_mark("tinyc_init:enter");
            tinyc_init_mark("bind:begin");
            tinyc_bind_imports();
            tinyc_init_mark("bind:done");
            tinyc_init_mark("relocator:begin");
            _pei386_runtime_relocator();
            tinyc_init_mark("relocator:done");
            tinyc_init_mark("initterm_xi:begin");
            _initterm(__xi_a, __xi_z);
            tinyc_init_mark("initterm_xi:done");
            tinyc_init_mark("initterm_xc:begin");
            _initterm(__xc_a, __xc_z);
            tinyc_init_mark("initterm_xc:done");
            tinyc_init_mark("main:begin");
            __main();
            tinyc_init_mark("main:done");
            crt_ready = 1;
            tinyc_init_mark("tinyc_init:initialized");
        }
    }
    tinyc_set_io(tcc_io_stdio());
    return 0;
}
#endif
