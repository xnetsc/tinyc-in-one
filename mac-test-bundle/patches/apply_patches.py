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

# ---- tcc.h: pull in the IO API, add the opt-exit flag field ---------
patch("tcc.h",
      '#include "config.h"\n',
      '#include "config.h"\n#include "tinyc_io.h"\n')
patch("tcc.h",
      "    int output_type;\n",
      "    int output_type;\n    int opt_exit_requested; /* tinyc: -dumpmachine/-dumpversion set this instead of calling exit(0) */\n")

# ---- libtcc.c: default error output goes through the current IO -----
patch("libtcc.c",
      """        /* default case: stderr */
        if (s1 && s1->output_type == TCC_OUTPUT_PREPROCESS && s1->ppfp == stdout)
            printf("\\n"); /* print a newline during tcc -E */
        fflush(stdout); /* flush -v output */
        fprintf(stderr, "%s\\n", (char*)cs.data);
        fflush(stderr); /* print error/warning now (win32) */""",
      """        /* default case: current IO err channel (real stderr by default) */
        if (s1 && s1->output_type == TCC_OUTPUT_PREPROCESS && s1->ppfp == stdout)
            tinyc_io_out_str("\\n"); /* print a newline during tcc -E */
        tinyc_io_flush_all(); /* flush -v output */
        tinyc_io_err_line((char*)cs.data);
        tinyc_io_flush_all(); /* print error/warning now */""")

# ---- libtcc.c: -dumpmachine/-dumpversion must not exit the host -----
patch("libtcc.c",
      """            printf("%s\\n", dumpmachine_str);
            exit(0);""",
      """            tinyc_io_out_fmt("%s\\n", dumpmachine_str);
            s->opt_exit_requested = 1;
            return 0;""")
patch("libtcc.c",
      """            printf ("%s\\n", TCC_VERSION);
            exit(0);""",
      """            tinyc_io_out_fmt("%s\\n", TCC_VERSION);
            s->opt_exit_requested = 1;
            return 0;""")

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

# ---- tcc.c: main becomes the exported tinyc_main ---------------------
patch("tcc.c",
      "int main(int argc, char **argv)",
      "int tinyc_main(int argc, char **argv)")
patch("tcc.c",
      """    s = s1 = tcc_new();
    opt = tcc_parse_args(s, &argc, &argv);
""",
      """    s = s1 = tcc_new();
    opt = tcc_parse_args(s, &argc, &argv);
    if (s->opt_exit_requested) {
        tcc_delete(s);
        return 0;
    }
""")

# ---- tcc.c: help/version/search-dirs go through the current IO ------
patch("tcc.c", "fputs(help, stdout);", "tinyc_io_out_str(help);")
patch("tcc.c", "help2: fputs(help2, stdout);", "help2: tinyc_io_out_str(help2);")
patch("tcc.c", 'printf("%s", version);', "tinyc_io_out_str(version);")
patch("tcc.c",
      'printf("%s:\\n%s", msg, nb_paths ? "" : "  -\\n");',
      'tinyc_io_out_fmt("%s:\\n%s", msg, nb_paths ? "" : "  -\\n");')
patch("tcc.c",
      'printf("  %s\\n", paths[i]);',
      'tinyc_io_out_fmt("  %s\\n", paths[i]);')
patch("tcc.c",
      'printf("install: %s\\n", s->tcc_lib_path);',
      'tinyc_io_out_fmt("install: %s\\n", s->tcc_lib_path);')
patch("tcc.c",
      'printf("libtcc1:\\n  %s/%s\\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);',
      'tinyc_io_out_fmt("libtcc1:\\n  %s/%s\\n", s->library_paths[0], CONFIG_TCC_CROSSPREFIX TCC_LIBTCC1);')
patch("tcc.c",
      'printf("elfinterp:\\n  %s\\n",  s->elfint);',
      'tinyc_io_out_fmt("elfinterp:\\n  %s\\n",  s->elfint);')
patch("tcc.c",
      'printf("-> %s\\n", f->name);',
      'tinyc_io_out_fmt("-> %s\\n", f->name);')

# ---- tcc.c: -E output goes through the current IO when one is set ---
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
                /* tinyc: route -E output through the current IO when
                 * one is installed (NULL stream keeps stdout). */
                ppfp = tinyc_io_open_out_stream();
            }""")

# ---- tcc.c: real process entry forwards to tinyc_main ----------------
p = SRC / "tcc.c"
text = p.read_text()
text += """
/* tinyc dual-form entry: executing the file runs this main, which
 * forwards to the exported tinyc_main; loading the file as a library
 * lets hosts call tinyc_main (or the tcc_* API) directly. */
int main(int argc, char **argv) { return tinyc_main(argc, argv); }
"""
p.write_text(text)
print("patched tcc.c: appended dual-form main wrapper")
print("all patches applied")
