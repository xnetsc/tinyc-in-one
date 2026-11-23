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
 * The deferred initialization is tinyc_init's job: a host that
 * loads the EXE calls tinyc_init() once before tinyc_main or the
 * tcc_* API (design doc 3.2). On the library path tinyc_init runs
 * the module-initialization sequence both CRT entry paths run, in
 * the same order (crtexe.c's __tmainCRTStartup for an EXE,
 * crtdll.c's _CRT_INIT for a DLL):
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
 * working directory, written with plain Win32 file calls (no CRT
 * stdio - stdio is part of what is being initialized). If the
 * process dies inside tinyc_init, the last marker in the file
 * names the step that was in flight.
 *
 * Compiled only into the Windows host variant; a no-op translation
 * unit everywhere else.
 */
#include "tinyc_io.h"

#ifdef _WIN32
#include <windows.h>

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

static int is_main_module(void)
{
    return GetModuleHandleA(NULL) == (HMODULE)&__ImageBase;
}

static void tinyc_init_mark(const char *step_name)
{
    HANDLE h = CreateFileA("tinyc_init.markers", FILE_APPEND_DATA,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD written;
    int len = 0;
    if (h == INVALID_HANDLE_VALUE)
        return;
    while (step_name[len])
        len++;
    WriteFile(h, step_name, (DWORD)len, &written, NULL);
    WriteFile(h, "\n", 1, &written, NULL);
    CloseHandle(h);
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
        tinyc_init_mark("tinyc_init:enter");
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
    tinyc_set_io(tcc_io_stdio());
    return 0;
}
#endif
