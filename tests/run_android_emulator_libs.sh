#!/bin/bash
# Default libraries battery on the Android emulator.
#
# Two phases, driven from the android-emulator CI job:
#
#   build <dist-tarball>
#       Extract the linux-x64 dist tarball (the release-assets
#       artifact the job already fetches for the AAR) and build
#       the battery programs for x86_64-linux-android with that
#       dist's packaged compiler against the staged default
#       libraries. The programs land in build/android-libs-test/.
#
#   run
#       With the emulator up and adb on PATH: push the two
#       battery executables to /data/local/tmp and run them
#       there - real execution of the shipped compiler's output
#       against the staged libcurl/PCRE2/mbedTLS archives on the
#       Android runtime. battery2 serves and fetches over
#       loopback inside the device and writes its CA file in
#       the working directory, as on the other platforms.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
WORK=$ROOT/build/android-libs-test
TRIPLE=x86_64-linux-android

case ${1:-} in
build)
    TARBALL=${2:?usage: run_android_emulator_libs.sh build <dist-tarball>}
    rm -rf "$WORK"
    mkdir -p "$WORK/dist"
    tar -xzf "$TARBALL" -C "$WORK/dist"
    TC=$WORK/dist/$TRIPLE/bin/tinyc
    [ -x "$TC" ] || { echo "FAIL: no $TRIPLE compiler in $TARBALL"; exit 1; }
    # The dist tarball's compiler bakes an absolute CONFIG_TCCDIR
    # (the linux-x64 build machine's build/dist path), so from an
    # extracted tarball on another runner it cannot find its own
    # sysroot - headers, crt objects, libtcc1.a - and the link
    # fails with `file 'crtbegin_dynamic.o' not found`. Point it
    # at the extracted lib/tcc with -B, the same mechanism the npm
    # package uses for a relocated compiler.
    BFLAG="-B$WORK/dist/$TRIPLE/lib/tcc"
    "$TC" "$BFLAG" "$ROOT/tests/default_libs.c" -lpcre2 -lmbedcrypto \
        -o "$WORK/battery1" || { echo "FAIL: battery1 build"; exit 1; }
    "$TC" "$BFLAG" "$ROOT/tests/default_libs_net.c" \
        -lcurl -lmbedtls -lmbedx509 -lmbedcrypto \
        -o "$WORK/battery2" || { echo "FAIL: battery2 build"; exit 1; }
    echo "android emulator libs battery: built for $TRIPLE"
    ;;
run)
    DEVDIR=/data/local/tmp/tinyc-libs-test
    adb shell mkdir -p "$DEVDIR" || exit 1
    adb push "$WORK/battery1" "$WORK/battery2" "$DEVDIR/" || exit 1
    adb shell chmod 755 "$DEVDIR/battery1" "$DEVDIR/battery2" || exit 1
    FAIL=0
    OUT=$(adb shell "cd $DEVDIR && ./battery1") || FAIL=1
    echo "$OUT"
    echo "$OUT" | grep -q "default libs battery 1 ok" || FAIL=1
    OUT=$(adb shell "cd $DEVDIR && ./battery2") || FAIL=1
    echo "$OUT"
    echo "$OUT" | grep -q "default libs battery 2 ok" || FAIL=1
    [ $FAIL = 0 ] && echo "android emulator libs battery: run ok" \
                  || echo "FAIL: android emulator libs battery run"
    exit $FAIL
    ;;
*)
    echo "usage: $0 build <dist-tarball> | run" >&2
    exit 2
    ;;
esac
