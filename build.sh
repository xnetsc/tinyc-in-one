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
#   aarch64-linux-android   cross, NDK bionic sysroot (see manifest)
#   x86_64-linux-android    native when the host is x86_64, else cross
#
# A linux-gnu target whose CPU matches the build host is built in
# native mode: upstream's configure finds the system sysroot itself.
# Any other linux-gnu target is built in cross mode with an explicit
# /usr/<triplet> sysroot, which the build host must provide (the
# cross libc packages). The same rule picks the compiler and sysroot
# for the host variants below.
#
# macOS hosts (arm64-apple-darwin, x86_64-apple-darwin) are recognized
# build entries: their dual-form kernel (PIE executable with an
# embedded dylib segment, see the design doc) is built on a macOS
# host. On any other host the build reports them as not built and
# produces no artifact.
#
# Host variants (the dual-form file compiled to RUN on another host,
# target == host) are built with the "host" subcommand:
#
#   ./build.sh host [host-triple ...]
#
# Supported here: the two linux-gnu triples and x86_64-w64-mingw32.
# A Linux variant whose CPU matches the build host is compiled with
# the host compiler and the system sysroot; any other Linux variant
# with the <triplet>-gcc cross compiler and a /usr/<triplet> sysroot
# (the binary is a native-configured TCC for that machine, verified
# under qemu-user). The MinGW variant uses a MinGW cross gcc; the
# recipe's CC can be overridden with TINYC_MINGW_CC, e.g. plain gcc
# on a Windows runner. Host variants land in build/dist-host/<triple>/
# with the same internal layout as build/dist plus a manifest that
# records host, target (== host) and how the build was produced.
#
# Each triple lands in build/dist/<triple>/ with the layout:
#   bin/tinyc            dual-form file (executable + dlopen-able)
#   include/             libtcc.h, tinyc_io.h
#   lib/tcc/libtcc1.a    target runtime library
#   lib/tcc/include/     TCC builtin headers
#   manifest.json        target, isa baseline, file locations
# The Windows variant keeps its support files under lib/tcc/win32/
# (include/ + lib/), mirroring upstream's win32 layout.
set -e
cd "$(dirname "$0")"
ROOT=$PWD

# Build host CPU (x86_64 | aarch64 | riscv64 on the Linux hosts this
# script supports); drives the native/cross choice described above.
HOST_ARCH=$(uname -m)

ALL="x86_64-linux-gnu aarch64-linux-gnu riscv64-linux-gnu x86_64-w64-mingw32 aarch64-linux-android x86_64-linux-android arm64-apple-darwin x86_64-apple-darwin"
if [ $# -gt 0 ]; then TRIPLES="$*"; else TRIPLES="$ALL"; fi

NDK_SYSROOT=${TINYC_NDK_SYSROOT:-$HOME/workspace/envs/android-toolchain/sdk/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot}
ANDROID_API=${TINYC_ANDROID_API:-30}
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
ISA_X86_64="x86-64 baseline; scalar floating point via SSE2 (xmm registers); no AVX code generation"
ISA_ARM64="ARMv8.0-A baseline (FP/NEON register file is architectural); scalar FP code generation only, no NEON vector instructions, no SVE"
ISA_RISCV64="RV64GC baseline (I/M/A/F/D/C); no Vector extension (RVV) support"

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
  \"sysroot\": \"$NDK_SYSROOT\",
  \"android_api\": $ANDROID_API"
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

    "$HGC" -O2 -fPIC -pie -Wl,--export-dynamic \
        -o "$DIST/bin/tinyc" "$SRC/tcc.c" "$ROOT/core/tcc_io.c" \
        -I"$SRC" -I"$ROOT/core" "$TDEF" "${DEFS[@]}" -ldl -lm -lpthread
    python3 "$ROOT/tools/clear_pie_flag.py" "$DIST/bin/tinyc"
    ln -sf tinyc "$DIST/bin/tcc"
    cp "$ROOT/core/tinyc_io.h" "$DIST/include/"

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
  "libtcc1": "lib/tcc/libtcc1.a",
  "built_on": "$HOST_ARCH-linux-gnu host, C compiler $HGC",
  "verification": "host smoke test (native execution when the variant matches the test host, qemu-user otherwise): tests/test_hosts.sh"
}
EOF
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
    # Companions next to the exe, in the places libtcc actually
    # searches with CONFIG_TCCDIR undefined: the runtime tccdir {B}
    # is the exe's own directory, and the PE defaults (tcc.h) put
    # the library path at {B}/lib - libtcc1.a and runmain.o are
    # looked up there by tcc_add_support, the .def files there by
    # tcc_add_library - and the include paths at {B}/include and
    # {B}/include/winapi.
    mkdir -p "$DIST/bin/lib"
    cp "$DIST/lib/tcc/win32/lib/libtcc1.a" "$DIST/bin/lib/"
    cp "$DIST/lib/tcc/win32/lib/runmain.o" "$DIST/bin/lib/"
    cp "$SRC"/win32/lib/*.def "$DIST/bin/lib/"
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

# build_host_for <triple> <cross-name> <target-def> <isa>
# A Linux host variant is built with the plain host compiler and no
# baked sysroot when its CPU is the build host's CPU; otherwise with
# the <triplet>-gcc cross compiler and a /usr/<triplet> sysroot.
build_host_for() {
    local H=$1 CT=$2 TDEF=$3 ISA=$4
    if [ "${H%%-*}" = "$HOST_ARCH" ]; then
        # Native: no sysroot is baked, but the system paths still
        # need the multiarch triplet directory. configure detects
        # the triplet and config.h carries it - inside a block
        # guarded on no TCC_TARGET_* being defined yet, which the
        # -DTCC_TARGET_* on the dual-form compile line suppresses.
        # Bake the paths the triplet would have produced (the same
        # values the matrix native build ends up with).
        build_host_linux "$H" "$CT" gcc "$TDEF" "$ISA" \
            "-DCONFIG_TCC_CRTPREFIX=\"/usr/lib/$H\"" \
            "-DCONFIG_TCC_LIBPATHS=\"{B}:/usr/lib/$H:/usr/lib\"" \
            "-DCONFIG_TCC_SYSINCLUDEPATHS=\"{B}/include:/usr/include/$H:/usr/include\""
    else
        build_host_linux "$H" "$CT" "$H-gcc" "$TDEF" "$ISA" \
            "-DCONFIG_SYSROOT=\"/usr/$H\"" \
            '-DCONFIG_TCC_CRTPREFIX="{R}/lib"' \
            '-DCONFIG_TCC_LIBPATHS="{R}/lib:{B}"' \
            '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{R}/include"'
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

# build_glibc <triple> <cross-name> <target-def> <isa>
# linux-gnu targets: native mode when the target CPU is the build
# host's CPU (configure finds the system sysroot), cross mode with a
# /usr/<triplet> sysroot otherwise.
build_glibc() {
    local TRIPLE=$1 CT=$2 TDEF=$3 ISA=$4
    if [ "${TRIPLE%%-*}" = "$HOST_ARCH" ]; then
        build_one "$TRIPLE" "" native "" "$ISA"
    else
        build_one "$TRIPLE" "$CT" cross "$TDEF" "$ISA" \
            "-DCONFIG_SYSROOT=\"/usr/$TRIPLE\"" \
            '-DCONFIG_TCC_CRTPREFIX="{R}/lib"' \
            '-DCONFIG_TCC_LIBPATHS="{R}/lib:{B}"' \
            '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{R}/include"'
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
            build_one "$TRIPLE" riscv64 cross "-DTCC_TARGET_RISCV64" "$ISA_RISCV64" \
                '-DCONFIG_SYSROOT="/usr/riscv64-linux-gnu"' \
                '-DCONFIG_TCC_CRTPREFIX="{R}/lib"' \
                '-DCONFIG_TCC_LIBPATHS="{R}/lib:{B}"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{R}/include"'
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
                "-DCONFIG_SYSROOT=\"$NDK_SYSROOT\"" \
                "-DCONFIG_TCC_CRTPREFIX=\"{R}/usr/lib/aarch64-linux-android/$ANDROID_API\"" \
                "-DCONFIG_TCC_LIBPATHS=\"{R}/usr/lib/aarch64-linux-android/$ANDROID_API:{B}\"" \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{R}/usr/include:{R}/usr/include/aarch64-linux-android"' \
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
                "-DCONFIG_SYSROOT=\"$NDK_SYSROOT\"" \
                "-DCONFIG_TCC_CRTPREFIX=\"{R}/usr/lib/x86_64-linux-android/$ANDROID_API\"" \
                "-DCONFIG_TCC_LIBPATHS=\"{R}/usr/lib/x86_64-linux-android/$ANDROID_API:{B}\"" \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{R}/usr/include:{R}/usr/include/x86_64-linux-android"' \
                '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"'
            ;;
        arm64-apple-darwin|x86_64-apple-darwin)
            echo "===== $TRIPLE ====="
            echo "not built: the macOS dual-form kernel is built on a macOS host; this machine produces no artifact for it"
            ;;
        *)
            echo "unknown triple: $TRIPLE" >&2
            exit 1
            ;;
    esac
done

echo "matrix build done"
