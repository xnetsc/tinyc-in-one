# tinyc-embedded

TinyC (TCC 0.9.28rc) packaged as an embedded compiler: the compiler
itself plus its headers and libraries, distributed as one dual-form
native file per target and as per-ecosystem packages.

## The dual-form file

`build/dist/x86_64-linux-gnu/bin/tinyc` is a single ELF file that is
both:

- an executable — run it and it behaves as the `tcc` command;
- a library — `dlopen` it and call the libtcc `tcc_*` API, or call
  the exported `tinyc_main(argc, argv)` / `tinyc_main_io(argc, argv,
  io)` for command-line semantics inside a host process. Calling
  `tinyc_main` returns the exit code; it does not exit the host.

How it is built (see `build.sh`, `patches/apply_patches.py`):

- `tcc.c` (a unity build including `libtcc.c`) is compiled `-fPIC`
  and linked `-pie -Wl,--export-dynamic`, with its `main` renamed to
  the exported `tinyc_main` and a thin real `main` forwarding to it.
- glibc refuses to dlopen executables carrying the `DF_1_PIE` flag,
  so `tools/clear_pie_flag.py` clears that bit after linking;
  execution is unaffected and the same file then dlopens.
- `-dumpmachine` / `-dumpversion` set a flag on the state instead of
  calling `exit(0)`; `tinyc_main` checks the flag and returns 0.

## IO

Compiler-owned output goes through the `TCCIO` interface
(`core/tinyc_io.h`): `write_out` / `write_err` / `read_in` / `flush`
plus a context pointer. The default implementation writes directly
to the process's real stdio. Ready-made implementations:
`tcc_io_stdio`, `tcc_io_buffer` (separate or merged channels),
`tcc_io_file`, `tcc_io_fd`, `tcc_io_FILE`, `tcc_io_callback`,
`tcc_io_null`, `tcc_io_tee`. The compiler's error path, help,
version, search-path output and `-E` preprocessor output are routed
through the current IO; the host process's fd 0/1/2 are never
replaced.

## Dynamic dependency addition

Headers and libraries beyond the bundled set are given as local real
filesystem paths at use time and used as-is — directories via
`-I`/`-L` on the command line or `tcc_add_include_path` /
`tcc_add_library_path` in the API, single library files via
`tcc_add_file`. User paths are registered before the bundled sysroot
and shadow it.

## Layout

```
build/dist/x86_64-linux-gnu/
  bin/tinyc          dual-form file (bin/tcc is a symlink to it)
  include/           TCC headers + tinyc_io.h
  lib/tcc/libtcc1.a  TCC runtime library
  manifest.json
```

## Packages

- `packages/python/` — ctypes package (`tinyc`), bundled native
  distribution inside the wheel layout.
- `packages/npm/` — koffi package (`tinyc`) with the same two entry
  styles and JS IO callbacks.
- `packages/java/`, `packages/android/`, `packages/browser/` — see
  each package's README.
- `wasm-backend/` — the wasm32 target backend work.

## Build and test

```
bash build.sh            # pristine TCC -> patched src -> dist
bash tests/run_all.sh    # IO unit tests, kernel tests, package tests
```
