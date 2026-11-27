#!/bin/bash
# Build / verify entry points for the TinyC Android package.
#
#   ./build.sh           full AAR build via Gradle (needs JDK 17,
#                        Android SDK platform 35, and gradle on PATH
#                        or GRADLE_HOME set)
#   ./build.sh native-libs
#                        build the real libtinyc-<target>.so for
#                        arm64-v8a and x86_64, one per compile target
#                        (2 ABIs x 9 targets = 18 files, kernel unity
#                        build + JNI glue + kernel adapter) into
#                        src/main/jniLibs/<abi>/, and stage the
#                        toolchain assets (TCC builtin headers +
#                        libtcc1.a) and the sysroot for all 9 targets
#                        into src/main/assets/.
#                        Needs an NDK (NDK_HOME / ANDROID_NDK_HOME)
#                        and the top-level build.sh dists
#                        (build/dist/<triple>/, all 9) present.
#   ./build.sh jvm-test  JVM-side checks only: compiles all Java sources
#                        against android.jar (from ANDROID_HOME or
#                        ANDROID_JAR), generates the JNI header with
#                        javac -h, cross-compiles the JNI glue for
#                        syntax/type checking when an NDK is present,
#                        and runs TinycJvmTest (pure-logic tests that
#                        never call Android APIs at runtime).
#                        Output is tee'd to test-output.txt.
#
# Environment overrides: JAVA_HOME, ANDROID_HOME (or ANDROID_SDK_ROOT),
# GRADLE, NDK_HOME (or ANDROID_NDK_HOME).

set -e
cd "$(dirname "$0")"

MODE="${1:-aar}"

if [ "$MODE" = "jvm-test" ]; then
    exec > >(tee test-output.txt) 2>&1
    echo "== javac version =="
    javac -version

    OUT=build/jvm-test
    rm -rf "$OUT"
    mkdir -p "$OUT/classes" "$OUT/jni-header"

    ANDROID_JAR="${ANDROID_JAR:-}"
    if [ -z "$ANDROID_JAR" ]; then
        SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
        if [ -n "$SDK" ]; then
            ANDROID_JAR=$(ls "$SDK"/platforms/*/android.jar 2>/dev/null | sort | tail -1)
        fi
    fi
    if [ -z "$ANDROID_JAR" ] || [ ! -f "$ANDROID_JAR" ]; then
        echo "android.jar not found; set ANDROID_JAR or ANDROID_HOME" >&2
        exit 1
    fi
    echo "android.jar: $ANDROID_JAR"

    echo "== compile Java (all main sources, against android.jar) =="
    javac -encoding UTF-8 -cp "$ANDROID_JAR" -d "$OUT/classes" \
        src/main/java/com/tinyc/*.java
    echo "OK"

    echo "== compile JVM test =="
    javac -encoding UTF-8 -cp "$OUT/classes" -d "$OUT/classes" \
        src/test/java/com/tinyc/TinycJvmTest.java
    echo "OK"

    echo "== run JVM test =="
    java -cp "$OUT/classes:$ANDROID_JAR" com.tinyc.TinycJvmTest

    echo "== javac -h: generate JNI header =="
    javac -encoding UTF-8 -cp "$OUT/classes" -h "$OUT/jni-header" -d "$OUT/classes" \
        src/main/java/com/tinyc/TinycNative.java
    ls -la "$OUT/jni-header"
    grep -c "JNIEXPORT" "$OUT/jni-header/com_tinyc_TinycNative.h" || true

    echo "== check JNI glue implements every generated prototype =="
    MISSING=0
    for sym in $(grep -o 'Java_com_tinyc_TinycNative_[A-Za-z]*' \
            "$OUT/jni-header/com_tinyc_TinycNative.h" | sort -u); do
        if ! grep -q "$sym" src/main/jni/tinyc_jni.c; then
            echo "MISSING in tinyc_jni.c: $sym"
            MISSING=1
        fi
    done
    [ "$MISSING" = "0" ] && echo "all native methods implemented"

    NDK="${NDK_HOME:-${ANDROID_NDK_HOME:-}}"
    if [ -n "$NDK" ] && [ -d "$NDK/toolchains/llvm/prebuilt" ]; then
        echo "== NDK compile-check of JNI glue (arm64-v8a, x86_64) =="
        PREBUILT=$(ls "$NDK/toolchains/llvm/prebuilt" | head -1)
        TC="$NDK/toolchains/llvm/prebuilt/$PREBUILT/bin"
        LIBTCC_H="${LIBTCC_INCLUDE:-../../vendor-tinycc}"
        API=24
        for ABI_TRIPLE in aarch64-linux-android x86_64-linux-android; do
            CC="$TC/$ABI_TRIPLE$API-clang"
            [ -x "$CC" ] || CC="$TC/$ABI_TRIPLE-clang"
            # The pristine vendor libtcc.h has no tcc_set_io /
            # tcc_get_io declarations (the patches add them in the
            # real kernel tree), so this syntax check treats the
            # resulting implicit-function declarations as warnings,
            # not errors; the real build compiles against the
            # patched tree header and keeps them as errors.
            "$CC" -c src/main/jni/tinyc_jni.c \
                -o "$OUT/tinyc_jni-$ABI_TRIPLE.o" \
                -I"$LIBTCC_H" -I../../core -Isrc/main/jni -Wall -Wextra \
                -Wno-error=implicit-function-declaration
            echo "OK $ABI_TRIPLE"
        done
    else
        echo "== NDK not found, skipping JNI glue compile-check =="
    fi

    echo "== jvm-test done =="
    exit 0
fi

if [ "$MODE" = "manual-aar" ]; then
    # Assemble a structurally valid .aar with SDK tools only (javac +
    # aapt2 + zip). Same layout the Gradle build produces; useful where
    # the Gradle daemon cannot run. jniLibs content is whatever is in
    # src/main/jniLibs (placeholders until the kernel build lands).
    SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
    [ -n "$SDK" ] || { echo "ANDROID_HOME not set" >&2; exit 1; }
    ANDROID_JAR=$(ls "$SDK"/platforms/*/android.jar | sort | tail -1)
    AAPT2=$(ls "$SDK"/build-tools/*/aapt2 | sort | tail -1)
    echo "android.jar: $ANDROID_JAR"
    echo "aapt2: $AAPT2"

    WORK=build/manual-aar
    rm -rf "$WORK"
    mkdir -p "$WORK/classes" "$WORK/stage/jni" "$WORK/stage/META-INF/com/android/build/gradle"

    echo "== compile Java =="
    javac -encoding UTF-8 -cp "$ANDROID_JAR" -d "$WORK/classes" \
        src/main/java/com/tinyc/*.java

    echo "== classes.jar =="
    (cd "$WORK/classes" && "$JAVA_HOME/bin/jar" cf ../stage/classes.jar com)

    echo "== binary AndroidManifest.xml + R.txt via aapt2 =="
    # aapt2 wants a package attribute; the shipped manifest omits it
    # (AGP 8 takes it from the namespace), so inject it into a temp copy.
    sed 's|<manifest |<manifest package="com.tinyc" |' \
        src/main/AndroidManifest.xml > "$WORK/AndroidManifest.xml"
    "$AAPT2" link -o "$WORK/lib.ap_" \
        --manifest "$WORK/AndroidManifest.xml" \
        -I "$ANDROID_JAR" \
        --output-text-symbols "$WORK/stage/R.txt"
    (cd "$WORK" && unzip -q -o lib.ap_ AndroidManifest.xml -d stage)

    echo "== aar-metadata.properties =="
    cat > "$WORK/stage/META-INF/com/android/build/gradle/aar-metadata.properties" <<'EOF'
aarFormatVersion=1.0
aarMetadataVersion=1.0
minCompileSdk=35
minCompileSdkExtension=0
minAndroidGradlePluginVersion=8.6.1
coreLibraryDesugaringEnabled=false
EOF

    echo "== assets + jni =="
    cp -r src/main/assets/. "$WORK/stage/assets/"
    for abi in arm64-v8a x86_64; do
        mkdir -p "$WORK/stage/jni/$abi"
        cp -r "src/main/jniLibs/$abi/." "$WORK/stage/jni/$abi/"
    done

    echo "== zip .aar =="
    mkdir -p build/outputs/aar
    # zip -r updates an existing archive in place and never removes
    # entries that are no longer in the source, so an AAR rebuilt
    # after files were deleted (e.g. the jniLibs placeholders
    # native-libs removes) would keep the stale entries. Delete
    # the output first so the archive is exactly the stage dir.
    rm -f build/outputs/aar/tinyc-android-release.aar
    (cd "$WORK/stage" && zip -q -r "$OLDPWD/build/outputs/aar/tinyc-android-release.aar" .)
    unzip -l build/outputs/aar/tinyc-android-release.aar
    echo "AAR: build/outputs/aar/tinyc-android-release.aar"
    exit 0
fi

# ---- native libraries: real libtinyc-<target>.so, 2 ABIs x 9 targets ----
if [ "$MODE" = "native-libs" ]; then
    ROOT=$(cd ../.. && pwd)
    NDK=${NDK_HOME:-${ANDROID_NDK_HOME:-}}
    if [ -z "$NDK" ] && [ -d "$HOME/workspace/envs/android-toolchain/sdk/ndk" ]; then
        NDK=$HOME/workspace/envs/android-toolchain/sdk/ndk
    fi
    if [ -z "$NDK" ] || [ ! -d "$NDK/toolchains/llvm/prebuilt" ]; then
        echo "Android NDK not found; set NDK_HOME" >&2
        exit 1
    fi
    PREBUILT=$(ls "$NDK/toolchains/llvm/prebuilt" | head -1)
    TC=$NDK/toolchains/llvm/prebuilt/$PREBUILT/bin
    SYSROOT=$NDK/toolchains/llvm/prebuilt/$PREBUILT/sysroot
    API=${TINYC_ANDROID_API:-30}

    # Assemble the patched kernel tree the same way the top-level
    # build.sh does; the native make plus the cross makes below
    # produce one libtcc1.a per target, staged into the toolchain
    # assets. The wasm32 target's .so needs the wasm backend
    # sources and patches merged into the same tree (the same
    # staging build_wasm32 does).
    SRC=build/native-src
    rm -rf "$SRC" build/native-dist
    mkdir -p "$SRC"
    tar cf - --exclude=.git --exclude='*.o' --exclude='./tcc' \
        --exclude='./libtcc.a' --exclude='./libtcc1.a' \
        --exclude='./*-libtcc1.a' --exclude='./*-tcc' \
        --exclude='./c2str.exe' --exclude='./config.h' \
        --exclude='./config.mak' --exclude='./tccdefs_.h' \
        -C "$ROOT/vendor-tinycc" . | tar xf - --no-same-owner -C "$SRC"
    cp "$ROOT/core/tinyc_io.h" "$ROOT/core/tcc_io.c" "$SRC/"
    python3 "$ROOT/patches/apply_patches.py" "$SRC" >/dev/null
    cp "$ROOT/wasm-backend/src/wasm-gen.c" \
       "$ROOT/wasm-backend/src/wasm-link.c" \
       "$ROOT/wasm-backend/src/wasm-int.h" "$SRC/"
    python3 "$ROOT/wasm-backend/patches/apply-patches.py" "$SRC" >/dev/null
    (cd "$SRC" && ./configure --prefix="$PWD/build/native-dist" >/dev/null)
    python3 -c "
import sys
p = sys.argv[1]
t = open(p).read()
old = '|| TCC_TARGET_C67)'
assert old in t, 'TCC_TARGET_C67 guard not found in config.h'
open(p, 'w').write(t.replace(old, '|| TCC_TARGET_C67 || TCC_TARGET_WASM32)', 1))
" "$SRC/config.h"
    (cd "$SRC" && make -j"$(nproc)" >/dev/null)
    for ct in x86_64 arm64 riscv64 x86_64-win32 arm64-osx x86_64-osx; do
        (cd "$SRC" && make "cross-$ct" >/dev/null)
    done

    # The win32 target's pthread surface is implemented by
    # win32/lib/pthread.c over the Win32 primitives in kernel32;
    # it ships statically inside libtcc1.a (the same merge
    # build_one's add_win32_pthread does), so pthread programs
    # need no extra link flag and no runtime DLL beyond the
    # Windows system ones.
    XTCC="$SRC/x86_64-win32-tcc"
    [ -x "$XTCC" ] || XTCC="$SRC/x86_64-win32-tcc.exe"
    "$XTCC" -B"$SRC/win32" -I"$SRC/include" -I"$SRC" \
        -c "$SRC/win32/lib/pthread.c" -o "$SRC/tinyc-win32-pthread.o"
    (cd "$SRC" && ar r "x86_64-win32-libtcc1.a" \
        "tinyc-win32-pthread.o")

    # runmain.o is the support object tcc_run loads for its entry
    # shim. lib/Makefile builds it only in a native lib build (its
    # COMMON_O gains runmain.o only when the lib build is not a
    # cross build), so the cross-arm64 build does not produce one.
    # Build both with the tree's own TCC for each target (the same
    # approach as the Windows host build) and stage them next to
    # libtcc1.a below.
    "$SRC/tcc" -I"$SRC/include" -I"$SRC" \
        -c "$SRC/lib/runmain.c" -o "$SRC/runmain.o"
    "$SRC/arm64-tcc" -I"$SRC/include" -I"$SRC" \
        -c "$SRC/lib/runmain.c" -o "$SRC/arm64-runmain.o"

    # One .so per (device ABI, compile target): 2 ABIs x 9 targets
    # = 18 files, named libtinyc-<triple with '-' as '_>.so so the
    # Java layer can load exactly the target it was asked for
    # (TinycNative.ensureLoaded). Every .so is a native library for
    # the device ABI that runs on the device; only the compiler
    # target baked into it differs. The JNI glue is
    # target-independent, so the same four sources build all 18.
    #
    # The target defines (TDEF) are the same ones the top-level
    # build.sh passes for that target: -DTCC_TARGET_<arch>, plus
    # -DTCC_TARGET_MACHO for the darwin targets, -DTCC_TARGET_PE
    # for win32, -DTCC_TARGET_WASM32 for wasm32, and
    # -DTARGETOS_ANDROID=1 only for the two android targets (it
    # selects the Android ELF interpreter and library model, which
    # is wrong for any other target). The ELF interpreter is baked
    # only for the android targets, for the same reason: the other
    # targets use TCC's default for their own platform.
    #
    # The baked search paths stay the android-style device paths
    # for every target: {B} is the extracted toolchain directory
    # the Java layer registers at runtime, and the system headers
    # and libraries come from the Java-registered sysroot paths
    # (Tinyc.applySysrootIfNeeded), not from these baked strings.
    # No CONFIG_TCCDIR is baked (unlike the top-level win32 dist,
    # which bakes an absolute build-machine path): libtcc1.a sits
    # at the standard <toolchain>/lib/libtcc1.a location for every
    # target, and the win32 headers reach user programs through
    # the Java-registered sysroot.
    #
    # Link-time note, same as on every other host: linking a
    # hosted executable for a linux-gnu target needs the target's
    # runtime libc.so.6 findable through the baked lib paths, and
    # the device has no glibc runtime. The three linux-gnu targets
    # therefore compile to object and link freestanding programs
    # on the device; the other six link fully (darwin via the
    # bundled .tbd stubs, android via the device's bionic, wasm32
    # needs no libc, win32 via msvcrt.dll).
    rm -f src/main/jniLibs/arm64-v8a/libtinyc*.so \
          src/main/jniLibs/x86_64/libtinyc*.so
    build_so() {
        local abi=$1 triple=$2 tdef=$3 elflib=$4
        # The compiler is selected by the DEVICE ABI, never by the
        # compile target: every .so for one ABI is built by that
        # ABI's NDK clang (the .so runs on the device); only the
        # baked TDEF differs between the 9 targets of one ABI.
        case $abi in
            arm64-v8a) local cc=$TC/aarch64-linux-android$API-clang ;;
            x86_64)    local cc=$TC/x86_64-linux-android$API-clang ;;
            *) echo "unknown ABI: $abi" >&2; exit 1 ;;
        esac
        [ -x "$cc" ] || cc=${cc%$API-clang}-clang
        local soname="libtinyc-${triple//-/_}.so"
        echo "building $soname for $abi ($cc)"
        # The output directory is not tracked by git (it holds only
        # build products), so it may not exist on a fresh checkout:
        # create it here, before the link writes into it.
        mkdir -p "src/main/jniLibs/$abi"
        # Device-side search paths: {B} is the extracted toolchain
        # directory the Java layer registers at runtime; its lib/
        # holds libtcc1.a and runmain.o (staged below), and the
        # device's own libc.so lives in /system/lib64. The NDK
        # sysroot path is a build-machine path and does not exist
        # on the device, so it is not a library path.
        local elfopt=()
        [ -n "$elflib" ] && elfopt=("-DCONFIG_TCC_ELFINTERP=\"$elflib\"")
        "$cc" -shared -fPIC -O2 \
            -o "src/main/jniLibs/$abi/$soname" \
            "$SRC/tcc.c" "$SRC/tcc_io.c" \
            src/main/jni/tinyc_jni.c src/main/jni/tinyc_kernel_adapter.c \
            -I"$SRC" -I"$ROOT/core" -Isrc/main/jni \
            $tdef \
            "-DCONFIG_SYSROOT=\"$SYSROOT\"" \
            "-DCONFIG_TCC_CRTPREFIX=\"{B}/lib\"" \
            "-DCONFIG_TCC_LIBPATHS=\"{B}/lib:/system/lib64\"" \
            "-DCONFIG_TCC_SYSINCLUDEPATHS=\"{B}/include:{R}/usr/include:{R}/usr/include/$triple\"" \
            "${elfopt[@]}" \
            -llog -ldl -lm
        file "src/main/jniLibs/$abi/$soname"
    }
    for abi in arm64-v8a x86_64; do
        build_so $abi x86_64-linux-gnu "-DTCC_TARGET_X86_64" ""
        build_so $abi aarch64-linux-gnu "-DTCC_TARGET_ARM64" ""
        build_so $abi riscv64-linux-gnu "-DTCC_TARGET_RISCV64" ""
        build_so $abi x86_64-w64-mingw32 "-DTCC_TARGET_X86_64 -DTCC_TARGET_PE" ""
        build_so $abi aarch64-linux-android "-DTCC_TARGET_ARM64 -DTARGETOS_ANDROID=1" "/system/bin/linker64"
        build_so $abi x86_64-linux-android "-DTCC_TARGET_X86_64 -DTARGETOS_ANDROID=1" "/system/bin/linker64"
        build_so $abi arm64-apple-darwin "-DTCC_TARGET_ARM64 -DTCC_TARGET_MACHO" ""
        build_so $abi x86_64-apple-darwin "-DTCC_TARGET_X86_64 -DTCC_TARGET_MACHO" ""
        build_so $abi wasm32-wasi "-DTCC_TARGET_WASM32" ""
    done
    rm -f src/main/jniLibs/arm64-v8a/.gitkeep src/main/jniLibs/arm64-v8a/PLACEHOLDER.md \
          src/main/jniLibs/x86_64/.gitkeep src/main/jniLibs/x86_64/PLACEHOLDER.md

    # Stage the toolchain assets from the same tree. If the assets
    # were placeholders before, the assets version changes (the Java
    # side keys its extraction cache on ASSETS_VERSION).
    PLACEHOLDER_ASSETS=0
    [ -f src/main/assets/toolchain/aarch64-linux-android/include/.gitkeep ] && PLACEHOLDER_ASSETS=1
    [ -f src/main/assets/sysroot/aarch64-linux-android/include/.gitkeep ] && PLACEHOLDER_ASSETS=1
    ALL_TARGETS="x86_64-linux-gnu aarch64-linux-gnu riscv64-linux-gnu x86_64-w64-mingw32 aarch64-linux-android x86_64-linux-android arm64-apple-darwin x86_64-apple-darwin wasm32-wasi"
    # Manifest assertion: every target's dist must exist and say
    # it is that target. A missing or wrong dist fails the build
    # here, never ships an AAR with a silently empty toolchain or
    # sysroot for that target (the lesson of the once-empty AAR
    # sysroot: any program including a system header fails on the
    # device at its first #include).
    assert_dist() {
        local triple=$1
        local m="$ROOT/build/dist/$triple/manifest.json"
        [ -f "$m" ] || { echo "dist manifest missing for $triple: $m" >&2; echo "run the top-level build.sh first" >&2; exit 1; }
        python3 -c "
import json, sys
m = json.load(open(sys.argv[1]))
assert m.get('target') == sys.argv[2], 'dist manifest target mismatch: %r != %r' % (m.get('target'), sys.argv[2])
" "$m" "$triple" || exit 1
    }
    for triple in $ALL_TARGETS; do assert_dist "$triple"; done
    for triple in $ALL_TARGETS; do
        dst=src/main/assets/toolchain/$triple
        mkdir -p "$dst/include" "$dst/lib"
        cp "$SRC"/include/*.h "$SRC/tcclib.h" "$dst/include/"
        # One libtcc1.a per target from its cross make; the android
        # targets share the same-CPU archive as their linux-gnu
        # twin, and the win32 one carries the merged pthread code.
        # wasm32 has no libtcc1.a: its runtime helpers are
        # host-provided imports, so there is no archive to stage.
        case $triple in
            x86_64-linux-gnu|x86_64-linux-android) ct=x86_64 ;;
            aarch64-linux-gnu|aarch64-linux-android) ct=arm64 ;;
            riscv64-linux-gnu) ct=riscv64 ;;
            x86_64-w64-mingw32) ct=x86_64-win32 ;;
            arm64-apple-darwin) ct=arm64-osx ;;
            x86_64-apple-darwin) ct=x86_64-osx ;;
            wasm32-wasi) ct="" ;;
        esac
        if [ -n "$ct" ]; then
            cp "$SRC/$ct-libtcc1.a" "$dst/lib/libtcc1.a"
        fi
        # runmain.o only for the two android targets: it is the
        # entry shim tcc_run loads, and only a program for the
        # device's own platform can run on the device. The other
        # seven targets follow the top-level dists, which stage no
        # runmain.o for cross targets.
        case $triple in
            aarch64-linux-android) cp "$SRC/arm64-runmain.o" "$dst/lib/runmain.o" ;;
            x86_64-linux-android) cp "$SRC/runmain.o" "$dst/lib/runmain.o" ;;
        esac
        # Target-specific toolchain files from the dist: the darwin
        # .tbd link stubs live in the tccdir's lib/ (the same place
        # the top-level dist puts them), and the win32 tree
        # (headers + .def files) in the tccdir's win32/.
        case $triple in
            arm64-apple-darwin|x86_64-apple-darwin)
                cp "$ROOT/build/dist/$triple/lib/tcc/lib/"*.tbd "$dst/lib/"
                ;;
            x86_64-w64-mingw32)
                mkdir -p "$dst/win32"
                cp -r "$ROOT/build/dist/$triple/lib/tcc/win32/." "$dst/win32/"
                ;;
        esac
        rm -f "$dst/.gitkeep" "$dst/include/.gitkeep" "$dst/lib/.gitkeep"
    done

    # Stage the sysroot for all 9 targets from the top-level
    # build's dists (built by the top-level build.sh before this
    # mode runs in CI; assert_dist above already failed the build
    # if any dist is missing). The device-side compiler reads the
    # system headers and link inputs only from the extracted
    # sysroot (Tinyc.java registers sysrootDir/include and
    # sysrootDir/lib), so an AAR built without this step ships an
    # empty sysroot and any program including a system header fails
    # to compile on the device at its first #include.
    #
    # Per-target sysroot source in the dist (an empty lib means
    # the target has no sysroot lib dir: darwin links the .tbd
    # stubs already staged in the toolchain's lib/, and wasm32
    # needs no libc at all):
    #   linux-gnu, android: include-libc -> include, libc -> lib
    #   darwin:             include-libc -> include
    #   win32:              win32/include -> include, win32/lib -> lib
    #   wasm32:             lib/tcc/include -> include
    for triple in $ALL_TARGETS; do
        case $triple in
            *-linux-gnu|*-linux-android)
                src_inc="$ROOT/build/dist/$triple/lib/tcc/include-libc"
                src_lib="$ROOT/build/dist/$triple/lib/tcc/libc"
                ;;
            *-apple-darwin)
                src_inc="$ROOT/build/dist/$triple/lib/tcc/include-libc"
                src_lib=""
                ;;
            x86_64-w64-mingw32)
                src_inc="$ROOT/build/dist/$triple/lib/tcc/win32/include"
                src_lib="$ROOT/build/dist/$triple/lib/tcc/win32/lib"
                ;;
            wasm32-wasi)
                src_inc="$ROOT/build/dist/$triple/lib/tcc/include"
                src_lib=""
                ;;
        esac
        if [ ! -d "$src_inc" ] || { [ -n "$src_lib" ] && [ ! -d "$src_lib" ]; }; then
            echo "dist sysroot missing for $triple: $src_inc" >&2
            echo "run the top-level build.sh first" >&2
            exit 1
        fi
        dst=src/main/assets/sysroot/$triple
        mkdir -p "$dst/include"
        cp -r "$src_inc/." "$dst/include/"
        if [ -n "$src_lib" ]; then
            mkdir -p "$dst/lib"
            cp -r "$src_lib/." "$dst/lib/"
            rm -f "$dst/lib/.gitkeep"
        fi
        rm -f "$dst/.gitkeep" "$dst/include/.gitkeep"
    done
    if [ $PLACEHOLDER_ASSETS -eq 1 ]; then
        python3 - <<'PYEOF'
import re
p = "src/main/java/com/tinyc/TinycAssets.java"
t = open(p).read()
m = re.search(r'ASSETS_VERSION = "(\d+)"', t)
assert m, "ASSETS_VERSION not found"
t = t.replace(m.group(0), 'ASSETS_VERSION = "%d"' % (int(m.group(1)) + 1))
open(p, "w").write(t)
print("ASSETS_VERSION bumped to", int(m.group(1)) + 1)
PYEOF
    fi
    echo "native libs built: 18 libtinyc-<target>.so in src/main/jniLibs/{arm64-v8a,x86_64}/; toolchain + sysroot assets staged for 9 targets"
    exit 0
fi

# ---- full AAR build ----
GRADLE_BIN="${GRADLE:-gradle}"
if ! command -v "$GRADLE_BIN" >/dev/null 2>&1; then
    echo "gradle not found on PATH; set GRADLE=/path/to/gradle" >&2
    exit 1
fi
if [ -z "$ANDROID_HOME" ] && [ -z "$ANDROID_SDK_ROOT" ]; then
    echo "ANDROID_HOME / ANDROID_SDK_ROOT not set" >&2
    exit 1
fi
SDK="${ANDROID_HOME:-$ANDROID_SDK_ROOT}"
printf 'sdk.dir=%s\n' "$SDK" > local.properties
"$GRADLE_BIN" assembleRelease
echo "AAR: build/outputs/aar/tinyc-android-release.aar"
