# Sysroot link inputs

The per-target directories here hold the link-time inputs of the
browser package's sysroots: the files the target manifests list
under `lib/<target>/` other than `libtcc1.a`. Programs the package
compiles are dynamically linked — at run time the target
platform's own loader and shared libraries serve them — so a
sysroot carries no library implementations, only what the link
step needs. Each target's set is the native dist's link input
set for that target, drawn from the same platform sources:

- **Linux (glibc) targets**: `crt1.o`, `crti.o`, `crtn.o` are
  copied from the vendored glibc tree (`vendor-glibc/<triple>/
  crt`), the same copies the native dist bundles. `libc.so` and
  `libm.so` are stub shared libraries generated from the dynamic
  symbol tables of the system's `libc.so.6`/`libm.so.6`: their
  dynamic symbol tables are the real libraries', entry for entry
  — every exported name at every version it is exported at,
  with the real symbol types and sizes and the real SONAMEs —
  and no code. An export the real library marks as an IFUNC
  (`sin`, `strcpy`, `memset` and their kin) is recorded as a
  function, matching how a program calls it through the PLT.
- **Android targets**: every link input is copied verbatim
  from the vendored bionic set (`vendor-android/<triple>/30/
  lib`), the API level the native dist bundles: the crt
  objects under their own names (`crtbegin_dynamic.o`,
  `crtbegin_so.o`, `crtbegin_static.o`, `crtend_android.o`,
  `crtend_so.o` — the startup objects an Android-target
  compiler looks up) and `libc.so`, `libm.so`, `libdl.so`.
  Bionic is part of the Android Open Source Project
  (BSD-style license; NDK r26d).
- **Windows**: the five `.def` import definition files of
  `vendor-tinycc/win32/lib` (`msvcrt.def`, `kernel32.def`,
  `libgcc_s_dw2-1.def`, `libmingwex.def`, `libmsvcrt-os.def`),
  copied verbatim — the same files the native dist bundles.
- **Darwin targets**: the five `.tbd` text stubs of
  `vendor-darwin/lib` (libSystem, libc, libm, libpthread,
  libdl), copied verbatim — the same stubs the native dist
  bundles.

`gen_sysroot_lib.py` regenerates every file here and verifies
that the headers' declared function sets and the crt objects'
undefined symbols are covered by the stub exports; see its
docstring for the inputs it reads.
