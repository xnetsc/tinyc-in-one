#!/usr/bin/env python3
"""Generate the .def export list for the Windows dual-form file.

Usage: gen_windows_def.py <libtcc.h> <output.def>

Exports (design doc 3.1/3.2): main, tinyc_main, tinyc_main_io,
tinyc_init, tinyc_ping and tinyc_noop (the entry points and
library-path diagnostic entries in core/), the tcc_io_* surface
declared in core/tinyc_io.h that the packages bind by name, and
every LIBTCCAPI function declared in libtcc.h. The libtcc.h list
is derived from the header at build time so it cannot drift from
the API surface; the core lists are fixed here next to it.
"""
import re
import sys

def main():
    header, out = sys.argv[1], sys.argv[2]
    text = open(header).read()
    names = []
    for m in re.finditer(r"LIBTCCAPI\s+[^;(]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(", text):
        name = m.group(1)
        # every API function is tcc_* (plus _tcc_setjmp); anything else
        # is a word caught from a comment between declarations
        if not (name.startswith("tcc_") or name.startswith("_tcc_")):
            continue
        if name not in names:
            names.append(name)
    if not names:
        print("gen_windows_def: no LIBTCCAPI functions found", file=sys.stderr)
        sys.exit(1)
    fixed = ["main", "tinyc_main", "tinyc_main_io",
             "tinyc_init", "tinyc_ping", "tinyc_noop"]
    # The tcc_io_* constructors and accessors from core/tinyc_io.h
    # that the Java, Python and npm packages resolve by name at load
    # time. tcc_io_FILE and tcc_io_tee are declared in the same
    # header but no package binds them, so they stay unexported.
    core_io = ["tcc_io_stdio", "tcc_io_null", "tcc_io_buffer",
               "tcc_io_buffer_merged", "tcc_io_file", "tcc_io_fd",
               "tcc_io_callback", "tcc_io_free", "tcc_io_buffer_data",
               "tcc_io_buffer_clear"]
    with open(out, "w") as f:
        f.write("EXPORTS\n")
        for name in fixed + core_io + names:
            f.write(f"    {name}\n")
    print(f"gen_windows_def: {out} "
          f"({len(fixed) + len(core_io) + len(names)} exports)")

if __name__ == "__main__":
    main()
