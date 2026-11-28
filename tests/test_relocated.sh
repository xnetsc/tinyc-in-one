#!/bin/bash
# Relocated-dist test: the packaged compiler finds its own lib/tcc
# from its own location, without -B.
#
# Copies of three built dists go to a fresh directory and the
# originals are moved aside for the duration of the test, so the
# baked CONFIG_TCCDIR (the build tree's absolute path) cannot mask
# a broken self-location resolution:
#
#   process form: the copied x86_64-linux-gnu dist's bin/tinyc
#       compiles and runs hello.c with no -B;
#   android target: the copied x86_64-linux-android dist's bin/tcc
#       builds the default-libs battery 1 with no -B (compile and
#       link only - the binary runs on the emulator leg in CI);
#   library form: a host dlopen()s the copied x86_64-linux-gnu
#       host dist's dual-form file and compiles through both
#       tinyc_main and the tcc_* API, setting no library path.
#
# Expects the dists under build/dist and build/dist-host (run
# build.sh first). Linux x86_64 hosts only, like the binaries it
# runs.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD

CLI_DIST=${CLI_DIST:-build/dist/x86_64-linux-gnu}
AND_DIST=${AND_DIST:-build/dist/x86_64-linux-android}
HOST_DIST=${HOST_DIST:-build/dist-host/x86_64-linux-gnu}
for d in "$CLI_DIST" "$AND_DIST" "$HOST_DIST"; do
    [ -x "$ROOT/$d/bin/tinyc" ] || { echo "FAIL: $d/bin/tinyc missing - run build.sh first"; exit 1; }
done

# A previous killed run may have left the originals aside.
[ -d build/dist.away ] && [ ! -d build/dist ] && mv build/dist.away build/dist
[ -d build/dist-host.away ] && [ ! -d build/dist-host ] && mv build/dist-host.away build/dist-host

WORK=$(mktemp -d "${TMPDIR:-/tmp}/tinyc-relocated.XXXXXX")
restore() {
    [ -d "$ROOT/build/dist.away" ] && mv "$ROOT/build/dist.away" "$ROOT/build/dist"
    [ -d "$ROOT/build/dist-host.away" ] && mv "$ROOT/build/dist-host.away" "$ROOT/build/dist-host"
    rm -rf "$WORK"
}
trap restore EXIT

mkdir -p "$WORK/cli" "$WORK/android" "$WORK/hostlib" "$WORK/out"
cp -a "$CLI_DIST" "$WORK/cli/dist"
cp -a "$AND_DIST" "$WORK/android/dist"
cp -a "$HOST_DIST" "$WORK/hostlib/dist"
mv build/dist build/dist.away
mv build/dist-host build/dist-host.away

cat > "$WORK/hello.c" <<'EOF'
#include <stdio.h>
int main(void) { printf("hello from tinyc\n"); return 0; }
EOF

FAIL=0

echo "== process form: copied dist compiles and runs hello, no -B"
"$WORK/cli/dist/bin/tinyc" "$WORK/hello.c" -o "$WORK/out/hello" \
    || { echo "FAIL: relocated CLI compile"; FAIL=1; }
if [ $FAIL = 0 ]; then
    OUT=$("$WORK/out/hello") || FAIL=1
    echo "$OUT"
    [ "$OUT" = "hello from tinyc" ] || { echo "FAIL: hello output"; FAIL=1; }
fi

echo "== android target: copied dist builds battery 1, no -B"
"$WORK/android/dist/bin/tcc" "$ROOT/tests/default_libs.c" \
    -lpcre2 -lmbedcrypto -o "$WORK/out/battery1" \
    || { echo "FAIL: relocated battery1 build"; FAIL=1; }

echo "== library form: dlopen the copied host dist, no lib path set"
gcc -O2 -o "$WORK/api_host" "$ROOT/tests/consumer/api_compile_host.c" -ldl \
    || { echo "FAIL: api host build"; exit 1; }
"$WORK/api_host" "$WORK/hostlib/dist/bin/tinyc" "$WORK/hello.c" "$WORK/out" \
    || { echo "FAIL: relocated library form"; FAIL=1; }

[ $FAIL = 0 ] && echo "relocated dist test: ok" || echo "relocated dist test: FAIL"
exit $FAIL
