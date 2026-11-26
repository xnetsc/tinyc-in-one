#!/bin/bash
# build-mac.sh - build the macOS dual-form kernel on a Mac.
#
# Steps: copy the repo's vendor tinycc tree (the source every
# platform builds from), apply the project patches, build the
# native osx TCC with clang (gives libtcc1.a and the builtin
# headers via make install), then assemble the dual-form
# distribution file per the design: a Mach-O executable carrying
# the library form of the same build as an embedded section
# (__TINYC,__dylib), plus the small extractor the glue uses to
# pull that section back out into a cache directory for dlopen.
#
# Defensive by design: no set -e. Every step logs what it did; a
# failed step is recorded and the script carries on with whatever
# independent steps remain, so result.log always tells the story.
# Exit status is the number of failed build steps (0 = all built).

cd "$(dirname "$0")" || exit 99
BUNDLE=$PWD
ROOT=$(cd "$BUNDLE/.." && pwd)
SRC_COPY=$BUNDLE/build/src
DIST=$BUNDLE/dist
WORK=$BUNDLE/build
FAILS=0

step() { echo; echo "=== build: $* ==="; }
note() { echo "  $*"; }
fail() { echo "BUILD-FAIL: $*"; FAILS=$((FAILS + 1)); }

step "environment"
sw_vers 2>/dev/null
echo "uname -m: $(uname -m)"
echo "shell: $SHELL"
clang --version 2>&1 | head -2
echo "sdk: $(xcrun --show-sdk-path 2>&1)"
python3 --version 2>&1
node --version 2>&1
if [ "$(uname -m)" != "arm64" ]; then
    note "WARNING: this bundle targets arm64-apple-darwin; host is $(uname -m), continuing anyway"
fi

step "prepare source copy"
rm -rf "$BUNDLE/build" "$DIST"
mkdir -p "$SRC_COPY" "$WORK"
# The vendor tree, copied without its build artifacts exactly as
# build.sh copies it for the other platforms' dists.
tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
    --exclude='./libtcc.a' --exclude='./libtcc1.a' \
    --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
    --exclude='./c2str.exe' --exclude='./config.h' \
    --exclude='./config.mak' --exclude='./tccdefs_.h' \
    -C "$ROOT/vendor-tinycc" . | tar xf - -C "$SRC_COPY" \
    || fail "copy vendor sources"
cp "$ROOT/core/tcc_io.c" "$ROOT/core/tinyc_io.h" "$SRC_COPY/" \
    || fail "copy IO unit into sources"

step "apply project patches"
python3 "$ROOT/patches/apply_patches.py" "$SRC_COPY"
[ $? -eq 0 ] || fail "apply_patches.py"

step "configure (upstream, native osx)"
(cd "$SRC_COPY" && ./configure --prefix="$DIST") \
    || fail "./configure"

step "bundled Darwin headers: stage headers + libSystem stubs into dist"
# Stage first: the repoint below points config.h at the staged
# include-libc, and the libtcc1.a build needs stdio.h from there.
python3 "$BUNDLE/darwin-dist.py" stage "$ROOT/vendor-darwin" "$DIST" \
    || fail "darwin-dist stage"
# The libtcc1 build runs the freshly built tcc with -B pointing at
# the source tree (lib/Makefile XFLAGS-unx), so {B} in the repointed
# config.h resolves to the source tree during the build. Mirror the
# staged headers there so the build finds the bundled stdio.h.
cp -r "$DIST/lib/tcc/include-libc" "$SRC_COPY/include-libc" \
    || fail "mirror include-libc into sources"

step "bundled Darwin headers: repoint search paths at the package"
# Upstream configure bakes the build machine's SDK include path
# into config.h.  Replace it with the package paths so the built
# compiler uses the bundled Darwin headers (vendor-darwin/) and
# the bundled libSystem .tbd stubs, never the SDK.
python3 "$BUNDLE/darwin-dist.py" paths "$SRC_COPY/config.h" \
    || fail "darwin-dist paths"

step "make + make install (upstream TCC, libtcc1.a, headers)"
if [ -f "$SRC_COPY/config.mak" ]; then
    (cd "$SRC_COPY" && make -j4) || fail "make"
    (cd "$SRC_COPY" && make install) || fail "make install"
else
    fail "no config.mak, skipping make"
fi
if [ -x "$DIST/bin/tcc" ]; then
    note "upstream tcc built: $("$DIST/bin/tcc" -dumpmachine 2>&1) / $("$DIST/bin/tcc" -dumpversion 2>&1)"
else
    fail "dist/bin/tcc missing after make install"
fi
if [ -f "$DIST/lib/tcc/libtcc1.a" ]; then
    note "libtcc1.a present: $DIST/lib/tcc/libtcc1.a"
else
    fail "dist/lib/tcc/libtcc1.a missing"
fi

step "library form: libtinyc.dylib (clang, unity build of patched tcc.c)"
DYLIB=$WORK/libtinyc.dylib
clang -O2 -dynamiclib -install_name @rpath/libtinyc.dylib \
    -o "$DYLIB" "$SRC_COPY/tcc.c" "$SRC_COPY/tcc_io.c" \
    -I"$SRC_COPY" || fail "clang -dynamiclib"
if [ -f "$DYLIB" ]; then
    file "$DYLIB"
    nm -gU "$DYLIB" 2>/dev/null | grep -E " (tinyc_main|tcc_new|tcc_compile_string)$" \
        | sed 's/^/  export: /'
fi

step "dual-form file: executable with embedded dylib section"
DUAL=$DIST/bin/tinyc
if [ -f "$DYLIB" ]; then
    clang -O2 -o "$DUAL" "$SRC_COPY/tcc.c" "$SRC_COPY/tcc_io.c" \
        -I"$SRC_COPY" \
        -Wl,-sectcreate,__TINYC,__dylib,"$DYLIB" \
        || fail "clang dual-form link"
else
    fail "no dylib to embed, skipping dual-form link"
fi
if [ -x "$DUAL" ]; then
    file "$DUAL"
    otool -l "$DUAL" 2>/dev/null | grep -A4 "sectname __dylib" | sed 's/^/  /'
    "$DUAL" --version 2>&1 | head -1 | sed 's/^/  dual --version: /'
else
    fail "dual-form executable missing"
fi

step "glue extractor (extract_dylib)"
clang -O2 -o "$WORK/extract_dylib" "$BUNDLE/glue/extract_dylib.c" \
    || fail "build extract_dylib"

step "dlopen test host"
clang -O2 -o "$WORK/host_dlopen" "$BUNDLE/glue/host_dlopen.c" \
    -I"$SRC_COPY" || fail "build host_dlopen"

step "sample local library for the dynamic-add test"
mkdir -p "$WORK/local/include" "$WORK/local/lib"
cp "$BUNDLE/testdata/local/include/mylib.h" "$WORK/local/include/"
clang -O2 -c "$BUNDLE/testdata/local/src/mylib.c" \
    -I"$WORK/local/include" -o "$WORK/local/mylib.o" || fail "compile mylib.c"
ar rcs "$WORK/local/lib/libmylib.a" "$WORK/local/mylib.o" || fail "ar libmylib.a"
[ -f "$WORK/local/lib/libmylib.a" ] && note "libmylib.a ready"

step "dist layout"
find "$DIST" -type f | sort | sed "s|$BUNDLE/|  |"

echo
echo "build-mac.sh done: $FAILS failed step(s)"
exit $FAILS
