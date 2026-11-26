#!/bin/sh
# tests/run-regression.sh - full wasm-backend regression: rebuild,
# scalar suite, vector suite, and the three SIMD header suites.
# Writes the transcript to tests/test-output.txt.
# The exit code is the verdict: 0 only when every stage passed
# (the -run stage passes only when tcc refuses with exit 1, the
# explicit wasm error). Callers (CI and other harnesses) judge
# this script by its exit code alone.
set -u
cd "$(dirname "$0")/.."
OUT=tests/test-output.txt
TCC=./build-src/wasm32-tcc
FAIL=0
exec > "$OUT" 2>&1

stage_rc() { # stage_rc <rc> <label>
    echo "$2 exit=$1"
    [ "$1" -eq 0 ] || FAIL=1
}

echo "== build =="
./build.sh; stage_rc $? "build"

echo "== compile test-input.c (scalar) =="
$TCC -nostdlib -o tests/test.wasm tests/test-input.c; stage_rc $? "compile"
echo "== scalar assertions =="
node tests/run-test.mjs tests/test.wasm; stage_rc $? "node"

echo "== compile test-scalar2.c (float/control/conversions/i64) =="
$TCC -nostdlib -o tests/test-scalar2.wasm tests/test-scalar2.c; stage_rc $? "compile"
echo "== scalar2 assertions =="
node tests/run-scalar2.mjs; stage_rc $? "node"

echo "== two-state registry isolation (libtcc API, one process) =="
gcc -o tests/wasm32-libtcc.o -c build-src/libtcc.c -DTCC_TARGET_WASM32 -DCONFIG_TCC_CROSSPREFIX='"wasm32-"' -Ibuild-src -Wall -O2; stage_rc $? "libtcc build"
gcc -o tests/test-two-states tests/test-two-states.c tests/wasm32-libtcc.o -Ibuild-src -lm -ldl -lpthread; stage_rc $? "host build"
./tests/test-two-states; stage_rc $? "host"
node tests/run-two-states.mjs; stage_rc $? "node"

echo "== -run must be an explicit wasm error =="
$TCC -nostdlib -run tests/test-input.c
rc=$?
echo "run exit=$rc"
[ "$rc" -eq 1 ] || FAIL=1

echo "== compile test-vec.c =="
$TCC -nostdlib -o tests/test-vec.wasm tests/test-vec.c; stage_rc $? "compile"
echo "== vector assertions =="
node tests/run-vec.mjs tests/test-vec.wasm; stage_rc $? "node"

echo "== compile test-simd128.c (wasm_simd128.h) =="
$TCC -nostdlib -I include -o tests/test-simd128.wasm tests/test-simd128.c; stage_rc $? "compile"
echo "== wasm_simd128.h assertions =="
node tests/run-simd128.mjs tests/test-simd128.wasm; stage_rc $? "node"

echo "== compile test-neon.c (arm_neon.h) =="
$TCC -nostdlib -I include -o tests/test-neon.wasm tests/test-neon.c; stage_rc $? "compile"
echo "== arm_neon.h assertions =="
node tests/run-neon.mjs tests/test-neon.wasm; stage_rc $? "node"

echo "== compile test-sse.c (xmmintrin.h/emmintrin.h) =="
$TCC -nostdlib -I include -o tests/test-sse.wasm tests/test-sse.c; stage_rc $? "compile"
echo "== SSE/SSE2 assertions =="
node tests/run-sse.mjs tests/test-sse.wasm; stage_rc $? "node"

echo "== compile test-datareloc.c (data-section pointer initializers) =="
$TCC -nostdlib -o tests/test-datareloc.wasm tests/test-datareloc.c; stage_rc $? "compile"
echo "== datareloc assertions =="
node tests/run-datareloc.mjs tests/test-datareloc.wasm; stage_rc $? "node"

echo "== dylink data relocations (side + main through the process loader) =="
node tests/run-datareloc-dylink.mjs; stage_rc $? "node"

echo "== compile test-fpcall.c (calls through function-pointer lvalues) =="
$TCC -nostdlib -o tests/test-fpcall.wasm tests/test-fpcall.c; stage_rc $? "compile"
echo "== fpcall assertions =="
node tests/run-fpcall.mjs tests/test-fpcall.wasm; stage_rc $? "node"

echo "== dylink function-pointer calls (side + main through the process loader) =="
node tests/run-fpcall-dylink.mjs; stage_rc $? "node"

echo "== dylink data GOT references (static initializers naming imported symbols) =="
node tests/run-datagot-dylink.mjs; stage_rc $? "node"

echo "== dylink address-only function classification (GOT.func by symbol type) =="
node tests/run-addronly-dylink.mjs; stage_rc $? "node"

echo "== compile test-varargs.c (variadic definitions and calls) =="
$TCC -nostdlib -I include -I ../vendor-tinycc/include -o tests/test-varargs.wasm tests/test-varargs.c; stage_rc $? "compile"
echo "== varargs assertions =="
node tests/run-varargs.mjs tests/test-varargs.wasm; stage_rc $? "node"

echo "== compile test-varargs-printf.c (imported variadic / shim ABI) =="
$TCC -nostdlib -I include -I ../vendor-tinycc/include -o tests/test-varargs-printf.wasm tests/test-varargs-printf.c; stage_rc $? "compile"
echo "== varargs printf assertions =="
node tests/run-varargs-printf.mjs tests/test-varargs-printf.wasm; stage_rc $? "node"

echo "== dylink variadic calls (stdio shims through the process loader) =="
node tests/run-varargs-dylink.mjs; stage_rc $? "node"

if [ "$FAIL" -eq 0 ]; then
    echo "regression result: PASS"
else
    echo "regression result: FAIL"
fi
exit $FAIL
