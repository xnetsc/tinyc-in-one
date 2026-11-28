#!/bin/bash
# build-mac.sh - build the macOS dual-form kernels on a Mac.
#
# Each target lands in mac-test-bundle/dist/<triple>/ with the
# layout:
#   bin/tinyc            dual-form file (Mach-O executable carrying
#                        the library form as an embedded section)
#   include/             libtcc.h, tinyc_io.h
#   lib/tcc/libtcc1.a    target runtime library (every target but wasm32)
#   lib/tcc/include/     TCC builtin headers
#   lib/tcc/include-libc/  target libc headers (every target but wasm32)
#   lib/tcc/libc/        glibc crt + linker scripts (linux-gnu),
#                        bionic crt + stubs (android)
#   lib/tcc/lib/         .tbd link stubs (darwin)
#   lib/tcc/win32/       win32 headers + lib (win32 target)
#   manifest.json        host, target, isa, file locations
#
# Usage: bash build-mac.sh [triple ...]
#   No arguments: build all 9 targets for this Mac host.
#   Arguments: build only those triples.
#
# The dual-form file is a Mach-O executable for the HOST cpu: it
# runs on this Mac, and the compiler inside targets <triple>. The
# target is set by the -DTCC_TARGET_* defines (the same defines
# build.sh uses for the same target family), never by a -arch
# flag - so both darwin archs are built on either Mac host, and
# stage_layout tells them apart by the manifest's target field,
# not by the binary's cpu type (which is the host cpu for both).
# The sysroot is the target's own, staged from the same vendor
# trees build.sh stages from, so a mac-built dist for a target
# carries the same sysroot as the Linux-built dist for it.
#
# Defensive by design: no set -e. Every step logs what it did; a
# failed step is recorded and the script carries on with whatever
# independent steps remain, so result.log always tells the story.
# Exit status is the number of failed build steps (0 = all built).
#
# Per-target build (build_mac_target), mirroring build.sh's
# per-family recipes for the same target:
#   native darwin triple: the original bundle recipe (native make
#     + make install, clang dylib + dual with no -D flags beyond
#     the target define - unchanged from before).
#   other darwin arch: build_darwin's recipe (native make first,
#     then make cross-<arch>-osx for libtcc1.a; the darwin sysroot
#     staged by darwin-dist.py; the dylib + dual compiled by clang
#     with the darwin target defines).
#   linux-gnu: build_one's cross recipe (native make first, then
#     make cross-<ct>; the glibc sysroot staged from vendor-glibc;
#     config.h repointed at the staged headers so the lib build
#     uses them, not the SDK's).
#   android: the same with the bionic sysroot from vendor-android.
#   win32: build_one's win32 recipe (make cross-x86_64-win32,
#     pthread merged into libtcc1.a, the win32 tree staged).
#   wasm32: build_wasm32's recipe (wasm sources + patches, the
#     config.h guard edit, headers only, no libtcc1.a).
#
# The host tools (extract_dylib, host_dlopen, the mylib fixtures)
# are built once, not per target: they run on this Mac and do not
# depend on the target.

cd "$(dirname "$0")" || exit 99
BUNDLE=$PWD
ROOT=$(cd "$BUNDLE/.." && pwd)
WORK=$BUNDLE/build
FAILS=0

step() { echo; echo "=== build: $* ==="; }
note() { echo "  $*"; }
fail() { echo "BUILD-FAIL: $*"; FAILS=$((FAILS + 1)); }

HOST_ARCH=$(uname -m)
case $HOST_ARCH in
    arm64) HOST_TRIPLE=arm64-apple-darwin ;;
    x86_64) HOST_TRIPLE=x86_64-apple-darwin ;;
    *) echo "BUILD-FAIL: unsupported host arch: $HOST_ARCH"; exit 99 ;;
esac
VER=$(cat "$ROOT/vendor-tinycc/VERSION")
ALL="x86_64-linux-gnu aarch64-linux-gnu riscv64-linux-gnu x86_64-w64-mingw32 aarch64-linux-android x86_64-linux-android arm64-apple-darwin x86_64-apple-darwin wasm32-wasi"
if [ $# -gt 0 ]; then TRIPLES="$*"; else TRIPLES="$ALL"; fi
ANDROID_API=${TINYC_ANDROID_API:-30}
GLIBC_VER=2.39
ANDROID_NDK_VER=r26d

# isa values: the darwin ones are the matrix isa JSON objects the
# macOS CI job writes into the manifest (kept identical here);
# the rest are the ISA strings build.sh writes for the same target.
ISA_X86_64="x86-64 baseline; scalar floating point via SSE2 (xmm registers); no AVX code generation"
ISA_ARM64="ARMv8.0-A baseline (FP/NEON register file is architectural); scalar FP code generation only, no NEON vector instructions, no SVE"
ISA_RISCV64="RV64GC baseline (I/M/A/F/D/C); no Vector extension (RVV) support"
ISA_WASM32="WebAssembly MVP with SIMD128 (v128); no threads/atomics target variant"
ISA_DARWIN_ARM64='{"arch":"aarch64","bits":64,"endian":"little","simd":["neon"]}'
ISA_DARWIN_X86_64='{"arch":"x86_64","bits":64,"endian":"little","simd":["sse","sse2","avx","avx2"]}'

step "environment"
sw_vers 2>/dev/null
echo "uname -m: $HOST_ARCH (host triple: $HOST_TRIPLE)"
echo "shell: $SHELL"
clang --version 2>&1 | head -2
echo "sdk: $(xcrun --show-sdk-path 2>&1)"
python3 --version 2>&1
node --version 2>&1
mkdir -p "$WORK" || fail "mkdir $WORK"

# repoint_config <config.h> <sysincludepaths> <libpaths>
# Rewrite the search paths upstream configure baked into
# config.h so the lib build (make cross-...) uses the staged
# target headers, not the build machine's SDK headers. Same
# mechanism as darwin-dist.py paths, parameterized by the target
# family's package paths. The values must be identical to the
# -D flags passed to the unity compile (same strings), so a
# redefinition is benign. config.h may or may not define the
# macros (macOS configure defines SYSINCLUDEPATHS with the SDK
# path; a Linux configure defines neither) - either way the end
# state is asserted, not the edit.
repoint_config() {
    python3 - "$1" "$2" "$3" <<'PYEOF'
import re, sys
path, sysinc, libpaths = sys.argv[1], sys.argv[2], sys.argv[3]
text = open(path).read()
pat = re.compile(r'^#define CONFIG_TCC_SYSINCLUDEPATHS "[^"]*"$', re.M)
if pat.search(text):
    text = pat.sub(f'#define CONFIG_TCC_SYSINCLUDEPATHS "{sysinc}"', text, count=1)
else:
    text += f'\n#define CONFIG_TCC_SYSINCLUDEPATHS "{sysinc}"\n'
pat_lib = re.compile(r'^#define CONFIG_TCC_LIBPATHS "[^"]*"$', re.M)
if pat_lib.search(text):
    text = pat_lib.sub(f'#define CONFIG_TCC_LIBPATHS "{libpaths}"', text, count=1)
else:
    anchor = f'#define CONFIG_TCC_SYSINCLUDEPATHS "{sysinc}"\n'
    text = text.replace(anchor, anchor + f'#define CONFIG_TCC_LIBPATHS "{libpaths}"\n', 1)
for name, want in (("CONFIG_TCC_SYSINCLUDEPATHS", sysinc),
                   ("CONFIG_TCC_LIBPATHS", libpaths)):
    m = re.search(rf'^#define {name} "([^"]*)"$', text, re.M)
    assert m and m.group(1) == want, f"{name} not set to {want}"
    assert "MacOSX.sdk" not in m.group(1), f"SDK path left in {name}"
open(path, "w").write(text)
print(f"build-mac: paths rewritten in {path}")
PYEOF
    [ $? -eq 0 ] || fail "repoint_config $1"
}

# write_manifest <dist> <target> <isa-json> <libtcc1-rel-or-empty> <extra-json-or-empty>
# The per-target manifest, same shape as build.sh's dist manifests
# (plus the host field, as in the host-variant manifests). The isa
# is a JSON literal: an object for the darwin targets, a string for
# the rest. The extra JSON carries the family's libc fields.
write_manifest() {
    python3 - "$1" "$HOST_TRIPLE" "$2" "$3" "$4" "$5" "$VER" <<'PYEOF'
import json, sys
dist, host, target, isa_s, libtcc1, extra_s, ver = sys.argv[1:8]
m = {"name": "tinyc", "version": ver, "host": host, "target": target,
     "isa": json.loads(isa_s),
     "outputs": {"exe": {"format": "Mach-O executable", "ext": ""},
                 "obj": {"format": "Mach-O object", "ext": ".o"},
                 "shared": {"format": "Mach-O dylib", "ext": ".dylib"},
                 "ar": {"format": "ar archive", "ext": ".a"}},
     "dual_file": "bin/tinyc", "toolchain_include": "include"}
if libtcc1:
    m["libtcc1"] = libtcc1
extra = json.loads(extra_s) if extra_s else {}
m.update(extra)
m["built_on"] = f"macOS host {host}, C compiler clang"
m["verification"] = ("Mach-O format and embedded dylib section checked "
                     "with otool; the bundle test suite, the SDK-free "
                     "battery and the SIMD smoke run on the macOS CI runner")
with open(dist + "/manifest.json", "w") as f:
    json.dump(m, f, indent=2)
    f.write("\n")
print(f"build-mac: manifest written in {dist}")
PYEOF
    [ $? -eq 0 ] || fail "write_manifest $1"
}

# build_mac_target <triple> <cross-name> <target-def> <isa-json> <sysinc> <libpaths> [extra -D flags...]
# <cross-name> is the vendor Makefile cross name (x86_64, arm64,
# riscv64, x86_64-osx, arm64-osx, x86_64-win32) or empty for wasm32.
# <sysinc>/<libpaths> are the family's package search paths for the
# config.h repoint (empty for win32/wasm32, which need none).
# The extra -D flags are the baked search paths of the Linux dist
# for the same target family.
build_mac_target() {
    local TRIPLE=$1 CT=$2 TDEF=$3 ISA_JSON=$4 SYSINC=$5 LIBPATHS=$6
    shift 6
    local DEFS=("$@")
    local SRC_COPY=$BUNDLE/build/$TRIPLE/src
    local DIST=$BUNDLE/dist/$TRIPLE
    local TWORK=$BUNDLE/build/$TRIPLE
    local DYLIB=$TWORK/libtinyc.dylib
    local DUAL=$DIST/bin/tinyc
    local XTCC EXTRA_JSON LIBTCC1_REL VDIR LIBSRC

    step "prepare source copy ($TRIPLE)"
    rm -rf "$BUNDLE/build/$TRIPLE" "$DIST"
    mkdir -p "$SRC_COPY" "$TWORK" "$DIST/bin" "$DIST/include" \
        "$DIST/lib/tcc/include" || fail "mkdir $TRIPLE"
    # The vendor tree, copied without its build artifacts exactly as
    # build.sh copies it for the other platforms' dists.
    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C "$ROOT/vendor-tinycc" . | tar xf - -C "$SRC_COPY" \
        || fail "copy vendor sources ($TRIPLE)"
    cp "$ROOT/core/tcc_io.c" "$ROOT/core/tinyc_io.h" "$SRC_COPY/" \
        || fail "copy IO unit into sources ($TRIPLE)"

    step "apply project patches ($TRIPLE)"
    python3 "$ROOT/patches/apply_patches.py" "$SRC_COPY"
    [ $? -eq 0 ] || fail "apply_patches.py ($TRIPLE)"

    # wasm32 only: the backend sources and patches, as in build_wasm32.
    if [ "$TRIPLE" = wasm32-wasi ]; then
        cp "$ROOT/wasm-backend/src/wasm-gen.c" \
           "$ROOT/wasm-backend/src/wasm-link.c" \
           "$ROOT/wasm-backend/src/wasm-int.h" "$SRC_COPY/" \
            || fail "copy wasm sources ($TRIPLE)"
        python3 "$ROOT/wasm-backend/patches/apply-patches.py" "$SRC_COPY"
        [ $? -eq 0 ] || fail "wasm apply-patches.py ($TRIPLE)"
    fi

    step "configure (upstream, native osx) ($TRIPLE)"
    (cd "$SRC_COPY" && ./configure --prefix="$DIST") \
        || fail "./configure ($TRIPLE)"
    # wasm32 only: the config.h guard edit, as in build_wasm32.
    if [ "$TRIPLE" = wasm32-wasi ]; then
        grep -q 'TCC_TARGET_C67)' "$SRC_COPY/config.h" \
            || fail "config.h guard not found ($TRIPLE)"
        sed -i '' 's/|| TCC_TARGET_C67)/|| TCC_TARGET_C67 || TCC_TARGET_WASM32)/' \
            "$SRC_COPY/config.h" \
            || fail "config.h guard edit ($TRIPLE)"
        grep -q 'TCC_TARGET_WASM32' "$SRC_COPY/config.h" \
            || fail "config.h guard edit did not take ($TRIPLE)"
    fi

    # Non-native triples only: run the native make here, while
    # config.h still points at the host SDK. The cross rules need
    # the files it generates (tccdefs_.h, libtcc.a), but the native
    # make compiles the host's own libtcc1.a against the host's
    # headers: once the repoint below points config.h at the
    # target's sysroot, the target's glibc/bionic headers do not
    # compile under the host compiler's predefined macros
    # (__APPLE__, host arch), so a native make after the repoint
    # fails. The native triple keeps its own make + make install
    # below, which needs the repointed paths; win32 and wasm32
    # have no repoint, so the move is a no-op for them.
    if [ "$TRIPLE" != "$HOST_TRIPLE" ]; then
        if [ -f "$SRC_COPY/config.mak" ]; then
            (cd "$SRC_COPY" && make -j4) || fail "make ($TRIPLE)"
        else
            fail "no config.mak, skipping make ($TRIPLE)"
        fi
    fi

    step "stage target sysroot ($TRIPLE)"
    # Stage first: the repoint below points config.h at the staged
    # include-libc, and the libtcc1.a build needs stdio.h from there.
    case $TRIPLE in
        *-linux-gnu)
            VDIR=$ROOT/vendor-glibc/$TRIPLE
            if [ ! -d "$VDIR" ]; then
                fail "missing vendored glibc tree: $VDIR"
            else
                mkdir -p "$DIST/lib/tcc/include-libc" "$DIST/lib/tcc/libc"
                cp -r "$VDIR/include/." "$DIST/lib/tcc/include-libc/" \
                    || fail "stage glibc headers ($TRIPLE)"
                cp "$VDIR"/crt/*.o "$DIST/lib/tcc/libc/" \
                    || fail "stage glibc crt ($TRIPLE)"
                printf 'GROUP ( libc.so.6 )\n' > "$DIST/lib/tcc/libc/libc.so"
                printf 'GROUP ( libm.so.6 )\n' > "$DIST/lib/tcc/libc/libm.so"
                cp "$VDIR/COPYING.LIB" "$DIST/lib/tcc/libc/" \
                    || fail "stage glibc COPYING.LIB ($TRIPLE)"
            fi
            ;;
        *-linux-android)
            LIBSRC=$ROOT/vendor-android/$TRIPLE/$ANDROID_API/lib
            if [ ! -d "$LIBSRC" ]; then
                fail "missing vendored bionic tree: $LIBSRC"
            else
                mkdir -p "$DIST/lib/tcc/include-libc" "$DIST/lib/tcc/libc"
                cp -r "$ROOT/vendor-android/include/." "$DIST/lib/tcc/include-libc/" \
                    || fail "stage bionic headers ($TRIPLE)"
                cp "$LIBSRC"/* "$DIST/lib/tcc/libc/" \
                    || fail "stage bionic stubs ($TRIPLE)"
                cp "$ROOT/vendor-android/NOTICE" "$DIST/lib/tcc/libc/" \
                    || fail "stage bionic NOTICE ($TRIPLE)"
            fi
            ;;
        *-apple-darwin)
            python3 "$BUNDLE/darwin-dist.py" stage "$ROOT/vendor-darwin" "$DIST" \
                || fail "darwin-dist stage ($TRIPLE)"
            ;;
        x86_64-w64-mingw32)
            # Headers and .def files now; libtcc1.a after the cross make.
            mkdir -p "$DIST/lib/tcc/win32/lib" "$DIST/lib/tcc/win32/include"
            cp "$SRC_COPY"/include/*.h "$SRC_COPY/tcclib.h" "$DIST/lib/tcc/win32/include/" \
                || fail "stage win32 builtin headers ($TRIPLE)"
            cp -r "$SRC_COPY"/win32/include/. "$DIST/lib/tcc/win32/include/" \
                || fail "stage win32 headers ($TRIPLE)"
            cp "$SRC_COPY"/win32/lib/*.def "$DIST/lib/tcc/win32/lib/" \
                || fail "stage win32 .def files ($TRIPLE)"
            ;;
        wasm32-wasi)
            cp "$ROOT"/wasm-backend/include/*.h "$DIST/lib/tcc/include/" \
                || fail "stage wasm headers ($TRIPLE)"
            ;;
    esac

    step "mirror headers into sources + repoint config.h ($TRIPLE)"
    # The libtcc1 build runs the freshly built tcc with -B pointing at
    # the source tree (lib/Makefile XFLAGS-unx), so {B} in the repointed
    # config.h resolves to the source tree during the build. Mirror the
    # staged headers there so the build finds the bundled stdio.h.
    # darwin uses darwin-dist.py paths (as before); the other staged
    # families use the same mechanism with their own package paths.
    # win32 and wasm32 need neither: the win32 lib build uses the
    # build machine's headers as on Linux, and wasm32 builds no
    # libtcc1.a.
    case $TRIPLE in
        *-apple-darwin)
            cp -r "$DIST/lib/tcc/include-libc" "$SRC_COPY/include-libc" \
                || fail "mirror include-libc into sources ($TRIPLE)"
            python3 "$BUNDLE/darwin-dist.py" paths "$SRC_COPY/config.h" \
                || fail "darwin-dist paths ($TRIPLE)"
            ;;
        *-linux-gnu|*-linux-android)
            cp -r "$DIST/lib/tcc/include-libc" "$SRC_COPY/include-libc" \
                || fail "mirror include-libc into sources ($TRIPLE)"
            repoint_config "$SRC_COPY/config.h" "$SYSINC" "$LIBPATHS"
            ;;
    esac

    step "target runtime library ($TRIPLE)"
    # Native triple: the original recipe (native make + make install).
    # Every other triple: the native make already ran right after
    # configure (see the comment there); only the cross make for
    # libtcc1.a runs here. wasm32 has no libtcc1.a: the native make
    # alone generates what the dual-form compile needs.
    if [ "$TRIPLE" = "$HOST_TRIPLE" ]; then
        if [ -f "$SRC_COPY/config.mak" ]; then
            (cd "$SRC_COPY" && make -j4) || fail "make ($TRIPLE)"
            (cd "$SRC_COPY" && make install) || fail "make install ($TRIPLE)"
        else
            fail "no config.mak, skipping make ($TRIPLE)"
        fi
        if [ -x "$DIST/bin/tcc" ]; then
            note "upstream tcc built: $("$DIST/bin/tcc" -dumpmachine 2>&1) / $("$DIST/bin/tcc" -dumpversion 2>&1)"
        else
            fail "dist/bin/tcc missing after make install ($TRIPLE)"
        fi
        if [ -f "$DIST/lib/tcc/libtcc1.a" ]; then
            note "libtcc1.a present: $DIST/lib/tcc/libtcc1.a"
        else
            fail "dist/lib/tcc/libtcc1.a missing ($TRIPLE)"
        fi
    else
        # The native make already ran, right after configure and
        # before the config.h repoint (see the comment there): the
        # cross make below only needs its generated files.
        case $TRIPLE in
            wasm32-wasi) : ;;
            *)
                (cd "$SRC_COPY" && make "cross-$CT") \
                    || fail "cross make $CT ($TRIPLE)"
                if [ "$TRIPLE" = x86_64-w64-mingw32 ]; then
                    # The win32 target's pthread surface, as in
                    # build.sh's add_win32_pthread: compile pthread.c
                    # with the just-built cross compiler and merge it
                    # into that compiler's libtcc1.a.
                    XTCC="$SRC_COPY/x86_64-win32-tcc"
                    [ -x "$XTCC" ] || XTCC="$SRC_COPY/x86_64-win32-tcc.exe"
                    "$XTCC" -B"$SRC_COPY/win32" -I"$SRC_COPY/include" -I"$SRC_COPY" \
                        -c "$SRC_COPY/win32/lib/pthread.c" \
                        -o "$SRC_COPY/tinyc-win32-pthread.o" \
                        || fail "win32 pthread compile ($TRIPLE)"
                    (cd "$SRC_COPY" && ar r "x86_64-win32-libtcc1.a" \
                        "tinyc-win32-pthread.o") \
                        || fail "win32 pthread merge ($TRIPLE)"
                    cp "$SRC_COPY/$CT-libtcc1.a" "$DIST/lib/tcc/win32/lib/libtcc1.a" \
                        || fail "stage win32 libtcc1.a ($TRIPLE)"
                else
                    cp "$SRC_COPY/$CT-libtcc1.a" "$DIST/lib/tcc/libtcc1.a" \
                        || fail "stage libtcc1.a ($TRIPLE)"
                fi
                ;;
        esac
        # TCC builtin headers and libtcc.h (make install does this for
        # the native triple; the cross triples stage them by hand, as
        # in build_one/build_darwin).
        cp "$SRC_COPY"/include/*.h "$SRC_COPY/tcclib.h" "$DIST/lib/tcc/include/" \
            || fail "stage builtin headers ($TRIPLE)"
        cp "$SRC_COPY/libtcc.h" "$DIST/include/" \
            || fail "stage libtcc.h ($TRIPLE)"
        cp "$ROOT/core/tinyc_io.h" "$DIST/include/" \
            || fail "stage tinyc_io.h ($TRIPLE)"
        ln -sf tinyc "$DIST/bin/tcc"
    fi

    step "library form: libtinyc.dylib ($TRIPLE)"
    # The dylib is compiled for the HOST cpu (it runs on this Mac);
    # the target is set by the -D flags, never by -arch. The native
    # triple keeps its original flag-free invocation.
    if [ "$TRIPLE" = "$HOST_TRIPLE" ]; then
        clang -O2 -dynamiclib -install_name @rpath/libtinyc.dylib \
            -o "$DYLIB" "$SRC_COPY/tcc.c" "$SRC_COPY/tcc_io.c" \
            -I"$SRC_COPY" || fail "clang -dynamiclib ($TRIPLE)"
    else
        clang -O2 -dynamiclib -install_name @rpath/libtinyc.dylib \
            -o "$DYLIB" "$SRC_COPY/tcc.c" "$SRC_COPY/tcc_io.c" \
            -I"$SRC_COPY" $TDEF "${DEFS[@]}" || fail "clang -dynamiclib ($TRIPLE)"
    fi
    if [ -f "$DYLIB" ]; then
        file "$DYLIB"
        nm -gU "$DYLIB" 2>/dev/null | grep -E " (tinyc_main|tcc_new|tcc_compile_string)$" \
            | sed 's/^/  export: /'
    fi

    step "dual-form file: executable with embedded dylib section ($TRIPLE)"
    if [ -f "$DYLIB" ]; then
        if [ "$TRIPLE" = "$HOST_TRIPLE" ]; then
            clang -O2 -o "$DUAL" "$SRC_COPY/tcc.c" "$SRC_COPY/tcc_io.c" \
                -I"$SRC_COPY" \
                -Wl,-sectcreate,__TINYC,__dylib,"$DYLIB" \
                || fail "clang dual-form link ($TRIPLE)"
        else
            clang -O2 -o "$DUAL" "$SRC_COPY/tcc.c" "$SRC_COPY/tcc_io.c" \
                -I"$SRC_COPY" $TDEF "${DEFS[@]}" \
                -Wl,-sectcreate,__TINYC,__dylib,"$DYLIB" \
                || fail "clang dual-form link ($TRIPLE)"
        fi
    else
        fail "no dylib to embed, skipping dual-form link ($TRIPLE)"
    fi
    if [ -x "$DUAL" ]; then
        file "$DUAL"
        otool -l "$DUAL" 2>/dev/null | grep -A4 "sectname __dylib" | sed 's/^/  /'
        "$DUAL" --version 2>&1 | head -1 | sed 's/^/  dual --version: /'
    else
        fail "dual-form executable missing ($TRIPLE)"
    fi

    step "manifest ($TRIPLE)"
    case $TRIPLE in
        *-linux-gnu)
            EXTRA_JSON="{\"libc\": \"glibc $GLIBC_VER headers and crt objects bundled; programs link the system libc.so.6/libm.so.6 dynamically\", \"libc_headers\": \"lib/tcc/include-libc\", \"libc_lib\": \"lib/tcc/libc\"}"
            LIBTCC1_REL="lib/tcc/libtcc1.a"
            ;;
        *-linux-android)
            EXTRA_JSON="{\"libc\": \"bionic headers (the bionic surface at the C/POSIX layer, plus the kernel headers its include closure pulls in) and crt objects bundled (Android NDK $ANDROID_NDK_VER sysroot); programs link the system libc.so/libm.so/libdl.so dynamically\", \"libc_headers\": \"lib/tcc/include-libc\", \"libc_lib\": \"lib/tcc/libc\", \"android_api\": $ANDROID_API}"
            LIBTCC1_REL="lib/tcc/libtcc1.a"
            ;;
        *-apple-darwin)
            EXTRA_JSON="{\"libc\": \"Darwin libc headers bundled (vendor-darwin, self-written); programs link the system libSystem.B.dylib dynamically\", \"libc_headers\": \"lib/tcc/include-libc\", \"libc_lib\": \"lib/tcc/lib\"}"
            LIBTCC1_REL="lib/tcc/libtcc1.a"
            ;;
        x86_64-w64-mingw32)
            EXTRA_JSON=""
            LIBTCC1_REL="lib/tcc/win32/lib/libtcc1.a"
            ;;
        wasm32-wasi)
            EXTRA_JSON="{\"libc\": \"no target libc archive: in the wasm backend's model the runtime helpers are host-provided imports; the sysroot carries the bundled headers only (wasm-backend/include and TCC's builtin headers)\"}"
            LIBTCC1_REL=""
            ;;
    esac
    write_manifest "$DIST" "$TRIPLE" "$ISA_JSON" "$LIBTCC1_REL" "$EXTRA_JSON"

    # The bundled default libraries (libcurl, PCRE2, mbedTLS;
    # see libs-config/README.md), built with this dist's own
    # compiler, as in build.sh's dists.
    local LIBS_FAMILY=""
    case $TRIPLE in
        *-linux-gnu)     LIBS_FAMILY=linux-gnu ;;
        *-linux-android) LIBS_FAMILY=android ;;
        *-apple-darwin)  LIBS_FAMILY=darwin ;;
        *-w64-mingw32)   LIBS_FAMILY=windows ;;
    esac
    if [ -n "$LIBS_FAMILY" ]; then
        step "default libraries ($TRIPLE)"
        bash "$ROOT/tools/build_default_libs.sh" --tcc "$DUAL" \
            --triple "$TRIPLE" --family "$LIBS_FAMILY" --dist "$DIST" \
            || fail "default libraries ($TRIPLE)"
    fi

    step "dist layout ($TRIPLE)"
    find "$DIST" -type f | sort | sed "s|$BUNDLE/|  |"
}

for TRIPLE in $TRIPLES; do
    case $TRIPLE in
        x86_64-linux-gnu)
            build_mac_target "$TRIPLE" x86_64 "-DTCC_TARGET_X86_64" "\"$ISA_X86_64\"" \
                "{B}/include-libc:{B}/include" "{B}/libc:{B}:/usr/x86_64-linux-gnu/lib" \
                '-DCONFIG_SYSROOT="/usr/x86_64-linux-gnu"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include-libc:{B}/include"' \
                '-DCONFIG_TCC_CRTPREFIX="{B}/libc"' \
                '-DCONFIG_TCC_LIBPATHS="{B}/libc:{B}:/usr/x86_64-linux-gnu/lib"'
            ;;
        aarch64-linux-gnu)
            build_mac_target "$TRIPLE" arm64 "-DTCC_TARGET_ARM64" "\"$ISA_ARM64\"" \
                "{B}/include-libc:{B}/include" "{B}/libc:{B}:/usr/aarch64-linux-gnu/lib" \
                '-DCONFIG_SYSROOT="/usr/aarch64-linux-gnu"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include-libc:{B}/include"' \
                '-DCONFIG_TCC_CRTPREFIX="{B}/libc"' \
                '-DCONFIG_TCC_LIBPATHS="{B}/libc:{B}:/usr/aarch64-linux-gnu/lib"'
            ;;
        riscv64-linux-gnu)
            build_mac_target "$TRIPLE" riscv64 "-DTCC_TARGET_RISCV64" "\"$ISA_RISCV64\"" \
                "{B}/include-libc:{B}/include" "{B}/libc:{B}:/usr/riscv64-linux-gnu/lib" \
                '-DCONFIG_SYSROOT="/usr/riscv64-linux-gnu"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include-libc:{B}/include"' \
                '-DCONFIG_TCC_CRTPREFIX="{B}/libc"' \
                '-DCONFIG_TCC_LIBPATHS="{B}/libc:{B}:/usr/riscv64-linux-gnu/lib"'
            ;;
        x86_64-w64-mingw32)
            build_mac_target "$TRIPLE" x86_64-win32 "-DTCC_TARGET_X86_64 -DTCC_TARGET_PE" "\"$ISA_X86_64\"" \
                "" "" \
                "-DCONFIG_TCCDIR=\"$BUNDLE/dist/$TRIPLE/lib/tcc/win32\""
            ;;
        aarch64-linux-android)
            build_mac_target "$TRIPLE" arm64 "-DTCC_TARGET_ARM64 -DTARGETOS_ANDROID=1" "\"$ISA_ARM64\"" \
                "{B}/include:{B}/include-libc:{B}/include-libc/aarch64-linux-android" "{B}/libc:{B}" \
                '-DCONFIG_TCC_CRTPREFIX="{B}/libc"' \
                '-DCONFIG_TCC_LIBPATHS="{B}/libc:{B}"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{B}/include-libc:{B}/include-libc/aarch64-linux-android"' \
                '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"'
            ;;
        x86_64-linux-android)
            build_mac_target "$TRIPLE" x86_64 "-DTCC_TARGET_X86_64 -DTARGETOS_ANDROID=1" "\"$ISA_X86_64\"" \
                "{B}/include:{B}/include-libc:{B}/include-libc/x86_64-linux-android" "{B}/libc:{B}" \
                '-DCONFIG_TCC_CRTPREFIX="{B}/libc"' \
                '-DCONFIG_TCC_LIBPATHS="{B}/libc:{B}"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include:{B}/include-libc:{B}/include-libc/x86_64-linux-android"' \
                '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"'
            ;;
        arm64-apple-darwin)
            build_mac_target "$TRIPLE" arm64-osx "-DTCC_TARGET_ARM64 -DTCC_TARGET_MACHO" "$ISA_DARWIN_ARM64" \
                "{B}/include-libc:{B}/include" "{B}/lib:{B}" \
                '-DCONFIG_TCC_CRTPREFIX="{B}/lib"' \
                '-DCONFIG_TCC_LIBPATHS="{B}/lib:{B}"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include-libc:{B}/include"'
            ;;
        x86_64-apple-darwin)
            build_mac_target "$TRIPLE" x86_64-osx "-DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO" "$ISA_DARWIN_X86_64" \
                "{B}/include-libc:{B}/include" "{B}/lib:{B}" \
                '-DCONFIG_TCC_CRTPREFIX="{B}/lib"' \
                '-DCONFIG_TCC_LIBPATHS="{B}/lib:{B}"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include-libc:{B}/include"'
            ;;
        wasm32-wasi)
            build_mac_target "$TRIPLE" "" "-DTCC_TARGET_WASM32" "\"$ISA_WASM32\"" \
                "" "" \
                '-DCONFIG_TCC_CRTPREFIX="{B}"' \
                '-DCONFIG_TCC_LIBPATHS="{B}"' \
                '-DCONFIG_TCC_SYSINCLUDEPATHS="{B}/include"'
            ;;
        *)
            echo "BUILD-FAIL: unknown target triple: $TRIPLE"
            FAILS=$((FAILS + 1))
            ;;
    esac
done

step "glue extractor (extract_dylib)"
clang -O2 -o "$WORK/extract_dylib" "$BUNDLE/glue/extract_dylib.c" \
    || fail "build extract_dylib"

step "dlopen test host"
# host_dlopen is compiled against the host triple's sources (the
# native target's tree), as before - it is a host tool, not a
# per-target one.
clang -O2 -o "$WORK/host_dlopen" "$BUNDLE/glue/host_dlopen.c" \
    -I"$BUNDLE/build/$HOST_TRIPLE/src" || fail "build host_dlopen"

step "sample local library for the dynamic-add test"
mkdir -p "$WORK/local/include" "$WORK/local/lib"
cp "$BUNDLE/testdata/local/include/mylib.h" "$WORK/local/include/"
clang -O2 -c "$BUNDLE/testdata/local/src/mylib.c" \
    -I"$WORK/local/include" -o "$WORK/local/mylib.o" || fail "compile mylib.c"
ar rcs "$WORK/local/lib/libmylib.a" "$WORK/local/mylib.o" || fail "ar libmylib.a"
[ -f "$WORK/local/lib/libmylib.a" ] && note "libmylib.a ready"

step "dist layout (all)"
find "$BUNDLE/dist" -type f | sort | sed "s|$BUNDLE/|  |"

echo
echo "build-mac.sh done: $FAILS failed step(s)"
exit $FAILS
