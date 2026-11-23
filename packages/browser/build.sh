#!/bin/sh
# Build TinyC (TCC) to WebAssembly with Emscripten.
#
# Prerequisite: an activated Emscripten SDK (emcc on PATH). This
# script does not install anything; without emcc it exits with an
# error. See README.md.
#
# Output: tcc-<target>.mjs (ES6 MODULARIZE factory) + tcc-<target>.wasm,
# next to this script, which is exactly what index.mjs loads for that
# target (glue file tcc-<target>.mjs, wasm file tcc-<target>.wasm).
#
# What is being built: the dual-form TCC - the libtcc sources plus
# tcc.c, so the module exports both the fine-grained tcc_* API and
# the command-line main (as _main; _tinyc_main is the alias added by
# the dual-form patch and is exported when present).
#
# Code generation target: a TCC build has exactly one target, picked
# at compile time via TCC_TARGET_* macros, so multi-target support
# means one run of this script per target:
#   TARGET=x86_64-linux-gnu ./build.sh
#   TARGET=aarch64-linux-gnu ./build.sh
# TARGET defaults to x86_64-linux-gnu. The wasm32-wasi build needs a
# wasm code generation backend in TCC, which does not exist yet; the
# script says so and stops for that target.
set -eu

cd "$(dirname "$0")"

if ! command -v emcc >/dev/null 2>&1; then
    echo "build.sh: emcc not found. Install and activate the Emscripten SDK first (https://emscripten.org/docs/getting_started/downloads.html), then re-run." >&2
    exit 1
fi

UPSTREAM="${TINYC_UPSTREAM:-../../vendor-tinycc}"
if [ ! -f "$UPSTREAM/libtcc.h" ]; then
    echo "build.sh: upstream TCC sources not found at $UPSTREAM (set TINYC_UPSTREAM)" >&2
    exit 1
fi

TARGET="${TARGET:-x86_64-linux-gnu}"
case "$TARGET" in
    x86_64-linux-gnu)  TARGET_DEFS="-DTCC_TARGET_X86_64";  TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c $UPSTREAM/x86_64-asm.c" ;;
    aarch64-linux-gnu) TARGET_DEFS="-DTCC_TARGET_ARM64";   TARGET_SRC="$UPSTREAM/arm64-gen.c $UPSTREAM/arm64-link.c $UPSTREAM/arm64-asm.c" ;;
    riscv64-linux-gnu) TARGET_DEFS="-DTCC_TARGET_RISCV64"; TARGET_SRC="$UPSTREAM/riscv64-gen.c $UPSTREAM/riscv64-link.c $UPSTREAM/riscv64-asm.c" ;;
    x86_64-windows)    TARGET_DEFS="-DTCC_TARGET_X86_64 -DTCC_TARGET_PE"; TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c $UPSTREAM/x86_64-asm.c" ;;
    # Android targets use the same ELF backends and target macros as
    # the Linux targets; they differ only in the sysroot they are
    # paired with (bionic, see sysroot-manifests/<target>.json).
    aarch64-linux-android) TARGET_DEFS="-DTCC_TARGET_ARM64";   TARGET_SRC="$UPSTREAM/arm64-gen.c $UPSTREAM/arm64-link.c $UPSTREAM/arm64-asm.c" ;;
    x86_64-linux-android)  TARGET_DEFS="-DTCC_TARGET_X86_64";  TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c $UPSTREAM/x86_64-asm.c" ;;
    # Darwin targets follow upstream's Makefile: DEF-x86_64-osx is
    # "-DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO", DEF-arm64-osx adds
    # -DTCC_TARGET_MACHO to the arm64 defines, and the *-osx file
    # lists add tccmacho.c to the corresponding ELF file list
    # (tccelf.c stays in CORE_SRC: Mach-O builds still read and
    # write ELF object files, per ELF_OBJ_ONLY in tcc.h). These two
    # branches follow that upstream check but have not been built
    # or verified in this environment.
    x86_64-apple-darwin) TARGET_DEFS="-DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO"; TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c $UPSTREAM/x86_64-asm.c $UPSTREAM/tccmacho.c" ;;
    arm64-apple-darwin)  TARGET_DEFS="-DTCC_TARGET_ARM64 -DTCC_TARGET_MACHO";  TARGET_SRC="$UPSTREAM/arm64-gen.c $UPSTREAM/arm64-link.c $UPSTREAM/arm64-asm.c $UPSTREAM/tccmacho.c" ;;
    wasm32-wasi) echo "build.sh: target wasm32-wasi needs a wasm code generation backend in TCC, which does not exist yet" >&2; exit 1 ;;
    *) echo "build.sh: unsupported TARGET=$TARGET (supported: x86_64-linux-gnu, aarch64-linux-gnu, riscv64-linux-gnu, x86_64-windows, aarch64-linux-android, x86_64-linux-android, arm64-apple-darwin, x86_64-apple-darwin, wasm32-wasi)" >&2; exit 1 ;;
esac

# Core sources: upstream Makefile's CORE_FILES minus tcctools.c
# (the standalone ar/ld tools are not part of the compiler module),
# plus the target backend above.
CORE_SRC="$UPSTREAM/tcc.c $UPSTREAM/libtcc.c $UPSTREAM/tccpp.c $UPSTREAM/tccgen.c \
$UPSTREAM/tccdbg.c $UPSTREAM/tccelf.c $UPSTREAM/tccasm.c $UPSTREAM/tccrun.c $TARGET_SRC"

# Export list: every LIBTCCAPI function in upstream libtcc.h
# (tcc_set_realloc, tcc_new, tcc_delete, tcc_set_lib_path,
# tcc_set_error_func, tcc_set_options, tcc_add_include_path,
# tcc_add_sysinclude_path, tcc_define_symbol, tcc_undefine_symbol,
# tcc_add_file, tcc_compile_string, tcc_set_output_type,
# tcc_add_library_path, tcc_add_library, tcc_add_symbol,
# tcc_output_file, tcc_run, tcc_relocate, tcc_get_symbol,
# tcc_list_symbols, tcc_set_backtrace_func), plus main / tinyc_main
# and malloc/free, which the glue layer needs to assemble argv and
# pass strings.
EXPORTED_FUNCTIONS='["_main","_tinyc_main","_malloc","_free","_tcc_set_realloc","_tcc_new","_tcc_delete","_tcc_set_lib_path","_tcc_set_error_func","_tcc_set_options","_tcc_add_include_path","_tcc_add_sysinclude_path","_tcc_define_symbol","_tcc_undefine_symbol","_tcc_add_file","_tcc_compile_string","_tcc_set_output_type","_tcc_add_library_path","_tcc_add_library","_tcc_add_symbol","_tcc_output_file","_tcc_run","_tcc_relocate","_tcc_get_symbol","_tcc_list_symbols","_tcc_set_backtrace_func"]'

EXPORTED_RUNTIME_METHODS='["ccall","cwrap","FS","UTF8ToString","stringToUTF8","lengthBytesUTF8","addFunction","removeFunction"]'

# Notes on the less obvious flags:
#   MODULARIZE + EXPORT_ES6   index.mjs imports the default factory
#   INVOKE_RUN=0              main must not run at load; callMain
#                             invokes it explicitly, like a caller of
#                             the exported main in the native packages
#   FORCE_FILESYSTEM          the virtual FS mirrors into module FS
#   ALLOW_MEMORY_GROWTH       compiled inputs are unbounded
#   ENVIRONMENT=web,worker    same build serves page and worker
emcc $CORE_SRC \
    -I"$UPSTREAM" \
    $TARGET_DEFS \
    -DCONFIG_TCC_PREDEFS=1 \
    -O2 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sINVOKE_RUN=0 \
    -sENVIRONMENT=web,worker \
    -sFORCE_FILESYSTEM=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sEXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS" \
    -sEXPORTED_RUNTIME_METHODS="$EXPORTED_RUNTIME_METHODS" \
    -o "tcc-$TARGET.mjs"

echo "built: tcc-$TARGET.mjs + tcc-$TARGET.wasm (target: $TARGET)"
