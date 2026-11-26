# Bundled bionic headers and link inputs (android targets)

Each android target's package carries the header tree and the
link inputs it ships:

- `include/` — the bundled header set, collected from the
  Android NDK r26d sysroot's `usr/include` tree and pruned by
  `prune.py` to the bionic surface at the C / POSIX layer: the
  C standard library and the POSIX layer, plus the pthread,
  socket and dlfcn exception clusters, plus the individual
  kernel UAPI headers their #include closure pulls in
  (239 headers). Kernel UAPI trees, other libraries' headers,
  OS interfaces beyond that layer and application-level
  surfaces are not bundled; a program that needs them adds
  the include path itself. One tree serves every
  triple; the per-architecture part is the `<triple>`
  subdirectory inside it (the asm headers). The tree's licenses
  are collected in `NOTICE`, the sysroot's own NOTICE file
  (Apache License 2.0 and BSD-style terms). `collect.sh`
  regenerates the tree from an NDK r26d sysroot.
- `<triple>/30/lib/` — API level 30's link inputs from the same
  sysroot: the crt objects (`crtbegin_dynamic.o`,
  `crtbegin_so.o`, `crtbegin_static.o`, `crtend_android.o`,
  `crtend_so.o`) and the link-time stubs `libc.so`, `libm.so`,
  `libdl.so`. TCC's Android link rules reference the crtbegin /
  crtend objects; bionic ships no `crti.o`.

Programs compiled with these headers link the system
`libc.so` / `libm.so` / `libdl.so` dynamically at run time (the
link-time stubs carry those SONAMEs); only the headers, crt
objects and stubs are bundled, so no NDK installation is needed
on the machine running the compiler.
