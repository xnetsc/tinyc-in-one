#!/bin/bash
# Run the full test suite for the parts built on this machine.
# Each section reports PASS/FAIL; the script exits nonzero if any fail.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
FAIL=0
section() { echo; echo "===== $1 ====="; }
ok() { if [ "$1" -eq 0 ]; then echo "PASS: $2"; else echo "FAIL: $2"; FAIL=1; fi }

# Every section is judged by the test command's own exit code, never
# by a display filter (tail/grep) downstream of it: each suite writes
# its full output to a log, ok() reads the suite's status, and only
# then is the log excerpted for display.
section "kernel: C unit tests for TCCIO implementations"
rm -f /tmp/tinyc_test_io
gcc -Wall -I core tests/test_io.c core/tcc_io.c -o /tmp/tinyc_test_io \
  && /tmp/tinyc_test_io
ok $? "test_io"

section "kernel: dual-form file (exec mode, dlopen mode, API, IO)"
python3 -m unittest discover -s tests -p "test_kernel.py" \
  > /tmp/tinyc_kernel_tests.log 2>&1
ok $? "test_kernel"
tail -3 /tmp/tinyc_kernel_tests.log

section "python package"
(cd packages/python && python3 -m unittest discover -s tests) \
  > /tmp/tinyc_python_tests.log 2>&1
ok $? "python package"
tail -3 /tmp/tinyc_python_tests.log

section "npm package"
(cd packages/npm && npm test) > /tmp/tinyc_npm_tests.log 2>&1
ok $? "npm package"
grep -E "^ℹ (tests|pass|fail)" /tmp/tinyc_npm_tests.log

section "java package"
rm -f /tmp/tinyc_java_tests.log
(cd packages/java && ./build.sh > /tmp/tinyc_java_build.log 2>&1 && \
  ./run-tests.sh > /tmp/tinyc_java_tests.log 2>&1)
ok $? "java package"
tail -4 /tmp/tinyc_java_tests.log 2>/dev/null

section "outputs consistency (dist manifest is the authority)"
python3 tests/check_outputs_consistency.py
ok $? "outputs consistency"

section "native target matrix smoke tests"
bash tests/test_matrix.sh
ok $? "matrix smoke (details: tests/matrix-test-output.txt)"

section "host variant smoke tests (cross-built hosts, qemu-user)"
bash tests/test_hosts.sh
ok $? "host smoke (details: tests/host-test-output.txt)"

echo
if [ $FAIL -eq 0 ]; then echo "ALL SUITES PASS"; else echo "SOME SUITES FAILED"; fi
exit $FAIL
