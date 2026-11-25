#!/bin/bash
# Tests for parse-level expansion of single-return static inline
# wrappers (vendor-tinycc tccgen).  Covers the fixture differential
# (header-layered wrappers vs the same computations written
# directly against the builtins), the negative immediate case, the
# entity-emission observables, and the aarch64 compile-level check
# through clang-style NEON layers.
#
# Usage: run_inline_tests.sh
set -u
cd "$(dirname "$0")"
ROOT=../..
V=$ROOT/vendor-tinycc
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "inline: $*"; }
bad() { echo "inline FAIL: $*"; FAIL=1; }
ok() { if [ "$1" -eq 0 ]; then note "PASS: $2"; else bad "$2"; fi }

[ -x $V/tcc ] || (cd $V && make >/dev/null) || { bad "host build"; exit 1; }
TCC="$V/tcc -B$V"

# fixture: layered-header values equal the direct-builtin values
$TCC -run fixture_consumer.c > $WORK/consumer.out 2>$WORK/consumer.err
ok $? "fixture consumer runs"
$TCC -run fixture_direct.c > $WORK/direct.out 2>$WORK/direct.err
ok $? "fixture direct runs"
cmp -s $WORK/consumer.out $WORK/direct.out
ok $? "fixture outputs identical"

# negative: a runtime immediate must fail, naming the header body
if $TCC -c negative.c -o $WORK/negative.o 2>$WORK/negative.err; then
    bad "negative case compiled"
else
    grep -q 'constant expression expected' $WORK/negative.err \
        && grep -q 'lane_header.h' $WORK/negative.err
    ok $? "negative case fails in the header body"
fi

# entity cases: values, then which entities were emitted
$TCC -run entity_cases.c > $WORK/entity.out 2>$WORK/entity.err
ok $? "entity cases run"
grep -qx '7 15 42 42 22' $WORK/entity.out
ok $? "entity cases values"
$TCC -c entity_cases.c -o $WORK/entity.o 2>$WORK/entity.err
ok $? "entity cases compile to object"
for s in rsum at_wrap multi_stmt; do
    nm $WORK/entity.o | grep -q " t $s\$"
    ok $? "entity emitted: $s"
done
if nm $WORK/entity.o | grep -q 'wsum'; then
    bad "wsum entity emitted (calls should all expand)"
else
    note "PASS: wsum fully expanded (no entity)"
fi

# aarch64 compile-level: clang-style NEON layers
[ -x $V/arm64-tcc ] || (cd $V && make cross-arm64 >/dev/null) \
    || { bad "arm64 build"; exit 1; }
$V/arm64-tcc -B$V -I$V/include -c neon_consumer.c -o $WORK/neon.o \
    2>$WORK/neon.err
ok $? "neon-style layers compile (aarch64)"

exit $FAIL
