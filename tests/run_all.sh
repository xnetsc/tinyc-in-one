#!/bin/bash
# Run the full test suite for the parts built on this machine.
# Each section reports PASS/FAIL; the script exits nonzero if any fail.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
FAIL=0
section() { echo; echo "===== $1 ====="; }
ok() { if [ "$1" -eq 0 ]; then echo "PASS: $2"; else echo "FAIL: $2"; FAIL=1; fi }

section "kernel: C unit tests for TCCIO implementations"
gcc -Wall -I core tests/test_io.c core/tcc_io.c -o /tmp/tinyc_test_io
/tmp/tinyc_test_io
ok $? "test_io"

section "kernel: dual-form file (exec mode, dlopen mode, API, IO)"
python3 -m unittest discover -s tests -p "test_kernel.py" 2>&1 | tail -3
ok ${PIPESTATUS[0]} "test_kernel"

section "python package"
(cd packages/python && python3 -m unittest discover -s tests 2>&1 | tail -3)
ok ${PIPESTATUS[0]} "python package"

section "npm package"
(cd packages/npm && npm test 2>&1 | grep -E "^ℹ (tests|pass|fail)")
ok ${PIPESTATUS[0]} "npm package"

section "java package"
(cd packages/java && ./build.sh > /tmp/tinyc_java_build.log 2>&1 && \
  ./run-tests.sh > /tmp/tinyc_java_tests.log 2>&1)
ok $? "java package"
tail -4 /tmp/tinyc_java_tests.log

section "outputs consistency (dist manifest is the authority)"
python3 tests/check_outputs_consistency.py
ok $? "outputs consistency"

section "native target matrix smoke tests"
bash tests/test_matrix.sh
ok $? "matrix smoke (details: tests/matrix-test-output.txt)"

echo
if [ $FAIL -eq 0 ]; then echo "ALL SUITES PASS"; else echo "SOME SUITES FAILED"; fi
exit $FAIL
