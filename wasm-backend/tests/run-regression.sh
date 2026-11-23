#!/bin/sh
# tests/run-regression.sh - full wasm-backend regression: rebuild,
# scalar suite, vector suite, and the three SIMD header suites.
# Writes the transcript to tests/test-output.txt.
set -u
cd "$(dirname "$0")/.."
OUT=tests/test-output.txt
TCC=./build-src/wasm32-tcc
exec > "$OUT" 2>&1

echo "== build =="
if ./build.sh; then echo "build exit=0"; else echo "build exit=$?"; fi

echo "== compile test-input.c (scalar) =="
$TCC -nostdlib -o tests/test.wasm tests/test-input.c
echo "compile exit=$?"
echo "== scalar assertions =="
node tests/run-test.mjs tests/test.wasm
echo "node exit=$?"

echo "== compile test-scalar2.c (float/control/conversions/i64) =="
$TCC -nostdlib -o tests/test-scalar2.wasm tests/test-scalar2.c
echo "compile exit=$?"
echo "== scalar2 assertions =="
node tests/run-scalar2.mjs
echo "node exit=$?"

echo "== -run must be an explicit wasm error =="
$TCC -nostdlib -run tests/test-input.c
echo "run exit=$?"

echo "== compile test-vec.c =="
$TCC -nostdlib -o tests/test-vec.wasm tests/test-vec.c
echo "compile exit=$?"
echo "== vector assertions =="
node tests/run-vec.mjs tests/test-vec.wasm
echo "node exit=$?"

echo "== compile test-simd128.c (wasm_simd128.h) =="
$TCC -nostdlib -I include -o tests/test-simd128.wasm tests/test-simd128.c
echo "compile exit=$?"
echo "== wasm_simd128.h assertions =="
node tests/run-simd128.mjs tests/test-simd128.wasm
echo "node exit=$?"

echo "== compile test-neon.c (arm_neon.h) =="
$TCC -nostdlib -I include -o tests/test-neon.wasm tests/test-neon.c
echo "compile exit=$?"
echo "== arm_neon.h assertions =="
node tests/run-neon.mjs tests/test-neon.wasm
echo "node exit=$?"

echo "== compile test-sse.c (xmmintrin.h/emmintrin.h) =="
$TCC -nostdlib -I include -o tests/test-sse.wasm tests/test-sse.c
echo "compile exit=$?"
echo "== SSE/SSE2 assertions =="
node tests/run-sse.mjs tests/test-sse.wasm
echo "node exit=$?"
