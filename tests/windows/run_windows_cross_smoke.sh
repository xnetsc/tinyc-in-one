#!/usr/bin/env bash
# Cross-target compile smoke for the Windows host build: each of the
# 8 non-native dists in build/dist-host compiles a hello program for
# its own target and the output format magic is checked per target
# family. The native triple (x86_64-w64-mingw32) is already covered
# by the CLI smoke and the LoadLibrary diagnostic; it is skipped
# here. linux-gnu legs compile to object only (no target libc.so.6
# on the Windows host); darwin legs link a hosted exe against the
# dist's own sysroot (.tbd stubs); wasm32 links with -nostdlib.
# Any mismatch fails the job; a PASS line is printed per target.
set -u
cd "$(dirname "$0")/../.."

fail=0
hello_c=$(mktemp -d)/hello.c
printf 'int main(void) { return 0; }\n' > "$hello_c"

elf_machine() { # $1 = ELF file; prints e_machine as a decimal
  od -A n -j 18 -N 2 -t u2 "$1" | tr -d ' '
}

magic4() { # $1 = file; prints the first 4 bytes as lowercase hex
  od -A n -N 4 -t x1 "$1" | tr -d ' \n'
}

for dist in build/dist-host/*/; do
  triple=$(basename "$dist")
  [ "$triple" = "x86_64-w64-mingw32" ] && continue
  tcc="$dist/bin/tinyc.exe"
  [ -x "$tcc" ] || { echo "FAIL: $triple: $tcc missing"; fail=1; continue; }
  out=$(mktemp -d)
  case "$triple" in
    *-linux-gnu)
      "$tcc" -c "$hello_c" -o "$out/hello.o" >"$out/log" 2>&1 || {
        echo "FAIL: $triple: compile failed"; cat "$out/log"; fail=1; continue; }
      got=$(elf_machine "$out/hello.o")
      case "$triple" in
        x86_64-linux-gnu) want=62 ;;
        aarch64-linux-gnu) want=183 ;;
        riscv64-linux-gnu) want=243 ;;
        *) want=0 ;;
      esac
      [ "$got" = "$want" ] && echo "PASS: $triple (ELF e_machine $got)" \
        || { echo "FAIL: $triple: ELF e_machine $got, want $want"; fail=1; }
      ;;
    *-apple-darwin)
      "$tcc" "$hello_c" -o "$out/hello" >"$out/log" 2>&1 || {
        echo "FAIL: $triple: link failed"; cat "$out/log"; fail=1; continue; }
      got=$(magic4 "$out/hello")
      [ "$got" = "cffaedfe" ] && echo "PASS: $triple (Mach-O magic $got)" \
        || { echo "FAIL: $triple: magic $got, want cffaedfe"; fail=1; }
      ;;
    wasm32-wasi)
      "$tcc" "$hello_c" -o "$out/hello.wasm" -nostdlib >"$out/log" 2>&1 || {
        echo "FAIL: $triple: link failed"; cat "$out/log"; fail=1; continue; }
      got=$(magic4 "$out/hello.wasm")
      [ "$got" = "0061736d" ] && echo "PASS: $triple (wasm magic $got)" \
        || { echo "FAIL: $triple: magic $got, want 0061736d"; fail=1; }
      ;;
    *-linux-android)
      "$tcc" -c "$hello_c" -o "$out/hello.o" >"$out/log" 2>&1 || {
        echo "FAIL: $triple: compile failed"; cat "$out/log"; fail=1; continue; }
      got=$(elf_machine "$out/hello.o")
      case "$triple" in
        x86_64-linux-android) want=62 ;;
        aarch64-linux-android) want=183 ;;
        *) want=0 ;;
      esac
      [ "$got" = "$want" ] && echo "PASS: $triple (ELF e_machine $got)" \
        || { echo "FAIL: $triple: ELF e_machine $got, want $want"; fail=1; }
      ;;
    *) echo "FAIL: $triple: unknown target family"; fail=1 ;;
  esac
  rm -rf "$out"
done

rm -f "$hello_c"
[ "$fail" -eq 0 ] || exit 1
echo "cross smoke: all 8 non-native targets OK"
