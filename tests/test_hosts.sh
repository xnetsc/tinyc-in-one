#!/bin/bash
# Smoke tests for the host variants in build/dist-host.
#
# A variant whose CPU differs from this machine's cannot execute
# natively, so it is run under qemu-user: this is emulator-level
# verification (the binaries are real host binaries; only their
# execution here is emulated). A variant matching this machine's CPU
# runs natively, with no qemu involved. For each Linux host:
#   1. the dual-form file runs under qemu and reports its version;
#   2. it compiles hello.c under qemu; the product is checked with
#      readelf (class/machine/interp) and executed under qemu;
#   3. a small dlopen host (compiled for the same architecture) loads
#      the dual-form file, calls tinyc_main --version and the tcc_*
#      API, proving the execute+dlopen duality on that host's glibc.
# The MinGW host variant cannot execute here (no Windows): its PE
# format and export table are checked with objdump only, and the log
# says so. Execution of the Windows variant is covered on the Windows
# CI runner.
#
# Hosts whose dist-host is absent, or whose qemu binary is missing,
# are reported SKIP, not failed. Full output: tests/host-test-output.txt.
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
LOG=$ROOT/tests/host-test-output.txt
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0

{
echo "tinyc host variant smoke tests (native for this host's CPU, qemu-user for the others)"
echo "==========================================================="
} > "$LOG"

cat > "$TMP/hello.c" <<'EOF'
#include <stdio.h>
int main(void) { printf("hello from tinyc\n"); return 0; }
EOF

cat > "$TMP/dlopen_host.c" <<'EOF'
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

typedef int (*main_fn)(int, char **);
typedef void *(*new_fn)(void);
typedef void (*del_fn)(void *);

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: dlopen_host <dual-file>\n"); return 2; }
    void *h = dlopen(argv[1], RTLD_NOW);
    if (!h) { fprintf(stderr, "dlopen failed: %s\n", dlerror()); return 1; }
    main_fn m = (main_fn)dlsym(h, "tinyc_main");
    new_fn n = (new_fn)dlsym(h, "tcc_new");
    del_fn d = (del_fn)dlsym(h, "tcc_delete");
    if (!m || !n || !d) { fprintf(stderr, "missing exports\n"); return 1; }
    char a0[] = "tinyc", a1[] = "--version";
    char *args[] = { a0, a1, NULL };
    if (m(2, args) != 0) { fprintf(stderr, "tinyc_main --version rc != 0\n"); return 1; }
    void *s = n();
    if (!s) { fprintf(stderr, "tcc_new returned NULL\n"); return 1; }
    d(s);
    printf("HOST-DLOPEN-ALIVE\n");
    return 0;
}
EOF

note() { echo "$1" | tee -a "$LOG"; }
fail() { note "FAIL: $1"; FAIL=1; }

HOST_ARCH=$(uname -m)

# smoke_linux_host <triple> <qemu|"native"> <sysroot> <gcc> <machine-regex> <interp>
# With qemu "native" the variant runs directly on this machine and
# the dlopen host is compiled with the plain host gcc; otherwise the
# variant runs under the given qemu-user with the given sysroot and
# the dlopen host is compiled with the given cross gcc.
smoke_linux_host() {
    local triple=$1 qemu=$2 sysroot=$3 xgcc=$4 machine=$5 interp=$6
    local dist=$ROOT/build/dist-host/$triple
    if [ ! -x "$dist/bin/tinyc" ]; then
        note "SKIP: $triple (not built)"
        return
    fi
    local -a RUN
    if [ "$qemu" = native ]; then
        RUN=()
        note "--- $triple native execution ---"
    else
        if ! command -v "$qemu" >/dev/null 2>&1; then
            note "SKIP: $triple ($qemu not installed)"
            return
        fi
        RUN=("$qemu" -L "$sysroot")
        note "--- $triple via $qemu -L $sysroot (emulator-level) ---"
    fi
    file "$dist/bin/tinyc" >> "$LOG" 2>&1

    local out
    out=$("${RUN[@]}" "$dist/bin/tinyc" --version 2>&1)
    echo "$out" >> "$LOG"
    case "$out" in
        *"tcc version"*) note "PASS: $triple dual-form executes (--version)" ;;
        *) fail "$triple: --version failed: $out" ;;
    esac

    local exe=$TMP/hello-$triple
    if "${RUN[@]}" "$dist/bin/tinyc" "$TMP/hello.c" -o "$exe" >> "$LOG" 2>&1; then
        local hdr int
        hdr=$(readelf -h "$exe" 2>/dev/null)
        int=$(readelf -l "$exe" 2>/dev/null | sed -n 's/.*\[Requesting program interpreter: \(.*\)\]/\1/p')
        if echo "$hdr" | grep -q "Machine:.*$machine" && [ "$int" = "$interp" ]; then
            note "PASS: $triple compiles hello (machine $machine, interp $int)"
        else
            fail "$triple: hello header mismatch (machine=$(echo "$hdr" | grep Machine), interp=$int)"
        fi
        out=$("${RUN[@]}" "$exe" 2>&1)
        if [ "$out" = "hello from tinyc" ]; then
            note "PASS: $triple hello executes with correct output"
        else
            fail "$triple: hello output mismatch: [$out]"
        fi
    else
        fail "$triple: hello compile failed"
    fi

    if "$xgcc" -O2 -o "$TMP/dlopen_host-$triple" "$TMP/dlopen_host.c" -ldl 2>>"$LOG"; then
        out=$("${RUN[@]}" "$TMP/dlopen_host-$triple" "$dist/bin/tinyc" 2>&1)
        echo "$out" >> "$LOG"
        case "$out" in
            *"HOST-DLOPEN-ALIVE"*) note "PASS: $triple dlopen host: tinyc_main + tcc_* API alive" ;;
            *) fail "$triple: dlopen host failed: $out" ;;
        esac
    else
        fail "$triple: dlopen host build failed"
    fi
}

if [ "$HOST_ARCH" = aarch64 ]; then
    smoke_linux_host aarch64-linux-gnu native "" \
        gcc "AArch64" "/lib/ld-linux-aarch64.so.1"
else
    smoke_linux_host aarch64-linux-gnu qemu-aarch64-static /usr/aarch64-linux-gnu \
        aarch64-linux-gnu-gcc "AArch64" "/lib/ld-linux-aarch64.so.1"
fi
if [ "$HOST_ARCH" = x86_64 ]; then
    smoke_linux_host x86_64-linux-gnu native "" \
        gcc "Advanced Micro Devices X86-64" "/lib64/ld-linux-x86-64.so.2"
else
    smoke_linux_host x86_64-linux-gnu qemu-x86_64-static /usr/x86_64-linux-gnu \
        x86_64-linux-gnu-gcc "Advanced Micro Devices X86-64" "/lib64/ld-linux-x86-64.so.2"
fi
# ---- MinGW host: format + exports only (cannot execute here) ----
MT=x86_64-w64-mingw32
MDIST=$ROOT/build/dist-host/$MT
if [ -f "$MDIST/bin/tinyc.exe" ]; then
    if file "$MDIST/bin/tinyc.exe" | grep -q "PE32+ executable.*x86-64"; then
        note "PASS: $MT dual-form is a PE32+ x86-64 executable (not executed: no Windows host here)"
    else
        fail "$MT: not a PE32+ x86-64 executable ($(file "$MDIST/bin/tinyc.exe"))"
    fi
    file "$MDIST/bin/tinyc.exe" >> "$LOG"
    # the triplet objdump may not exist on a non-x86_64 build host;
    # any objdump whose BFD knows the PE target reads the same table
    OBJDUMP=$(command -v x86_64-w64-mingw32-objdump || command -v objdump || true)
    if [ -z "$OBJDUMP" ]; then
        note "SKIP: $MT export table (no objdump available)"
    else
        EXPORTS=$("$OBJDUMP" -p "$MDIST/bin/tinyc.exe" 2>/dev/null \
            | sed -n '/Export Address Table/,$p')
        "$OBJDUMP" -p "$MDIST/bin/tinyc.exe" >> "$LOG" 2>&1
        # the export name table follows the address table in objdump output
        NAMES=$("$OBJDUMP" -p "$MDIST/bin/tinyc.exe" 2>/dev/null \
            | awk '/\[Ordinal\/Name Pointer\] Table/{f=1} f' | grep -oE '\b(main|tinyc_main_io|tinyc_main|tinyc_init|tcc_new|tcc_compile_string|tcc_run|tcc_io_stdio|tcc_io_null|tcc_io_buffer_merged|tcc_io_buffer|tcc_io_file|tcc_io_fd|tcc_io_callback|tcc_io_free|tcc_io_buffer_data|tcc_io_buffer_clear)\b' | sort -u)
        MISSING=""
        for sym in main tinyc_main tinyc_main_io tinyc_init tcc_new tcc_compile_string tcc_run \
                   tcc_io_stdio tcc_io_null tcc_io_buffer tcc_io_buffer_merged \
                   tcc_io_file tcc_io_fd tcc_io_callback tcc_io_free \
                   tcc_io_buffer_data tcc_io_buffer_clear; do
            echo "$NAMES" | grep -qx "$sym" || MISSING="$MISSING $sym"
        done
        if [ -z "$MISSING" ]; then
            note "PASS: $MT export table lists the entry points, the tcc_* API and the tcc_io_* surface"
        else
            fail "$MT: export table missing:$MISSING"
        fi
    fi
else
    note "SKIP: $MT (not built)"
fi

echo >> "$LOG"
if [ $FAIL -eq 0 ]; then note "HOST SMOKE: ALL PASS"; else note "HOST SMOKE: FAILURES PRESENT"; fi
exit $FAIL
