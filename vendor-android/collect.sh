#!/bin/bash
# Collect the vendored bionic tree into vendor-android/ from an
# Android NDK r26d installation:
#
#   collect.sh [ndk-sysroot] [api]
#
# ndk-sysroot defaults to $TINYC_NDK_SYSROOT, then to the NDK in
# ~/workspace/envs/android-toolchain. api defaults to 30.
#
# The header set starts from the sysroot's usr/include tree and
# is then pruned by prune.py to the bundled set: the bionic
# surface at the C / POSIX layer (the C standard library and
# the POSIX layer, plus the pthread, socket and dlfcn
# exception clusters), plus the individual kernel UAPI headers
# its #include closure pulls in. Kernel UAPI trees, other
# libraries' headers, OS interfaces beyond that layer and
# application-level surfaces are not bundled. One tree serves
# every triple; the per-architecture part is the
# <triple> subdirectory inside it (the asm headers). The link
# inputs are, per triple, the requested API level's crt objects
# and its libc / libm / libdl link-time stubs. The sysroot's
# NOTICE file is copied alongside the tree.
set -e
cd "$(dirname "$0")"
SYSROOT=${1:-${TINYC_NDK_SYSROOT:-$HOME/workspace/envs/android-toolchain/sdk/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot}}
API=${2:-30}
TRIPLES="aarch64-linux-android x86_64-linux-android"
[ -d "$SYSROOT/usr/include" ] || { echo "no NDK sysroot at $SYSROOT" >&2; exit 1; }
rm -rf include
cp -r "$SYSROOT/usr/include" include
python3 prune.py --apply include
cp "$SYSROOT/NOTICE" NOTICE
for T in $TRIPLES; do
    SRC="$SYSROOT/usr/lib/$T/$API"
    DEST=$PWD/$T/$API/lib
    rm -rf "$PWD/$T/$API"
    mkdir -p "$DEST"
    cp "$SRC"/crtbegin_dynamic.o "$SRC"/crtbegin_so.o \
       "$SRC"/crtbegin_static.o "$SRC"/crtend_android.o \
       "$SRC"/crtend_so.o "$DEST/"
    cp "$SRC"/libc.so "$SRC"/libm.so "$SRC"/libdl.so "$DEST/"
    echo "$T (API $API): $(ls "$DEST" | wc -l) link inputs"
done
echo "include: $(find include -type f | wc -l) files"
