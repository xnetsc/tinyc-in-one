#!/usr/bin/env python3
"""Darwin self-containment wiring for the macOS bundle build.

Two subcommands, called from build-mac.sh:

  paths <config.h>
    Rewrite the search paths upstream configure baked into
    config.h.  On Darwin, configure sets CONFIG_TCC_SYSINCLUDEPATHS
    to "{B}/include:<build machine's SDK>/usr/include" - the SDK
    path of the machine that built the compiler, which is both a
    dependency on the system headers and a path that does not
    exist on a user's machine.  Replace it with the package paths
    "{B}/include-libc:{B}/include" (bundled Darwin headers first,
    TCC's builtin layer second) and set CONFIG_TCC_LIBPATHS to
    "{B}/lib:{B}" so the bundled .tbd stubs are found before any
    other library path.  The end state is asserted, not the edit.

  stage <vendor-darwin> <dist>
    Copy the bundled Darwin headers into the dist at
    lib/tcc/include-libc/ and the .tbd link stubs at
    lib/tcc/lib/, mirroring the linux-gnu dist layout
    (include-libc/ + a library directory next to libtcc1.a).

Mach-O needs no crt objects: TCC's Mach-O path adds none (the
loader and libSystem initialize the process), so the library
directory carries only the stubs.
"""
import os
import re
import shutil
import sys

SYSINCLUDEPATHS = "{B}/include-libc:{B}/include"
LIBPATHS = "{B}/lib:{B}"


def do_paths(config_h):
    with open(config_h) as f:
        text = f.read()
    pat = re.compile(r'^#define CONFIG_TCC_SYSINCLUDEPATHS "[^"]*"$', re.M)
    if not pat.search(text):
        sys.exit(f"CONFIG_TCC_SYSINCLUDEPATHS not found in {config_h}")
    text = pat.sub(f'#define CONFIG_TCC_SYSINCLUDEPATHS "{SYSINCLUDEPATHS}"',
                   text, count=1)
    pat_lib = re.compile(r'^#define CONFIG_TCC_LIBPATHS "[^"]*"$', re.M)
    if pat_lib.search(text):
        text = pat_lib.sub(f'#define CONFIG_TCC_LIBPATHS "{LIBPATHS}"',
                           text, count=1)
    else:
        anchor = f'#define CONFIG_TCC_SYSINCLUDEPATHS "{SYSINCLUDEPATHS}"\n'
        text = text.replace(
            anchor, anchor + f'#define CONFIG_TCC_LIBPATHS "{LIBPATHS}"\n', 1)
    for name, want in (("CONFIG_TCC_SYSINCLUDEPATHS", SYSINCLUDEPATHS),
                       ("CONFIG_TCC_LIBPATHS", LIBPATHS)):
        m = re.search(rf'^#define {name} "([^"]*)"$', text, re.M)
        assert m and m.group(1) == want, f"{name} not set to {want}"
        assert "MacOSX.sdk" not in m.group(1), f"SDK path left in {name}"
    with open(config_h, "w") as f:
        f.write(text)
    print(f"darwin-dist: paths rewritten in {config_h}")


def do_stage(vendor, dist):
    inc_src = os.path.join(vendor, "include")
    lib_src = os.path.join(vendor, "lib")
    inc_dst = os.path.join(dist, "lib", "tcc", "include-libc")
    lib_dst = os.path.join(dist, "lib", "tcc", "lib")
    if os.path.isdir(inc_dst):
        shutil.rmtree(inc_dst)
    shutil.copytree(inc_src, inc_dst)
    os.makedirs(lib_dst, exist_ok=True)
    tbds = [f for f in sorted(os.listdir(lib_src)) if f.endswith(".tbd")]
    if not tbds:
        sys.exit(f"no .tbd stubs in {lib_src}")
    for name in tbds:
        shutil.copy2(os.path.join(lib_src, name), os.path.join(lib_dst, name))
    n_headers = sum(len(files) for _r, _d, files in os.walk(inc_dst))
    print(f"darwin-dist: staged {n_headers} headers into {inc_dst}, "
          f"{len(tbds)} stubs into {lib_dst}")


def main(argv):
    if len(argv) == 3 and argv[1] == "paths":
        do_paths(argv[2])
    elif len(argv) == 4 and argv[1] == "stage":
        do_stage(argv[2], argv[3])
    else:
        sys.exit(__doc__)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
