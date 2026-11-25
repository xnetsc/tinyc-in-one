#!/bin/bash
# Differential tests for the x86_64 SIMD surface: every probe
# source is compiled twice, once by the vendor tcc with the
# shipped intrinsic headers and once by gcc with its own
# headers; both binaries run natively on this host and their
# outputs must agree byte for byte.  A probe whose scalar
# reference disagrees prints MISMATCH and fails the run.
#
# The probes are compiled at x86-64-v4 (AVX-512) whether or
# not the host can execute that level: the compile legs are
# checks of their own.  The runtime legs need an AVX-512 host
# (without one the binaries die with SIGILL, which says
# nothing about the compiler), so on a host without AVX-512
# they are skipped -- each skip is printed, the summary
# counts them, and the run still passes on the compile legs.
#
# Usage: run_x86_tests.sh [area ...]   (default: all areas)
set -u
cd "$(dirname "$0")"
ROOT=../..
V=$ROOT/vendor-tinycc
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0
SKIPPED=0

note() { echo "x86: $*"; }
bad() { echo "x86 FAIL: $*"; FAIL=1; }

[ -x $V/tcc ] || (cd $V && make >/dev/null) || { bad "tcc build"; exit 1; }

# the feature level the probes are compiled at (see
# run_gating_tests.sh for the level-specific checks)
MARCH_TCC=x86-64-v4,avx512vbmi,avx512vnni
MARCH_GCC="x86-64-v4 -mavx512vbmi -mavx512vnni"

HAVE_AVX512=1
grep -qw avx512f /proc/cpuinfo || HAVE_AVX512=0

if [ $# -gt 0 ]; then
    AREAS="$@"
else
    AREAS=$(cd probes && ls x86_*.c 2>/dev/null | grep -v tcconly |
            sed 's/^x86_//;s/\.c$//')
fi

for area in $AREAS; do
    src=probes/x86_$area.c
    [ -f $src ] || { bad "no probe file for area $area"; continue; }
    if ! $V/tcc -B $V -I $V/include -march=$MARCH_TCC \
            -o $WORK/tcc.elf $src -lm 2>$WORK/tcc.err; then
        bad "$area: tcc compile: $(head -3 $WORK/tcc.err)"
        continue
    fi
    if ! gcc -O0 -frounding-math -march=$MARCH_GCC -o $WORK/gcc.elf $src -lm \
            2>$WORK/gcc.err; then
        bad "$area: gcc compile: $(head -3 $WORK/gcc.err)"
        continue
    fi
    if [ $HAVE_AVX512 -eq 0 ]; then
        echo "x86: $area: runtime skipped (host has no AVX-512); compile legs ok"
        SKIPPED=$((SKIPPED+1))
        continue
    fi
    if [ $area = vbmi ] && ! grep -qw avx512vbmi /proc/cpuinfo; then
        echo "x86: vbmi: runtime skipped (host has no AVX512VBMI); compile legs ok"
        SKIPPED=$((SKIPPED+1))
        continue
    fi
    $WORK/tcc.elf > $WORK/tcc.out; trc=$?
    $WORK/gcc.elf > $WORK/gcc.out; grc=$?
    if [ $trc -ne 0 ]; then bad "$area: tcc binary exit $trc"; continue; fi
    if [ $grc -ne 0 ]; then bad "$area: gcc binary exit $grc"; continue; fi
    if grep -q MISMATCH $WORK/tcc.out; then
        bad "$area: tcc scalar reference mismatch"
        continue
    fi
    if grep -q MISMATCH $WORK/gcc.out; then
        bad "$area: gcc scalar reference mismatch"
        continue
    fi
    if ! cmp -s $WORK/tcc.out $WORK/gcc.out; then
        bad "$area: outputs differ: $(diff $WORK/tcc.out $WORK/gcc.out | head -6)"
        continue
    fi
    note "$area: ok ($(wc -l < $WORK/tcc.out) cases)"
done

# TCC-only probes: surface the reference compiler does not
# declare the same way; the scalar references still validate
for src in probes/x86_*_tcconly.c; do
    [ -f $src ] || continue
    area=$(basename $src .c)
    if ! $V/tcc -B $V -I $V/include -march=$MARCH_TCC \
            -o $WORK/tcc.elf $src -lm 2>$WORK/tcc.err; then
        bad "$area: tcc compile: $(head -3 $WORK/tcc.err)"
        continue
    fi
    if [ $HAVE_AVX512 -eq 0 ]; then
        echo "x86: $area: runtime skipped (host has no AVX-512); compile leg ok"
        SKIPPED=$((SKIPPED+1))
        continue
    fi
    $WORK/tcc.elf > $WORK/tcc.out; trc=$?
    if [ $trc -ne 0 ]; then bad "$area: tcc binary exit $trc"; continue; fi
    if grep -q MISMATCH $WORK/tcc.out; then
        bad "$area: scalar reference mismatch"
        continue
    fi
    note "$area: ok ($(wc -l < $WORK/tcc.out) cases, tcc only)"
done

if [ $SKIPPED -gt 0 ]; then
    note "$SKIPPED runtime leg(s) skipped (host has no AVX-512); compile legs ran for every area"
fi
if [ $FAIL -eq 0 ]; then
    note "all x86 differential tests passed"
else
    note "FAILURES"
fi
exit $FAIL
