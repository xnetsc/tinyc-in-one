#!/bin/bash
# run-tests.sh - macOS host tests for the dual-form kernel.
#
# Every check prints one "PASS: ..." or "FAIL: ..." line plus the raw
# output it was based on, so result.log is self-contained evidence.
# No set -e: a failed check never stops the remaining ones.
# Exit status is the number of failed checks (0 = all passed).

cd "$(dirname "$0")" || exit 99
BUNDLE=$PWD
DIST=$BUNDLE/dist
DUAL=$DIST/bin/tinyc
TCCLIB=$DIST/lib/tcc
WORK=$BUNDLE/build/test-work
CACHE=$BUNDLE/cache
DYLIB_CACHE=$CACHE/libtinyc.dylib
HELLO=$BUNDLE/testdata/hello.c
FAILS=0

mkdir -p "$WORK" "$CACHE"

ok()   { echo "PASS: $*"; }
bad()  { echo "FAIL: $*"; FAILS=$((FAILS + 1)); }
note() { echo "  $*"; }
step() { echo; echo "=== test: $* ==="; }

# Run an executable produced by TCC and compare its stdout. TCC signs
# its output ad-hoc on arm64 macOS in most configurations; if the
# first run does not produce the expected output, ad-hoc sign it and
# retry, and record whether that step was necessary.
run_and_check() {
    local exe=$1 want=$2 label=$3 out rc
    if [ ! -x "$exe" ]; then
        bad "$label (executable missing or not executable: $exe)"
        return
    fi
    out=$("$exe" 2>&1); rc=$?
    if [ "$out" = "$want" ] && [ $rc -eq 0 ]; then
        echo "FACT: $label ran as produced (no extra codesign needed)"
        ok "$label"
        return
    fi
    note "first run: rc=$rc out=[$out]"
    note "applying ad-hoc codesign and retrying"
    codesign -s - "$exe" 2>&1 | sed 's/^/  codesign: /'
    out=$("$exe" 2>&1); rc=$?
    if [ "$out" = "$want" ] && [ $rc -eq 0 ]; then
        echo "FACT: $label required ad-hoc codesign (codesign -s -) before it would run"
        ok "$label"
    else
        bad "$label (rc=$rc, got [$out], want [$want])"
    fi
}

step "dual-form executable: --version / -dumpmachine"
if [ -x "$DUAL" ]; then
    V=$("$DUAL" --version 2>&1 | head -1); note "--version: $V"
    case "$V" in *0.9.28*) ok "--version reports 0.9.28";; *) bad "--version unexpected: $V";; esac
    M=$("$DUAL" -dumpmachine 2>&1 | head -1); note "-dumpmachine: $M"
    if [ -n "$M" ]; then ok "-dumpmachine prints a target"; else bad "-dumpmachine empty"; fi
else
    bad "dual-form executable missing ($DUAL)"
fi

step "glue extraction: embedded dylib segment -> cache"
if [ -x "$BUNDLE/build/extract_dylib" ] && [ -x "$DUAL" ]; then
    rm -f "$DYLIB_CACHE"
    "$BUNDLE/build/extract_dylib" "$DUAL" "$DYLIB_CACHE"
    if [ -f "$DYLIB_CACHE" ]; then
        file "$DYLIB_CACHE" | sed 's/^/  /'
        case "$(file "$DYLIB_CACHE")" in
            *"Mach-O"*"dylib"*|*"Mach-O"*"dynamically linked shared library"*)
                ok "extracted file is a Mach-O dylib";;
            *) bad "extracted file is not a dylib: $(file "$DYLIB_CACHE")";;
        esac
        if cmp -s "$DYLIB_CACHE" "$BUNDLE/build/libtinyc.dylib"; then
            ok "embedded segment is byte-identical to the built dylib"
        else
            bad "embedded segment differs from the built dylib"
        fi
    else
        bad "extraction produced no file"
    fi
else
    bad "extractor or dual-form executable missing"
fi

step "compile hello.c to a Mach-O executable and run it"
if [ -x "$DUAL" ]; then
    "$DUAL" -B "$TCCLIB" -o "$WORK/hello" "$HELLO" 2>&1 | sed 's/^/  tcc: /'
    if [ -f "$WORK/hello" ]; then
        case "$(file "$WORK/hello")" in
            *"Mach-O"*"executable"*) ok "hello is a Mach-O executable";;
            *) bad "hello has unexpected format: $(file "$WORK/hello")";;
        esac
        run_and_check "$WORK/hello" "hello from tinyc: 42" "hello executable runs"
    else
        bad "hello executable not produced"
    fi
else
    bad "dual-form executable missing"
fi

step "-run (JIT) hello.c"
if [ -x "$DUAL" ]; then
    OUT=$("$DUAL" -B "$TCCLIB" -run "$HELLO" 2>&1); RC=$?
    note "-run rc=$RC out=[$OUT]"
    if [ "$OUT" = "hello from tinyc: 42" ] && [ $RC -eq 0 ]; then
        ok "-run executes hello.c in-process"
    else
        bad "-run failed (rc=$RC)"
    fi
else
    bad "dual-form executable missing"
fi

step "dlopen path: tinyc_main + tcc_* API from a C host"
if [ -x "$BUNDLE/build/host_dlopen" ] && [ -f "$DYLIB_CACHE" ]; then
    "$BUNDLE/build/host_dlopen" "$DYLIB_CACHE" "$TCCLIB" "$HELLO" "$WORK" 2>&1 \
        | sed 's/^/  host: /'
    HOSTRC=${PIPESTATUS[0]}
    if [ $HOSTRC -eq 0 ]; then
        ok "dlopen host completed (HOST-ALIVE reached, no exit() took the host)"
    else
        bad "dlopen host exited rc=$HOSTRC"
    fi
    [ -f "$WORK/hello_via_main" ] && \
        run_and_check "$WORK/hello_via_main" "hello from tinyc: 42" \
            "executable built via tinyc_main runs"
    [ -f "$WORK/hello_via_api" ] && \
        run_and_check "$WORK/hello_via_api" "hello via api: 42" \
            "executable built via tcc_* API runs"
else
    bad "host_dlopen binary or extracted dylib missing"
fi

step "Python ctypes path: tinyc_main from python3"
if [ -f "$DYLIB_CACHE" ]; then
    python3 "$BUNDLE/glue/ctypes_test.py" "$DYLIB_CACHE" "$TCCLIB" \
        "$HELLO" "$WORK/hello_ctypes" 2>&1 | sed 's/^/  py: /'
    PYRC=${PIPESTATUS[0]}
    if [ $PYRC -eq 0 ]; then
        ok "ctypes tinyc_main compiled and the Python process survived (CTYPES-ALIVE)"
    else
        bad "ctypes test exited rc=$PYRC"
    fi
    [ -f "$WORK/hello_ctypes" ] && \
        run_and_check "$WORK/hello_ctypes" "hello from tinyc: 42" \
            "executable built via ctypes runs"
else
    bad "extracted dylib missing for ctypes test"
fi

step "dynamic add: local include dir + local library via -I/-L"
LOCAL=$BUNDLE/build/local
if [ -x "$DUAL" ] && [ -f "$LOCAL/lib/libmylib.a" ]; then
    note "clang/ar archive under test:"
    file "$LOCAL/lib/libmylib.a" | sed 's/^/  /'
    ar t "$LOCAL/lib/libmylib.a" 2>&1 | sed 's/^/  ar t: /'

    note "case A: link the clang-built .o directly"
    note "established behavior: rejected - TCC on macOS reads ELF"
    note "objects only, a Mach-O .o is refused (platform limitation,"
    note "design doc 5.3); this check pins that failure signature"
    rm -f "$WORK/dynadd_obj"
    OUT=$("$DUAL" -B "$TCCLIB" -I"$LOCAL/include" \
        "$BUNDLE/testdata/dynadd_main.c" "$LOCAL/mylib.o" \
        -o "$WORK/dynadd_obj" 2>&1)
    printf '%s\n' "$OUT" | sed 's/^/  tcc: /'
    SIG=no
    case "$OUT" in *"unrecognized file type"*) SIG=yes;; esac
    if [ "$SIG" = yes ] && [ ! -f "$WORK/dynadd_obj" ]; then
        ok "dynamic add: clang .o rejected ('unrecognized file type'; platform limitation)"
    else
        bad "dynamic add: direct .o case deviates from the established limitation (see tcc output above)"
    fi

    note "case B: archive built by TCC itself, consumed via -L/-l"
    rm -f "$WORK/mylib_tcc.o" "$WORK/libmylib_tcc.a" "$WORK/dynadd_tcca"
    "$DUAL" -B "$TCCLIB" -I"$LOCAL/include" \
        -c "$BUNDLE/testdata/local/src/mylib.c" \
        -o "$WORK/mylib_tcc.o" 2>&1 | sed 's/^/  tcc: /'
    "$DUAL" -ar rcs "$WORK/libmylib_tcc.a" "$WORK/mylib_tcc.o" \
        2>&1 | sed 's/^/  tcc -ar: /'
    if [ -f "$WORK/libmylib_tcc.a" ]; then
        "$DUAL" -B "$TCCLIB" -I"$LOCAL/include" -L"$WORK" \
            "$BUNDLE/testdata/dynadd_main.c" -lmylib_tcc \
            -o "$WORK/dynadd_tcca" 2>&1 | sed 's/^/  tcc: /'
        if [ -f "$WORK/dynadd_tcca" ]; then
            run_and_check "$WORK/dynadd_tcca" "dynadd mylib: 42" \
                "dynamic add: TCC-built .a via -L/-l runs"
        else
            bad "dynamic add: TCC-built .a link produced no executable"
        fi
    else
        bad "dynamic add: TCC failed to build its own .a"
    fi

    note "case C: clang/ar-built .a via -L/-l"
    note "established behavior: link fails - the archive's Mach-O"
    note "member is not consumed, so the reference stays unresolved"
    note "(platform limitation, design doc 5.3); this check pins"
    note "that failure signature"
    rm -f "$WORK/dynadd"
    OUT=$("$DUAL" -B "$TCCLIB" \
        -I"$LOCAL/include" -L"$LOCAL/lib" \
        "$BUNDLE/testdata/dynadd_main.c" -lmylib \
        -o "$WORK/dynadd" 2>&1)
    printf '%s\n' "$OUT" | sed 's/^/  tcc: /'
    SIG=no
    case "$OUT" in *"unresolved reference"*) SIG=yes;; esac
    if [ "$SIG" = yes ]; then
        ok "dynamic add: clang/ar .a not consumed ('unresolved reference'; platform limitation)"
    else
        bad "dynamic add: clang/ar .a case deviates from the established limitation (see tcc output above)"
    fi

    note "case D: clang-built .dylib via -L/-l"
    rm -f "$WORK/libmylibdyn.dylib" "$WORK/dynadd_dyn"
    clang -dynamiclib -install_name "$WORK/libmylibdyn.dylib" \
        -o "$WORK/libmylibdyn.dylib" "$LOCAL/mylib.o" \
        2>&1 | sed 's/^/  clang: /'
    if [ -f "$WORK/libmylibdyn.dylib" ]; then
        "$DUAL" -B "$TCCLIB" -I"$LOCAL/include" -L"$WORK" \
            "$BUNDLE/testdata/dynadd_main.c" -lmylibdyn \
            -o "$WORK/dynadd_dyn" 2>&1 | sed 's/^/  tcc: /'
        if [ -f "$WORK/dynadd_dyn" ]; then
            run_and_check "$WORK/dynadd_dyn" "dynadd mylib: 42" \
                "dynamic add: clang-built .dylib via -L/-l runs"
        else
            bad "dynamic add: clang-built .dylib link produced no executable"
        fi
    else
        bad "dynamic add: clang failed to build the .dylib"
    fi

    note "case E: clang-built .dylib passed as a direct file argument"
    rm -f "$WORK/dynadd_dynfile"
    if [ -f "$WORK/libmylibdyn.dylib" ]; then
        "$DUAL" -B "$TCCLIB" -I"$LOCAL/include" \
            "$BUNDLE/testdata/dynadd_main.c" "$WORK/libmylibdyn.dylib" \
            -o "$WORK/dynadd_dynfile" 2>&1 | sed 's/^/  tcc: /'
        if [ -f "$WORK/dynadd_dynfile" ]; then
            run_and_check "$WORK/dynadd_dynfile" "dynadd mylib: 42" \
                "dynamic add: clang-built .dylib as file argument runs"
        else
            bad "dynamic add: dylib-as-file link produced no executable"
        fi
    else
        bad "dynamic add: .dylib missing for the file-argument case"
    fi

    note "case F: clang .o repacked into an archive by TCC's own -ar"
    note "established behavior: -ar rejects the Mach-O object -"
    note "TCC's archive reader is ELF-only (platform limitation,"
    note "design doc 5.3); this check pins that failure signature"
    rm -f "$WORK/libmylib_repack.a" "$WORK/dynadd_repack"
    OUT=$("$DUAL" -ar rcs "$WORK/libmylib_repack.a" "$LOCAL/mylib.o" 2>&1)
    printf '%s\n' "$OUT" | sed 's/^/  tcc -ar: /'
    SIG=no
    case "$OUT" in *"Unsupported Elf Class"*) SIG=yes;; esac
    if [ "$SIG" = yes ]; then
        ok "dynamic add: TCC -ar rejects clang .o ('Unsupported Elf Class'; platform limitation)"
    else
        bad "dynamic add: -ar repack case deviates from the established limitation (see tcc -ar output above)"
    fi
else
    bad "dual-form executable or sample libmylib.a missing"
fi

echo
echo "run-tests.sh done: $FAILS failed check(s)"
exit $FAILS
