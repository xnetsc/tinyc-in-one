#!/bin/bash
# Native SIMD smoke tests on Windows (PE executables, run
# natively, outputs diffed against MinGW gcc).
#
# The compiler under test is the dual-form tinyc.exe the job
# built (build/dist-host/x86_64-w64-mingw32/bin/tinyc.exe).
# The probe is the x86-64-v3 smoke source assembled by
# tests/x86/build_smoke_v3.sh (cases extracted from the
# generated differential probes), run natively when the CPU
# has AVX2, plus a compile-only leg of a full probe file at
# x86-64-v4 (the windows-latest Azure CPUs have no AVX-512).
#
# This script runs under Git Bash (the CI step uses
# shell: bash); the MinGW toolchain the job's msys2 steps
# installed is located explicitly, since Git Bash does not
# inherit the MSYS2 environment.
set -u
cd "$(dirname "$0")"
ROOT=$(cd ../.. && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "windows-simd: $*"; }
bad() { echo "windows-simd FAIL: $*"; FAIL=1; }

TINYC=$ROOT/build/dist-host/x86_64-w64-mingw32/bin/tinyc.exe
if [ ! -f "$TINYC" ]; then
    bad "built tinyc.exe not found at $TINYC"
    exit 1
fi

GCC=$(command -v gcc || true)
if [ -z "$GCC" ] && [ -x /c/msys64/mingw64/bin/gcc.exe ]; then
    GCC=/c/msys64/mingw64/bin/gcc.exe
fi
if [ -z "$GCC" ]; then
    bad "MinGW gcc not found (not on PATH, not at /c/msys64/mingw64/bin/gcc.exe)"
    exit 1
fi
# gcc.exe's own directory also carries the runtime DLLs the
# gcc-built probe needs at run time.
export PATH="$(dirname "$GCC"):$PATH"
GCC=gcc
note "reference compiler: $($GCC --version | head -1)"

# CPU capability probe, answered by the reference compiler's
# own runtime detection (there is no /proc/cpuinfo here).
# Exit code is a bitmask: 1 = AVX2, 2 = AVX512F.
cat > $WORK/cpu.c <<'EOF'
int main(void)
{
    int r = 0;
    if (__builtin_cpu_supports("avx2")) r |= 1;
    if (__builtin_cpu_supports("avx512f")) r |= 2;
    return r;
}
EOF
if ! $GCC -O2 $WORK/cpu.c -o $WORK/cpu.exe 2>$WORK/cpu.err; then
    bad "cpu probe compile: $(head -3 $WORK/cpu.err)"
    exit 1
fi
$WORK/cpu.exe; CPU=$?
HAVE_AVX2=$((CPU & 1))
HAVE_AVX512=$(((CPU >> 1) & 1))
note "cpu: AVX2=$HAVE_AVX2 AVX512F=$HAVE_AVX512 (__builtin_cpu_supports)"

bash $ROOT/tests/x86/build_smoke_v3.sh $WORK/x86_smoke_v3.c || { bad "smoke source assembly"; exit 1; }

# v4 compile leg: a full probe file at the differential
# suite's level; compile only.
if ! "$TINYC" -march=x86-64-v4,avx512vbmi,avx512vnni \
        -c $ROOT/tests/x86/probes/x86_misc.c -o $WORK/misc-v4.o 2>$WORK/misc-v4.tcc.err; then
    bad "x86_misc (v4 compile leg): tcc compile: $(head -3 $WORK/misc-v4.tcc.err)"
elif ! $GCC -O0 -frounding-math -march=x86-64-v4 -mavx512vbmi -mavx512vnni \
        -I $ROOT/tests/x86/probes -c $ROOT/tests/x86/probes/x86_misc.c -o $WORK/misc-v4-gcc.o 2>$WORK/misc-v4.gcc.err; then
    bad "x86_misc (v4 compile leg): gcc compile: $(head -3 $WORK/misc-v4.gcc.err)"
else
    note "x86_misc: v4 compile legs ok (not run: AVX-512 level)"
fi

if ! "$TINYC" -I $ROOT/tests/x86/probes -march=x86-64-v3 \
        $WORK/x86_smoke_v3.c -o $WORK/smoke-tcc.exe 2>$WORK/smoke.tcc.err; then
    bad "x86_smoke_v3: tcc compile: $(head -3 $WORK/smoke.tcc.err)"
elif ! $GCC -O0 -frounding-math -march=x86-64-v3 \
        -I $ROOT/tests/x86/probes $WORK/x86_smoke_v3.c -o $WORK/smoke-gcc.exe 2>$WORK/smoke.gcc.err; then
    bad "x86_smoke_v3: gcc compile: $(head -3 $WORK/smoke.gcc.err)"
elif [ $HAVE_AVX2 -eq 0 ]; then
    note "x86_smoke_v3: runtime skipped (host has no AVX2); compile legs ok"
else
    $WORK/smoke-tcc.exe > $WORK/smoke.tcc.out 2>&1; trc=$?
    $WORK/smoke-gcc.exe > $WORK/smoke.gcc.out 2>&1; grc=$?
    if [ $trc -ne 0 ]; then
        bad "x86_smoke_v3: tcc binary exit $trc"
    elif [ $grc -ne 0 ]; then
        bad "x86_smoke_v3: gcc binary exit $grc"
    elif grep -q MISMATCH $WORK/smoke.tcc.out; then
        bad "x86_smoke_v3: tcc scalar reference mismatch: $(grep MISMATCH $WORK/smoke.tcc.out | head -3)"
    elif ! cmp -s $WORK/smoke.tcc.out $WORK/smoke.gcc.out; then
        bad "x86_smoke_v3: outputs differ: $(diff $WORK/smoke.tcc.out $WORK/smoke.gcc.out | head -6)"
    else
        note "x86_smoke_v3: ok ($(wc -l < $WORK/smoke.tcc.out) cases)"
    fi
fi

if [ $FAIL -eq 0 ]; then
    note "all Windows SIMD smoke tests passed"
else
    note "FAILURES"
fi
exit $FAIL
