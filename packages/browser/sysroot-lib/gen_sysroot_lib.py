#!/usr/bin/env python3
"""Generate the sysroot link inputs shipped in this directory.

The browser sysroots are hosted, dynamically linked sysroots: a
program the package compiles for a non-wasm target links against
the target platform's own shared libraries at run time, so the
sysroot carries link-time inputs only -- the platform's crt
objects and stub libraries that carry the real libraries' dynamic
symbol tables (names, types and object sizes, no code).  This
script (re)builds those inputs from the platform toolchains
installed on the machine that runs it:

  Linux (glibc) targets -- crt1.o/crti.o/crtn.o are the platform's
    own objects, copied from the installed glibc development
    files; libc.so/libm.so are stubs generated from the dynamic
    symbol tables of the installed libc.so.6/libm.so.6.
  Android targets -- crtbegin_dynamic.o and crtend_android.o from
    the NDK sysroot, copied in under the crt1.o/crtn.o names the
    ELF linker looks up (the android compiler modules are built
    without TARGETOS_ANDROID, so they use the generic ELF crt
    names); crti.o is an empty object (bionic ships none).  The
    libc.so/libm.so/libdl.so stubs mirror the NDK sysroot
    libraries at API level 21.
  Windows -- msvcrt.def/kernel32.def are TinyCC's own import
    definition files from vendor-tinycc/win32/lib, copied verbatim.
  Darwin -- libc.tbd is a text stub written by this script: the
    install name of libSystem and the export names of the two
    headers whose facilities it provides (math.h, tcclib.h).

Requirements: readelf and nm (binutils), and a clang with an lld
that can link ELF shared objects for x86_64, aarch64 and riscv64
(the Android NDK's clang is used when found).  Environment
overrides: NDK_CLANG, RISCV64_SYSROOT (an extracted riscv64 glibc
sysroot, default /usr/riscv64-linux-gnu), AARCH64_SYSROOT
(default /usr/aarch64-linux-gnu), X86_64_LIBDIR (default
/usr/lib/x86_64-linux-gnu).

The script verifies its output: for every ELF target, the shipped
headers' declared function sets (math.h from
wasm-backend/include, tcclib.h from vendor-tinycc) and the crt
objects' undefined symbols must be covered by the stub exports.
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
WORK = os.path.join(HERE, ".gen-work")

NDK_CLANG = os.environ.get(
    "NDK_CLANG",
    os.path.expanduser(
        "~/workspace/envs/android-toolchain/sdk/ndk/toolchains/"
        "llvm/prebuilt/linux-x86_64/bin/clang"))
NDK_SYSROOT = os.path.expanduser(
    "~/workspace/envs/android-toolchain/sdk/ndk/toolchains/"
    "llvm/prebuilt/linux-x86_64/sysroot/usr/lib")
X86_64_LIBDIR = os.environ.get("X86_64_LIBDIR", "/usr/lib/x86_64-linux-gnu")
AARCH64_SYSROOT = os.environ.get("AARCH64_SYSROOT", "/usr/aarch64-linux-gnu")
RISCV64_SYSROOT = os.environ.get("RISCV64_SYSROOT", "/usr/riscv64-linux-gnu")
ANDROID_API = "21"

GNU_TARGETS = {  # target -> (clang triple, crt dir, libc, libm)
    "x86_64-linux-gnu": ("x86_64-linux-gnu", X86_64_LIBDIR,
                         "/lib/x86_64-linux-gnu/libc.so.6",
                         "/lib/x86_64-linux-gnu/libm.so.6"),
    "aarch64-linux-gnu": ("aarch64-linux-gnu",
                          os.path.join(AARCH64_SYSROOT, "lib"),
                          os.path.join(AARCH64_SYSROOT, "lib", "libc.so.6"),
                          os.path.join(AARCH64_SYSROOT, "lib", "libm.so.6")),
    "riscv64-linux-gnu": ("riscv64-linux-gnu",
                          os.path.join(RISCV64_SYSROOT, "lib"),
                          os.path.join(RISCV64_SYSROOT, "lib", "libc.so.6"),
                          os.path.join(RISCV64_SYSROOT, "lib", "libm.so.6")),
}
ANDROID_TARGETS = {  # target -> (clang triple, ndk arch dir)
    "aarch64-linux-android": ("aarch64-linux-android", "aarch64-linux-android"),
    "x86_64-linux-android": ("x86_64-linux-android", "x86_64-linux-android"),
}


def run(cmd, **kw):
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True, **kw)


def dynsyms(path):
    """Defined dynamic symbols of an ELF shared object:
    {name: (kind, size)} with kind 'func' or 'obj'."""
    out = subprocess.run(
        ["readelf", "--dyn-syms", "-W", path],
        check=True, capture_output=True, text=True).stdout
    syms = {}
    for line in out.splitlines():
        m = re.match(
            r"\s*\d+:\s+([0-9a-f]+)\s+(\d+)\s+(\w+)\s+(\w+)\s+\w+\s+(\S+)\s+(\S+)$",
            line)
        if not m:
            continue
        _val, size, typ, bind, ndx, name = m.groups()
        if ndx == "UND" or bind not in ("GLOBAL", "WEAK"):
            continue
        if typ == "TLS":
            continue  # a link stub cannot model thread-local storage
        name = name.split("@")[0]
        if not name or not re.fullmatch(r"[A-Za-z_.$][A-Za-z0-9_.$]*", name):
            raise SystemExit(f"unusable symbol name {name!r} in {path}")
        # An IFUNC is called through the PLT exactly like a plain
        # function; classifying it as an object would make the
        # linker emit a COPY relocation for it and the resulting
        # program would jump into copied data (glibc's sin, cos,
        # strcpy, strlen, memset and friends are IFUNCs).
        kind = "func" if typ in ("FUNC", "IFUNC") else "obj"
        size = int(size)
        if name in syms:
            okind, osize = syms[name]
            if okind == "func" or kind == "func":
                kind = "func"
            syms[name] = (kind, max(osize, size))
        else:
            syms[name] = (kind, size)
    return syms


def write_stub_asm(syms, path):
    with open(path, "w") as f:
        f.write("\t.text\n")
        for name in sorted(syms):
            if syms[name][0] != "func":
                continue
            f.write(f"\t.globl\t{name}\n\t.type\t{name}, %function\n"
                    f"{name}:\n\t.size\t{name}, 0\n")
        f.write("\t.section .bss.stub,\"aw\",%nobits\n")
        for name in sorted(syms):
            kind, size = syms[name]
            if kind != "obj":
                continue
            size = max(size, 8)
            f.write(f"\t.globl\t{name}\n\t.type\t{name}, %object\n"
                    f"\t.p2align\t3\n{name}:\n\t.zero\t{size}\n"
                    f"\t.size\t{name}, {size}\n")


def build_stub(triple, syms, soname, out):
    asm = os.path.join(WORK, os.path.basename(out) + "." + triple + ".s")
    write_stub_asm(syms, asm)
    run([NDK_CLANG, "-target", triple, "-nostdlib", "-shared",
         "-Wl,-soname," + soname, asm, "-o", out])


DECL = re.compile(
    r"(?:double|float|long\s+double|int|long\s+long|long|void|char|"
    r"FILE|size_t|unsigned\s+char)\s*\*?\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def declared_functions():
    """Function names declared by the shipped math.h and tcclib.h
    (return-type anchored, so macro names in the headers' comments
    and preprocessor lines are not picked up)."""
    math_h = open(os.path.join(
        REPO, "wasm-backend", "include", "math.h")).read()
    math_h = "\n".join(
        l for l in math_h.splitlines() if not l.strip().startswith("#"))
    tcclib_h = open(os.path.join(REPO, "vendor-tinycc", "tcclib.h")).read()
    return set(DECL.findall(math_h)), set(DECL.findall(tcclib_h))


def undefined_syms(path):
    """Strong undefined symbols of an object file (weak undefineds,
    nm's 'w' lines, are optional at link time and excluded)."""
    out = subprocess.run(["nm", "-u", path],
                         check=True, capture_output=True, text=True).stdout
    syms = set()
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[0] == "U":
            syms.add(parts[1])
    return syms


# alloca is the one tcclib.h declaration the compiler runtime
# itself provides: it is defined by every target's libtcc1.a.
LIBTCC1_PROVIDED = {"alloca"}


def check(target, covered_by, math_set, tcclib_set, crt_undefs):
    covered_by = covered_by | LIBTCC1_PROVIDED
    problems = []
    for name in sorted(math_set - covered_by):
        problems.append(f"math.h: {name} not covered")
    for name in sorted(tcclib_set - covered_by):
        problems.append(f"tcclib.h: {name} not covered")
    for name in sorted(crt_undefs - covered_by - {"main",
                                                  "_GLOBAL_OFFSET_TABLE_",
                                                  "__global_pointer$"}):
        problems.append(f"crt: {name} not covered")
    if problems:
        raise SystemExit(f"{target}: " + "; ".join(problems[:12]) +
                         f" ({len(problems)} problems)")
    print(f"{target}: declared sets and crt references covered")


def gen_gnu(target, triple, crt_dir, libc, libm, math_set, tcclib_set):
    outdir = os.path.join(HERE, target)
    os.makedirs(outdir, exist_ok=True)
    crt_undefs = set()
    for name in ("crt1.o", "crti.o", "crtn.o"):
        src = os.path.join(crt_dir, name)
        shutil.copyfile(src, os.path.join(outdir, name))
        crt_undefs |= undefined_syms(src)
    exports = {}
    for real, stub_name, soname in (
            (libc, "libc.so", "libc.so.6"), (libm, "libm.so", "libm.so.6")):
        syms = dynsyms(real)
        build_stub(triple, syms, soname, os.path.join(outdir, stub_name))
        exports.update(syms)
    check(target, set(exports), math_set, tcclib_set, crt_undefs)


def gen_android(target, triple, arch, math_set, tcclib_set):
    outdir = os.path.join(HERE, target)
    os.makedirs(outdir, exist_ok=True)
    libdir = os.path.join(NDK_SYSROOT, arch, ANDROID_API)
    crt1 = os.path.join(libdir, "crtbegin_dynamic.o")
    shutil.copyfile(crt1, os.path.join(outdir, "crt1.o"))
    shutil.copyfile(os.path.join(libdir, "crtend_android.o"),
                    os.path.join(outdir, "crtn.o"))
    empty = os.path.join(WORK, "empty.s")
    with open(empty, "w") as f:
        f.write("")
    run([NDK_CLANG, "-target", triple, "-c", empty,
         "-o", os.path.join(outdir, "crti.o")])
    crt_undefs = undefined_syms(crt1)
    exports = {}
    for lib in ("libc.so", "libm.so", "libdl.so"):
        syms = dynsyms(os.path.join(libdir, lib))
        build_stub(triple, syms, lib, os.path.join(outdir, lib))
        exports.update(syms)
    check(target, set(exports), math_set, tcclib_set, crt_undefs)


# The facilities math.h and tcclib.h declare that the Windows C
# runtime (msvcrt.dll) does not export -- the C99 math additions,
# the long double forms, snprintf/vsnprintf and the dl* quartet
# among them -- are why those two headers are not shipped for
# x86_64-windows; see README.md.  The .def files themselves are
# TinyCC's own, copied verbatim: they cover the hosted link of the
# remaining headers (the C89 core, stdio and the string functions
# are all in msvcrt.dll).
def gen_windows():
    outdir = os.path.join(HERE, "x86_64-windows")
    os.makedirs(outdir, exist_ok=True)
    for name in ("msvcrt.def", "kernel32.def"):
        shutil.copyfile(
            os.path.join(REPO, "vendor-tinycc", "win32", "lib", name),
            os.path.join(outdir, name))
    print("x86_64-windows: def files copied")


def gen_darwin(target, math_set, tcclib_set):
    outdir = os.path.join(HERE, target)
    os.makedirs(outdir, exist_ok=True)
    syms = sorted("_" + n for n in (math_set | tcclib_set) - LIBTCC1_PROVIDED)
    with open(os.path.join(outdir, "libc.tbd"), "w") as f:
        f.write("--- !tapi-tbd\n")
        f.write("tbd-version: 4\n")
        f.write("targets: [ x86_64-macos, arm64-macos ]\n")
        f.write("install-name: /usr/lib/libSystem.B.dylib\n")
        f.write("exports:\n  - symbols: [ ")
        f.write(", ".join(syms))
        f.write(" ]\n")
    print(f"{target}: libc.tbd written ({len(syms)} symbols)")


def main():
    math_set, tcclib_set = declared_functions()
    os.makedirs(WORK, exist_ok=True)
    for target, args in GNU_TARGETS.items():
        gen_gnu(target, *args, math_set, tcclib_set)
    for target, args in ANDROID_TARGETS.items():
        gen_android(target, *args, math_set, tcclib_set)
    gen_windows()
    for target in ("arm64-apple-darwin", "x86_64-apple-darwin"):
        gen_darwin(target, math_set, tcclib_set)
    shutil.rmtree(WORK)


if __name__ == "__main__":
    main()
