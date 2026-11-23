#!/usr/bin/env python3
"""Generate the .def export list for the Windows dual-form file.

Usage: gen_windows_def.py <libtcc.h> <output.def>

Exports (design doc 3.1/3.2): main, tinyc_main, tinyc_init, and every
LIBTCCAPI function declared in libtcc.h. The list is derived from the
header at build time so it cannot drift from the API surface.
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
    with open(out, "w") as f:
        f.write("EXPORTS\n")
        for name in ["main", "tinyc_main", "tinyc_init"] + names:
            f.write(f"    {name}\n")
    print(f"gen_windows_def: {out} ({len(names) + 3} exports)")

if __name__ == "__main__":
    main()
