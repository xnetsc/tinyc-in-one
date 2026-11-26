#!/bin/sh
# Build TinyC (TCC) to WebAssembly with Emscripten.
#
# Prerequisite: an activated Emscripten SDK (emcc on PATH). This
# script does not install anything; without emcc it exits with an
# error. See README.md.
#
# Output: the asset envelope tcc-<target>.tinyc.js, next to this
# script — the only per-target file the package ships and the one
# index.mjs loads (see build-asset-envelopes.mjs). The emcc run
# below first emits the intermediate pair tcc-<target>.mjs (ES6
# MODULARIZE factory) + tcc-<target>.wasm; the envelope step at the
# end of this script packs that pair into the envelope and is the
# build's finishing step.
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
# TARGET defaults to x86_64-linux-gnu. The wasm32-wasi target is
# built from the project's wasm-backend tree (the wasm32 code
# generator lives there, not in upstream TCC): the branch below
# assembles vendor + wasm-gen.c/wasm-link.c + the backend patches
# the same way wasm-backend/build.sh does, generates tccdefs_.h with
# the native make, and compiles that tree. The glue layer drives the
# module through its exported main (the backend tree has no
# tinyc_main; index.mjs prefers tinyc_main and falls back to main).
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

if [ "$TARGET" != "wasm32-wasi" ]; then
    # The eight general targets compile the unpatched upstream
    # tree, and tcc.h includes two files that only a configure +
    # native make run generates: config.h and tccdefs_.h. Prepare
    # a copy of the tree the same way the wasm32 branch below
    # prepares its own: copy (dropping any build leftovers the
    # source tree may carry), configure, and a native make, which
    # regenerates both files. No project patches are applied to
    # this tree. The preparation is target-independent - the
    # emcc compile picks the target through the -DTCC_TARGET_*
    # macros, and config.h's target guard names every general
    # target's arch macro, so the command-line macro wins - so
    # every general-target run prepares the same tree.
    GSRC=build/general-src
    rm -rf "$GSRC"
    mkdir -p "$GSRC"
    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./*-tcc' --exclude='./libtcc.a' \
        --exclude='./libtcc1.a' --exclude='./*-libtcc1.a' \
        --exclude='./c2str' --exclude='./c2str.exe' \
        --exclude='./config.h' --exclude='./config.mak' \
        --exclude='./tccdefs_.h' \
        -C "$UPSTREAM" . | tar xf - --no-same-owner -C "$GSRC"
    ABS_GSRC=$PWD/$GSRC
    (cd "$GSRC" && ./configure --prefix="$ABS_GSRC/install" >/dev/null)
    (cd "$GSRC" && make -j"$(nproc)" >/dev/null)
    UPSTREAM=$GSRC
fi

# Target file lists follow upstream's Makefile (*_FILES): the
# x86_64 assembler is part of i386-asm.c (the tree has no
# x86_64-asm.c), the PE backend is tccpe.c, and the Mach-O
# backend is tccmacho.c.
case "$TARGET" in
    x86_64-linux-gnu)  TARGET_DEFS="-DTCC_TARGET_X86_64";  TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c" ;;
    aarch64-linux-gnu) TARGET_DEFS="-DTCC_TARGET_ARM64";   TARGET_SRC="$UPSTREAM/arm64-gen.c $UPSTREAM/arm64-link.c $UPSTREAM/arm64-asm.c" ;;
    riscv64-linux-gnu) TARGET_DEFS="-DTCC_TARGET_RISCV64"; TARGET_SRC="$UPSTREAM/riscv64-gen.c $UPSTREAM/riscv64-link.c $UPSTREAM/riscv64-asm.c" ;;
    x86_64-windows)    TARGET_DEFS="-DTCC_TARGET_X86_64 -DTCC_TARGET_PE"; TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c $UPSTREAM/tccpe.c" ;;
    # Android targets use the same ELF backends and target macros as
    # the Linux targets; they differ only in the sysroot they are
    # paired with (bionic, see sysroot-manifests/<target>.json).
    aarch64-linux-android) TARGET_DEFS="-DTCC_TARGET_ARM64";   TARGET_SRC="$UPSTREAM/arm64-gen.c $UPSTREAM/arm64-link.c $UPSTREAM/arm64-asm.c" ;;
    x86_64-linux-android)  TARGET_DEFS="-DTCC_TARGET_X86_64";  TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c" ;;
    # Darwin targets follow upstream's Makefile: DEF-x86_64-osx is
    # "-DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO", DEF-arm64-osx adds
    # -DTCC_TARGET_MACHO to the arm64 defines, and the *-osx file
    # lists add tccmacho.c to the corresponding ELF file list
    # (tccelf.c stays in CORE_SRC: Mach-O builds still read and
    # write ELF object files, per ELF_OBJ_ONLY in tcc.h). These two
    # branches follow that upstream check but have not been built
    # or verified in this environment.
    x86_64-apple-darwin) TARGET_DEFS="-DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO"; TARGET_SRC="$UPSTREAM/x86_64-gen.c $UPSTREAM/x86_64-link.c $UPSTREAM/i386-asm.c $UPSTREAM/tccmacho.c" ;;
    arm64-apple-darwin)  TARGET_DEFS="-DTCC_TARGET_ARM64 -DTCC_TARGET_MACHO";  TARGET_SRC="$UPSTREAM/arm64-gen.c $UPSTREAM/arm64-link.c $UPSTREAM/arm64-asm.c $UPSTREAM/tccmacho.c" ;;
    wasm32-wasi)
        WB=../../wasm-backend
        ASRC=build/wasm32-src
        if [ ! -f "$WB/patches/apply-patches.py" ]; then
            echo "build.sh: wasm-backend tree not found at $WB" >&2; exit 1
        fi
        rm -rf "$ASRC"
        mkdir -p "$ASRC"
        tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
            --exclude='./libtcc.a' --exclude='./libtcc1.a' \
            --exclude='./c2str.exe' --exclude='./config.h' \
            --exclude='./config.mak' --exclude='./tccdefs_.h' \
            -C "$UPSTREAM" . | tar xf - --no-same-owner -C "$ASRC"
        cp "$WB/src/wasm-gen.c" "$WB/src/wasm-link.c" "$WB/src/wasm-int.h" "$ASRC/"
        python3 "$WB/patches/apply-patches.py" "$ASRC" >/dev/null
        ABS_ASRC=$PWD/$ASRC
        (cd "$ASRC" && ./configure --prefix="$ABS_ASRC/install" >/dev/null)
        # configure's config.h guard would define the native target
        # even for the cross build; extend it with WASM32 (same edit
        # as wasm-backend/build.sh).
        grep -q 'TCC_TARGET_C67)' "$ASRC/config.h"
        sed -i 's/|| TCC_TARGET_C67)/|| TCC_TARGET_C67 || TCC_TARGET_WASM32)/' "$ASRC/config.h"
        grep -q 'TCC_TARGET_WASM32' "$ASRC/config.h"
        # native build first: generates tccdefs_.h for the emcc compile
        (cd "$ASRC" && make -j"$(nproc)" >/dev/null)
        UPSTREAM=$ASRC
        TARGET_DEFS="-DTCC_TARGET_WASM32"
        TARGET_SRC="$ASRC/wasm-gen.c $ASRC/wasm-link.c"
        # the backend tree has no tinyc_main; export main only
        # (export-list policy: see the EXPORTED_FUNCTIONS comment below)
        EXPORTED_FUNCTIONS_OVERRIDE='["_main","_malloc","_free","_tcc_set_realloc","_tcc_new","_tcc_delete","_tcc_set_lib_path","_tcc_set_error_func","_tcc_set_options","_tcc_add_include_path","_tcc_add_sysinclude_path","_tcc_define_symbol","_tcc_undefine_symbol","_tcc_add_file","_tcc_compile_string","_tcc_set_output_type","_tcc_add_library_path","_tcc_add_library","_tcc_add_symbol","_tcc_output_file","_tcc_run","_tcc_get_symbol","_tcc_list_symbols","_wasmfs_create_opfs_backend","_wasmfs_create_directory"]'
        ;;
    *) echo "build.sh: unsupported TARGET=$TARGET (supported: x86_64-linux-gnu, aarch64-linux-gnu, riscv64-linux-gnu, x86_64-windows, aarch64-linux-android, x86_64-linux-android, arm64-apple-darwin, x86_64-apple-darwin, wasm32-wasi)" >&2; exit 1 ;;
esac

# Core sources: upstream Makefile's CORE_FILES minus tcctools.c
# (the standalone ar/ld tools are not part of the compiler module),
# plus the target backend above.  They are compiled as separate
# translation units, which is upstream's ONE_SOURCE=no mode: tcc.c
# and libtcc.c default ONE_SOURCE to 1 (each would textually include
# every other file, duplicating every definition at link time), so
# the emcc invocation below passes -DONE_SOURCE=0, the same define
# the Makefile uses for its per-file objects.
CORE_SRC="$UPSTREAM/tcc.c $UPSTREAM/libtcc.c $UPSTREAM/tccpp.c $UPSTREAM/tccgen.c \
$UPSTREAM/tccdbg.c $UPSTREAM/tccelf.c $UPSTREAM/tccasm.c $UPSTREAM/tccrun.c $TARGET_SRC"

# Export list: the LIBTCCAPI functions in upstream libtcc.h that
# this build defines (tcc_set_realloc, tcc_new, tcc_delete,
# tcc_set_lib_path, tcc_set_error_func, tcc_set_options,
# tcc_add_include_path, tcc_add_sysinclude_path, tcc_define_symbol,
# tcc_undefine_symbol, tcc_add_file, tcc_compile_string,
# tcc_set_output_type, tcc_add_library_path, tcc_add_library,
# tcc_add_symbol, tcc_output_file, tcc_get_symbol,
# tcc_list_symbols), plus main and malloc/free, which
# the glue layer needs to assemble argv and pass strings, plus the
# two WasmFS functions the glue layer calls to mount the OPFS
# backend (wasmfs_create_opfs_backend and wasmfs_create_directory,
# from <emscripten/wasmfs.h>; they exist only in the -sWASMFS
# build).
#
# tcc_relocate and tcc_set_backtrace_func are not in either
# list: tccrun.c defines them only for a native-host build
# (TCC_IS_NATIVE), and this module's host is WASM for every
# target, so neither symbol exists in the objects. The glue layer
# never calls tcc_set_backtrace_func. TinycState.relocate() is
# the only caller of tcc_relocate, and the bridge answers it with
# TinycMissingExportError naming the export; running compiled
# code goes through outputFile plus host-side instantiation
# (TinycState.run), not through in-module relocation.
#
# tcc_run is in the wasm32 list only, for the same reason:
# tccrun.c defines it under TCC_IS_NATIVE, so the eight
# native-target objects do not contain it, while the wasm32
# tree's wasm-link.c provides it. The glue layer never calls
# tcc_run for any target (TinycState.run is the outputFile path
# above, and the package tests assert tcc_run is never called).
#
# tinyc_main is not in the general list: the project patches
# generate it in the kernel trees, while the eight general
# targets are compiled from the unpatched upstream tree and
# export main only. The glue layer prefers tinyc_main when a
# module provides it and falls back to main, which is also how
# the wasm32 module is driven.
EXPORTED_FUNCTIONS='["_main","_malloc","_free","_tcc_set_realloc","_tcc_new","_tcc_delete","_tcc_set_lib_path","_tcc_set_error_func","_tcc_set_options","_tcc_add_include_path","_tcc_add_sysinclude_path","_tcc_define_symbol","_tcc_undefine_symbol","_tcc_add_file","_tcc_compile_string","_tcc_set_output_type","_tcc_add_library_path","_tcc_add_library","_tcc_add_symbol","_tcc_output_file","_tcc_get_symbol","_tcc_list_symbols","_wasmfs_create_opfs_backend","_wasmfs_create_directory"]'

EXPORTED_FUNCTIONS="${EXPORTED_FUNCTIONS_OVERRIDE:-$EXPORTED_FUNCTIONS}"

# HEAPU8 is in the runtime methods because the glue layer assembles
# argv through it (NativeBridge assembles the argument strings and
# the argv array in the module heap): current Emscripten exposes the
# heap views on the module object only when they are listed here.
EXPORTED_RUNTIME_METHODS='["HEAPU8","ccall","cwrap","FS","UTF8ToString","stringToUTF8","lengthBytesUTF8","addFunction","removeFunction"]'

# Notes on the less obvious flags:
#   MODULARIZE + EXPORT_ES6   index.mjs imports the default factory
#   INVOKE_RUN=0              main must not run at load; callMain
#                             invokes it explicitly, like a caller of
#                             the exported main in the native packages
#   WASMFS                    the module filesystem is WasmFS; its
#                             root backend is memory. Where it can,
#                             the glue layer mounts the WasmFS OPFS
#                             backend at /opfs (see index.mjs)
#   FORCE_FILESYSTEM          with WasmFS the JS FS API is opt-in;
#                             this setting is what includes it, and
#                             the virtual FS mirrors into the module
#                             FS through it (FS.mkdirTree, FS.writeFile
#                             and FS.unlink)
#   ALLOW_MEMORY_GROWTH       compiled inputs are unbounded
#   ALLOW_TABLE_GROWTH        the glue layer registers JS callbacks
#                             in the module's function table through
#                             addFunction (the per-state error
#                             callback); a fixed-size table has no
#                             free slot for the first registration
#                             and it throws
#   ENVIRONMENT=web,worker    same build serves page and worker
#   CONFIG_TCC_CRTPREFIX /    the compiler's crt and library search
#   CONFIG_TCC_LIBPATHS       prefixes point at the package sysroot
#                             in the virtual FS (/sysroot/lib): a
#                             hosted link finds the sysroot's crt
#                             objects and stub libraries there, and
#                             no host path is ever searched
emcc $CORE_SRC \
    -I"$UPSTREAM" \
    $TARGET_DEFS \
    -DONE_SOURCE=0 \
    -DCONFIG_TCC_PREDEFS=1 \
    -DCONFIG_TCC_CRTPREFIX='"/sysroot/lib"' \
    -DCONFIG_TCC_LIBPATHS='"/sysroot/lib"' \
    -O2 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sINVOKE_RUN=0 \
    -sENVIRONMENT=web,worker \
    -sWASMFS=1 \
    -sFORCE_FILESYSTEM=1 \
    -sALLOW_MEMORY_GROWTH=1 \
    -sALLOW_TABLE_GROWTH=1 \
    -sEXPORTED_FUNCTIONS="$EXPORTED_FUNCTIONS" \
    -sEXPORTED_RUNTIME_METHODS="$EXPORTED_RUNTIME_METHODS" \
    -o "tcc-$TARGET.mjs"

echo "built: tcc-$TARGET.mjs + tcc-$TARGET.wasm (target: $TARGET)"

# The compiler asset envelope (design §14): the glue text and the
# module bytes as one classic script registering them in
# __TINYC_ASSET_REGISTRY__. This envelope is the shipped form of
# the compiler module — the only per-target asset file the package
# carries — and loadTinyc retrieves it through the same transport
# as library unit files under both protocols (fetch reads it as
# text, file: injects it as a script), so no standalone .wasm/.mjs
# pair is ever served. The sysroot envelope is built by the same
# script wherever the served sysroot is assembled (see
# build-asset-envelopes.mjs).
node "$(dirname "$0")/build-asset-envelopes.mjs" --target "$TARGET" --compiler-only
