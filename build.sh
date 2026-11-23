#!/bin/bash
# Build the TinyC dual-form kernel matrix.
#
# Usage: ./build.sh [triple ...]
# With no arguments the full matrix is built. Hosts and targets are
# 64-bit only. The dual-form files produced here run on this x86_64
# Linux host; each one is a TCC whose target is its triple (upstream
# TCC is one binary per target):
#
#   x86_64-linux-gnu        native, host == target
#   aarch64-linux-gnu       cross, glibc sysroot in /usr/aarch64-linux-gnu
#   riscv64-linux-gnu       cross, glibc sysroot in /usr/riscv64-linux-gnu
#   x86_64-w64-mingw32      cross, PE output (upstream x86_64-win32 target)
#   aarch64-linux-android   cross, NDK bionic sysroot (see manifest)
#   x86_64-linux-android    cross, NDK bionic sysroot (see manifest)
#
# macOS hosts (arm64-apple-darwin, x86_64-apple-darwin) are recognized
# build entries: their dual-form kernel (PIE executable with an
# embedded dylib segment, see the design doc) is built on a macOS
# host. On any other host the build reports them as not built and
# produces no artifact.
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

for TRIPLE in $TRIPLES; do
    case $TRIPLE in
        x86_64-linux-gnu)
            build_one "$TRIPLE" "" native "" "$ISA_X86_64"
            ;;
        aarch64-linux-gnu)
            build_one "$TRIPLE" arm64 cross "-DTCC_TARGET_ARM64" "$ISA_ARM64" \
                '-DCONFIG_SYSROOT="/usr/aarch64-linux-gnu"' \
                '-DCONFIG_TCC_CRTPREFIX="{R}/lib"' \
                '-DCONFIG_TCC_LIBPATHS="{R}/lib:{B}"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{R}/include"'
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
            build_one "$TRIPLE" "" native \
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
