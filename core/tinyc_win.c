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
 * the two startup steps the CRT entry would have run before any
 * user code - _pei386_runtime_relocator (fixes up the image's data
 * imports from the CRT DLLs) and __main (the global-constructor
 * walk) - the same pair a MinGW DLL's DllMainCRTStartup runs on
 * DLL_PROCESS_ATTACH. stdio and malloc need nothing: they live in
 * the CRT DLL (msvcrt.dll), which was initialized when the host
 * process started.
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

extern IMAGE_DOS_HEADER __ImageBase;

static int is_main_module(void)
{
    return GetModuleHandleA(NULL) == (HMODULE)&__ImageBase;
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
        _pei386_runtime_relocator();
        __main();
        crt_ready = 1;
    }
    tinyc_set_io(tcc_io_stdio());
    return 0;
}
#endif
