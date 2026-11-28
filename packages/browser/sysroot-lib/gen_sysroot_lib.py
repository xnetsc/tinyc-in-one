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
  Android targets -- every link input is copied verbatim
    from the vendored NDK collection
    (vendor-android/<triple>/<api>/lib, the same files the
    native dist bundles): the crt objects under their own
    names (crtbegin_dynamic.o/crtbegin_so.o/
    crtbegin_static.o, crtend_android.o/crtend_so.o) and
    libc.so/libm.so/libdl.so.
  Windows -- the *.def import definition files are TinyCC's
    own, from vendor-tinycc/win32/lib, copied verbatim.
  Darwin -- the *.tbd text stubs are the vendored set from
    vendor-darwin/lib, copied verbatim (the same files the
    native dist bundles); this script verifies their exports
    cover the shipped headers' declared function sets.

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
X86_64_LIBDIR = os.environ.get("X86_64_LIBDIR", "/usr/lib/x86_64-linux-gnu")
AARCH64_SYSROOT = os.environ.get("AARCH64_SYSROOT", "/usr/aarch64-linux-gnu")
RISCV64_SYSROOT = os.environ.get("RISCV64_SYSROOT", "/usr/riscv64-linux-gnu")

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
    {(name, version): (kind, size, is_default)} with kind 'func'
    or 'obj'; version is None for an unversioned symbol. A name
    the real library exports under several versions (glibc's
    memcpy@@GLIBC_2.14 next to memcpy@GLIBC_2.2.5, bionic's
    LIBC / LIBC_N / ... sets) yields one entry per version, so
    the stub can carry the same symbol-version bindings: a
    program linked against the stub must record the same
    version references as one linked against the real library."""
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
        version = None
        is_default = True
        if "@@" in name:
            name, version = name.split("@@", 1)
        elif "@" in name:
            name, version = name.split("@", 1)
            is_default = False
        if not name or not re.fullmatch(r"[A-Za-z_.$][A-Za-z0-9_.$]*", name):
            raise SystemExit(f"unusable symbol name {name!r} in {path}")
        # An IFUNC is called through the PLT exactly like a plain
        # function; classifying it as an object would make the
        # linker emit a COPY relocation for it and the resulting
        # program would jump into copied data (glibc's sin, cos,
        # strcpy, strlen, memset and friends are IFUNCs).
        kind = "func" if typ in ("FUNC", "IFUNC") else "obj"
        size = int(size)
        key = (name, version)
        if key in syms:
            okind, osize, odef = syms[key]
            if okind == "func" or kind == "func":
                kind = "func"
            syms[key] = (kind, max(osize, size), odef or is_default)
        else:
            syms[key] = (kind, size, is_default)
    return syms


def write_stub_asm(syms, path):
    """Emit the stub's definitions. A symbol the real library
    binds to a version is defined once per version under an
    internal label, with .symver binding the label to the
    public name and version: versions of one name can differ
    in kind and size in the real library (glibc's sys_errlist
    exists at four versions with four sizes), and a linker
    copying the symbol into a product takes st_size from the
    definition of the version it bound, so each version needs
    its own definition carrying its own size. The matching
    version nodes are declared by the version script
    build_stub writes, so the stub's .gnu.version_d names the
    same versions as the real library's."""
    by_name = {}
    for (name, version), (kind, size, is_default) in syms.items():
        by_name.setdefault(name, []).append((version, kind, size, is_default))
    with open(path, "w") as f:
        def emit(body_name, kind, size):
            # Internal labels (the per-version bodies of a
            # multi-version name) are defined global so .symver
            # binds them, and the version script's
            # `local: __tinyc_stub_*` keeps them out of the
            # stub's .dynsym: only the versioned public names
            # are exported, exactly the real library's set.
            if kind == "func":
                # The recorded size is the real library's: a
                # linker copying this symbol into a product's
                # .dynsym takes st_size from here, so it must
                # match the real library's, not the stub's
                # empty body.
                f.write(f"\t.globl\t{body_name}\n"
                        f"\t.type\t{body_name}, %function\n"
                        f"{body_name}:\n\t.size\t{body_name}, {size}\n")
            else:
                f.write(f"\t.globl\t{body_name}\n"
                        f"\t.type\t{body_name}, %object\n"
                        f"\t.p2align\t3\n{body_name}:\n"
                        f"\t.zero\t{max(size, 8)}\n"
                        f"\t.size\t{body_name}, {size}\n")
        f.write("\t.text\n")
        # Function bodies first (the .text above), then object
        # bodies in their .bss section; two passes over the
        # same plan keep the sections contiguous.
        plan = []
        for name in sorted(by_name):
            entries = by_name[name]
            versions = sorted((e for e in entries if e[0] is not None),
                              key=lambda e: e[0])
            bare = [e for e in entries if e[0] is None]
            for i, (version, kind, size, is_default) in enumerate(versions):
                # The public name itself carries the definition
                # only for a lone default version with no bare
                # entry next to it; every other versioned entry
                # gets an internal label, so no name is defined
                # twice.
                if len(versions) == 1 and is_default and not bare:
                    label = name
                else:
                    label = f"__tinyc_stub_{name}_{i}"
                plan.append((label, name, version, kind, size, is_default))
            if bare:
                _v, kind, size, _d = bare[0]
                plan.append((name, name, None, kind, size, True))
        for want in ("func", "obj"):
            if want == "obj":
                f.write("\t.section .bss.stub,\"aw\",%nobits\n")
            for label, name, version, kind, size, is_default in plan:
                if kind != want:
                    continue
                if label == name and version is None:
                    emit(name, kind, size)
                elif label == name:
                    emit(name, kind, size)
                    mark = "@@" if is_default else "@"
                    f.write(f"\t.symver\t{name},{name}{mark}{version}\n")
                else:
                    emit(label, kind, size)
                    mark = "@@" if is_default else "@"
                    f.write(f"\t.symver\t{label},{name}{mark}{version}\n")
    return sorted({v for vs in by_name.values() for v, *_ in vs if v})


def build_stub(triple, syms, soname, out):
    asm = os.path.join(WORK, os.path.basename(out) + "." + triple + ".s")
    versions = write_stub_asm(syms, asm)
    cmd = [NDK_CLANG, "-target", triple, "-nostdlib", "-shared",
           "-Wl,-soname," + soname, asm, "-o", out]
    if versions:
        vscript = os.path.join(
            WORK, os.path.basename(out) + "." + triple + ".ver")
        by_version = {}
        for (name, version) in syms:
            if version is not None:
                by_version.setdefault(version, []).append(name)
        with open(vscript, "w") as f:
            for version in versions:
                names = "; ".join(sorted(set(by_version[version])))
                f.write(f"{version} {{ global: {names}; "
                        f"local: __tinyc_stub_*; }};\n")
        cmd.insert(-2, "-Wl,--version-script," + vscript)
    run(cmd)


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
        exports.update(dict.fromkeys(name for name, _version in syms))
    check(target, set(exports), math_set, tcclib_set, crt_undefs)


def gen_android(target, triple, arch, math_set, tcclib_set):
    outdir = os.path.join(HERE, target)
    os.makedirs(outdir, exist_ok=True)
    # Every link input is copied verbatim from the vendored NDK
    # collection at the API level the native dist is built with
    # (build.sh's TINYC_ANDROID_API, default 30): the dist
    # bundles the same files, so the browser sysroot's Android
    # link inputs are the dist's, byte for byte. The compiler's
    # crt search names the bionic files directly (tccelf.c's
    # TARGETOS_ANDROID branches): crtbegin_dynamic.o /
    # crtbegin_so.o at the start of an exe / shared-library
    # link, crtend_android.o / crtend_so.o at the end;
    # crtbegin_static.o is the static-link variant of the same
    # set. libc.so / libm.so / libdl.so are the sysroot's own
    # link-time libraries.
    dist_api = os.environ.get("TINYC_ANDROID_API", "30")
    libdir = os.path.join(REPO, "vendor-android", target, dist_api, "lib")
    crt_undefs = set()
    for name in sorted(os.listdir(libdir)):
        src = os.path.join(libdir, name)
        shutil.copyfile(src, os.path.join(outdir, name))
        if name.startswith("crtbegin"):
            crt_undefs |= undefined_syms(src)
    exports = {}
    for lib in ("libc.so", "libm.so", "libdl.so"):
        syms = dynsyms(os.path.join(libdir, lib))
        exports.update(dict.fromkeys(name for name, _version in syms))
    check(target, set(exports), math_set, tcclib_set, crt_undefs)


# The .def files are TinyCC's own import definitions, copied
# verbatim: they cover the hosted link against the Windows
# system libraries (the C89 core, stdio and the string
# functions are all in msvcrt.dll). Facilities the headers
# declare that msvcrt.dll does not export -- the C99 math
# additions, the long double forms, snprintf/vsnprintf and the
# dl* quartet among them -- fail to link here exactly as they
# do against the native Windows sysroot, whose .def files are
# the same ones.
def gen_windows():
    outdir = os.path.join(HERE, "x86_64-windows")
    os.makedirs(outdir, exist_ok=True)
    libdir = os.path.join(REPO, "vendor-tinycc", "win32", "lib")
    for name in sorted(os.listdir(libdir)):
        if name.endswith(".def"):
            shutil.copyfile(os.path.join(libdir, name),
                            os.path.join(outdir, name))
    print("x86_64-windows: def files copied")


def gen_darwin(target, math_set, tcclib_set):
    outdir = os.path.join(HERE, target)
    os.makedirs(outdir, exist_ok=True)
    # The .tbd stubs are the vendored set the native dist bundles
    # (mac-test-bundle/darwin-dist.py copies vendor-darwin/lib's
    # *.tbd into the dist verbatim); copy them the same way, and
    # verify the shipped headers' declared sets are covered by
    # their exports, the check the earlier hand-written single
    # stub was generated to pass.
    lib_src = os.path.join(REPO, "vendor-darwin", "lib")
    tbds = sorted(f for f in os.listdir(lib_src) if f.endswith(".tbd"))
    if not tbds:
        raise SystemExit(f"no .tbd stubs in {lib_src}")
    exports = set()
    for name in tbds:
        text = open(os.path.join(lib_src, name)).read()
        exports |= set(re.findall(r"(?<![A-Za-z0-9_])_[A-Za-z0-9_]+", text))
        shutil.copyfile(os.path.join(lib_src, name),
                        os.path.join(outdir, name))
    want = {"_" + n for n in (math_set | tcclib_set) - LIBTCC1_PROVIDED}
    missing = sorted(want - exports)
    if missing:
        raise SystemExit(
            f"{target}: vendored .tbd stubs do not cover declared "
            f"symbols: {' '.join(missing)}")
    print(f"{target}: {len(tbds)} .tbd stubs copied from "
          f"vendor-darwin/lib ({len(exports)} exports, declared "
          f"sets covered)")


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
