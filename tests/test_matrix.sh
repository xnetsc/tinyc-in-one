#!/bin/bash
# Smoke tests for the native target matrix.
#
# For every triple present in build/dist, compile a hello-world with
# that variant's dual-form file and verify the output against the
# triple: ELF class/machine and PT_INTERP for ELF targets (via
# readelf), PE32+ for the Windows target (via file/objdump). The
# native variant's output is also executed. Triples whose dist is
# absent are reported SKIP, not failed; a present-but-broken variant
# fails the run. Full output is kept in tests/matrix-test-output.txt.
#
# The android triples get a second, hosted battery
# (tests/android_battery.c): it links against the bundled bionic
# headers, crt objects and link-time stubs, and the product's
# PT_INTERP and DT_NEEDED are checked. The product runs only on
# Android (the emulator job); here the check is link-level - a
# failed link is itself the unresolved-symbol failure.
#
# The linux-gnu triples get a hosted battery of their own
# (tests/gnu_battery.c): stdio file IO, pthread, a socket
# loopback exchange and netdb lookup, compiled with the
# triple's packaged compiler against the bundled glibc headers
# and crt objects. The product runs here: natively for the
# host triple, under qemu-user for the other two (SKIP when
# the qemu binary is absent, the test_hosts.sh rule).
set -u
cd "$(dirname "$0")/.."
ROOT=$PWD
LOG=$ROOT/tests/matrix-test-output.txt
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FAIL=0

{
echo "tinyc native matrix smoke tests"
echo "==============================="
} > "$LOG"

cat > "$TMP/hello.c" <<'EOF'
#include <stdio.h>
int main(void) { printf("hello from tinyc\n"); return 0; }
EOF

note() { echo "$1" | tee -a "$LOG"; }
fail() { note "FAIL: $1"; FAIL=1; }

# The one matrix target whose output can execute on this machine:
# the linux-gnu triple matching the build host's CPU.
HOST_TRIPLE="$(uname -m)-linux-gnu"

# check_elf <triple> <outfile> <machine-regex> <expected-interp>
check_elf() {
    local triple=$1 out=$2 machine=$3 interp=$4
    local hdr int
    hdr=$(readelf -h "$out" 2>/dev/null)
    echo "$hdr" | grep -q "Class:.*ELF64" \
        || { fail "$triple: not ELF64"; return; }
    echo "$hdr" | grep -q "Machine:.*$machine" \
        || { fail "$triple: machine mismatch ($(echo "$hdr" | grep Machine))"; return; }
    int=$(readelf -l "$out" 2>/dev/null | sed -n 's/.*\[Requesting program interpreter: \(.*\)\]/\1/p')
    if [ "$int" = "$interp" ]; then
        note "PASS: $triple (ELF64, $machine, interp $int)"
    else
        fail "$triple: interp '$int' != expected '$interp'"
    fi
    readelf -h "$out" >> "$LOG" 2>&1
}

# smoke <triple> <kind> [machine-regex] [interp]
smoke() {
    local triple=$1 kind=$2 machine=${3:-} interp=${4:-}
    local dist=$ROOT/build/dist/$triple
    if [ ! -x "$dist/bin/tinyc" ]; then
        note "SKIP: $triple (not built)"
        return
    fi
    local out=$TMP/hello-$triple
    [ "$kind" = pe ] && out=$out.exe
    if ! "$dist/bin/tinyc" "$TMP/hello.c" -o "$out" >> "$LOG" 2>&1; then
        fail "$triple: hello-world compile/link failed"
        return
    fi
    case $kind in
        elf) check_elf "$triple" "$out" "$machine" "$interp" ;;
        pe)
            # file(1) reads the PE header directly (machine + PE32+
            # magic) on every host. objdump's BFD target support
            # varies with the host binutils build, so it corroborates
            # only when this objdump knows the pei-x86-64 target.
            if file "$out" | grep -q "PE32+ executable.*x86-64" \
               && { ! objdump --info 2>/dev/null | grep -q "pei-x86-64" \
                    || objdump -f "$out" 2>/dev/null | grep -q "pei-x86-64"; }; then
                note "PASS: $triple (PE32+ executable, x86-64)"
            else
                fail "$triple: not a PE32+ x86-64 executable ($(file "$out"))"
            fi
            file "$out" >> "$LOG"
            ;;
    esac
    # the native variant (target == this host) runs here; check its
    # output too
    if [ "$triple" = "$HOST_TRIPLE" ]; then
        if [ "$("$out")" = "hello from tinyc" ]; then
            note "PASS: $triple output executes and prints hello"
        else
            fail "$triple: compiled program output mismatch"
        fi
    fi
}

# battery_android <triple>
battery_android() {
    local triple=$1
    local dist=$ROOT/build/dist/$triple
    if [ ! -x "$dist/bin/tinyc" ]; then
        note "SKIP: $triple battery (not built)"
        return
    fi
    local out=$TMP/battery-$triple
    if ! "$dist/bin/tinyc" "$ROOT/tests/android_battery.c" -lm -ldl \
            -o "$out" >> "$LOG" 2>&1; then
        fail "$triple: hosted battery compile/link failed"
        return
    fi
    local int needed lib
    int=$(readelf -l "$out" 2>/dev/null \
        | sed -n 's/.*\[Requesting program interpreter: \(.*\)\]/\1/p')
    if [ "$int" != "/system/bin/linker64" ]; then
        fail "$triple: battery interp '$int' != /system/bin/linker64"
        return
    fi
    needed=$(readelf -d "$out" 2>/dev/null | grep 'NEEDED')
    for lib in libc.so libm.so libdl.so; do
        if ! echo "$needed" | grep -q "\[$lib\]"; then
            fail "$triple: battery DT_NEEDED lacks $lib"
            return
        fi
    done
    note "PASS: $triple battery (interp /system/bin/linker64, NEEDED libc.so/libm.so/libdl.so)"
    readelf -d "$out" >> "$LOG" 2>&1
}

# battery_gnu <triple> <qemu-binary-or-native> <qemu-sysroot>
battery_gnu() {
    local triple=$1 qemu=$2 sysroot=$3
    local dist=$ROOT/build/dist/$triple
    if [ ! -x "$dist/bin/tinyc" ]; then
        note "SKIP: $triple gnu battery (not built)"
        return
    fi
    local out=$TMP/gnu-battery-$triple
    if ! "$dist/bin/tinyc" "$ROOT/tests/gnu_battery.c" -lpthread \
            -o "$out" >> "$LOG" 2>&1; then
        fail "$triple: gnu battery compile/link failed"
        return
    fi
    local -a RUN=()
    if [ "$qemu" != native ]; then
        if ! command -v "$qemu" >/dev/null 2>&1; then
            note "SKIP: $triple gnu battery run ($qemu not installed; link passed)"
            return
        fi
        RUN=("$qemu" -L "$sysroot")
    fi
    local bout rc
    bout=$("${RUN[@]}" "$out" 2>&1) && rc=0 || rc=$?
    echo "$bout" >> "$LOG"
    if [ $rc -eq 0 ] && echo "$bout" | grep -q "BATTERY OK"; then
        note "PASS: $triple gnu battery (stdio, pthread, socket, netdb; ran ${qemu})"
    else
        fail "$triple: gnu battery run failed (${qemu}): $(echo "$bout" | grep FAIL | head -3 | tr '\n' ' ')"
    fi
}

smoke x86_64-linux-gnu    elf "Advanced Micro Devices X86-64" "/lib64/ld-linux-x86-64.so.2"
smoke aarch64-linux-gnu   elf "AArch64"                       "/lib/ld-linux-aarch64.so.1"
smoke riscv64-linux-gnu   elf "RISC-V"                        "/lib/ld-linux-riscv64-lp64d.so.1"
smoke x86_64-w64-mingw32  pe
smoke aarch64-linux-android elf "AArch64"                     "/system/bin/linker64"
smoke x86_64-linux-android  elf "Advanced Micro Devices X86-64" "/system/bin/linker64"
battery_android aarch64-linux-android
battery_android x86_64-linux-android
if [ "$HOST_TRIPLE" = x86_64-linux-gnu ]; then
    battery_gnu x86_64-linux-gnu native ""
else
    battery_gnu x86_64-linux-gnu qemu-x86_64-static /usr/x86_64-linux-gnu
fi
if [ "$HOST_TRIPLE" = aarch64-linux-gnu ]; then
    battery_gnu aarch64-linux-gnu native ""
else
    battery_gnu aarch64-linux-gnu qemu-aarch64-static /usr/aarch64-linux-gnu
fi
battery_gnu riscv64-linux-gnu qemu-riscv64-static /usr/riscv64-linux-gnu

echo >> "$LOG"
if [ $FAIL -eq 0 ]; then note "MATRIX SMOKE: ALL PASS"; else note "MATRIX SMOKE: FAILURES PRESENT"; fi
exit $FAIL
