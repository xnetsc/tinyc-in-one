#!/usr/bin/env python3
"""Apply the tinyc patches to a pristine tinycc copy in src/.
Each replacement must match exactly once (or a stated count);
a mismatch aborts the build instead of silently skipping a patch."""
import pathlib, sys

SRC = (pathlib.Path(sys.argv[1]).resolve() if len(sys.argv) > 1
       else pathlib.Path(__file__).resolve().parent.parent / "src")

def patch(fname, old, new, count=1):
    p = SRC / fname
    text = p.read_text()
    found = text.count(old)
    if found != count:
        print(f"PATCH FAIL {fname}: expected {count} occurrence(s), found {found}:\n---\n{old}\n---")
        sys.exit(1)
    p.write_text(text.replace(old, new))
    print(f"patched {fname}: {old.splitlines()[0][:60]}")

# ---- tcc.h: pull in the IO API, add the opt-exit flag + IO fields ---
patch("tcc.h",
      '#include "config.h"\n',
      '#include "config.h"\n#include "tinyc_io.h"\n')
patch("tcc.h",
      "    int output_type;\n",
      "    int output_type;\n    int opt_exit_requested; /* tinyc: -dumpmachine/-dumpversion set this instead of calling exit(0) */\n    TCCIO *io; /* tinyc: per-state IO for compiler output; NULL = stdio */\n")

# ---- libtcc.h: declare the per-state IO accessors --------------------
patch("libtcc.h",
      "typedef struct TCCState TCCState;\n",
      "typedef struct TCCState TCCState;\ntypedef struct TCCIO TCCIO;\n")
patch("libtcc.h",
      "LIBTCCAPI void tcc_set_error_func(TCCState *s, void *error_opaque, TCCErrorFunc *error_func);\n",
      """LIBTCCAPI void tcc_set_error_func(TCCState *s, void *error_opaque, TCCErrorFunc *error_func);

/* tinyc: install a TCCIO (tinyc_io.h) on a state; NULL restores the
 * stdio default. All of the state's compiler output - diagnostics
 * without an error callback, -E output, run stdio - goes through it. */
LIBTCCAPI void tcc_set_io(TCCState *s, TCCIO *io);
/* tinyc: the state's IO, or the stdio singleton when none (or a NULL
 * state) is given. Never returns NULL. */
LIBTCCAPI TCCIO *tcc_get_io(TCCState *s);
""")

# ---- libtcc.c: implement the per-state IO accessors ------------------
patch("libtcc.c",
      """LIBTCCAPI void tcc_set_error_func(TCCState *s, void *error_opaque, TCCErrorFunc *error_func)
{
    s->error_opaque = error_opaque;
    s->error_func = error_func;
}
""",
      """LIBTCCAPI void tcc_set_error_func(TCCState *s, void *error_opaque, TCCErrorFunc *error_func)
{
    s->error_opaque = error_opaque;
    s->error_func = error_func;
}

LIBTCCAPI void tcc_set_io(TCCState *s, TCCIO *io)
{
    s->io = io;
}

LIBTCCAPI TCCIO *tcc_get_io(TCCState *s)
{
    return (s && s->io) ? s->io : tcc_io_stdio();
}
""")

# ---- libtcc.c: default error output goes through the state's IO ----
patch("libtcc.c",
      """        /* default case: stderr */
        if (s1 && s1->output_type == TCC_OUTPUT_PREPROCESS && s1->ppfp == stdout)
            printf("\\n"); /* print a newline during tcc -E */
        fflush(stdout); /* flush -v output */
        fprintf(stderr, "%s\\n", (char*)cs.data);
        fflush(stderr); /* print error/warning now (win32) */""",
      """        /* default case: the state's IO err channel (real stderr by default) */
        if (s1 && s1->output_type == TCC_OUTPUT_PREPROCESS && s1->ppfp == stdout)
            tinyc_io_out_str(tcc_get_io(s1), "\\n"); /* print a newline during tcc -E */
        tinyc_io_flush(tcc_get_io(s1)); /* flush -v output */
        tinyc_io_err_line(tcc_get_io(s1), (char*)cs.data);
        tinyc_io_flush(tcc_get_io(s1)); /* print error/warning now */""")

# ---- libtcc.c: -dumpmachine/-dumpversion must not exit the host -----
patch("libtcc.c",
      """            printf("%s\\n", dumpmachine_str);
            exit(0);""",
      """            tinyc_io_out_fmt(tcc_get_io(s), "%s\\n", dumpmachine_str);
            s->opt_exit_requested = 1;
            return 0;""")
patch("libtcc.c",
      """            printf ("%s\\n", TCC_VERSION);
            exit(0);""",
      """            tinyc_io_out_fmt(tcc_get_io(s), "%s\\n", TCC_VERSION);
            s->opt_exit_requested = 1;
            return 0;""")

# ---- libtcc.c: verbose file-open trace goes through the state's IO --
patch("libtcc.c",
      """    if ((s1->verbose == 2 && fd >= 0) || s1->verbose == 3)
        printf("%s %*s%s\\n", fd < 0 ? "nf":"->",
               (int)(s1->include_stack_ptr - s1->include_stack), "", filename);""",
      """    if ((s1->verbose == 2 && fd >= 0) || s1->verbose == 3)
        tinyc_io_out_fmt(tcc_get_io(s1), "%s %*s%s\\n", fd < 0 ? "nf":"->",
               (int)(s1->include_stack_ptr - s1->include_stack), "", filename);""")

# ---- libtcc.c: -bench stats go through the state's IO ---------------
patch("libtcc.c",
      """    fprintf(stderr, "# %d idents, %d lines, %u bytes\\n"
                    "# %0.3f s, %u lines/s, %0.1f MB/s\\n",
           total_idents, total_lines, total_bytes,
           (double)total_time/1000,
           (unsigned)total_lines*1000/total_time,
           (double)total_bytes/1000/total_time);
    fprintf(stderr, "# text %u, data.rw %u, data.ro %u, bss %u bytes\\n",
           s1->total_output[0],
           s1->total_output[1],
           s1->total_output[2],
           s1->total_output[3]
           );""",
      """    tinyc_io_err_fmt(tcc_get_io(s1), "# %d idents, %d lines, %u bytes\\n"
                    "# %0.3f s, %u lines/s, %0.1f MB/s\\n",
           total_idents, total_lines, total_bytes,
           (double)total_time/1000,
           (unsigned)total_lines*1000/total_time,
           (double)total_bytes/1000/total_time);
    tinyc_io_err_fmt(tcc_get_io(s1), "# text %u, data.rw %u, data.ro %u, bss %u bytes\\n",
           s1->total_output[0],
           s1->total_output[1],
           s1->total_output[2],
           s1->total_output[3]
           );""")

# ---- libtcc.c: tccdir is this image's directory, not the host's ----
# config_tccdir_w32 asks GetModuleFileNameA(tcc_module). Upstream
# records tcc_module in a DllMain that exists only in the
# LIBTCC_AS_DLL build; in this build it is the NULL macro, which
# names the process image. Loaded as a library, the process is the
# host, so {B} - and with it the include path and the libtcc1.a /
# runmain.o lookup - lands in the host's directory. Resolve the
# handle of the image containing this code instead; in process mode
# that is the main module, so the resulting path is unchanged. The
# macro stays a bare definition (non-Windows builds never expand
# it); the function is emitted only in the CONFIG_TCCDIR-less
# region, which is the win32 configuration.
patch("libtcc.c",
      "#define tcc_module NULL /* NULL means executable itself */",
      "#define tcc_module tinyc_self_module()")
patch("libtcc.c",
      """#ifndef CONFIG_TCCDIR
/* on win32, we suppose the lib and includes are at the location of 'tcc.exe' */""",
      """#ifndef CONFIG_TCCDIR
/* tinyc: the handle of the image containing this code */
static HMODULE tinyc_self_module(void)
{
    HMODULE h = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)(void *)&tinyc_self_module, &h);
    return h;
}
/* on win32, we suppose the lib and includes are at the location of 'tcc.exe' */""")

# ---- libtcc.c: default tccdir from this binary's own location ------
# configure bakes an absolute CONFIG_TCCDIR (the build machine's dist
# path) into config.h; a dist copied or extracted anywhere else finds
# none of its own files through it (builtin headers, crt objects and
# libtcc1.a all hang off {B}). The default therefore resolves at
# runtime from the binary itself: the executable's own path in
# process form (/proc/self/exe on Linux/Android hosts,
# _NSGetExecutablePath on macOS hosts), the containing file's path
# from dladdr in library form. The tccdir is <own dir>/../lib/tcc
# (the win32-target layout <own dir>/../lib/tcc/win32 is tried as
# well); a candidate is adopted only if it carries the layout
# markers (the builtin headers, or a libtcc1.a in either archive
# spot the layouts use). A binary that does not sit in a dist - an
# in-tree build, the Android AAR .so, whose host sets the path
# through the API - falls back to the baked CONFIG_TCCDIR. Windows
# keeps the upstream runtime resolution (config_tccdir_w32 over
# tinyc_self_module(), which already names this image in both
# forms) and Emscripten keeps the baked path; both take the macro
# branch. An explicit -B or tcc_set_lib_path() still overrides the
# default afterwards, as before: only the tcc_new() default changes.
# Allocator discipline inside the resolver: libtcc.c poisons the
# bare free/realloc names below its allocator section, so own
# buffers go through tcc_malloc/tcc_free and realpath() results
# through libc_free().
patch("libtcc.c",
      "LIBTCCAPI TCCState *tcc_new(void)\n",
      """/* tinyc: the default tccdir, resolved from this binary's own
 * location (see the note in patches/apply_patches.py). */
#if defined _WIN32 || defined __EMSCRIPTEN__ || defined CONFIG_TCC_STATIC
# define tinyc_default_tccdir() CONFIG_TCCDIR
#else
# include <limits.h>
# include <sys/stat.h>
# ifdef __APPLE__
#  include <mach-o/dyld.h>
# endif

static int tinyc_layout_file(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

/* The process executable's canonical path, or NULL. */
static const char *tinyc_exe_path(char *buf, size_t bufsz)
{
# ifdef __APPLE__
    uint32_t size = (uint32_t)bufsz;
    char *rp;
    if (_NSGetExecutablePath(buf, &size) == -1) {
        /* buf too small: size now holds the required size */
        char *tmp = (char *)tcc_malloc(size);
        if (!tmp)
            return NULL;
        if (_NSGetExecutablePath(tmp, &size) != 0) {
            tcc_free(tmp);
            return NULL;
        }
        rp = realpath(tmp, NULL);
        tcc_free(tmp);
        if (!rp || strlen(rp) + 1 > bufsz) {
            libc_free(rp);
            return NULL;
        }
        strcpy(buf, rp);
        libc_free(rp);
        return buf;
    }
    rp = realpath(buf, NULL);
    if (rp) {
        if (strlen(rp) + 1 <= bufsz)
            strcpy(buf, rp);
        libc_free(rp);
    }
    return buf;
# else
    ssize_t n = readlink("/proc/self/exe", buf, bufsz - 1);
    if (n <= 0)
        return NULL;
    buf[n] = 0;
    return buf;
# endif
}

/* Derive the tccdir from one own-file path: the canonical tccdir
 * goes to out and 1 is returned; 0 when the path does not sit in
 * a dist layout. */
static int tinyc_derive_tccdir(const char *own, char *out, size_t outsz)
{
    static const char * const subs[] = { "/../lib/tcc", "/../lib/tcc/win32" };
    char cand[PATH_MAX], marker[PATH_MAX];
    const char *slash = strrchr(own, '/');
    size_t dirlen, i;
    if (!slash)
        return 0;
    dirlen = (size_t)(slash - own);
    for (i = 0; i < sizeof subs / sizeof subs[0]; ++i) {
        char *rp;
        if (dirlen + strlen(subs[i]) + 1 > sizeof cand)
            continue;
        memcpy(cand, own, dirlen);
        strcpy(cand + dirlen, subs[i]);
        snprintf(marker, sizeof marker, "%s/include/stdarg.h", cand);
        if (!tinyc_layout_file(marker)) {
            snprintf(marker, sizeof marker, "%s/libtcc1.a", cand);
            if (!tinyc_layout_file(marker)) {
                snprintf(marker, sizeof marker, "%s/lib/libtcc1.a", cand);
                if (!tinyc_layout_file(marker))
                    continue;
            }
        }
        rp = realpath(cand, NULL);
        if (!rp || strlen(rp) + 1 > outsz) {
            libc_free(rp);
            continue;
        }
        strcpy(out, rp);
        libc_free(rp);
        return 1;
    }
    return 0;
}

static const char *tinyc_default_tccdir(void)
{
    static char resolved[PATH_MAX];
    char exe[PATH_MAX], lib[PATH_MAX];
    const char *exe_p, *lib_p = NULL;
    Dl_info info;
    exe_p = tinyc_exe_path(exe, sizeof exe);
    /* Library form: the file containing this code. When it is not
     * the process executable, this image was loaded as a library
     * and its own location - not the host's - is the base. */
    if (dladdr((const void *)&tinyc_default_tccdir, &info) && info.dli_fname) {
        char *rp = realpath(info.dli_fname, NULL);
        const char *abs = rp ? rp : info.dli_fname;
        if (!exe_p || strcmp(abs, exe_p) != 0) {
            strncpy(lib, abs, sizeof lib - 1);
            lib[sizeof lib - 1] = 0;
            lib_p = lib;
        }
        libc_free(rp);
    }
    if (lib_p && tinyc_derive_tccdir(lib_p, resolved, sizeof resolved))
        return resolved;
    if (exe_p && tinyc_derive_tccdir(exe_p, resolved, sizeof resolved))
        return resolved;
    return CONFIG_TCCDIR;
}
#endif

LIBTCCAPI TCCState *tcc_new(void)
""")
patch("libtcc.c",
      "    tcc_set_lib_path(s, CONFIG_TCCDIR);\n",
      "    tcc_set_lib_path(s, tinyc_default_tccdir());\n")

# ---- Makefile: the IO unit is its own object in every build ---------
patch("Makefile",
      "# native tcc always made from tcc.o and libtcc.[so|a]",
      "LIBTCC_OBJ += $(X)tcc_io.o\n# native tcc always made from tcc.o and libtcc.[so|a]")
# ONE_SOURCE builds (all cross compilers) link only $(X)tcc.o via
# TCC_FILES, so the IO unit must be added there as well.
patch("Makefile",
      "TCC_FILES = $(X)tcc.o\n",
      "TCC_FILES = $(X)tcc.o $(X)tcc_io.o\n")

# ---- arm64-link.c: R_AARCH64_NONE means "no relocation" -------------
# Objects produced by other compilers (e.g. the NDK's
# crtbegin_dynamic.o) may carry R_AARCH64_NONE entries. The x86_64
# backend maps its non-GOT types to NO_GOTPLT_ENTRY; the arm64
# backend has no case for type 0 and errors out in build_got_entries.
patch("arm64-link.c",
      "    switch (reloc_type) {\n\tcase R_AARCH64_PREL32:",
      "    switch (reloc_type) {\n        case R_AARCH64_NONE:\n\tcase R_AARCH64_PREL32:")
patch("arm64-link.c",
      "        case R_AARCH64_RELATIVE:\n#ifdef TCC_TARGET_PE",
      "        case R_AARCH64_NONE:\n            /* no relocation */\n            return;\n        case R_AARCH64_RELATIVE:\n#ifdef TCC_TARGET_PE")

# ---- tcc.c: main becomes a static impl behind the exported entries --
# The impl takes the IO tinyc_main_with_io installs on every state it
# creates (one per pass through the redo loop); tinyc_main passes
# NULL. See the appended entry wrappers at the bottom of this file.
patch("tcc.c",
      "int main(int argc, char **argv)",
      "static int tinyc_main_impl(int argc, char **argv, TCCIO *main_io)")
patch("tcc.c",
      """    s = s1 = tcc_new();
    opt = tcc_parse_args(s, &argc, &argv);
""",
      """    s = s1 = tcc_new();
    if (main_io)
        tcc_set_io(s, main_io);
    opt = tcc_parse_args(s, &argc, &argv);
    if (s->opt_exit_requested) {
        tcc_delete(s);
        return 0;
    }
""")

# ---- tcc.c: help/version/search-dirs go through the state's IO ------
patch("tcc.c", "fputs(help, stdout);", "tinyc_io_out_str(tcc_get_io(s), help);")
patch("tcc.c", "help2: fputs(help2, stdout);", "help2: tinyc_io_out_str(tcc_get_io(s), help2);")
patch("tcc.c", 'printf("%s", version);', "tinyc_io_out_str(tcc_get_io(s), version);")
patch("tcc.c",
      "static void print_dirs(const char *msg, char **paths, int nb_paths)",
      "static void print_dirs(TCCIO *io, const char *msg, char **paths, int nb_paths)")
patch("tcc.c",
      'printf("%s:\\n%s", msg, nb_paths ? "" : "  -\\n");',
      'tinyc_io_out_fmt(io, "%s:\\n%s", msg, nb_paths ? "" : "  -\\n");')
patch("tcc.c",
      'printf("  %s\\n", paths[i]);',
      'tinyc_io_out_fmt(io, "  %s\\n", paths[i]);')
patch("tcc.c",
      'print_dirs("include", s->sysinclude_paths, s->nb_sysinclude_paths);',
      'print_dirs(tcc_get_io(s), "include", s->sysinclude_paths, s->nb_sysinclude_paths);')
patch("tcc.c",
      'print_dirs("libraries", s->library_paths, s->nb_library_paths);',
      'print_dirs(tcc_get_io(s), "libraries", s->library_paths, s->nb_library_paths);')
patch("tcc.c",
      'print_dirs("crt", s->crt_paths, s->nb_crt_paths);',
      'print_dirs(tcc_get_io(s), "crt", s->crt_paths, s->nb_crt_paths);')
patch("tcc.c",
      'printf("install: %s\\n", s->tcc_lib_path);',
      'tinyc_io_out_fmt(tcc_get_io(s), "install: %s\\n", s->tcc_lib_path);')
patch("tcc.c",
      'printf("libtcc1:\\n  %s/%s\\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);',
      'tinyc_io_out_fmt(tcc_get_io(s), "libtcc1:\\n  %s/%s\\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);')
patch("tcc.c",
      'printf("elfinterp:\\n  %s\\n",  s->elfint);',
      'tinyc_io_out_fmt(tcc_get_io(s), "elfinterp:\\n  %s\\n",  s->elfint);')
patch("tcc.c",
      'printf("-> %s\\n", f->name);',
      'tinyc_io_out_fmt(tcc_get_io(s), "-> %s\\n", f->name);')

# ---- tcc.c: the -ar/-impdef/-m tools run under main's state IO -------
patch("tcc.c",
      "ret = tcc_tool_ar(argc, argv);",
      "ret = tcc_tool_ar(argc, argv, tcc_get_io(s));")
patch("tcc.c",
      "ret = tcc_tool_impdef(argc, argv);",
      "ret = tcc_tool_impdef(argc, argv, tcc_get_io(s));")
patch("tcc.c",
      "ret = tcc_tool_cross(argv, opt);",
      "ret = tcc_tool_cross(argv, opt, tcc_get_io(s));")

# ---- libtcc.c: API-form -E output goes through the state's IO ------
# The tcc.c patch below routes -E output for the CLI form, which sets
# s->ppfp itself. In API form nothing replaces the ppfp that tcc_new
# sets to stdout, so preprocessed output bypassed the state's IO
# entirely (and tinyc_io_open_out_stream was never called). Route it
# for the duration of tcc_compile when ppfp still points at stdout:
# the stream writes synchronously into the state's IO (the Windows
# temp-file stream is drained by tinyc_io_flush on the way out).
# The local is assigned before setjmp and never modified inside the
# protected block, so it survives an error longjmp; the tail runs on
# both paths.
patch("libtcc.c",
      """    tcc_enter_state(s1);
    s1->error_set_jmp_enabled = 1;
""",
      """    tcc_enter_state(s1);
    s1->error_set_jmp_enabled = 1;

    /* tinyc: route API-form -E output through the state's IO */
    FILE *tinyc_pp_stream = NULL;
    if (s1->output_type == TCC_OUTPUT_PREPROCESS && s1->ppfp == stdout) {
        tinyc_pp_stream = tinyc_io_open_out_stream(tcc_get_io(s1));
        if (tinyc_pp_stream)
            s1->ppfp = tinyc_pp_stream;
    }
""")
patch("libtcc.c",
      """    tccgen_finish(s1);
    preprocess_end(s1);
    s1->error_set_jmp_enabled = 0;
    tcc_exit_state(s1);
    return s1->nb_errors != 0 ? -1 : 0;
""",
      """    tccgen_finish(s1);
    preprocess_end(s1);
    s1->error_set_jmp_enabled = 0;
    tcc_exit_state(s1);
    if (tinyc_pp_stream) {
        fclose(tinyc_pp_stream);
        s1->ppfp = stdout;
        tinyc_io_flush(tcc_get_io(s1));
    }
    return s1->nb_errors != 0 ? -1 : 0;
""")

# ---- tcc.c: -E output goes through the state's IO when one is set ---
patch("tcc.c",
      """            if (s->outfile && 0!=strcmp("-",s->outfile)) {
                ppfp = tcc_fopen(s->outfile, "wb");
                if (!ppfp)
                    tcc_error_noabort("could not write '%s'", s->outfile);
            }""",
      """            if (s->outfile && 0!=strcmp("-",s->outfile)) {
                ppfp = tcc_fopen(s->outfile, "wb");
                if (!ppfp)
                    tcc_error_noabort("could not write '%s'", s->outfile);
            } else {
                /* tinyc: route -E output through the state's IO when
                 * one is installed (NULL stream keeps stdout). */
                ppfp = tinyc_io_open_out_stream(tcc_get_io(s));
            }""")

# ---- tccrun.c: no host kill / crash on a relocated state -----------
# A successful tcc_relocate ends in cleanup_sections, which frees
# every section except the symbol tables and NULLs the slots while
# nb_sections stays: the state then serves symbol lookup and calls
# only. Composing the public API as relocate() then run() on one
# state sent tcc_run into tcc_add_support, whose object load matches
# section names by dereferencing s1->sections[j] unconditionally -
# a NULL slot reads its name at +0x80 and the host dies at fault
# address 0x80. The composition cannot work (runmain.o was never in
# the relocated image and a second relocation is unsupported), so
# fail cleanly at the door instead. Likewise, a second tcc_relocate
# called exit() into the host process; report and return like every
# other libtcc error.
patch("tccrun.c",
      """    /* tcc -dt -run ... nothing to do if no main() */
    if ((s1->dflag & 16) && (addr_t)-1 == get_sym_addr(s1, "main", 0, 1))
        return 0;""",
      """    /* tinyc: an already-relocated state cannot run (see above) */
    if (s1->run_ptr) {
        tcc_error_noabort("tcc_run: state was already relocated");
        return -1;
    }

    /* tcc -dt -run ... nothing to do if no main() */
    if ((s1->dflag & 16) && (addr_t)-1 == get_sym_addr(s1, "main", 0, 1))
        return 0;""")
patch("tccrun.c",
      """    if (s1->run_ptr)
        exit(tcc_error_noabort("'tcc_relocate()' twice is no longer supported"));""",
      """    if (s1->run_ptr) {
        tcc_error_noabort("'tcc_relocate()' twice is no longer supported");
        return -1;
    }""")

# ---- tccrun.c: a missing runmain.o ends tcc_run at the load --------
# tcc_run loads runmain.o for the entry shim it calls after
# relocation. When the file is not found, tcc_add_support has
# already reported it; upstream presses on into relocation, which
# on the PE path does not survive to the _runmain lookup that
# would fail cleanly. Return the error here instead; with the
# file present the call succeeds and nothing changes.
patch("tccrun.c",
      """    if (s1->elf_entryname)
        s1->run_main = top_sym = s1->elf_entryname;
    tcc_add_support(s1, "runmain.o");""",
      """    if (s1->elf_entryname)
        s1->run_main = top_sym = s1->elf_entryname;
    if (tcc_add_support(s1, "runmain.o"))
        return -1;""")

# ---- tccrun.c: -run program stdio goes through the state's IO -------
# The wrapper calls bracket the program's entry point; tcc_run's own
# semantics (arguments, return value, exit handling) are unchanged.
# With no IO or the stdio IO on the state both calls are no-ops.
patch("tccrun.c",
      """    errno = 0; /* clean errno value */
    fflush(stdout);
    fflush(stderr);""",
      """    /* tinyc: with a non-stdio IO on the state, redirect the run
     * program's process stdio through it for the duration of the
     * run (no-op for the stdio IO). run_stdin means tcc_run has
     * already reopened stdin from a file: leave fd 0 alone. */
    tinyc_run_redirect_begin(tcc_get_io(s1), s1->run_stdin != NULL);

    errno = 0; /* clean errno value */
    fflush(stdout);
    fflush(stderr);""")

# ---- tccrun.c: relocate verbose trace goes through the state's IO ---
patch("tccrun.c",
      '        printf(&"-----------------------------------------------------\\n"[PTR_SIZE*2 - 8]);',
      '        tinyc_io_out_str(tcc_get_io(s1), &"-----------------------------------------------------\\n"[PTR_SIZE*2 - 8]);')
patch("tccrun.c",
      """                    printf("%d: %-16s %p  len %05x  align %04x\\n",
                        k, s->name, (void*)s->sh_addr, length, s->sh_addralign);""",
      """                    tinyc_io_out_fmt(tcc_get_io(s1), "%d: %-16s %p  len %05x  align %04x\\n",
                        k, s->name, (void*)s->sh_addr, length, s->sh_addralign);""")
patch("tccrun.c",
      """                printf("protect         %3s %p  len %05x\\n",
                    &"rx\\0ro\\0rw\\0rwx"[f*3], (void*)addr, (unsigned)n);""",
      """                tinyc_io_out_fmt(tcc_get_io(s1), "protect         %3s %p  len %05x\\n",
                    &"rx\\0ro\\0rw\\0rwx"[f*3], (void*)addr, (unsigned)n);""")

# ---- tcctools.c: gen_makedeps verbose note goes through the IO ------
patch("tcctools.c",
      """    if (s1->verbose)
        printf("<- %s\\n", filename);""",
      """    if (s1->verbose)
        tinyc_io_out_fmt(tcc_get_io(s1), "<- %s\\n", filename);""")

# ---- tcctools.c: the -ar/-impdef/-m tools take main's state IO ------
# tcctools.c is included into tcc.c, whose tinyc_main_impl passes the
# IO of the state it created. The tools' diagnostics and listings go
# through that IO; file outputs (the archive itself, the .def file,
# the deps file) are data channels and stay direct.
patch("tcctools.c",
      """static int ar_usage(int ret) {
    fprintf(stderr, "usage: tcc -ar [crstvx] lib [files]\\n");
    fprintf(stderr, "create library ([abdiopN] not supported).\\n");
    return ret;
}""",
      """static int ar_usage(int ret, TCCIO *io) {
    tinyc_io_err_fmt(io, "usage: tcc -ar [crstvx] lib [files]\\n");
    tinyc_io_err_fmt(io, "create library ([abdiopN] not supported).\\n");
    return ret;
}""")
patch("tcctools.c",
      "ST_FUNC int tcc_tool_ar(int argc, char **argv)",
      "ST_FUNC int tcc_tool_ar(int argc, char **argv, TCCIO *io)")
patch("tcctools.c",
      "        return ar_usage(ret);",
      "        return ar_usage(ret, io);")
patch("tcctools.c",
      '            fprintf(stderr, "tcc: ar: can\'t open file %s\\n", argv[i_lib]);',
      '            tinyc_io_err_fmt(io, "tcc: ar: can\'t open file %s\\n", argv[i_lib]);')
patch("tcctools.c",
      '            fprintf(stderr, "tcc: ar: not an ar archive %s\\n", argv[i_lib]);',
      '            tinyc_io_err_fmt(io, "tcc: ar: not an ar archive %s\\n", argv[i_lib]);')
patch("tcctools.c",
      '\t\t    printf("%s%s\\n", extract ? "x - " : "", arhdr.ar_name);',
      '\t\t    tinyc_io_out_fmt(io, "%s%s\\n", extract ? "x - " : "", arhdr.ar_name);')
patch("tcctools.c",
      "\t\t\tfprintf(stderr, \"tcc: ar: can't create file %s\\n\",\n\t\t\t\tarhdr.ar_name);",
      "\t\t\ttinyc_io_err_fmt(io, \"tcc: ar: can't create file %s\\n\",\n\t\t\t\tarhdr.ar_name);")
patch("tcctools.c",
      '        fprintf(stderr, "tcc: ar: can\'t create file %s\\n", argv[i_lib]);',
      '        tinyc_io_err_fmt(io, "tcc: ar: can\'t create file %s\\n", argv[i_lib]);')
patch("tcctools.c",
      '        fprintf(stderr, "tcc: ar: can\'t create temporary file %s\\n", tfile);',
      '        tinyc_io_err_fmt(io, "tcc: ar: can\'t create temporary file %s\\n", tfile);')
patch("tcctools.c",
      '            fprintf(stderr, "tcc: ar: can\'t open file %s \\n", argv[i_obj]);',
      '            tinyc_io_err_fmt(io, "tcc: ar: can\'t open file %s \\n", argv[i_obj]);')
patch("tcctools.c",
      '            printf("a - %s\\n", argv[i_obj]);',
      '            tinyc_io_out_fmt(io, "a - %s\\n", argv[i_obj]);')
patch("tcctools.c",
      '            fprintf(stderr, "tcc: ar: Unsupported Elf Class: %s\\n", argv[i_obj]);',
      '            tinyc_io_err_fmt(io, "tcc: ar: Unsupported Elf Class: %s\\n", argv[i_obj]);')
patch("tcctools.c",
      "ST_FUNC int tcc_tool_impdef(int argc, char **argv)",
      "ST_FUNC int tcc_tool_impdef(int argc, char **argv, TCCIO *io)")
patch("tcctools.c",
      """        fprintf(stderr,
            "usage: tcc -impdef library.dll [-v] [-o outputfile]\\n"
            "create export definition file (.def) from dll\\n"
            );""",
      """        tinyc_io_err_fmt(io,
            "usage: tcc -impdef library.dll [-v] [-o outputfile]\\n"
            "create export definition file (.def) from dll\\n"
            );""")
patch("tcctools.c",
      """        fprintf(stderr, "tcc: impdef: %s '%s'\\n",""",
      """        tinyc_io_err_fmt(io, "tcc: impdef: %s '%s'\\n",""")
patch("tcctools.c",
      '        printf("-> %s\\n", file);',
      '        tinyc_io_out_fmt(io, "-> %s\\n", file);')
patch("tcctools.c",
      '        fprintf(stderr, "tcc: impdef: could not create output file: %s\\n", outfile);',
      '        tinyc_io_err_fmt(io, "tcc: impdef: could not create output file: %s\\n", outfile);')
patch("tcctools.c",
      '        printf("<- %s (%d symbol%s)\\n", outfile, i, &"s"[i<2]);',
      '        tinyc_io_out_fmt(io, "<- %s (%d symbol%s)\\n", outfile, i, &"s"[i<2]);')
patch("tcctools.c",
      """ST_FUNC int tcc_tool_cross(char **argv, int option)
{
    fprintf(stderr, "tcc -m%d not implemented\\n", option);
    return 1;
}""",
      """ST_FUNC int tcc_tool_cross(char **argv, int option, TCCIO *io)
{
    tinyc_io_err_fmt(io, "tcc -m%d not implemented\\n", option);
    return 1;
}""")
patch("tcctools.c",
      "ST_FUNC int tcc_tool_cross(char **argv, int target)",
      "ST_FUNC int tcc_tool_cross(char **argv, int target, TCCIO *io)")
patch("tcctools.c",
      '    fprintf(stderr, "tcc: could not run \'%s\'\\n", program);',
      '    tinyc_io_err_fmt(io, "tcc: could not run \'%s\'\\n", program);')
patch("tccrun.c",
      """    if (0 == ret) {
        ret = prog_main(argc, argv, envp);
    } else if (RT_EXIT_ZERO == ret) {
        ret = 0;
    }""",
      """    if (0 == ret) {
        ret = prog_main(argc, argv, envp);
    } else if (RT_EXIT_ZERO == ret) {
        ret = 0;
    }

    tinyc_run_redirect_end();""")

# ---- tccrun.c / tccpe.c: stage markers on the run path --------------
# A crash inside tcc_run in library mode otherwise leaves no
# trace of how far the run got. Mark the stage boundaries in the
# diagnostic markers file (tinyc_diag_mark; a no-op off Windows):
# runmain.o loaded, the PE_RUN import resolution bracketed in
# pe_build_imports, relocation done, the entry address resolved,
# and the call itself bracketed.
patch("tccrun.c",
      """    if (tcc_add_support(s1, "runmain.o"))
        return -1;""",
      """    if (tcc_add_support(s1, "runmain.o"))
        return -1;
    tinyc_diag_mark("run:add-support:ok");""")
patch("tccrun.c",
      """    if (tcc_relocate(s1) < 0)
        return -1;""",
      """    if (tcc_relocate(s1) < 0)
        return -1;
    tinyc_diag_mark("run:relocated");""")
patch("tccrun.c",
      """    prog_main = (void*)get_sym_addr(s1, s1->run_main, 1, 1);
    if ((addr_t)-1 == (addr_t)prog_main)
        return -1;""",
      """    prog_main = (void*)get_sym_addr(s1, s1->run_main, 1, 1);
    if ((addr_t)-1 == (addr_t)prog_main)
        return -1;
    tinyc_diag_mark_ptr("run:entry", (const void *)prog_main);""")
patch("tccrun.c",
      "        ret = prog_main(argc, argv, envp);",
      """        tinyc_diag_mark("run:calling");
        ret = prog_main(argc, argv, envp);""")
patch("tccrun.c",
      "    tinyc_run_redirect_end();",
      """    tinyc_diag_mark("run:returned");

    tinyc_run_redirect_end();""")
patch("tccpe.c",
      """    if (0 == sym_cnt)
        return;

    pe_align_section(pe->thunk, 16);""",
      """    if (0 == sym_cnt)
        return;

    if (pe->type == PE_RUN)
        tinyc_diag_mark("imports:begin");

    pe_align_section(pe->thunk, 16);""")
patch("tccpe.c",
      """        dll_ptr += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }
}""",
      """        dll_ptr += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }

    if (pe->type == PE_RUN)
        tinyc_diag_mark("imports:done");
}""")

# ---- tcc.c: exported entries + real process entry --------------------
p = SRC / "tcc.c"
text = p.read_text()
text += """
/* tinyc dual-form entries: executing the file runs main, which
 * forwards to the exported tinyc_main; loading the file as a library
 * lets hosts call tinyc_main (or the tcc_* API) directly. The
 * _with_io form installs its IO on every state the impl creates;
 * states created through the tcc_* API keep their own IO. */
int tinyc_main(int argc, char **argv) { return tinyc_main_impl(argc, argv, NULL); }
int tinyc_main_with_io(int argc, char **argv, TCCIO *io) { return tinyc_main_impl(argc, argv, io); }
int main(int argc, char **argv) { return tinyc_main(argc, argv); }
"""
p.write_text(text)
print("patched tcc.c: appended dual-form entry wrappers")
print("all patches applied")
