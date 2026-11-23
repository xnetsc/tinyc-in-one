/*
 * tinyc_win.c - Windows library-mode initialization entry.
 *
 * The Windows dual-form file is an EXE with an export table. A host
 * loads it with LoadLibrary, which does not run DllMain for an EXE
 * module, so the initialization a DLL would do on load is instead an
 * explicit exported call: the host calls tinyc_init() once after
 * loading, before using tinyc_main or the tcc_* API (design doc 3.2).
 *
 * The kernel proper needs no global init (state is per-TCCState and
 * the IO layer falls back to stdio lazily); tinyc_init makes that
 * default explicit: it installs the stdio TCCIO as the current IO,
 * the same default the command-line entry uses, and returns 0.
 *
 * Compiled only into the Windows host variant; a no-op translation
 * unit everywhere else.
 */
#include "tinyc_io.h"

#ifdef _WIN32
int tinyc_init(void)
{
    tinyc_set_io(tcc_io_stdio());
    return 0;
}
#endif
