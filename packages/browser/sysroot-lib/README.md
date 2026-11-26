# Sysroot link inputs

The per-target directories here hold the link-time inputs of the
browser package's sysroots: the files the target manifests list
under `lib/<target>/` other than `libtcc1.a`. Programs the package
compiles are dynamically linked — at run time the target
platform's own loader and shared libraries serve them — so a
sysroot carries no library implementations, only what the link
step needs:

- **Linux (glibc) targets**: `crt1.o`, `crti.o`, `crtn.o` are the
  platform's own C runtime startup objects, copied from the
  installed glibc development files (glibc is under the GNU
  Lesser General Public License v2.1 or later; the objects are
  distributed as part of glibc). `libc.so` and `libm.so` are stub
  shared libraries generated from the dynamic symbol tables of
  the system's `libc.so.6`/`libm.so.6`: they carry the real
  libraries' exported names, symbol types and object sizes and
  the real SONAMEs, and no code. An export the real library
  marks as an IFUNC (`sin`, `strcpy`, `memset` and their kin)
  is recorded as a function, matching how a program calls it
  through the PLT.
- **Android targets**: `crt1.o` and `crtn.o` are the NDK sysroot's
  `crtbegin_dynamic.o` and `crtend_android.o` under the names the
  ELF linker looks up; `crti.o` is an empty object (bionic ships
  no crti). Bionic is part of the Android Open Source Project
  (BSD-style license; NDK r26d). `libc.so`, `libm.so` and
  `libdl.so` are stubs generated the same way from the NDK
  sysroot libraries at API level 21.
- **Windows**: `msvcrt.def` and `kernel32.def` are TinyCC's own
  import definition files, copied verbatim from
  `vendor-tinycc/win32/lib`. The C99 math additions, the long
  double forms, `snprintf`/`vsnprintf` and the `dl*` functions
  that `math.h` and `tcclib.h` declare are not exports of
  `msvcrt.dll`, so `math.h`, `tgmath.h` and `tcclib.h` are not
  part of the x86_64-windows sysroot; the definition files
  cover the hosted link of the remaining headers.
- **Darwin targets**: `libc.tbd` is a text stub for
  `/usr/lib/libSystem.B.dylib` listing the exports the shipped
  headers use. libSystem provides the full C99 math set and the
  `tcclib.h` surface, so both headers are part of the Darwin
  sysroots.

`gen_sysroot_lib.py` regenerates every file here from the
platform toolchains installed on the machine that runs it and
verifies that the shipped headers' declared function sets and the
crt objects' undefined symbols are covered by the stub exports;
see its docstring for the inputs it reads.
