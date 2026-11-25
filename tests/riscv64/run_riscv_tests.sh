#!/bin/bash
# Differential tests for the riscv64 RVV fixed-length surface
# (design section 7): every probe source is compiled twice, once
# by the vendor riscv64-tcc with the shipped riscv_vector.h and
# once by the riscv64 cross gcc with its own riscv_vector.h; both
# binaries run under qemu-riscv64-static with a matching VLEN and
# their outputs must agree byte for byte.
#
# Usage: run_riscv_tests.sh [area ...]   (default: all areas)
set -u
cd "$(dirname "$0")"
ROOT=../..
V=$ROOT/vendor-tinycc
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "riscv64: $*"; }
bad() { echo "riscv64 FAIL: $*"; FAIL=1; }

[ -x $V/riscv64-tcc ] || (cd $V && make cross-riscv64 >/dev/null) || { bad "riscv64 build"; exit 1; }

# tcc -march and the qemu VLEN it is paired with
CONFIGS="rv64gcv_zvl128b:128 rv64gcv_zvl256b:256"

if [ $# -gt 0 ]; then
    AREAS="$@"
else
    AREAS=$(cd probes && ls riscv_*.c | grep -v tcconly | sed 's/^riscv_//;s/\.c$//')
fi

for cfg in $CONFIGS; do
    MARCH=${cfg%%:*}
    VLEN=${cfg##*:}
    # the gcc leg is compiled once per VLEN so its code matches the
    # fixed-length assumptions of the probe sources
    for area in $AREAS; do
        src=probes/riscv_$area.c
        [ -f $src ] || { bad "no probe file for area $area"; continue; }
        # probes named *_v128 pin their lane counts to VLEN 128;
        # lane-generic probes (vl from sizeof) run at every VLEN
        case $area in *_v128) [ $VLEN = 128 ] || continue ;; esac
        if ! $V/riscv64-tcc -B $V -I $V/include -march=$MARCH \
                -fno-asynchronous-unwind-tables -c $src -o $WORK/tcc.o 2>$WORK/tcc.err; then
            bad "$area [$MARCH]: tcc compile: $(head -3 $WORK/tcc.err)"
            continue
        fi
        if ! riscv64-linux-gnu-gcc -o $WORK/tcc.elf $WORK/tcc.o 2>$WORK/link.err; then
            bad "$area [$MARCH]: link of tcc object: $(head -3 $WORK/link.err)"
            continue
        fi
        if ! riscv64-linux-gnu-gcc -O2 -march=rv64gcv -o $WORK/gcc.elf $src 2>$WORK/gcc.err; then
            bad "$area [$MARCH]: gcc compile: $(head -3 $WORK/gcc.err)"
            continue
        fi
        qemu-riscv64-static -cpu rv64,v=true,vlen=$VLEN -L /usr/riscv64-linux-gnu $WORK/tcc.elf > $WORK/tcc.out; trc=$?
        qemu-riscv64-static -cpu rv64,v=true,vlen=$VLEN -L /usr/riscv64-linux-gnu $WORK/gcc.elf > $WORK/gcc.out; grc=$?
        if [ $trc -ne 0 ]; then bad "$area [$MARCH]: tcc binary exit $trc"; continue; fi
        if [ $grc -ne 0 ]; then bad "$area [$MARCH]: gcc binary exit $grc"; continue; fi
        if ! cmp -s $WORK/tcc.out $WORK/gcc.out; then
            bad "$area [$MARCH]: outputs differ: $(diff $WORK/tcc.out $WORK/gcc.out | head -6)"
            continue
        fi
        note "$area [$MARCH]: ok ($(wc -l < $WORK/tcc.out) cases)"
    done
done

# TCC-only probes: names the reference gcc header does not declare
# in this form; the scalar references still validate them
for src in probes/riscv_*_tcconly.c; do
    [ -f $src ] || continue
    area=$(basename $src .c)
    if ! $V/riscv64-tcc -B $V -I $V/include -march=rv64gcv_zvl128b \
            -fno-asynchronous-unwind-tables -c $src -o $WORK/tcc.o 2>$WORK/tcc.err; then
        bad "$area: tcc compile: $(head -3 $WORK/tcc.err)"
        continue
    fi
    if ! riscv64-linux-gnu-gcc -o $WORK/tcc.elf $WORK/tcc.o 2>$WORK/link.err; then
        bad "$area: link of tcc object: $(head -3 $WORK/link.err)"
        continue
    fi
    qemu-riscv64-static -cpu rv64,v=true,vlen=128 -L /usr/riscv64-linux-gnu $WORK/tcc.elf > $WORK/tcc.out; trc=$?
    if [ $trc -ne 0 ]; then bad "$area: tcc binary exit $trc"; continue; fi
    note "$area: ok ($(wc -l < $WORK/tcc.out) cases, tcc only)"
done

if [ $FAIL -eq 0 ]; then
    note "all RVV differential tests passed"
else
    note "FAILURES"
fi
exit $FAIL
