#!/bin/bash
# Feature-gating tests for the -march= / target feature word
# machinery (design section 5): the predefined macros follow the
# selected level, unknown -march components are errors naming the
# component, and a builtin whose required feature is missing fails
# with a named error while the same call compiles (and runs
# correctly) at a level that has the feature.
#
# Uses the in-tree vendor compilers; builds them if missing.
set -u
cd "$(dirname "$0")"
ROOT=../..
V=$ROOT/vendor-tinycc
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "gating: $*"; }
bad() { echo "gating FAIL: $*"; FAIL=1; }

[ -x $V/tcc ] || (cd $V && make -j4 >/dev/null) || { bad "host build"; exit 1; }
[ -x $V/arm64-tcc ] || (cd $V && make cross-arm64 >/dev/null) || { bad "arm64 build"; exit 1; }
[ -x $V/riscv64-tcc ] || (cd $V && make cross-riscv64 >/dev/null) || { bad "riscv64 build"; exit 1; }

# --- x86-64 macro levels, run on the host compiler ---
$V/tcc -B $V -run probes/gate_macros_x86.c; rc=$?
[ $rc -eq 0 ] || bad "x86-64 default macros (rc=$rc, want 0)"
$V/tcc -B $V -march=x86-64-v3 -run probes/gate_macros_x86.c; rc=$?
[ $rc -eq 3 ] || bad "x86-64 -march=x86-64-v3 macros (rc=$rc, want 3)"
$V/tcc -B $V -march=x86-64-v4 -run probes/gate_macros_x86.c; rc=$?
[ $rc -eq 4 ] || bad "x86-64 -march=x86-64-v4 macros (rc=$rc, want 4)"
$V/tcc -B $V -mno-sse -run probes/gate_macros_x86.c 2>/dev/null; rc=$?
[ $rc -ne 0 ] || bad "x86-64 -mno-sse must remove the SSE macros"

# --- unknown -march components are named errors ---
out=$($V/tcc -B $V -march=bogus -c probes/gate_macros_x86.c -o /dev/null 2>&1)
case "$out" in *"unknown -march component 'bogus'"*) ;;
*) bad "x86-64 -march=bogus error text: $out";; esac
out=$($V/arm64-tcc -B $V -I $V/include -march=armv8.2-a+bogus -c probes/gate_dotprod.c -o /dev/null 2>&1)
case "$out" in *"unknown -march component 'bogus'"*) ;;
*) bad "arm64 -march=+bogus error text: $out";; esac
out=$($V/riscv64-tcc -B $V -march=rv32gcv -c probes/gate_macros_riscv.c -o /dev/null 2>&1)
case "$out" in *"unknown -march string 'rv32gcv'"*) ;;
*) bad "riscv64 -march=rv32gcv error text: $out";; esac

# --- riscv64 vector macros ---
$V/riscv64-tcc -B $V -march=rv64gcv_zvl256b -c probes/gate_macros_riscv.c -o /dev/null 2>/dev/null \
    || bad "riscv64 rv64gcv_zvl256b macros"
$V/riscv64-tcc -B $V -c probes/gate_macros_riscv.c -o /dev/null 2>/dev/null \
    && bad "riscv64 default must not define __riscv_vector"

# --- the dot product gating pair ---
out=$($V/arm64-tcc -B $V -I $V/include -c probes/gate_dotprod.c -o /dev/null 2>&1)
case "$out" in *"requires DOTPROD, target features: NEON"*) ;;
*) bad "dotprod at armv8-a must fail naming DOTPROD: $out";; esac

aarch64-linux-gnu-as -o $WORK/start.o harness/start.s || { bad "as harness"; exit 1; }
aarch64-linux-gnu-gcc -c -o $WORK/rt.o harness/rt.c || { bad "gcc harness"; exit 1; }
run_dotprod() {
    $V/arm64-tcc -B $V -I $V/include -march=$1 -c probes/gate_dotprod.c -o $WORK/dot.o || return 1
    aarch64-linux-gnu-gcc -static -nostdlib -Wl,--no-eh-frame-hdr \
        -o $WORK/dot.elf $WORK/start.o $WORK/dot.o $WORK/rt.o || return 1
    qemu-aarch64-static -cpu max $WORK/dot.elf
}
run_dotprod armv8.2-a+dotprod; rc=$?
[ $rc -eq 0 ] || bad "dotprod at armv8.2-a+dotprod (run rc=$rc, want 0)"
run_dotprod armv8.4-a; rc=$?
[ $rc -eq 0 ] || bad "dotprod at armv8.4-a, where it is mandatory (run rc=$rc, want 0)"

if [ $FAIL -eq 0 ]; then
    # vector integer division and remainder are rejected by name
out=$($V/arm64-tcc -B $V -I $V/include -c probes/gate_vecdiv.c -o /dev/null 2>&1)
case "$out" in *"vector integer division"*) ;;
*) bad "vector integer division error text: $out";; esac
out=$($V/arm64-tcc -B $V -I $V/include -c probes/gate_vecmod.c -o /dev/null 2>&1)
case "$out" in *"vector integer remainder"*) ;;
*) bad "vector integer remainder error text: $out";; esac

note "all gating tests passed"
else
    note "FAILURES"
fi
exit $FAIL
