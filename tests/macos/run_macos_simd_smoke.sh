#!/bin/bash
# Native SIMD smoke tests on macOS (Mach-O executables, run
# natively, outputs diffed against the system clang).
#
# The compiler under test is the shipped macOS kernel,
# mac-test-bundle/dist/<native-triple>/bin/tinyc, built by the bundle step of
# the macOS job from the repo's vendor tree.  Probes compile
# against the dist's own lib/tcc/include - the header set the
# packages ship.
#
# arm64: whole probe files from tests/neon/probes (arithmetic,
# logic, shift, multiply, convert, load/store, float, compare,
# plus the dot-product file when the CPU has dotprod), compiled
# at -march=armv8.2-a+dotprod, the Linux NEON suite's level.
# x86_64: the x86-64-v3 smoke source assembled by
# tests/x86/build_smoke_v3.sh (cases extracted from the
# generated probes), run natively when the CPU has AVX2, plus
# a compile-only leg of a full probe file at x86-64-v4.
#
# A probe binary that is killed before main (exit 137) was
# refused by the arm64 exec policy for want of a signature:
# TCC's Mach-O writer self-signs when its build has
# CONFIG_CODESIGN (the macOS configure sets it) and clang's
# linker ad-hoc signs its output, but if a binary still comes
# up killed this script applies an ad-hoc signature and
# retries once, and says so.
set -u
cd "$(dirname "$0")"
ROOT=../..
# build-mac.sh now lands one dist per target at
# mac-test-bundle/dist/<triple>/; the smoke tests the native
# triple's dist by default, TINYC_MAC_DIST overrides it.
case $(uname -m) in
    arm64) NATIVE_TRIPLE=arm64-apple-darwin ;;
    x86_64) NATIVE_TRIPLE=x86_64-apple-darwin ;;
esac
DIST=${TINYC_MAC_DIST:-$ROOT/mac-test-bundle/dist/$NATIVE_TRIPLE}
TCC=$DIST/bin/tinyc
TCCLIB=$DIST/lib/tcc
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "macos-simd: $*"; }
bad() { echo "macos-simd FAIL: $*"; FAIL=1; }

ARCH=$(uname -m)
note "host arch: $ARCH"

if [ ! -x "$TCC" ]; then
    bad "shipped kernel missing: $TCC (the bundle build step runs before this one)"
    exit 1
fi

# Run $1 into output file $2, retrying once after an ad-hoc
# codesign if the process was killed (exit 137).  Returns the
# binary's exit code.
run_maybe_sign() {
    local bin=$1 out=$2 rc
    "$bin" > "$out" 2>&1; rc=$?
    if [ $rc -eq 137 ]; then
        note "$(basename "$bin"): killed on first run (unsigned Mach-O); applying ad-hoc codesign and retrying"
        codesign -f -s - "$bin" >/dev/null 2>&1
        "$bin" > "$out" 2>&1; rc=$?
    fi
    return $rc
}

# Compile probe source $3 with the shipped kernel (flags $1) and
# with clang (flags $2), run both, compare byte for byte.
# $4 is the probe name used in reports.
diff_probe() {
    local tccflags=$1 refargs=$2 src=$3 name=$4
    if ! $TCC -B $TCCLIB $tccflags "$src" -o $WORK/$name.tcc 2>$WORK/$name.tcc.err; then
        bad "$name: tcc compile: $(head -3 $WORK/$name.tcc.err)"
        return
    fi
    if ! clang $refargs "$src" -o $WORK/$name.ref 2>$WORK/$name.ref.err; then
        bad "$name: clang compile: $(head -3 $WORK/$name.ref.err)"
        return
    fi
    run_maybe_sign $WORK/$name.tcc $WORK/$name.tcc.out; trc=$?
    run_maybe_sign $WORK/$name.ref $WORK/$name.ref.out; rrc=$?
    if [ $trc -ne 0 ]; then bad "$name: tcc binary exit $trc"; return; fi
    if [ $rrc -ne 0 ]; then bad "$name: clang binary exit $rrc"; return; fi
    if grep -q MISMATCH $WORK/$name.tcc.out; then
        bad "$name: tcc scalar reference mismatch: $(grep MISMATCH $WORK/$name.tcc.out | head -3)"
        return
    fi
    if ! cmp -s $WORK/$name.tcc.out $WORK/$name.ref.out; then
        bad "$name: outputs differ: $(diff $WORK/$name.tcc.out $WORK/$name.ref.out | head -6)"
        return
    fi
    note "$name: ok ($(wc -l < $WORK/$name.tcc.out) cases)"
}

if [ "$ARCH" = arm64 ]; then
    # The TCC leg is gated at the Linux suite's level; the
    # clang leg takes no -march (the arm64 macOS baseline
    # includes NEON and dotprod).
    MARCH=armv8.2-a+dotprod
    AREAS="arith cmp cvt float ldst logic mul shift"
    DOTPROD=$(sysctl -n hw.optional.arm.FEAT_DotProd 2>/dev/null || echo 0)
    if [ "$DOTPROD" = 1 ]; then
        AREAS="$AREAS dotprod"
    else
        note "dotprod: runtime area skipped (hw.optional.arm.FEAT_DotProd is not 1)"
    fi
    for area in $AREAS; do
        diff_probe "-march=$MARCH" "-O2 -ffp-contract=off" \
            $ROOT/tests/neon/probes/neon_$area.c neon_$area
    done
elif [ "$ARCH" = x86_64 ]; then
    LEAF7=$(sysctl -n machdep.cpu.leaf7_features 2>/dev/null || echo "")
    HAVE_AVX2=0; HAVE_AVX512=0
    case " $LEAF7 " in *" AVX2 "*) HAVE_AVX2=1 ;; esac
    case " $LEAF7 " in *" AVX512F "*) HAVE_AVX512=1 ;; esac
    note "cpu: AVX2=$HAVE_AVX2 AVX512F=$HAVE_AVX512 (machdep.cpu.leaf7_features)"

    bash $ROOT/tests/x86/build_smoke_v3.sh $WORK/x86_smoke_v3.c || { bad "smoke source assembly"; exit 1; }

    # v4 compile leg: a full probe file at the differential
    # suite's level; compile only (no x86 Mac has AVX-512).
    if ! $TCC -B $TCCLIB -march=x86-64-v4,avx512vbmi,avx512vnni \
            -c $ROOT/tests/x86/probes/x86_misc.c -o $WORK/misc-v4.o 2>$WORK/misc-v4.tcc.err; then
        bad "x86_misc (v4 compile leg): tcc compile: $(head -3 $WORK/misc-v4.tcc.err)"
    elif ! clang -O0 -frounding-math -march=x86-64-v4 -mavx512vbmi -mavx512vnni \
            -I $ROOT/tests/x86/probes -c $ROOT/tests/x86/probes/x86_misc.c -o $WORK/misc-v4-clang.o 2>$WORK/misc-v4.ref.err; then
        bad "x86_misc (v4 compile leg): clang compile: $(head -3 $WORK/misc-v4.ref.err)"
    else
        note "x86_misc: v4 compile legs ok (not run: AVX-512 level)"
    fi

    if ! $TCC -B $TCCLIB -I $ROOT/tests/x86/probes -march=x86-64-v3 \
            $WORK/x86_smoke_v3.c -o $WORK/smoke.tcc 2>$WORK/smoke.tcc.err; then
        bad "x86_smoke_v3: tcc compile: $(head -3 $WORK/smoke.tcc.err)"
    elif ! clang -O0 -frounding-math -ffp-contract=off -march=x86-64-v3 \
            -I $ROOT/tests/x86/probes $WORK/x86_smoke_v3.c -o $WORK/smoke.ref 2>$WORK/smoke.ref.err; then
        bad "x86_smoke_v3: clang compile: $(head -3 $WORK/smoke.ref.err)"
    elif [ $HAVE_AVX2 -eq 0 ]; then
        note "x86_smoke_v3: runtime skipped (host has no AVX2); compile legs ok"
    else
        run_maybe_sign $WORK/smoke.tcc $WORK/smoke.tcc.out; trc=$?
        run_maybe_sign $WORK/smoke.ref $WORK/smoke.ref.out; rrc=$?
        if [ $trc -ne 0 ]; then
            bad "x86_smoke_v3: tcc binary exit $trc"
        elif [ $rrc -ne 0 ]; then
            bad "x86_smoke_v3: clang binary exit $rrc"
        elif grep -q MISMATCH $WORK/smoke.tcc.out; then
            bad "x86_smoke_v3: tcc scalar reference mismatch: $(grep MISMATCH $WORK/smoke.tcc.out | head -3)"
        elif ! cmp -s $WORK/smoke.tcc.out $WORK/smoke.ref.out; then
            bad "x86_smoke_v3: outputs differ: $(diff $WORK/smoke.tcc.out $WORK/smoke.ref.out | head -6)"
        else
            note "x86_smoke_v3: ok ($(wc -l < $WORK/smoke.tcc.out) cases)"
        fi
    fi
else
    bad "unsupported host arch: $ARCH"
fi

if [ $FAIL -eq 0 ]; then
    note "all macOS SIMD smoke tests passed"
else
    note "FAILURES"
fi
exit $FAIL
