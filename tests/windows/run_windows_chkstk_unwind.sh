#!/bin/bash
# Cross-frame unwind evidence for the __chkstk prolog (PE, x86_64).
#
# A function whose frame is 4096 bytes or larger takes the __chkstk
# prolog (mov eax, size; call __chkstk; nop), so its frame pointer is
# only established when the call returns, at prolog offset 10.  The
# test program (tests/windows/chkstk_unwind_test.c) checks the
# function's UNWIND_INFO says so -- metadata through
# RtlLookupFunctionEntry, and real unwinding of a fabricated context
# from inside the prolog with RtlVirtualUnwind, the engine SEH uses.
# It is compiled by the built tinyc.exe and run natively.
#
# This script runs under Git Bash (the CI step uses shell: bash).
set -u
cd "$(dirname "$0")"
ROOT=$(cd ../.. && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "windows-chkstk: $*"; }
bad() { echo "windows-chkstk FAIL: $*"; FAIL=1; }

TINYC=$ROOT/build/dist-host/x86_64-w64-mingw32/bin/tinyc.exe
if [ ! -f "$TINYC" ]; then
    bad "built tinyc.exe not found at $TINYC"
    exit 1
fi

if ! "$TINYC" $ROOT/tests/windows/chkstk_unwind_test.c \
        -o $WORK/chkstk_unwind.exe 2>$WORK/chkstk.tcc.err; then
    bad "test compile: $(head -3 $WORK/chkstk.tcc.err)"
    exit 1
fi

$WORK/chkstk_unwind.exe > $WORK/chkstk.out 2>&1; rc=$?
cat $WORK/chkstk.out
if [ $rc -ne 0 ]; then
    bad "test binary exit $rc"
elif grep -q FAIL $WORK/chkstk.out; then
    bad "test reported FAIL"
else
    note "chkstk unwind checks passed"
fi

if [ $FAIL -eq 0 ]; then
    note "all Windows chkstk unwind tests passed"
else
    note "FAILURES"
fi
exit $FAIL
