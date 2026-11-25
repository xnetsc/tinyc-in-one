#!/bin/bash
# Feature-gating tests for the x86-64 SIMD surface (design
# section 5): the predefined macros follow the selected -march=
# level, unknown -march components are errors naming the
# component, and a builtin whose required feature is missing
# fails with a named error while the same call compiles and
# runs correctly at a level that has the feature.  Vector
# integer division is rejected by name at every level.
#
# Uses the in-tree vendor compiler; builds it if missing.
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

# --- macro levels, run on the host compiler ---
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

# --- the AVX2 gating pair ---
out=$($V/tcc -B $V -I $V/include -c probes/gate_avx2.c -o /dev/null 2>&1)
case "$out" in *"requires AVX2"*) ;;
*) bad "256-bit add at the default level must fail naming AVX2: $out";; esac
$V/tcc -B $V -march=x86-64-v3 -o $WORK/gate_avx2 probes/gate_avx2.c \
    && $WORK/gate_avx2; rc=$?
[ $rc -eq 0 ] || bad "256-bit add at x86-64-v3 (run rc=$rc, want 0)"

# --- the AVX-512 gating pair ---
out=$($V/tcc -B $V -I $V/include -march=x86-64-v3 -c probes/gate_avx512.c -o /dev/null 2>&1)
case "$out" in *"requires AVX512BW"*) ;;
*) bad "512-bit add at x86-64-v3 must fail naming AVX512BW: $out";; esac
$V/tcc -B $V -march=x86-64-v4 -o $WORK/gate_avx512 probes/gate_avx512.c \
    && $WORK/gate_avx512; rc=$?
[ $rc -eq 0 ] || bad "512-bit add at x86-64-v4 (run rc=$rc, want 0)"

# --- the SSE4.1 gating pair ---
out=$($V/tcc -B $V -I $V/include -c probes/gate_sse41.c -o /dev/null 2>&1)
case "$out" in *"requires SSE4.1"*) ;;
*) bad "round at the default level must fail naming SSE4.1: $out";; esac
$V/tcc -B $V -march=x86-64-v2 -o $WORK/gate_sse41 probes/gate_sse41.c \
    && $WORK/gate_sse41; rc=$?
[ $rc -eq 0 ] || bad "round at x86-64-v2 (run rc=$rc, want 0)"

# --- vector integer division is rejected by name ---
out=$($V/tcc -B $V -I $V/include -march=x86-64-v4 -c probes/gate_vecdiv.c -o /dev/null 2>&1)
case "$out" in *"vector integer division has no x86 SIMD instruction"*) ;;
*) bad "vector integer division error text: $out";; esac

if [ $FAIL -eq 0 ]; then
    note "all gating tests passed"
else
    note "FAILURES"
fi
exit $FAIL
