#!/bin/sh
# Assemble and build the wasm32 cross compiler from a pristine copy of
# the vendor tinycc tree.  The vendor tree itself is never modified.
# Everything (logs included) lands in this directory.
set -e
cd "$(dirname "$0")"
ROOT=$PWD
SRC=$ROOT/build-src
LOG=$ROOT/build.log

rm -rf "$SRC"
mkdir -p "$SRC"
# copy the vendor tree without build artifacts / VCS data
( cd ../vendor-tinycc && tar cf - --exclude=.git --exclude='*.o' \
    --exclude='tcc' --exclude='libtcc.a' --exclude='libtcc1.a' \
    --exclude='c2str.exe' --exclude='config.h' --exclude='config.mak' . ) \
  | ( cd "$SRC" && tar xf - --no-same-owner )

cp src/wasm-gen.c src/wasm-link.c "$SRC"/
python3 patches/apply-patches.py "$SRC" > "$LOG" 2>&1

cd "$SRC"
./configure --prefix="$ROOT/build-install" >> "$LOG" 2>&1
# configure's generated config.h guard (template at configure:646-647)
# would define the native target even for the cross build; add WASM32.
grep -q 'TCC_TARGET_C67)' config.h
sed -i 's/|| TCC_TARGET_C67)/|| TCC_TARGET_C67 || TCC_TARGET_WASM32)/' config.h
grep -q 'TCC_TARGET_WASM32' config.h
# native tools/objects first (tccdefs_.h generation uses the native build)
make -j4 >> "$LOG" 2>&1 || { echo "native build failed, see build.log"; exit 1; }
make wasm32-tcc >> "$LOG" 2>&1 || { echo "wasm32-tcc build failed, see build.log"; exit 1; }
ls -la wasm32-tcc >> "$LOG" 2>&1

# the wasm target's sysroot include: this package's headers
# (wasm_simd128.h, arm_neon.h, xmmintrin.h, emmintrin.h)
mkdir -p "$ROOT/sysroot/include"
cp "$ROOT"/include/*.h "$ROOT/sysroot/include"/

echo "BUILD OK: $SRC/wasm32-tcc"
