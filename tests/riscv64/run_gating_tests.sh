#!/bin/bash
# Feature-gating tests for the riscv64 RVV surface (design
# section 5): a vector operation without the V feature fails with
# a named error, the shipped header needs the -march-derived
# __riscv_v_min_vlen macro, the macro tracks the zvl component,
# a bad zvl value is a named -march error, and a vector type too
# wide for one register group at the minimum VLEN fails with a
# named error rather than a crash.
#
# The macro-level march checks shared with the other targets live
# in tests/neon/run_gating_tests.sh; this script covers the
# riscv64-specific operation and header gates.
#
# Uses the in-tree vendor compiler; builds it if missing.
set -u
cd "$(dirname "$0")"
ROOT=../..
V=$ROOT/vendor-tinycc
FAIL=0

note() { echo "gating: $*"; }
bad() { echo "gating FAIL: $*"; FAIL=1; }

[ -x $V/riscv64-tcc ] || (cd $V && make cross-riscv64 >/dev/null) || { bad "riscv64 build"; exit 1; }

# --- a vector operation without the V feature: named error ---
out=$($V/riscv64-tcc -B $V -c probes/gate_rvv_ops.c -o /dev/null 2>&1)
case "$out" in *"requires the RISC-V V extension"*) ;;
*) bad "vector op without -march error text: $out";; esac
$V/riscv64-tcc -B $V -march=rv64gcv_zvl128b -c probes/gate_rvv_ops.c -o /dev/null 2>/dev/null \
    || bad "vector op must compile with -march=rv64gcv_zvl128b"

# --- the shipped header needs the march-derived macros ---
$V/riscv64-tcc -B $V -I $V/include -march=rv64gcv_zvl128b -c probes/gate_rvv_use.c -o /dev/null 2>/dev/null \
    || bad "intrinsic use must compile with -march=rv64gcv_zvl128b"
out=$($V/riscv64-tcc -B $V -I $V/include -c probes/gate_rvv_use.c -o /dev/null 2>&1)
case "$out" in *"__riscv_v_min_vlen"*) ;;
*) bad "header without -march error text: $out";; esac

# --- __riscv_v_min_vlen tracks the zvl component ---
$V/riscv64-tcc -B $V -march=rv64gcv_zvl256b -c probes/gate_rvv_macros.c -o /dev/null 2>/dev/null \
    || bad "zvl256b must define __riscv_v_min_vlen == 256"
$V/riscv64-tcc -B $V -march=rv64gcv_zvl128b -c probes/gate_rvv_macros.c -o /dev/null 2>/dev/null \
    && bad "zvl128b must not satisfy the 256 macro assertion"

# --- a bad zvl value is a named -march error ---
out=$($V/riscv64-tcc -B $V -march=rv64gcv_zvl100b -c probes/gate_rvv_ops.c -o /dev/null 2>&1)
case "$out" in *"bad zvl value in -march string 'rv64gcv_zvl100b'"*) ;;
*) bad "zvl100b error text: $out";; esac

# --- too wide for one register group: named error, exit 1 ---
out=$($V/riscv64-tcc -B $V -march=rv64gcv_zvl128b -c probes/gate_rvv_wide.c -o /dev/null 2>&1); rc=$?
case "$out" in *"too wide for one register group"*) ;;
*) bad "oversize error text: $out";; esac
[ $rc -eq 1 ] || bad "oversize exit code $rc, want 1 (named error, not a crash)"

if [ $FAIL -eq 0 ]; then
    note "all riscv64 gating tests passed"
else
    note "FAILURES"
    exit 1
fi
