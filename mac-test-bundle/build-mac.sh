#!/bin/bash
# build-mac.sh - build the macOS dual-form kernel on a Mac.
#
# Steps: copy the pristine tinycc sources, apply the project patches,
# build upstream's native osx TCC with clang (gives libtcc1.a and the
# builtin headers via make install), then assemble the dual-form
# distribution file per the design: a Mach-O executable carrying the
# library form of the same build as an embedded section
# (__TINYC,__dylib), plus the small extractor the glue uses to pull
# that section back out into a cache directory for dlopen.
#
# Defensive by design: no set -e. Every step logs what it did; a
# failed step is recorded and the script carries on with whatever
# independent steps remain, so result.log always tells the story.
# Exit status is the number of failed build steps (0 = all built).

cd "$(dirname "$0")" || exit 99
BUNDLE=$PWD
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
cp -a "$BUNDLE/src/." "$SRC_COPY/" || fail "copy pristine sources"
cp "$BUNDLE/core/tcc_io.c" "$BUNDLE/core/tinyc_io.h" "$SRC_COPY/" \
    || fail "copy IO unit into sources"

step "apply project patches"
python3 "$BUNDLE/patches/apply_patches.py" "$SRC_COPY"
[ $? -eq 0 ] || fail "apply_patches.py"

step "configure (upstream, native osx)"
(cd "$SRC_COPY" && ./configure --prefix="$DIST") \
    || fail "./configure"

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
