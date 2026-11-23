#!/bin/bash
# Build / verify entry points for the TinyC Android package.
#
#   ./build.sh           full AAR build via Gradle (needs JDK 17,
#                        Android SDK platform 35, and gradle on PATH
#                        or GRADLE_HOME set)
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
