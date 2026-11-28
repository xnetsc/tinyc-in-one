#!/bin/bash
# C standard header probes driver.
#
# For every triple present in build/dist, build the probes in
# tests/cstd/ with that triple's packaged compiler against its
# staged sysroot:
#   probe_iso646.c   <iso646.h>   (filled this round for the
#                                  glibc, bionic and darwin trees)
#   probe_threads.c  <threads.h>  (filled this round for darwin
#                                  and MinGW; regression build
#                                  for the trees that had it)
#   probe_uchar.c    <uchar.h>    (filled this round for darwin;
#                                  regression build elsewhere)
# The linux-gnu triples also run the probes: natively for the
# host triple, under qemu-user for the other two (run legs SKIP
# when the qemu binary is absent, the test_matrix.sh rule; the
# build legs still run).  The android, darwin and windows
# triples get compile + link closure here; their execution
# belongs to the platform CI legs.
#
# Two extra legs prove the new implementations themselves on
# this machine: the darwin threads.c / uchar.c sources are
# compiled with the system compiler against the host pthread
# (with only the two new headers taken from vendor-darwin) and
# the probes run against them, and the wasm32 probes run in
# the browser package's simulated tier via
# tests/cstd/run_wasm_probes.mjs (SKIP when no wasm32 cross
# compiler is available).
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0
note() { echo "$1"; }
fail() { note "FAIL: $1"; FAIL=1; }

DIST=$ROOT/build/dist
PROBES=$ROOT/tests/cstd

build_probe() { # <triple> <probe> [extra tcc args...]
    local triple=$1 probe=$2; shift 2
    local tc=$DIST/$triple/bin/tinyc
    [ -x "$tc" ] || { note "SKIP $triple (no dist)"; return 1; }
    mkdir -p "$TMP/$triple"
    "$tc" "$PROBES/$probe.c" "$@" -o "$TMP/$triple/$probe" 2>"$TMP/$triple/$probe.log" \
        || { fail "$triple: $probe build"; sed 's/^/    /' "$TMP/$triple/$probe.log" | head -5; return 1; }
    return 0
}

run_probe() { # <triple> <probe> <marker> [runner...]
    local triple=$1 probe=$2 marker=$3; shift 3
    if "$@" "$TMP/$triple/$probe" > "$TMP/$triple/$probe.run.log" 2>&1 \
        && grep -q "$marker" "$TMP/$triple/$probe.run.log"; then
        note "$triple: $probe run ok"
    else
        fail "$triple: $probe run"; sed 's/^/    /' "$TMP/$triple/$probe.run.log" | head -5
    fi
}

leg() { # <triple> <mode> [runner...]
    local triple=$1 mode=$2; shift 2
    local built=0
    build_probe "$triple" probe_iso646 && built=1
    if [ "$mode" = run ]; then
        [ $built = 1 ] && run_probe "$triple" probe_iso646 "iso646 probe: OK" "$@"
    fi
    case $triple in
        *darwin*|*mingw32*|*linux-gnu*|*android*)
            if build_probe "$triple" probe_threads; then
                [ "$mode" = run ] && run_probe "$triple" probe_threads "threads probe: OK" "$@"
            fi
            if build_probe "$triple" probe_uchar; then
                [ "$mode" = run ] && run_probe "$triple" probe_uchar "uchar probe: OK" "$@"
            fi
            ;;
    esac
    note "$triple: cstd probes built (compile + link closure)"
}

# --- per-triple legs -------------------------------------------------
HOST_TRIPLE=x86_64-linux-gnu
leg $HOST_TRIPLE run
for t in aarch64-linux-gnu riscv64-linux-gnu; do
    qemu=
    case $t in
        aarch64-*) qemu=qemu-aarch64-static ;;
        riscv64-*) qemu=qemu-riscv64-static ;;
    esac
    if command -v $qemu >/dev/null 2>&1 && [ -d "/usr/$t" ]; then
        leg $t run $qemu -L "/usr/$t"
    else
        leg $t build
        note "$t: run legs SKIP (no $qemu or no /usr/$t sysroot)"
    fi
done
for t in x86_64-linux-android aarch64-linux-android \
         x86_64-apple-darwin arm64-apple-darwin x86_64-w64-mingw32; do
    leg $t build
done

# --- implementation evidence in the packaged archives ----------------
for t in x86_64-apple-darwin arm64-apple-darwin; do
    a=$DIST/$t/lib/tcc/libtcc1.a
    if [ -f "$a" ] && nm "$a" 2>/dev/null | grep -qE ' T _?thrd_create' \
        && nm "$a" 2>/dev/null | grep -qE ' T _?mbrtoc16'; then
        note "$t: libtcc1.a carries thrd_create + mbrtoc16"
    else
        fail "$t: libtcc1.a missing the threads/uchar implementations"
    fi
done
a=$DIST/x86_64-w64-mingw32/lib/tcc/win32/lib/libtcc1.a
if [ -f "$a" ] && nm "$a" 2>/dev/null | grep -qE ' T _?thrd_create' \
    && nm "$a" 2>/dev/null | grep -qE ' T _?mbrtoc16'; then
    note "x86_64-w64-mingw32: libtcc1.a carries thrd_create + mbrtoc16"
else
    fail "x86_64-w64-mingw32: libtcc1.a missing the threads/uchar implementations"
fi

# --- the darwin implementations, run on this host --------------------
SHIMINC=$TMP/shiminc
mkdir -p "$SHIMINC"
cp "$ROOT/vendor-darwin/include/threads.h" "$ROOT/vendor-darwin/include/uchar.h" "$SHIMINC/"
# The darwin uchar state overlays the front bytes of mbstate_t,
# which is a 128-byte union on the darwin target; the host's
# own mbstate_t is smaller, so the host leg supplies the
# darwin form (guarded as _WCHAR_H to keep the host header
# out after it).
cat > "$SHIMINC/wchar.h" <<'EOF'
#ifndef _WCHAR_H
#define _WCHAR_H
typedef union {
	char __mbstate8[128];
	long long __mbstateL;
} mbstate_t;
#endif
EOF
if cc -I"$SHIMINC" "$PROBES/probe_threads.c" "$ROOT/vendor-darwin/lib/threads.c" \
        -lpthread -o "$TMP/host-threads" 2>"$TMP/host-threads.log"; then
    if "$TMP/host-threads" | grep -q "threads probe: OK"; then
        note "host: darwin threads.c implementation runs the probe ok"
    else
        fail "host: darwin threads.c probe run"
    fi
else
    fail "host: darwin threads.c build"; sed 's/^/    /' "$TMP/host-threads.log" | head -5
fi
if cc -I"$SHIMINC" "$PROBES/probe_uchar.c" "$ROOT/vendor-darwin/lib/uchar.c" \
        -o "$TMP/host-uchar" 2>"$TMP/host-uchar.log"; then
    if "$TMP/host-uchar" | grep -q "uchar probe: OK"; then
        note "host: darwin uchar.c implementation runs the probe ok"
    else
        fail "host: darwin uchar.c probe run"
    fi
else
    fail "host: darwin uchar.c build"; sed 's/^/    /' "$TMP/host-uchar.log" | head -5
fi

# --- wasm32 probes ----------------------------------------------------
if [ -n "${TINYC_WASM32_TCC:-}" ] || [ -x "$ROOT/wasm-backend/build-src/wasm32-tcc" ]; then
    if node "$PROBES/run_wasm_probes.mjs" > "$TMP/wasm.log" 2>&1; then
        sed 's/^/wasm32-wasi: /' "$TMP/wasm.log"
    else
        fail "wasm32-wasi: cstd probes"; sed 's/^/    /' "$TMP/wasm.log" | head -10
    fi
else
    note "wasm32-wasi: SKIP (no wasm32 cross compiler)"
fi

if [ $FAIL = 0 ]; then
    note "cstd header probes: all legs ok"
else
    note "cstd header probes: FAILURES"
fi
exit $FAIL
