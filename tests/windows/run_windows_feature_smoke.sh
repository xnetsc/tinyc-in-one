#!/bin/bash
# Feature smoke for the Windows-host compiler's own target
# surface, run by the windows CI job after the SIMD smoke.
# Both programs are compiled with the just-built host
# compiler (build/dist-host/x86_64-w64-mingw32/bin/tinyc.exe)
# and run natively on the runner:
#   feature_pthread.c - the pthread/semaphore surface, linked
#     with NO -lpthread flag (the implementation ships inside
#     libtcc1.a). The product's imports are checked against
#     the Windows system DLLs (kernel32, msvcrt): a
#     libwinpthread or any other non-system runtime import
#     fails the smoke.
#   feature_socket.c - getaddrinfo (numeric and localhost)
#     and a loopback exchange over Winsock (-lws2_32).
#
# This script runs under Git Bash (the CI step uses
# shell: bash); objdump comes from the MinGW toolchain the
# job's msys2 steps installed, located the same way the SIMD
# smoke locates gcc.
set -u
cd "$(dirname "$0")"
ROOT=$(cd ../.. && pwd)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
FAIL=0

note() { echo "windows-feature: $*"; }
bad() { echo "windows-feature FAIL: $*"; FAIL=1; }

TINYC=$ROOT/build/dist-host/x86_64-w64-mingw32/bin/tinyc.exe
if [ ! -f "$TINYC" ]; then
    bad "built tinyc.exe not found at $TINYC"
    exit 1
fi

OBJDUMP=$(command -v objdump || true)
if [ -z "$OBJDUMP" ] && [ -x /c/msys64/mingw64/bin/objdump.exe ]; then
    OBJDUMP=/c/msys64/mingw64/bin/objdump.exe
fi

# build_run <source> <name> <marker> [extra tinyc args...]
# On success LAST_EXE holds the built product's path.
LAST_EXE=""
build_run() {
    local src=$1 name=$2 marker=$3
    shift 3
    local exe=$WORK/$name.exe
    LAST_EXE=""
    if ! "$TINYC" "$ROOT/$src" "$@" -o "$exe"; then
        bad "$name: compile failed"
        return
    fi
    local out
    out=$("$exe" 2>&1)
    local rc=$?
    if [ $rc -ne 0 ]; then
        bad "$name: run exit $rc: $out"
        return
    fi
    case "$out" in
        *"$marker"*) note "PASS: $name"; LAST_EXE=$exe ;;
        *) bad "$name: marker $marker missing in output: $out" ;;
    esac
}

build_run tests/windows/feature_pthread.c \
    feature_pthread FEATURE-PTHREAD-OK
PEXE=$LAST_EXE
if [ -n "$PEXE" ] && [ -f "$PEXE" ] && [ -n "$OBJDUMP" ]; then
    IMPORTS=$("$OBJDUMP" -p "$PEXE" | sed -n 's/.*DLL Name: //p' \
        | tr 'A-Z' 'a-z' | sort -u)
    note "feature_pthread imports: $(echo $IMPORTS)"
    for dll in $IMPORTS; do
        case "$dll" in
            kernel32.dll|msvcrt.dll) ;;
            *) bad "feature_pthread imports non-system DLL: $dll" ;;
        esac
    done
fi

build_run tests/windows/feature_socket.c \
    feature_socket FEATURE-SOCKET-OK -lws2_32

if [ $FAIL -ne 0 ]; then
    note "FAILURES present"
    exit 1
fi
note "all feature smokes passed"
