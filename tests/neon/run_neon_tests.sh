#!/bin/bash
# Differential tests for the aarch64 NEON surface (design section
# 4.3): every probe source is compiled twice, once by the vendor
# arm64-tcc with the shipped arm_neon.h and once by the aarch64
# cross gcc with its own arm_neon.h; both binaries run under
# qemu-aarch64-static and their outputs must agree byte for byte.
# Probes also carry scalar references for many cases: a reference
# disagreement prints MISMATCH and fails that binary's run.
#
# Usage: run_neon_tests.sh [area ...]   (default: all areas)
set -u
cd "$(dirname "$0")"
ROOT=../..
V=$ROOT/vendor-tinycc
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "neon: $*"; }
bad() { echo "neon FAIL: $*"; FAIL=1; }

[ -x $V/arm64-tcc ] || (cd $V && make cross-arm64 >/dev/null) || { bad "arm64 build"; exit 1; }

MARCH=armv8.2-a+dotprod
if [ $# -gt 0 ]; then
    AREAS="$@"
else
    AREAS=$(cd probes && ls neon_*.c | grep -v tcconly | sed 's/^neon_//;s/\.c$//')
fi

for area in $AREAS; do
    src=probes/neon_$area.c
    [ -f $src ] || { bad "no probe file for area $area"; continue; }
    if ! $V/arm64-tcc -B $V -I $V/include -march=$MARCH \
            -fno-asynchronous-unwind-tables -c $src -o $WORK/tcc.o 2>$WORK/tcc.err; then
        bad "$area: tcc compile: $(head -3 $WORK/tcc.err)"
        continue
    fi
    if ! aarch64-linux-gnu-gcc -o $WORK/tcc.elf $WORK/tcc.o 2>$WORK/link.err; then
        bad "$area: link of tcc object: $(head -3 $WORK/link.err)"
        continue
    fi
    if ! aarch64-linux-gnu-gcc -O2 -march=$MARCH -o $WORK/gcc.elf $src 2>$WORK/gcc.err; then
        bad "$area: gcc compile: $(head -3 $WORK/gcc.err)"
        continue
    fi
    qemu-aarch64-static -cpu max -L /usr/aarch64-linux-gnu $WORK/tcc.elf > $WORK/tcc.out; trc=$?
    qemu-aarch64-static -cpu max -L /usr/aarch64-linux-gnu $WORK/gcc.elf > $WORK/gcc.out; grc=$?
    if [ $trc -ne 0 ]; then bad "$area: tcc binary exit $trc"; continue; fi
    if [ $grc -ne 0 ]; then bad "$area: gcc binary exit $grc"; continue; fi
    if ! cmp -s $WORK/tcc.out $WORK/gcc.out; then
        bad "$area: outputs differ: $(diff $WORK/tcc.out $WORK/gcc.out | head -6)"
        continue
    fi
    note "$area: ok ($(wc -l < $WORK/tcc.out) cases)"
done

# TCC-only probes: ACLE names the reference gcc header does not
# declare; the scalar references still validate them
for src in probes/neon_*_tcconly.c; do
    [ -f $src ] || continue
    area=$(basename $src .c)
    if ! $V/arm64-tcc -B $V -I $V/include -march=$MARCH \
            -fno-asynchronous-unwind-tables -c $src -o $WORK/tcc.o 2>$WORK/tcc.err; then
        bad "$area: tcc compile: $(head -3 $WORK/tcc.err)"
        continue
    fi
    if ! aarch64-linux-gnu-gcc -o $WORK/tcc.elf $WORK/tcc.o 2>$WORK/link.err; then
        bad "$area: link of tcc object: $(head -3 $WORK/link.err)"
        continue
    fi
    qemu-aarch64-static -cpu max -L /usr/aarch64-linux-gnu $WORK/tcc.elf > $WORK/tcc.out; trc=$?
    if [ $trc -ne 0 ]; then bad "$area: tcc binary exit $trc"; continue; fi
    note "$area: ok ($(wc -l < $WORK/tcc.out) cases, tcc only)"
done

if [ $FAIL -eq 0 ]; then
    note "all NEON differential tests passed"
else
    note "FAILURES"
fi
exit $FAIL
