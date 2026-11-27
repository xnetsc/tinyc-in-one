#!/bin/bash
# SDK-free verification for the bundled Darwin headers
# (vendor-darwin/) on a macOS machine.
#
# Phases:
#   1. Reference: clang compiles sdkfree_probe.c and
#      sdkfree_battery.c against the platform SDK headers and
#      runs both.  This pins the platform's own ABI values and
#      proves the battery itself is sound C.
#   2. Isolation: the SDK directory is moved aside (restored on
#      exit, whatever happens) and SDKROOT is pointed at an empty
#      directory.  A negative control - compiling a TU that
#      includes an SDK-only header with the packaged tinyc -
#      must fail; if it compiles, SDK headers are leaking into
#      the packaged compiler's search paths and the run is void.
#   3. Packaged: the packaged tinyc (mac-test-bundle/dist) with
#      its baked-in package paths compiles and runs the battery
#      (functional sections) and the probe (ABI values).  The
#      probe output must diff empty against the reference, INFO
#      lines (deliberately oversized setjmp buffers) excluded.
#
# Why moving the SDK is necessary even though the packaged
# compiler's include paths are package paths: TCC's Mach-O
# support adds the SDK's library directory to the library search
# path at runtime (tcc_add_macos_sdkpath), and the compiler was
# built on a machine whose SDK must not silently satisfy a
# bundled-header gap either.  Hiding the SDK makes any residual
# SDK dependency fail loudly in both directions.
#
# Output is PASS:/FAIL: lines; exit status is the FAIL count.
# Intended CI step (macos jobs, after the bundle test step):
#   bash tests/macos/run_macos_sdkfree.sh \
#       > test-outputs/mac-sdkfree.log 2>&1
#
# No set -e: every phase reports, and the SDK is restored by the
# EXIT trap even on failure.

cd "$(dirname "$0")" || exit 99
HERE=$PWD
ROOT=$(cd "$HERE/../.." && pwd)
DIST="$ROOT/mac-test-bundle/dist"
TINYC="$DIST/bin/tinyc"
WORK=$(mktemp -d /tmp/tinyc-sdkfree.XXXXXX)
FAILS=0

pass() { echo "PASS: $*"; }
fail() { echo "FAIL: $*"; FAILS=$((FAILS + 1)); }

SDK=$(xcrun --show-sdk-path 2>/dev/null)
HIDDEN=""

restore_sdk() {
    if [ -n "$HIDDEN" ] && [ -e "$HIDDEN" ]; then
        sudo mv "$HIDDEN" "$SDK" 2>/dev/null \
            || echo "FAIL: SDK restore failed: $HIDDEN -> $SDK"
    fi
}
trap restore_sdk EXIT

echo "=== sdkfree: environment ==="
uname -m
echo "tinyc: $TINYC"
echo "sdk: $SDK"
[ -x "$TINYC" ] || { fail "packaged tinyc missing: $TINYC"; echo "sdkfree done: $FAILS failure(s)"; exit $FAILS; }
[ -d "$DIST/lib/tcc/include-libc" ] \
    || fail "bundled headers not staged in dist (include-libc missing)"
[ -f "$DIST/lib/tcc/lib/libc.tbd" ] \
    || fail "libSystem stub not staged in dist (libc.tbd missing)"

echo "=== sdkfree: reference (clang + SDK headers) ==="
if clang -O1 -o "$WORK/probe-ref" "$HERE/sdkfree_probe.c" 2>"$WORK/ref-probe.log"; then
    "$WORK/probe-ref" > "$WORK/probe-ref.txt" \
        && pass "reference probe runs" \
        || fail "reference probe run failed"
else
    fail "reference probe does not compile with SDK headers"
    sed 's/^/  /' "$WORK/ref-probe.log" | head -5
fi
if clang -O1 -o "$WORK/batt-ref" "$HERE/sdkfree_battery.c" 2>"$WORK/ref-batt.log"; then
    if "$WORK/batt-ref" > "$WORK/batt-ref.txt" 2>&1; then
        pass "reference battery passes with SDK headers"
    else
        fail "battery itself fails with SDK headers (battery bug, not a header bug)"
        sed 's/^/  /' "$WORK/batt-ref.txt" | head -10
    fi
else
    fail "battery does not compile with SDK headers"
    sed 's/^/  /' "$WORK/ref-batt.log" | head -5
fi

echo "=== sdkfree: isolate (hide the SDK) ==="
EMPTY_SDK="$WORK/empty-sdk"
mkdir -p "$EMPTY_SDK"
if [ -n "$SDK" ] && [ -e "$SDK" ]; then
    if sudo mv "$SDK" "$SDK.tinyc-hidden" 2>"$WORK/mv.log"; then
        HIDDEN="$SDK.tinyc-hidden"
        pass "SDK hidden: $SDK"
    else
        fail "could not hide SDK (sudo mv failed)"
        sed 's/^/  /' "$WORK/mv.log" | head -3
    fi
else
    fail "no SDK path to hide (xcrun --show-sdk-path: '$SDK')"
fi
export SDKROOT="$EMPTY_SDK"

echo "=== sdkfree: negative control (SDK-only header must not resolve) ==="
# The witness must be a header the platform SDK has and the
# bundled set deliberately does not ship: <resolv.h> is in the
# SDK and outside the bundled scope (the resolver library; the
# lookup surface, <netdb.h>, is bundled), so resolving it with
# the SDK hidden can only mean leakage.
# <dispatch/dispatch.h> must NOT be used here: the bundled set
# ships its own minimal dispatch/dispatch.h, so resolving it
# proves nothing about the SDK.
printf '#include <resolv.h>\nint main(void){return 0;}\n' > "$WORK/ctrl.c"
if "$TINYC" -B"$DIST/lib/tcc" -c "$WORK/ctrl.c" -o "$WORK/ctrl.o" 2>"$WORK/ctrl.log"; then
    fail "SDK leakage: <resolv.h> resolved with the SDK hidden"
else
    pass "SDK-only header correctly unresolvable with the SDK hidden"
fi

echo "=== sdkfree: packaged tinyc, bundled headers only ==="
if "$TINYC" -B"$DIST/lib/tcc" "$HERE/sdkfree_battery.c" -o "$WORK/batt" 2>"$WORK/batt.log"; then
    pass "battery compiles with bundled headers"
    if "$WORK/batt" > "$WORK/batt.txt" 2>&1; then
        pass "battery runs green against system libSystem"
    else
        fail "battery run reported failures"
    fi
    sed 's/^/  /' "$WORK/batt.txt"
else
    fail "battery does not compile with bundled headers"
    sed 's/^/  /' "$WORK/batt.log" | head -10
fi
if "$TINYC" -B"$DIST/lib/tcc" "$HERE/sdkfree_probe.c" -o "$WORK/probe" 2>"$WORK/probe.log"; then
    pass "probe compiles with bundled headers"
    if "$WORK/probe" > "$WORK/probe.txt" 2>&1; then
        grep -v '^INFO ' "$WORK/probe.txt" > "$WORK/probe-got.txt"
        grep -v '^INFO ' "$WORK/probe-ref.txt" > "$WORK/probe-want.txt"
        if diff -u "$WORK/probe-want.txt" "$WORK/probe-got.txt" > "$WORK/probe.diff"; then
            pass "probe output matches the SDK reference exactly"
        else
            fail "probe output differs from the SDK reference"
            sed 's/^/  /' "$WORK/probe.diff" | head -40
        fi
        grep '^INFO ' "$WORK/probe-ref.txt" | sed 's/^/  platform: /'
        grep '^INFO ' "$WORK/probe.txt" | sed 's/^/  bundled:  /'
    else
        fail "probe run failed"
    fi
else
    fail "probe does not compile with bundled headers"
    sed 's/^/  /' "$WORK/probe.log" | head -10
fi

restore_sdk
trap - EXIT
[ -e "$SDK" ] && pass "SDK restored" || fail "SDK not restored: $SDK"

echo
echo "sdkfree done: $FAILS failure(s)"
exit $FAILS
