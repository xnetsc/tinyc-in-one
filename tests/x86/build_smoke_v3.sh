#!/bin/bash
# Assemble the x86-64-v3 SIMD smoke probe source from cases of
# the generated differential probes in this directory's probes/.
#
# The generated probe files each mix every feature level up to
# AVX-512, so no whole file compiles at x86-64-v3.  The cases
# listed below were each verified to compile at x86-64-v3 under
# both the vendor TCC and gcc (each gates intrinsics above the
# selected -march in its own headers, and their surfaces differ
# -- e.g. gcc spells the plain _mm_or_epi32 AVX512VL-only), so
# the assembled source builds and runs on any x86-64-v3 host
# (SSE4.2, AVX, AVX2, BMI2, FMA, F16C).  Cases whose scalar
# references call libm (the FMA cases, via the header's MULADD
# helper) are excluded: the PE and Mach-O TCC link lines do
# not uniformly provide a libm for the reference to bind to.
# Case indices repeat across probe files, so each extracted
# case is renamed smoke_<area>_<idx> and its printed case
# number is rewritten to a unique smoke id; the header comment
# in the output maps each smoke id back to its origin.
#
# The macOS and Windows smoke scripts compile this same source
# with the platform TCC and the platform reference compiler
# and diff the outputs.
#
# Usage: build_smoke_v3.sh <output.c>
set -u
cd "$(dirname "$0")"

OUT=${1:?usage: build_smoke_v3.sh <output.c>}

# area:index pairs, spread over the arithmetic, logic, shift,
# conversion, float, permute, compare and misc families, mixing
# 128-bit and 256-bit forms.
CASES="
arith:0 arith:3 arith:147 arith:225 arith:275
logic:0 logic:1 logic:6 logic:7
shift:9 shift:12 shift:327 shift:397
cvt:0 cvt:3 cvt:93 cvt:162
float:0 float:3 float:120 float:396
permute:0 permute:3 permute:248 permute:335
cmp:0 cmp:1 cmp:37
misc:0 misc:26
"

{
    echo "/* Assembled by tests/x86/build_smoke_v3.sh from the generated"
    echo "   probes in tests/x86/probes/; do not edit by hand.  Smoke id"
    echo "   assignments (printed case numbers in the output):"
    uid=0
    for c in $CASES; do
        uid=$((uid + 1))
        echo "     $uid = probes/x86_${c%%:*}.c case_${c##*:}"
    done
    echo " */"
    echo '#include "x86_probe.h"'
    uid=0
    for c in $CASES; do
        area=${c%%:*}; idx=${c##*:}
        uid=$((uid + 1))
        echo ""
        echo "/* probes/x86_$area.c case_$idx (smoke id $uid) */"
        awk -v name="case_$idx" -v newname="smoke_${area}_$idx" -v uid="$uid" '
            $0 ~ "^static void " name "\\(void\\)" { sub(name, newname); on = 1 }
            on { gsub(/print_hex\([0-9]+,/, "print_hex(" uid ","); print }
            on && $0 == "}" { exit }
        ' "probes/x86_$area.c"
    done
    echo ""
    echo "int main(void) {"
    for c in $CASES; do
        echo "    smoke_${c%%:*}_${c##*:}();"
    done
    echo "    return fails != 0;"
    echo "}"
} > "$OUT"

# Guard against extraction drift (a regenerated probe file
# whose case layout no longer matches the patterns above):
# every listed case must have produced a definition.
n=$(grep -c '^static void smoke_' "$OUT")
want=$(echo $CASES | wc -w)
if [ "$n" -ne "$want" ]; then
    echo "build_smoke_v3: extracted $n of $want cases into $OUT" >&2
    exit 1
fi
echo "build_smoke_v3: assembled $n cases into $OUT"
