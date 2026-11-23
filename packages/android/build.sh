#!/bin/bash
# Build / verify entry points for the TinyC Android package.
#
#   ./build.sh           full AAR build via Gradle (needs JDK 17,
#                        Android SDK platform 35, and gradle on PATH
#                        or GRADLE_HOME set)
#   ./build.sh native-libs
#                        build the real libtinyc.so for arm64-v8a and
#                        x86_64 with the NDK clang (kernel unity build
#                        + JNI glue + kernel adapter, one shared library
#                        per ABI) into src/main/jniLibs/<abi>/, and
#                        stage the toolchain assets (TCC builtin
#                        headers + libtcc1.a) into src/main/assets/.
#                        Needs an NDK (NDK_HOME / ANDROID_NDK_HOME).
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
            "$CC" -c src/main/jni/tinyc_jni.c \
                -o "$OUT/tinyc_jni-$ABI_TRIPLE.o" \
                -I"$LIBTCC_H" -Isrc/main/jni -Wall -Wextra
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
    (cd "$WORK/stage" && zip -q -r "$OLDPWD/build/outputs/aar/tinyc-android-release.aar" .)
    unzip -l build/outputs/aar/tinyc-android-release.aar
    echo "AAR: build/outputs/aar/tinyc-android-release.aar"
    exit 0
fi

# ---- native libraries: real libtinyc.so for both ABIs ----
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
    # build.sh does; the native make also produces the libtcc1.a
    # files staged into the toolchain assets below.
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
    (cd "$SRC" && ./configure --prefix="$PWD/build/native-dist" >/dev/null)
    (cd "$SRC" && make -j"$(nproc)" >/dev/null)
    (cd "$SRC" && make cross-arm64 >/dev/null)

    build_so() {
        local abi=$1 triple=$2 tdef=$3
        local cc=$TC/$triple$API-clang
        [ -x "$cc" ] || cc=$TC/$triple-clang
        echo "building libtinyc.so for $abi ($cc)"
        "$cc" -shared -fPIC -O2 \
            -o "src/main/jniLibs/$abi/libtinyc.so" \
            "$SRC/tcc.c" "$SRC/tcc_io.c" \
            src/main/jni/tinyc_jni.c src/main/jni/tinyc_kernel_adapter.c \
            -I"$SRC" -I"$ROOT/core" -Isrc/main/jni \
            "$tdef" -DTARGETOS_ANDROID=1 \
            "-DCONFIG_SYSROOT=\"$SYSROOT\"" \
            "-DCONFIG_TCC_CRTPREFIX=\"{R}/usr/lib/$triple/$API\"" \
            "-DCONFIG_TCC_LIBPATHS=\"{R}/usr/lib/$triple/$API:{B}\"" \
            "-DCONFIG_TCC_SYSINCLUDEPATHS=\"{B}/include:{R}/usr/include:{R}/usr/include/$triple\"" \
            '-DCONFIG_TCC_ELFINTERP="/system/bin/linker64"' \
            -llog -ldl -lm
        file "src/main/jniLibs/$abi/libtinyc.so"
    }
    build_so arm64-v8a aarch64-linux-android -DTCC_TARGET_ARM64
    build_so x86_64 x86_64-linux-android -DTCC_TARGET_X86_64
    rm -f src/main/jniLibs/arm64-v8a/.gitkeep src/main/jniLibs/arm64-v8a/PLACEHOLDER.md \
          src/main/jniLibs/x86_64/.gitkeep src/main/jniLibs/x86_64/PLACEHOLDER.md

    # Stage the toolchain assets from the same tree. If the assets
    # were placeholders before, the assets version changes (the Java
    # side keys its extraction cache on ASSETS_VERSION).
    PLACEHOLDER_ASSETS=0
    [ -f src/main/assets/toolchain/aarch64-linux-android/include/.gitkeep ] && PLACEHOLDER_ASSETS=1
    for triple in aarch64-linux-android x86_64-linux-android; do
        dst=src/main/assets/toolchain/$triple
        mkdir -p "$dst/include" "$dst/lib"
        cp "$SRC"/include/*.h "$SRC/tcclib.h" "$dst/include/"
        if [ "$triple" = aarch64-linux-android ]; then
            cp "$SRC/arm64-libtcc1.a" "$dst/lib/libtcc1.a"
        else
            cp "$SRC/libtcc1.a" "$dst/lib/libtcc1.a"
        fi
        rm -f "$dst/.gitkeep" "$dst/include/.gitkeep" "$dst/lib/.gitkeep"
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
    echo "native libs built: src/main/jniLibs/{arm64-v8a,x86_64}/libtinyc.so"
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
