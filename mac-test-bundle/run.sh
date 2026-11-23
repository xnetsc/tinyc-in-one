#!/bin/bash
# run.sh - one-command entry point for the macOS host test bundle.
# Runs the build, then the tests, and captures everything into
# result.log next to this script. A final OVERALL line is printed on
# the console and appended to the log.

cd "$(dirname "$0")" || exit 99
rm -f result.log

bash ./build-mac.sh 2>&1 | tee -a result.log
bash ./run-tests.sh 2>&1 | tee -a result.log

P=$(grep -c '^PASS: ' result.log)
F=$(grep -c '^FAIL: ' result.log)
B=$(grep -c '^BUILD-FAIL: ' result.log)
echo | tee -a result.log
echo "OVERALL: checks passed=$P failed=$F, build failed steps=$B (details in result.log)" \
    | tee -a result.log
