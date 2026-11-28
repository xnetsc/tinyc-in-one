#!/bin/bash
# Build the TinyC dual-form kernel matrix.
#
# Usage: ./build.sh [triple ...]
# With no arguments the full matrix is built. Hosts and targets are
# 64-bit only. The dual-form files produced here run on the build
# host (a Linux host); each one is a TCC whose target is its triple
# (upstream TCC is one binary per target):
#
#   x86_64-linux-gnu        native when the host is x86_64, else cross
#   aarch64-linux-gnu       native when the host is aarch64, else cross
#   riscv64-linux-gnu       cross, glibc sysroot in /usr/riscv64-linux-gnu
#   x86_64-w64-mingw32      cross, PE output (upstream x86_64-win32 target)
#   aarch64-linux-android   cross, bundled bionic headers (vendor-android/)
#   x86_64-linux-android    native when the host is x86_64, else cross
#
# A linux-gnu target whose CPU matches the build host is built in
# native mode; any other linux-gnu target is built in cross mode.
# Either way the dist bundles the target's glibc header tree and
# crt objects (vendor-glibc/), and the dual-form file's baked
# search paths take headers and crt objects from the bundle. The
# programs it compiles link the system libc.so.6 / libm.so.6
# dynamically: the bundle carries libc.so / libm.so linker scripts
# that resolve to those runtime files, so no libc development
# package is needed on the machine running the compiler. The same
# rule picks the compiler for the host variants below.
#
# The Darwin targets (arm64-apple-darwin, x86_64-apple-darwin) are
# built here like the other cross targets: the dual-form file runs
# on this (Linux) host and targets Mach-O (see build_darwin). The
# Darwin HOST variant - a dual-form file that itself runs on macOS
# (Mach-O executable with an embedded dylib segment, see the design
# doc) - is a different artifact, built only on a macOS host
# (mac-test-bundle/build-mac.sh).
#
# Host variants (the dual-form file compiled to RUN on another host,
# target == host) are built with the "host" subcommand:
#
#   ./build.sh host [host-triple ...]
#
# Supported here: the two linux-gnu triples and x86_64-w64-mingw32.
# A Linux variant whose CPU matches the build host is compiled with
# the host compiler; any other Linux variant with the <triplet>-gcc
# cross compiler (the binary is a native-configured TCC for that
# machine, verified under qemu-user). The MinGW variant uses a MinGW cross gcc; the
# recipe's CC can be overridden with TINYC_MINGW_CC, e.g. plain gcc
# on a Windows runner. Host variants land in build/dist-host/<triple>/
# with the same internal layout as build/dist plus a manifest that
# records host, target (== host) and how the build was produced.
#
# A Windows host that targets every triple (not just its own) is
# built with the "host-win" subcommand:
#
#   ./build.sh host-win [target-triple ...]
#
# One PE dual-form compiler per target, all with host
# x86_64-w64-mingw32: the native win32 triple reuses build_host_mingw,
# every other triple uses build_host_mingw_for with the target defines
# and baked search paths of the Linux dist for that target family.
# The dists land in build/dist-host/<triple>/ with the same internal
# layout as build/dist/<triple>/, plus bin/ companions so the .exe
# also works when run directly. The MinGW C compiler is
# TINYC_MINGW_CC (default x86_64-w64-mingw32-gcc); the cross make
# that produces libtcc1.a and tccdefs_.h runs with the machine's own
# compiler (configure's ambient CC), so the cross compiler it builds
# is a binary that runs on the build machine.
#
# Each triple lands in build/dist/<triple>/ with the layout:
#   bin/tinyc            dual-form file (executable + dlopen-able)
#   include/             libtcc.h, tinyc_io.h
#   lib/tcc/libtcc1.a    target runtime library
#   lib/tcc/include/     TCC builtin headers
#   lib/tcc/include-libc/  libc headers: glibc (linux-gnu triples),
#                        bionic (android triples)
#   lib/tcc/libc/        glibc crt objects + libc.so/libm.so linker
#                        scripts (linux-gnu triples); bionic crt
#                        objects + libc/libm/libdl link-time stubs
#                        (android triples)
#   manifest.json        target, isa baseline, file locations
# The Windows variant keeps its support files under lib/tcc/win32/
# (include/ + lib/), mirroring upstream's win32 layout.
set -e
cd "$(dirname "$0")"
ROOT=$PWD

# Build host CPU (x86_64 | aarch64 | riscv64 on the Linux hosts this
# script supports); drives the native/cross choice described above.
HOST_ARCH=$(uname -m)

ALL="x86_64-linux-gnu aarch64-linux-gnu riscv64-linux-gnu x86_64-w64-mingw32 aarch64-linux-android x86_64-linux-android arm64-apple-darwin x86_64-apple-darwin wasm32-wasi"
if [ $# -gt 0 ]; then TRIPLES="$*"; else TRIPLES="$ALL"; fi

ANDROID_API=${TINYC_ANDROID_API:-30}
# The NDK release vendor-android/ was collected from; the android
# dists bundle its headers and link inputs, so building or running
# the android compilers needs no NDK on the machine.
ANDROID_NDK_VER=r26d
VER=$(cat vendor-tinycc/VERSION)

# Output formats per backend family, verified against the upstream
# sources (libtcc.h TCC_OUTPUT_*; ELF content from the ELF writer,
# COFF/PE content from tccpe.c, archives are ar files; tcc.c
# default_output_name gives .exe/.dll on PE targets and .o for
# object output on every target, a.out for ELF executables). The
# value depends only on the target backend, so the same triple
# carries a byte-identical "outputs" field on every host; it is the
# authority every package manifest copies verbatim
# (tests/check_outputs_consistency.py).
print_outputs() {
    case $1 in
        pe) cat <<'JSON'
  "outputs": {
    "exe": {"format": "PE executable", "ext": ".exe"},
    "obj": {"format": "COFF object", "ext": ".o"},
    "shared": {"format": "PE DLL", "ext": ".dll"},
    "ar": {"format": "ar archive", "ext": ".a"}
  },
JSON
            ;;
        macho) cat <<'JSON'
  "outputs": {
    "exe": {"format": "Mach-O executable", "ext": ""},
    "obj": {"format": "Mach-O object", "ext": ".o"},
    "shared": {"format": "Mach-O dylib", "ext": ".dylib"},
    "ar": {"format": "ar archive", "ext": ".a"}
  },
JSON
            ;;
        wasm) cat <<'JSON'
  "outputs": {
    "module": {"format": "WebAssembly", "ext": ".wasm"}
  },
JSON
            ;;
        *) cat <<'JSON'
  "outputs": {
    "exe": {"format": "ELF executable", "ext": ""},
    "obj": {"format": "ELF relocatable", "ext": ".o"},
    "shared": {"format": "ELF shared object", "ext": ".so"},
    "ar": {"format": "ar archive", "ext": ".a"}
  },
JSON
            ;;
    esac
}

# ISA baselines, verified against the upstream backend sources
# (x86_64-gen.c: xmm used for scalar FP, no ymm/AVX anywhere;
#  arm64-gen.c: scalar FP on the ARMv8.0-A v-register file, no vector
#  instruction emission, no SVE; riscv64-gen.c/-asm.c: no RVV).
# Search paths baked into the linux-gnu dual-form files. {B} is the
# target's lib/tcc directory inside the package sysroot. The bundled
# glibc headers (include-libc) are searched before TCC's builtin
# headers, and the crt objects come from the bundled copies in
# libc/, so compiling a hosted program needs no system header or
# crt file. libc/ also carries libc.so and libm.so linker scripts
# (GROUP entries naming the runtime shared objects); the library
# search continues into the system library directory passed to
# gnu_defs, where the runtime libc.so.6 / libm.so.6 themselves are
# found, so the produced program is an ordinary dynamic ELF whose
# DT_NEEDED names the system libraries.
#
# gnu_defs <system-lib-dir> [path-sep] fills GNU_DEFS for one linux-gnu build.
# The separator between the baked search paths must be the PATHSEP of
# the OS the dual-form file RUNS on, not of the build host: libtcc
# splits these strings on PATHSEP (tcc.h), which is ':' on Unix but
# ';' on Windows. Baking ':' paths into a Windows PE makes the whole
# string one nonexistent path, so the library search finds neither
# libc nor libtcc1.a and the link silently produces an empty file
# (the output stage skips the write once nb_errors is set, while
# tcc_main still exits 0). The default ':' is for the Linux/macOS
# builds; the Windows host dispatch passes ';'. The system library
# directory is a Linux build-host path: it only resolves on a Unix
# host, so it is appended only for the ':' form.
gnu_defs() {
    local sep=${2:-:}
    local libpaths="{B}/libc${sep}{B}"
    [ "$sep" = ":" ] && libpaths="${libpaths}${sep}$1"
    GNU_DEFS=(
        "-DCONFIG_TCC_SYSINCLUDEPATHS=\"{B}/include-libc${sep}{B}/include\""
        '-DCONFIG_TCC_CRTPREFIX="{B}/libc"'
        "-DCONFIG_TCC_LIBPATHS=\"${libpaths}\""
    )
}

# --- bundled glibc headers + crt for the linux-gnu targets --------
# vendor-glibc/<triple>/ carries the target's glibc header tree and
# crt objects (LGPL; see vendor-glibc/README.md and the COPYING.LIB
# in each triple directory). add_glibc_to_dist copies them into a
# dist and adds the libc.so / libm.so linker scripts described
# above.
GLIBC_VER=2.39

# add_glibc_to_dist <dist> <triple>   (linux-gnu triples only)
add_glibc_to_dist() {
    local DIST=$1 TRIPLE=$2
    local VDIR=$ROOT/vendor-glibc/$TRIPLE
    if [ ! -d "$VDIR" ]; then
        echo "missing vendored glibc tree: $VDIR" >&2
        exit 1
    fi
    mkdir -p "$DIST/lib/tcc/include-libc" "$DIST/lib/tcc/libc"
    cp -r "$VDIR/include/." "$DIST/lib/tcc/include-libc/"
    cp "$VDIR"/crt/*.o "$DIST/lib/tcc/libc/"
    printf 'GROUP ( libc.so.6 )\n' > "$DIST/lib/tcc/libc/libc.so"
    printf 'GROUP ( libm.so.6 )\n' > "$DIST/lib/tcc/libc/libm.so"
    cp "$VDIR/COPYING.LIB" "$DIST/lib/tcc/libc/"
}

# --- bundled bionic headers + link inputs for the android targets -
# vendor-android/ carries the bundled header set collected from
# the NDK sysroot (the bionic surface at the C / POSIX layer: the
# C standard library, the full POSIX surface and bionic's own
# libc-level extensions, plus the kernel UAPI headers their
# include closure pulls in) and, per
# triple and API level, its crt objects and libc/libm/libdl
# link-time stubs (Apache-2.0 and BSD-style terms; see
# vendor-android/README.md and the NOTICE there).
# add_android_to_dist copies them into a dist, and the android
# dual-form files' baked search paths take headers and crt objects
# from the bundle, so compiling a hosted program needs no NDK on
# the machine running the compiler. The stubs carry the SONAMEs
# libc.so / libm.so / libdl.so, so the produced program is an
# ordinary dynamic ELF whose DT_NEEDED names the system bionic
# libraries, loaded by /system/bin/linker64 at run time.

# add_android_to_dist <dist> <triple>   (android triples only)
add_android_to_dist() {
    local DIST=$1 TRIPLE=$2
    local VDIR=$ROOT/vendor-android
    local LIBSRC=$VDIR/$TRIPLE/$ANDROID_API/lib
    if [ ! -d "$LIBSRC" ]; then
        echo "missing vendored bionic tree: $LIBSRC" >&2
        exit 1
    fi
    mkdir -p "$DIST/lib/tcc/include-libc" "$DIST/lib/tcc/libc"
    cp -r "$VDIR/include/." "$DIST/lib/tcc/include-libc/"
    cp "$LIBSRC"/* "$DIST/lib/tcc/libc/"
    cp "$VDIR/NOTICE" "$DIST/lib/tcc/libc/"
}

ISA_X86_64="x86-64 baseline; scalar floating point via SSE2 (xmm registers); no AVX code generation"
ISA_ARM64="ARMv8.0-A baseline (FP/NEON register file is architectural); scalar FP code generation only, no NEON vector instructions, no SVE"
ISA_RISCV64="RV64GC baseline (I/M/A/F/D/C); no Vector extension (RVV) support"

# add_win32_pthread <SRC>
# The win32 target's pthread surface (win32/include/pthread.h,
# sched.h, semaphore.h) is implemented by win32/lib/pthread.c
# over the Win32 primitives in kernel32; the implementation
# ships statically inside libtcc1.a, which every program
# links, so pthread programs need no extra link flag and no
# runtime DLL beyond the Windows system ones. Compile it with
# the just-built cross compiler and merge it into that
# compiler's libtcc1.a before any dist copy is taken.
add_win32_pthread() {
    local SRC=$1
    local XTCC="$SRC/x86_64-win32-tcc"
    [ -x "$XTCC" ] || XTCC="$SRC/x86_64-win32-tcc.exe"
    "$XTCC" -B"$SRC/win32" -I"$SRC/include" -I"$SRC" \
        -c "$SRC/win32/lib/pthread.c" -o "$SRC/tinyc-win32-pthread.o"
    (cd "$SRC" && ar r "x86_64-win32-libtcc1.a" \
        "tinyc-win32-pthread.o")
}

# add_default_libs <triple> <dist>
# Build the bundled default libraries (libcurl, PCRE2, mbedTLS;
# see libs-config/README.md) into the dist's sysroot with a
# compiler that targets <triple> and runs on this machine: the
# dist's own binary when it runs here, else the matrix dist's
# binary for the same target, else $TINYC_LIBS_TCC when the
# caller provides one. The library bytes depend only on the
# target, so any of these compilers produces the same sysroot
# content. When no usable compiler exists the build fails,
# unless TINYC_LIBS_SKIP=1 is set (compiler-only dev builds of
# foreign-host dists), in which case the dist is left without
# the libraries and says so.
add_default_libs() {
    local TRIPLE=$1 DIST=$2 FAMILY
    case $TRIPLE in
        *-linux-gnu)     FAMILY=linux-gnu ;;
        *-linux-android) FAMILY=android ;;
        *-apple-darwin)  FAMILY=darwin ;;
        *-w64-mingw32)   FAMILY=windows ;;
        *) return 0 ;;
    esac
    local cand XTCC=""
    for cand in ${TINYC_LIBS_TCC:-} "$DIST/bin/tinyc" \
                "$ROOT/build/dist/$TRIPLE/bin/tinyc"; do
        [ -n "$cand" ] && [ -x "$cand" ] || continue
        if "$cand" -v </dev/null >/dev/null 2>&1; then XTCC=$cand; break; fi
    done
    if [ -z "$XTCC" ]; then
        if [ "${TINYC_LIBS_SKIP:-}" = 1 ]; then
            echo "WARNING: no runnable $TRIPLE compiler here; " \
                 "$DIST ships without the default libraries" >&2
            return 0
        fi
        echo "error: no runnable $TRIPLE compiler for the default " \
             "libraries (set TINYC_LIBS_SKIP=1 to build without them)" >&2
        exit 1
    fi
    bash "$ROOT/tools/build_default_libs.sh" --tcc "$XTCC" \
        --triple "$TRIPLE" --family "$FAMILY" --dist "$DIST"
}

build_one() {
    local TRIPLE=$1 CT=$2 LIBMODE=$3 TDEFS=$4 ISA=$5
    shift 5
    local DEFS=("$@")
    local DIST=$ROOT/build/dist/$TRIPLE
    local SRC=$ROOT/build/work/$TRIPLE/src

    echo "===== $TRIPLE ====="
    rm -rf "$ROOT/build/work/$TRIPLE" "$DIST"
    mkdir -p "$SRC" "$DIST/bin" "$DIST/include" "$DIST/lib/tcc/include"

    # pristine vendor copy (no build artifacts), patches, IO unit
    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C vendor-tinycc . | tar xf - --no-same-owner -C "$SRC"
    cp "$ROOT/core/tinyc_io.h" "$ROOT/core/tcc_io.c" "$SRC/"
    python3 "$ROOT/patches/apply_patches.py" "$SRC" >/dev/null

    (cd "$SRC" && ./configure --prefix="$DIST" >/dev/null)

    # ---- target runtime library (libtcc1.a) ----
    local LIBTCC1_REL="lib/tcc/libtcc1.a"
    if [ "$LIBMODE" = native ]; then
        (cd "$SRC" && make -j"$(nproc)" >/dev/null && make install >/dev/null)
    else
        # native build first: the cross rules need its generated files
        (cd "$SRC" && make -j"$(nproc)" >/dev/null)
        (cd "$SRC" && make "cross-$CT" >/dev/null)
        if [ "$TRIPLE" = x86_64-w64-mingw32 ]; then
            add_win32_pthread "$SRC"
            LIBTCC1_REL="lib/tcc/win32/lib/libtcc1.a"
            mkdir -p "$DIST/lib/tcc/win32/lib" "$DIST/lib/tcc/win32/include"
            cp "$SRC/$CT-libtcc1.a" "$DIST/$LIBTCC1_REL"
            cp "$SRC"/win32/lib/*.def "$DIST/lib/tcc/win32/lib/"
            cp "$SRC"/include/*.h "$SRC/tcclib.h" "$DIST/lib/tcc/win32/include/"
            cp -r "$SRC"/win32/include/. "$DIST/lib/tcc/win32/include/"
        else
            cp "$SRC/$CT-libtcc1.a" "$DIST/lib/tcc/libtcc1.a"
            cp "$SRC"/include/*.h "$SRC/tcclib.h" "$DIST/lib/tcc/include/"
        fi
        cp "$SRC/libtcc.h" "$DIST/include/"
    fi

    case $TRIPLE in
        *-linux-gnu) add_glibc_to_dist "$DIST" "$TRIPLE" ;;
        *-android) add_android_to_dist "$DIST" "$TRIPLE" ;;
    esac

    # ---- dual-form file: tcc.c is a unity build (it includes
    # libtcc.c), so one -fPIC compile plus the IO unit, linked as a
    # PIE with dynamic symbols exported, yields a file that both
    # executes and dlopens with the full libtcc API + tinyc_main.
    gcc -O2 -fPIC -pie -Wl,--export-dynamic \
        -o "$DIST/bin/tinyc" "$SRC/tcc.c" "$ROOT/core/tcc_io.c" \
        -I"$SRC" -I"$ROOT/core" $TDEFS "${DEFS[@]}" -ldl -lm -lpthread
    # glibc dlopen refuses DF_1_PIE executables; clearing the bit keeps
    # execution intact and makes the same file dlopen-able.
    python3 "$ROOT/tools/clear_pie_flag.py" "$DIST/bin/tinyc"
    ln -sf tinyc "$DIST/bin/tcc"
    cp "$ROOT/core/tinyc_io.h" "$DIST/include/"

    # ---- manifest ----
    local EXTRA_JSON="" FAMILY=elf
    case $TRIPLE in
        *-android)
            EXTRA_JSON=",
  \"libc\": \"bionic headers (the bionic surface at the C/POSIX layer, plus the kernel headers its include closure pulls in) and crt objects bundled (Android NDK $ANDROID_NDK_VER sysroot); programs link the system libc.so/libm.so/libdl.so dynamically\",
  \"libc_headers\": \"lib/tcc/include-libc\",
  \"libc_lib\": \"lib/tcc/libc\",
  \"android_api\": $ANDROID_API"
            ;;
        *-linux-gnu)
            EXTRA_JSON=",
  \"libc\": \"glibc $GLIBC_VER headers and crt objects bundled; programs link the system libc.so.6/libm.so.6 dynamically\",
  \"libc_headers\": \"lib/tcc/include-libc\",
  \"libc_lib\": \"lib/tcc/libc\""
            ;;
        *-mingw32)
            FAMILY=pe
            ;;
    esac
    cat > "$DIST/manifest.json" <<EOF
{
  "name": "tinyc",
  "version": "$VER",
  "target": "$TRIPLE",
  "isa": "$ISA",
$(print_outputs $FAMILY)
  "dual_file": "bin/tinyc",
  "toolchain_include": "include",
  "libtcc1": "$LIBTCC1_REL"$EXTRA_JSON
}
EOF
    add_default_libs "$TRIPLE" "$DIST"
    echo "built: $DIST"
    file "$DIST/bin/tinyc" | sed 's/^/  /'
}

# build_darwin <triple> <cross-name> <target-def> <isa>
# Darwin targets built on a Linux host: the dual-form file is a
# Linux ELF like every other dist binary this script produces (it
# runs on the build host); only its target is Mach-O. libtcc1.a
# comes from the upstream cross target (make cross-<arch>-osx);
# its objects are ELF by upstream design (tcc.h's ELF_OBJ_ONLY:
# Mach-O builds write ELF .o files and the Mach-O writer consumes
# them at link time), the same form the macOS bundle build ships.
# The sysroot is the bundled Darwin set, staged by
# mac-test-bundle/darwin-dist.py exactly as for the macOS bundle
# build: the self-written headers in include-libc/, the .tbd link
# stubs in lib/ (Mach-O needs no crt objects). The baked search
# paths are the package paths, as in darwin-dist.py's config.h
# rewrite for the bundle build.
build_darwin() {
    local TRIPLE=$1 CT=$2 TDEF=$3 ISA=$4
    local DIST=$ROOT/build/dist/$TRIPLE
    local SRC=$ROOT/build/work/$TRIPLE/src

    echo "===== $TRIPLE ====="
    rm -rf "$ROOT/build/work/$TRIPLE" "$DIST"
    mkdir -p "$SRC" "$DIST/bin" "$DIST/include" "$DIST/lib/tcc/include"

    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C vendor-tinycc . | tar xf - --no-same-owner -C "$SRC"
    cp "$ROOT/core/tinyc_io.h" "$ROOT/core/tcc_io.c" "$SRC/"
    python3 "$ROOT/patches/apply_patches.py" "$SRC" >/dev/null

    (cd "$SRC" && ./configure --prefix="$DIST" >/dev/null)
    # native build first: the cross rules need its generated files
    (cd "$SRC" && make -j"$(nproc)" >/dev/null)
    (cd "$SRC" && make "cross-$CT" >/dev/null)
    cp "$SRC/$CT-libtcc1.a" "$DIST/lib/tcc/libtcc1.a"
    cp "$SRC"/include/*.h "$SRC/tcclib.h" "$DIST/lib/tcc/include/"
    cp "$SRC/libtcc.h" "$DIST/include/"
    python3 "$ROOT/mac-test-bundle/darwin-dist.py" stage \
        "$ROOT/vendor-darwin" "$DIST" >/dev/null

    gcc -O2 -fPIC -pie -Wl,--export-dynamic \
        -o "$DIST/bin/tinyc" "$SRC/tcc.c" "$ROOT/core/tcc_io.c" \
        -I"$SRC" -I"$ROOT/core" $TDEF -DTCC_TARGET_MACHO \
        '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include-libc:{B}/include"' \
        '-DCONFIG_TCC_LIBPATHS="{B}/lib:{B}"' \
        '-DCONFIG_TCC_CRTPREFIX="{B}/lib"' \
        -ldl -lm -lpthread
    python3 "$ROOT/tools/clear_pie_flag.py" "$DIST/bin/tinyc"
    ln -sf tinyc "$DIST/bin/tcc"
    cp "$ROOT/core/tinyc_io.h" "$DIST/include/"

    cat > "$DIST/manifest.json" <<EOF
{
  "name": "tinyc",
  "version": "$VER",
  "target": "$TRIPLE",
  "isa": "$ISA",
$(print_outputs macho)
  "dual_file": "bin/tinyc",
  "toolchain_include": "include",
  "libtcc1": "lib/tcc/libtcc1.a",
  "libc": "Darwin libc headers bundled (vendor-darwin, self-written); programs link the system libSystem.B.dylib dynamically",
  "libc_headers": "lib/tcc/include-libc",
  "libc_lib": "lib/tcc/lib"
}
EOF
    add_default_libs "$TRIPLE" "$DIST"
    echo "built: $DIST"
    file "$DIST/bin/tinyc" | sed 's/^/  /'
}

# build_wasm32
# The wasm32-wasi target: the dual-form file runs on this (Linux)
# host and targets WebAssembly, using the project's wasm backend
# (wasm-backend/): the source copy takes the project patches and
# then the backend sources + backend patches (the two sets apply
# cleanly in that order), and configure's config.h guard is
# extended with TCC_TARGET_WASM32, the same edit
# wasm-backend/build.sh makes. The target has no libtcc1.a: in
# the backend's model the runtime helpers are host-provided
# imports, so the sysroot carries headers only - the
# wasm-backend/include set (the C library declarations and the
# SIMD compatibility headers) plus TCC's builtin headers, the
# same header set the browser package's wasm32 sysroot ships.
build_wasm32() {
    local TRIPLE=wasm32-wasi
    local DIST=$ROOT/build/dist/$TRIPLE
    local SRC=$ROOT/build/work/$TRIPLE/src

    echo "===== $TRIPLE ====="
    rm -rf "$ROOT/build/work/$TRIPLE" "$DIST"
    mkdir -p "$SRC" "$DIST/bin" "$DIST/include" "$DIST/lib/tcc/include"

    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C vendor-tinycc . | tar xf - --no-same-owner -C "$SRC"
    cp "$ROOT/core/tinyc_io.h" "$ROOT/core/tcc_io.c" "$SRC/"
    python3 "$ROOT/patches/apply_patches.py" "$SRC" >/dev/null
    cp "$ROOT/wasm-backend/src/wasm-gen.c" \
       "$ROOT/wasm-backend/src/wasm-link.c" \
       "$ROOT/wasm-backend/src/wasm-int.h" "$SRC/"
    python3 "$ROOT/wasm-backend/patches/apply-patches.py" "$SRC" >/dev/null

    (cd "$SRC" && ./configure --prefix="$DIST" >/dev/null)
    grep -q 'TCC_TARGET_C67)' "$SRC/config.h"
    sed -i 's/|| TCC_TARGET_C67)/|| TCC_TARGET_C67 || TCC_TARGET_WASM32)/' \
        "$SRC/config.h"
    grep -q 'TCC_TARGET_WASM32' "$SRC/config.h"
    # native build: generates the files the dual-form compile
    # needs (config.h consumers, tccdefs_.h); no cross rule is
    # used - the target runtime is not an archive (see above).
    (cd "$SRC" && make -j"$(nproc)" >/dev/null)

    cp "$ROOT"/wasm-backend/include/*.h "$DIST/lib/tcc/include/"
    cp "$SRC"/include/*.h "$SRC/tcclib.h" "$DIST/lib/tcc/include/"
    cp "$SRC/libtcc.h" "$DIST/include/"

    gcc -O2 -fPIC -pie -Wl,--export-dynamic \
        -o "$DIST/bin/tinyc" "$SRC/tcc.c" "$ROOT/core/tcc_io.c" \
        -I"$SRC" -I"$ROOT/core" -DTCC_TARGET_WASM32 \
        '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include"' \
        '-DCONFIG_TCC_LIBPATHS="{B}"' \
        '-DCONFIG_TCC_CRTPREFIX="{B}"' \
        -ldl -lm -lpthread
    python3 "$ROOT/tools/clear_pie_flag.py" "$DIST/bin/tinyc"
    ln -sf tinyc "$DIST/bin/tcc"
    cp "$ROOT/core/tinyc_io.h" "$DIST/include/"

    cat > "$DIST/manifest.json" <<EOF
{
  "name": "tinyc",
  "version": "$VER",
  "target": "$TRIPLE",
  "isa": "WebAssembly MVP with SIMD128 (v128); no threads/atomics target variant",
$(print_outputs wasm)
  "dual_file": "bin/tinyc",
  "toolchain_include": "include",
  "libc": "no target libc archive: in the wasm backend's model the runtime helpers are host-provided imports; the sysroot carries the bundled headers only (wasm-backend/include and TCC's builtin headers)"
}
EOF
    echo "built: $DIST"
    file "$DIST/bin/tinyc" | sed 's/^/  /'
}

# -------------------------------------------------------------------
# Host variants: ./build.sh host [host-triple ...]
# -------------------------------------------------------------------

# build_host_linux <triple> <cross-name> <host-gcc> <target-def> <isa> <defs...>
# Same recipe as build_one's cross path (native build for generated
# files, cross make target for libtcc1.a), except the dual-form file
# itself is compiled with the host's cross C compiler, so the result
# RUNS on the host triple and targets it.
build_host_linux() {
    local TRIPLE=$1 CT=$2 HGC=$3 TDEF=$4 ISA=$5
    shift 5
    local DEFS=("$@")
    local DIST=$ROOT/build/dist-host/$TRIPLE
    local SRC=$ROOT/build/work-host/$TRIPLE/src

    echo "===== host $TRIPLE (cross C compiler: $HGC) ====="
    rm -rf "$ROOT/build/work-host/$TRIPLE" "$DIST"
    mkdir -p "$SRC" "$DIST/bin" "$DIST/include" "$DIST/lib/tcc/include"

    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C vendor-tinycc . | tar xf - --no-same-owner -C "$SRC"
    cp "$ROOT/core/tinyc_io.h" "$ROOT/core/tcc_io.c" "$SRC/"
    python3 "$ROOT/patches/apply_patches.py" "$SRC" >/dev/null

    (cd "$SRC" && ./configure --prefix="$DIST" >/dev/null)
    (cd "$SRC" && make -j"$(nproc)" >/dev/null)
    (cd "$SRC" && make "cross-$CT" >/dev/null)
    cp "$SRC/$CT-libtcc1.a" "$DIST/lib/tcc/libtcc1.a"
    cp "$SRC"/include/*.h "$SRC/tcclib.h" "$DIST/lib/tcc/include/"
    cp "$SRC/libtcc.h" "$DIST/include/"
    add_glibc_to_dist "$DIST" "$TRIPLE"

    "$HGC" -O2 -fPIC -pie -Wl,--export-dynamic \
        -o "$DIST/bin/tinyc" "$SRC/tcc.c" "$ROOT/core/tcc_io.c" \
        -I"$SRC" -I"$ROOT/core" "$TDEF" "${DEFS[@]}" -ldl -lm -lpthread
    python3 "$ROOT/tools/clear_pie_flag.py" "$DIST/bin/tinyc"
    ln -sf tinyc "$DIST/bin/tcc"
    cp "$ROOT/core/tinyc_io.h" "$DIST/include/"

    local LIBC_JSON=",
  \"libc\": \"glibc $GLIBC_VER headers and crt objects bundled; programs link the system libc.so.6/libm.so.6 dynamically\",
  \"libc_headers\": \"lib/tcc/include-libc\",
  \"libc_lib\": \"lib/tcc/libc\""
    cat > "$DIST/manifest.json" <<EOF
{
  "name": "tinyc",
  "version": "$VER",
  "host": "$TRIPLE",
  "target": "$TRIPLE",
  "isa": "$ISA",
$(print_outputs elf)
  "dual_file": "bin/tinyc",
  "toolchain_include": "include",
  "libtcc1": "lib/tcc/libtcc1.a"$LIBC_JSON,
  "built_on": "$HOST_ARCH-linux-gnu host, C compiler $HGC",
  "verification": "host smoke test (native execution when the variant matches the test host, qemu-user otherwise): tests/test_hosts.sh"
}
EOF
    TINYC_LIBS_TCC="$SRC/$CT-tcc" add_default_libs "$TRIPLE" "$DIST"
    echo "built: $DIST"
    file "$DIST/bin/tinyc" | sed 's/^/  /'
}

# build_host_mingw [cc]
# The Windows dual-form file: an EXE with an export table (main,
# tinyc_main, tinyc_init and the tcc_* API, listed in a .def generated
# from libtcc.h). The PE entry point is tinyc_win_entry (core/
# tinyc_win.c), not mainCRTStartup: the loader calls the entry point
# with the DllMain convention when a host LoadLibrary's the EXE, and
# the dispatch forwards to mainCRTStartup only when the image is the
# process's main module; on the library path it returns at once and
# the host calls the exported tinyc_init() after loading, which runs
# the deferred CRT initialization (design doc 3.2). CONFIG_TCCDIR is
# left undefined so libtcc resolves the tcc directory from the
# executable's own location at runtime. The same recipe runs with a
# native Windows MinGW gcc (TINYC_MINGW_CC=gcc).
build_host_mingw() {
    local CC=${1:-${TINYC_MINGW_CC:-x86_64-w64-mingw32-gcc}}
    local TRIPLE=x86_64-w64-mingw32
    local DIST=$ROOT/build/dist-host/$TRIPLE
    local SRC=$ROOT/build/work-host/$TRIPLE/src

    echo "===== host $TRIPLE (C compiler: $CC) ====="
    rm -rf "$ROOT/build/work-host/$TRIPLE" "$DIST"
    mkdir -p "$SRC" "$DIST/bin" "$DIST/include" \
        "$DIST/lib/tcc/win32/lib" "$DIST/lib/tcc/win32/include"

    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C vendor-tinycc . | tar xf - --no-same-owner -C "$SRC"
    cp "$ROOT/core/tinyc_io.h" "$ROOT/core/tcc_io.c" "$SRC/"
    python3 "$ROOT/patches/apply_patches.py" "$SRC" >/dev/null

    (cd "$SRC" && ./configure --prefix="$DIST" >/dev/null)
    # Windows builds resolve the tcc dir from the exe location at
    # runtime, so config.h must not define CONFIG_TCCDIR. A configure
    # run on a Linux host bakes one in (the --prefix path); strip it.
    # A configure run on a Windows host defines none (upstream marks
    # the tcc dir auto-resolved there). Either way the end state is
    # asserted, not the edit.
    python3 - "$SRC/config.h" <<'PYEOF'
import re, sys
p = sys.argv[1]
t = open(p).read()
t2 = re.sub(r'#ifndef CONFIG_TCCDIR\n#define CONFIG_TCCDIR "[^"]*"\n#endif\n', '', t)
assert '#define CONFIG_TCCDIR' not in t2, "CONFIG_TCCDIR still defined in config.h"
open(p, 'w').write(t2)
PYEOF
    # Only the cross target is built. The recipe consumes
    # x86_64-win32-libtcc1.a and the generated tccdefs_.h, both of
    # which the cross build produces; everything else it copies is a
    # pristine source file. A full make is not an option: on a
    # Windows-configured tree it also links libtcc.dll from
    # $(LIBTCC_OBJ), which includes tcc_io.o, whose tinyc_main_io
    # references tinyc_main - a symbol that lives in tcc.c, outside
    # libtcc. ELF shared libraries tolerate the undefined symbol (the
    # final dual-form link resolves it); PE DLLs do not, so the full
    # make dies on a Windows host. No libtcc.dll is ever shipped:
    # the dual-form exe is the product.
    (cd "$SRC" && make cross-x86_64-win32 >/dev/null)
    # runmain.o is the support object tcc_run loads for its entry
    # shim. lib/Makefile builds it only in a native lib build (its
    # COMMON_O gains runmain.o only when the lib build is not a
    # cross build), so the cross target above does not produce it.
    # Build it here with the cross compiler, with the flags of
    # lib/Makefile's own rule for it (-B win32, the include dirs),
    # and ship it next to libtcc1.a in both layouts below.
    XTCC="$SRC/x86_64-win32-tcc"
    [ -x "$XTCC" ] || XTCC="$SRC/x86_64-win32-tcc.exe"
    "$XTCC" -B"$SRC/win32" -I"$SRC/include" -I"$SRC" \
        -c "$SRC/lib/runmain.c" -o "$SRC/runmain.o"
    add_win32_pthread "$SRC"
    cp "$SRC/x86_64-win32-libtcc1.a" "$DIST/lib/tcc/win32/lib/libtcc1.a"
    cp "$SRC/runmain.o" "$DIST/lib/tcc/win32/lib/"
    cp "$SRC"/win32/lib/*.def "$DIST/lib/tcc/win32/lib/"
    cp "$SRC"/include/*.h "$SRC/tcclib.h" "$DIST/lib/tcc/win32/include/"
    cp -r "$SRC"/win32/include/. "$DIST/lib/tcc/win32/include/"
    cp "$SRC/libtcc.h" "$DIST/include/"
    cp "$ROOT/core/tinyc_io.h" "$DIST/include/"

    python3 "$ROOT/tools/gen_windows_def.py" "$SRC/libtcc.h" "$SRC/tinyc.def"
    "$CC" -O2 -static -Wl,-e,tinyc_win_entry \
        -o "$DIST/bin/tinyc.exe" "$SRC/tcc.c" "$SRC/tcc_io.c" \
        "$ROOT/core/tinyc_win.c" \
        -I"$SRC" -I"$ROOT/core" \
        -DTCC_TARGET_X86_64 -DTCC_TARGET_PE \
        "$SRC/tinyc.def" -lm
    ln -sf tinyc.exe "$DIST/bin/tcc.exe"
    # Stage the default libraries before the companions below
    # are copied: the companions mirror the staged win32 tree,
    # so anything staged afterwards (the three libraries'
    # headers and archives, the bcrypt compat header and .def)
    # would be missing from the exe's own search paths.
    add_default_libs "$TRIPLE" "$DIST"
    # Companions next to the exe, in the places libtcc actually
    # searches with CONFIG_TCCDIR undefined: the runtime tccdir {B}
    # is the exe's own directory, and the PE defaults (tcc.h) put
    # the library path at {B}/lib - libtcc1.a and the default
    # libraries' archives are looked up there by tcc_add_library,
    # runmain.o there by tcc_add_support, the .def files there
    # too (the staged set, which adds bcrypt.def to the vendored
    # files) - and the include paths at {B}/include and
    # {B}/include/winapi.
    mkdir -p "$DIST/bin/lib"
    cp "$DIST/lib/tcc/win32/lib/"*.a "$DIST/bin/lib/"
    cp "$DIST/lib/tcc/win32/lib/runmain.o" "$DIST/bin/lib/"
    cp "$DIST/lib/tcc/win32/lib/"*.def "$DIST/bin/lib/"
    cp -r "$DIST/lib/tcc/win32/include" "$DIST/bin/include"

    cat > "$DIST/manifest.json" <<EOF
{
  "name": "tinyc",
  "version": "$VER",
  "host": "$TRIPLE",
  "target": "$TRIPLE",
  "isa": "$ISA_X86_64",
$(print_outputs pe)
  "dual_file": "bin/tinyc.exe",
  "toolchain_include": "include",
  "libtcc1": "lib/tcc/win32/lib/libtcc1.a",
  "built_on": "cross C compiler $CC",
  "verification": "PE format and export table checked with objdump; execution and the LoadLibrary path are exercised on the Windows CI runner (tests/consumer/win_host_test.c)"
}
EOF
    echo "built: $DIST"
    file "$DIST/bin/tinyc.exe" | sed 's/^/  /'
}

# build_host_mingw_for <triple> <cross-name> <target-def> <isa> [extra -D flags...]
# A Windows-host dual-form PE compiler for one target triple: the
# dual-form file is a PE .exe, so it runs on Windows, but the
# compiler inside targets $TRIPLE. Same recipe as build_host_mingw
# (the native win32 variant, which stays the single source for
# x86_64-w64-mingw32), with the target defines and baked search
# paths of the Linux dist for that target family:
#   linux-gnu  gnu_defs (cross form) + CONFIG_SYSROOT
#   android    the four CONFIG_TCC_* paths of the main dispatch
#   darwin     the three CONFIG_TCC_* paths of build_darwin
#   wasm32     the three CONFIG_TCC_* paths of build_wasm32
# Only the C compiler is Windows-specific: MinGW gcc, native on the
# Windows runner or the x86_64-w64-mingw32 cross compiler on Linux
# (TINYC_MINGW_CC). The PE file is statically linked; the .exe
# needs no DLL beyond the system ones. The cross make that produces
# libtcc1.a and tccdefs_.h runs with the machine's own compiler
# (configure's ambient CC), so the cross compiler it builds is a
# binary that runs on the build machine - the MinGW compiler is
# used only for the PE unity compile. One deliberate divergence
# from build_host_mingw: the .def export list is filtered to the
# symbols the compiled objects actually define, because a cross
# compiler does not define the -run support (see the note at the
# PE compile step below).
#
# The dist lands in build/dist-host/<triple>/ with the same
# internal layout as build/dist/<triple>/ (bin/tinyc.exe,
# include/, lib/tcc/...), plus bin/ companions (bin/lib,
# bin/include, and the family's include-libc/libc/lib trees) so
# the .exe also works when run directly: the baked paths are
# package-relative ({B}/...), and {B} is the directory holding
# the .exe.
#
# One cross-target link-time condition, identical on every host:
# linking a hosted executable for a linux-gnu target needs the
# target's runtime libc (libc.so.6) findable through the baked
# lib paths - on Linux that is the cross sysroot's lib directory,
# which Windows does not have. On the Windows host, the three
# linux-gnu targets compile to object and link freestanding
# programs; linking a hosted linux-gnu executable needs a sysroot
# that supplies libc.so.6. The darwin targets link against the
# bundled .tbd stubs, the android targets against the bundled
# bionic stubs, wasm32 needs no libc, and the win32 target links
# against msvcrt.dll - those six link fully on Windows.
build_host_mingw_for() {
    local TRIPLE=$1 CT=$2 TDEF=$3 ISA=$4
    shift 4
    local DEFS=("$@")
    local CC=${TINYC_MINGW_CC:-x86_64-w64-mingw32-gcc}
    local DIST=$ROOT/build/dist-host/$TRIPLE
    local SRC=$ROOT/build/work-host/$TRIPLE/src
    local XTCC LIBTCC1_LINE COMP

    echo "===== host-win $TRIPLE (C compiler: $CC) ====="
    rm -rf "$ROOT/build/work-host/$TRIPLE" "$DIST"
    mkdir -p "$SRC" "$DIST/bin" "$DIST/include" \
        "$DIST/lib/tcc/include" "$DIST/lib/tcc/lib"

    # Fresh source tree, same preparation as build_host_mingw.
    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C vendor-tinycc . | tar xf - --no-same-owner -C "$SRC"
    cp "$ROOT/core/tinyc_io.h" "$ROOT/core/tcc_io.c" "$SRC/"
    python3 "$ROOT/patches/apply_patches.py" "$SRC" >/dev/null

    (cd "$SRC" && ./configure --prefix="$DIST" >/dev/null)
    # Windows builds resolve the tcc dir from the exe location at
    # runtime, so config.h must not define CONFIG_TCCDIR. A configure
    # run on a Linux host bakes one in (the --prefix path); strip it.
    # A configure run on a Windows host defines none (upstream marks
    # the tcc dir auto-resolved there). Either way the end state is
    # asserted, not the edit.
    python3 - "$SRC/config.h" <<'PYEOF'
import re, sys
p = sys.argv[1]
t = open(p).read()
t2 = re.sub(r'#ifndef CONFIG_TCCDIR\n#define CONFIG_TCCDIR "[^"]*"\n#endif\n', '', t)
assert '#define CONFIG_TCCDIR' not in t2, "CONFIG_TCCDIR still defined in config.h"
open(p, 'w').write(t2)
PYEOF

    # Target runtime library and entry object, and the target's
    # sysroot. wasm32 has no libtcc1.a: its helpers are host
    # imports, so there is nothing to build or stage.
    case $TRIPLE in
        wasm32-wasi)
            cp "$ROOT/wasm-backend/src/wasm-gen.c" \
               "$ROOT/wasm-backend/src/wasm-link.c" \
               "$ROOT/wasm-backend/src/wasm-int.h" "$SRC/"
            python3 "$ROOT/wasm-backend/patches/apply-patches.py" "$SRC" >/dev/null
            grep -q 'TCC_TARGET_C67)' "$SRC/config.h"
            sed -i 's/|| TCC_TARGET_C67)/|| TCC_TARGET_C67 || TCC_TARGET_WASM32)/' \
                "$SRC/config.h"
            grep -q 'TCC_TARGET_WASM32' "$SRC/config.h"
            # Only the generated header the dual-form compile needs:
            # a native make is not an option on this host (on a
            # Windows-configured tree it dies linking libtcc.dll).
            (cd "$SRC" && make tccdefs_.h >/dev/null)
            cp "$ROOT"/wasm-backend/include/*.h "$DIST/lib/tcc/include/"
            ;;
        *-linux-gnu)
            add_glibc_to_dist "$DIST" "$TRIPLE"
            ;;
        *-linux-android)
            add_android_to_dist "$DIST" "$TRIPLE"
            ;;
        *-apple-darwin)
            python3 "$ROOT/mac-test-bundle/darwin-dist.py" stage \
                "$ROOT/vendor-darwin" "$DIST"
            ;;
    esac
    case $TRIPLE in
        wasm32-wasi) : ;;
        *)
            # Only the cross target is built. A full make is not an
            # option: on a Windows-configured tree it also links
            # libtcc.dll from $(LIBTCC_OBJ), which includes tcc_io.o,
            # whose tinyc_main_io references tinyc_main - a symbol
            # that lives in tcc.c, outside libtcc. ELF shared
            # libraries tolerate the undefined symbol (the final
            # dual-form link resolves it); PE DLLs do not, so the
            # full make dies on a Windows host. No libtcc.dll is
            # ever shipped: the dual-form exe is the product.
            (cd "$SRC" && make "cross-$CT" >/dev/null)
            # runmain.o is the support object tcc_run loads for its
            # entry shim. lib/Makefile builds it only in a native
            # lib build, so the cross target above does not produce
            # it. Build it here with the cross compiler, with the
            # flags of lib/Makefile's own rule for it (the include
            # dirs), and ship it next to libtcc1.a.
            XTCC="$SRC/$CT-tcc"
            [ -x "$XTCC" ] || XTCC="$SRC/$CT-tcc.exe"
            "$XTCC" -B"$SRC" -I"$SRC/include" -I"$SRC" \
                -c "$SRC/lib/runmain.c" -o "$SRC/runmain.o"
            cp "$SRC/$CT-libtcc1.a" "$DIST/lib/tcc/lib/libtcc1.a"
            cp "$SRC/runmain.o" "$DIST/lib/tcc/lib/"
            ;;
    esac

    # TCC builtin headers and libtcc.h.
    cp "$SRC"/include/*.h "$SRC/tcclib.h" "$DIST/lib/tcc/include/"
    cp "$SRC/libtcc.h" "$DIST/include/"
    cp "$ROOT/core/tinyc_io.h" "$DIST/include/"

    # The dual-form PE compiler itself. The export list is derived
    # from libtcc.h, but a cross compiler does not define the -run
    # support: tccrun.c is compiled only for a native compiler (the
    # target's code cannot run on the host CPU), so tcc_run (except
    # the wasm32 stub), tcc_relocate, _tcc_setjmp and
    # tcc_set_backtrace_func are undefined here. The ELF cross dists
    # ship the same way - their dual-form files simply lack those
    # symbols - but a PE link fails on an undefined export, so the
    # .def is filtered to the symbols the compiled objects actually
    # define. (The native win32 recipe needs no filter: every
    # exported name is defined there.)
    python3 "$ROOT/tools/gen_windows_def.py" "$SRC/libtcc.h" "$SRC/tinyc.def"
    "$CC" -O2 -c "$SRC/tcc.c" -I"$SRC" -I"$ROOT/core" \
        $TDEF "${DEFS[@]}" -o "$SRC/tcc.o"
    "$CC" -O2 -c "$SRC/tcc_io.c" -I"$SRC" -I"$ROOT/core" \
        $TDEF "${DEFS[@]}" -o "$SRC/tcc_io.o"
    "$CC" -O2 -c "$ROOT/core/tinyc_win.c" -I"$SRC" -I"$ROOT/core" \
        $TDEF "${DEFS[@]}" -o "$SRC/tinyc_win.o"
    NM=${TINYC_MINGW_NM:-${CC%gcc}nm}
    "$NM" --defined-only "$SRC/tcc.o" "$SRC/tcc_io.o" "$SRC/tinyc_win.o" \
        | python3 -c 'import sys; print("\n".join(sorted({l.split()[-1] for l in sys.stdin if len(l.split()) >= 3})))' \
        > "$SRC/defined.txt"
    python3 - "$SRC/tinyc.def" "$SRC/defined.txt" <<'PYEOF'
import sys
defp, definedp = sys.argv[1], sys.argv[2]
defined = set(open(definedp).read().split())
out, dropped = ["EXPORTS"], []
for line in open(defp):
    name = line.strip()
    if not name or name == "EXPORTS":
        continue
    if name in defined:
        out.append("    " + name)
    else:
        dropped.append(name)
open(defp, "w").write("\n".join(out) + "\n")
if dropped:
    print("gen_windows_def: dropped %d undefined exports for this target: %s"
          % (len(dropped), ", ".join(dropped)), file=sys.stderr)
PYEOF
    "$CC" -O2 -static -Wl,-e,tinyc_win_entry \
        -o "$DIST/bin/tinyc.exe" "$SRC/tcc.o" "$SRC/tcc_io.o" "$SRC/tinyc_win.o" \
        "$SRC/tinyc.def" -lm
    ln -sf tinyc.exe "$DIST/bin/tcc.exe"

    # Stage the default libraries before the companions below
    # are mirrored: the mirror copies the staged lib/tcc trees,
    # so anything staged afterwards would be missing from the
    # exe's own search paths (same ordering as build_host_mingw).
    add_default_libs "$TRIPLE" "$DIST"
    # Companions next to the exe, in the places libtcc actually
    # searches with CONFIG_TCCDIR undefined: the runtime tccdir {B}
    # is the exe's own directory, and the baked paths are
    # package-relative ({B}/...), so the family's include/lib trees
    # are mirrored under bin/. libtcc1.a and runmain.o are looked up
    # at {B}/lib.
    mkdir -p "$DIST/bin/include"
    cp "$DIST/lib/tcc/include/"*.h "$DIST/bin/include/"
    for COMP in include-libc libc lib; do
        if [ -d "$DIST/lib/tcc/$COMP" ]; then
            cp -r "$DIST/lib/tcc/$COMP" "$DIST/bin/"
        fi
    done

    # Manifest. wasm32 carries the no-libc note of build_wasm32
    # instead of a libtcc1 path.
    if [ "$TRIPLE" != "wasm32-wasi" ]; then
        LIBTCC1_LINE="  \"libtcc1\": \"lib/tcc/lib/libtcc1.a\","
    else
        LIBTCC1_LINE="  \"libc\": \"no target libc archive: in the wasm backend's model the runtime helpers are host-provided imports; the sysroot carries the bundled headers only (wasm-backend/include and TCC's builtin headers)\","
    fi
    cat > "$DIST/manifest.json" <<EOF
{
  "name": "tinyc",
  "version": "$VER",
  "host": "x86_64-w64-mingw32",
  "target": "$TRIPLE",
  "isa": "$ISA",
$(print_outputs pe)
  "dual_file": "bin/tinyc.exe",
  "toolchain_include": "include",
$LIBTCC1_LINE
  "built_on": "MinGW cross C compiler $CC",
  "verification": "PE format and export table checked with objdump; per-target compile smoke is exercised on the Windows CI runner (see HOST-CROSS-BUILDS.md)"
}
EOF
    echo "built: $DIST"
    file "$DIST/bin/tinyc.exe" | sed 's/^/  /'
}

# build_host_for <triple> <cross-name> <target-def> <isa>
# A Linux host variant is built with the plain host compiler when
# its CPU is the build host's CPU; otherwise with the <triplet>-gcc
# cross compiler and a /usr/<triplet> sysroot. Either way the baked
# search paths are the package paths (gnu_defs): the bundled glibc
# headers and crt objects, plus the system library directory that
# carries the runtime shared libraries on the machine the variant
# runs on (/usr/lib/<triplet> natively, the cross sysroot's lib
# directory for a cross-built variant).
build_host_for() {
    local H=$1 CT=$2 TDEF=$3 ISA=$4
    if [ "${H%%-*}" = "$HOST_ARCH" ]; then
        # Native: the dual-form file is compiled with the host
        # compiler. Leaving the search paths to configure would
        # bake the build host's header and crt directories instead
        # of the package paths.
        gnu_defs "/usr/lib/$H"
        build_host_linux "$H" "$CT" gcc "$TDEF" "$ISA" \
            "${GNU_DEFS[@]}"
    else
        gnu_defs "/usr/$H/lib"
        build_host_linux "$H" "$CT" "$H-gcc" "$TDEF" "$ISA" \
            "-DCONFIG_SYSROOT=\"/usr/$H\"" \
            "${GNU_DEFS[@]}"
    fi
}

if [ "${1:-}" = "host" ]; then
    shift
    HOSTS=${@:-"aarch64-linux-gnu x86_64-w64-mingw32"}
    for H in $HOSTS; do
        case $H in
            x86_64-linux-gnu)
                build_host_for "$H" x86_64 "-DTCC_TARGET_X86_64" "$ISA_X86_64"
                ;;
            aarch64-linux-gnu)
                build_host_for "$H" arm64 "-DTCC_TARGET_ARM64" "$ISA_ARM64"
                ;;
            x86_64-w64-mingw32)
                build_host_mingw
                ;;
            *)
                echo "unknown host triple: $H" >&2
                exit 1
                ;;
        esac
    done
    echo "host build done"
    exit 0
fi

# -------------------------------------------------------------------
# Windows host, all targets: ./build.sh host-win [target-triple ...]
# -------------------------------------------------------------------
if [ "${1:-}" = "host-win" ]; then
    shift
    HOSTS=${@:-"$ALL"}
    for H in $HOSTS; do
        case $H in
            x86_64-w64-mingw32)
                # The native triple keeps its proven recipe.
                build_host_mingw
                ;;
            x86_64-linux-gnu)
                gnu_defs "/usr/x86_64-linux-gnu/lib" ";"
                build_host_mingw_for "$H" x86_64 "-DTCC_TARGET_X86_64" "$ISA_X86_64" \
                    '-DCONFIG_SYSROOT="/usr/x86_64-linux-gnu"' \
                    "${GNU_DEFS[@]}"
                ;;
            aarch64-linux-gnu)
                gnu_defs "/usr/aarch64-linux-gnu/lib" ";"
                build_host_mingw_for "$H" arm64 "-DTCC_TARGET_ARM64" "$ISA_ARM64" \
                    '-DCONFIG_SYSROOT="/usr/aarch64-linux-gnu"' \
                    "${GNU_DEFS[@]}"
                ;;
            riscv64-linux-gnu)
                gnu_defs "/usr/riscv64-linux-gnu/lib" ";"
                build_host_mingw_for "$H" riscv64 "-DTCC_TARGET_RISCV64" "$ISA_RISCV64" \
                    '-DCONFIG_SYSROOT="/usr/riscv64-linux-gnu"' \
                    "${GNU_DEFS[@]}"
                ;;
            aarch64-linux-android)
                build_host_mingw_for "$H" arm64 "-DTCC_TARGET_ARM64 -DTARGETOS_ANDROID=1" "$ISA_ARM64" \
                    '-DCONFIG_TCC_CRTPREFIX="{B}/libc"' \
                    '-DCONFIG_TCC_LIBPATHS="{B}/libc;{B}"' \
                    '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include;{B}/include-libc;{B}/include-libc/aarch64-linux-android"' \
                    '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"'
                ;;
            x86_64-linux-android)
                build_host_mingw_for "$H" x86_64 "-DTCC_TARGET_X86_64 -DTARGETOS_ANDROID=1" "$ISA_X86_64" \
                    '-DCONFIG_TCC_CRTPREFIX="{B}/libc"' \
                    '-DCONFIG_TCC_LIBPATHS="{B}/libc;{B}"' \
                    '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include;{B}/include-libc;{B}/include-libc/x86_64-linux-android"' \
                    '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"'
                ;;
            arm64-apple-darwin)
                build_host_mingw_for "$H" arm64-osx "-DTCC_TARGET_ARM64 -DTCC_TARGET_MACHO" "$ISA_ARM64" \
                    '-DCONFIG_TCC_CRTPREFIX="{B}/lib"' \
                    '-DCONFIG_TCC_LIBPATHS="{B}/lib;{B}"' \
                    '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include;{B}/include-libc"'
                ;;
            x86_64-apple-darwin)
                build_host_mingw_for "$H" x86_64-osx "-DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO" "$ISA_X86_64" \
                    '-DCONFIG_TCC_CRTPREFIX="{B}/lib"' \
                    '-DCONFIG_TCC_LIBPATHS="{B}/lib;{B}"' \
                    '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include;{B}/include-libc"'
                ;;
            wasm32-wasi)
                build_host_mingw_for "$H" "" "-DTCC_TARGET_WASM32" "WebAssembly MVP with SIMD128 (v128); no threads/atomics target variant" \
                    '-DCONFIG_TCC_CRTPREFIX="{B}"' \
                    '-DCONFIG_TCC_LIBPATHS="{B}"' \
                    '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include"'
                ;;
            *)
                echo "unknown host-win target: $H" >&2
                exit 1
                ;;
        esac
    done
    echo "host-win build done"
    exit 0
fi

# build_glibc <triple> <cross-name> <target-def> <isa>
# linux-gnu targets: native mode when the target CPU is the build
# host's CPU, cross mode otherwise; both modes bundle the target's
# glibc headers and crt objects (see add_glibc_to_dist) and bake
# the package search paths (gnu_defs).
build_glibc() {
    local TRIPLE=$1 CT=$2 TDEF=$3 ISA=$4
    if [ "${TRIPLE%%-*}" = "$HOST_ARCH" ]; then
        gnu_defs "/usr/lib/$TRIPLE"
        build_one "$TRIPLE" "" native "" "$ISA" \
            "${GNU_DEFS[@]}"
    else
        gnu_defs "/usr/$TRIPLE/lib"
        build_one "$TRIPLE" "$CT" cross "$TDEF" "$ISA" \
            "-DCONFIG_SYSROOT=\"/usr/$TRIPLE\"" \
            "${GNU_DEFS[@]}"
    fi
}

for TRIPLE in $TRIPLES; do
    case $TRIPLE in
        x86_64-linux-gnu)
            build_glibc "$TRIPLE" x86_64 "-DTCC_TARGET_X86_64" "$ISA_X86_64"
            ;;
        aarch64-linux-gnu)
            build_glibc "$TRIPLE" arm64 "-DTCC_TARGET_ARM64" "$ISA_ARM64"
            ;;
        riscv64-linux-gnu)
            gnu_defs "/usr/riscv64-linux-gnu/lib"
            build_one "$TRIPLE" riscv64 cross "-DTCC_TARGET_RISCV64" "$ISA_RISCV64" \
                '-DCONFIG_SYSROOT="/usr/riscv64-linux-gnu"' \
                "${GNU_DEFS[@]}"
            ;;
        x86_64-w64-mingw32)
            DIST_W=$ROOT/build/dist/$TRIPLE
            build_one "$TRIPLE" x86_64-win32 cross \
                "-DTCC_TARGET_X86_64 -DTCC_TARGET_PE" "$ISA_X86_64" \
                "-DCONFIG_TCCDIR=\"$DIST_W/lib/tcc/win32\""
            ;;
        aarch64-linux-android)
            build_one "$TRIPLE" arm64 cross \
                "-DTCC_TARGET_ARM64 -DTARGETOS_ANDROID=1" "$ISA_ARM64" \
                "-DCONFIG_TCC_CRTPREFIX=\"{B}/libc\"" \
                "-DCONFIG_TCC_LIBPATHS=\"{B}/libc:{B}\"" \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{B}/include-libc:{B}/include-libc/aarch64-linux-android"' \
                '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"'
            ;;
        x86_64-linux-android)
            # native mode only when the build host is x86_64: the
            # native make produces the host CPU's libtcc1.a, so on
            # any other host this target needs the cross build to
            # get an x86_64 libtcc1.a.
            if [ "$HOST_ARCH" = x86_64 ]; then
                X64_ANDROID_MODE=native X64_ANDROID_CT=""
            else
                X64_ANDROID_MODE=cross X64_ANDROID_CT=x86_64
            fi
            build_one "$TRIPLE" "$X64_ANDROID_CT" "$X64_ANDROID_MODE" \
                "-DTCC_TARGET_X86_64 -DTARGETOS_ANDROID=1" "$ISA_X86_64" \
                "-DCONFIG_TCC_CRTPREFIX=\"{B}/libc\"" \
                "-DCONFIG_TCC_LIBPATHS=\"{B}/libc:{B}\"" \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{B}/include-libc:{B}/include-libc/x86_64-linux-android"' \
                '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"'
            ;;
        arm64-apple-darwin)
            build_darwin "$TRIPLE" arm64-osx "-DTCC_TARGET_ARM64" "$ISA_ARM64"
            ;;
        x86_64-apple-darwin)
            build_darwin "$TRIPLE" x86_64-osx "-DTCC_TARGET_X86_64" "$ISA_X86_64"
            ;;
        wasm32-wasi)
            build_wasm32
            ;;
        *)
            echo "unknown triple: $TRIPLE" >&2
            exit 1
            ;;
    esac
done

echo "matrix build done"
