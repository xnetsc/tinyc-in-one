# TinyC Java server package

**English** | [简体中文](README.zh-CN.md)

Java bindings for the TinyC compiler: call libtcc's C ABI directly
through Panama FFM — no JNI, no extra native components (the binding
layer is pure Java; on the native side there is only the TCC kernel
file itself).

## Requirements

- JDK 22 or newer (Panama FFM in its final form; development and
  tests run on JDK 25).
- Pass `--enable-native-access=ALL-UNNAMED` at runtime (JDK 24+'s
  requirement for native access).

## Jar layout

The Jar uses the same unified layout as the npm and Python packages,
packed per build host: one host's dual-form file × every target in
the dist at build time (the current build carries 6 targets), with
one sysroot per target shared by all of that host's combinations:

```
tinyc.jar
├─ tinyc/*.class                 Panama bindings, dual-entry wrappers, IO trampoline
├─ tinyc-native.properties       version number
├─ packaging-manifest.json       the (host, target) combinations inside the Jar
├─ natives/<host>/toolchain/<target>/tinyc[.exe]
└─ sysroot/<target>/             one per target: include/ and lib/tcc/
```

On first load, the detected host/target triples are looked up in the
manifest, and that combination's binary and target sysroot are
extracted to `~/.cache/tinyc-java/<version>/`; once the combination
marker is complete, the extraction is reused. The runtime directory
comes from the manifest's `libPath` field, and a new session passes
it to `tcc_set_lib_path` automatically. Host and target can be
overridden with the environment variables `TINYC_HOST_TRIPLE` /
`TINYC_TARGET_TRIPLE` (the system properties `tinyc.host.triple` /
`tinyc.target.triple` work too). Hosts and targets are 64-bit; a
32-bit host is reported as unsupported directly.

## Where the native files come from

`build.sh` uses the repository's `packaging/stage_layout.py` to
stage into the Jar every target in the mainline build
`../../build/dist` that can run on the build host. If the native
combination with host == target is not in the dist, the build fails
with an error.

## Usage

Session-style API (fine-grained, corresponding to `tcc_*`):

```java
try (TccSession s = TinyC.newSession()) {
    s.setOutputType(TinyC.TCC_OUTPUT_MEMORY);
    if (s.compileString("int add(int a, int b) { return a + b; }") != 0) {
        throw new IllegalStateException("compile failed");
    }
    s.relocate();
    MemorySegment fn = s.getSymbol("add");
    MethodHandle mh = TinyC.functionHandle(fn,
            FunctionDescriptor.of(JAVA_INT, JAVA_INT, JAVA_INT));
    int v = (int) mh.invokeExact(40, 2);   // 42
}
```

Command-line-style entry (coarse-grained; the arguments match the
tcc command line, and it returns the exit code):

```java
int rc = TinyC.runCli("-o", "hello", "hello.c");
```

`runCli` calls the real main exported by the native file
(`tinyc_main` preferred, `main` otherwise): the wrapper fills
`argv[0]` with the native file's real path; when the runtime
directory is known and the arguments contain no `-B`, it appends
`-B<runtime directory>` automatically; the call is serialized under
a lock for its whole duration, and C stdio is flushed after it
returns. When main returns, `runCli` returns — it does not end the
host process.

Adding local files dynamically (real paths, read directly, no
copying):

```java
s.addIncludePath("/home/user/mylib/include");      // a directory
s.addIncludePath("/opt/foo");                       // a single header /opt/foo/bar.h: its parent directory is taken
s.addLibraryPath("/home/user/mylib/lib");
s.addFile("/home/user/mylib/libfoo.a");             // a single library file, not via -l search
```

During development and testing, system properties can bypass the Jar
extraction and point at a local native file directly:

```
-Dtinyc.native.path=/path/to/tinyc -Dtinyc.lib.path=/path/to/runtime-dir
```

`tinyc.lib.path` is the directory holding `libtcc1.a` and
`include/`; when it is not given, if there is a `libtcc1.a` next to
the native file that directory is used, and otherwise no lib path
is set automatically.

## API map (libtcc.h)

| Java | C |
|---|---|
| `TinyC.newSession()` / `TccSession.close()` | `tcc_new` / `tcc_delete` |
| `setLibPath` | `tcc_set_lib_path` |
| `setOptions` | `tcc_set_options` |
| `addIncludePath` / `addSysincludePath` | `tcc_add_include_path` / `tcc_add_sysinclude_path` |
| `addLibraryPath` / `addLibrary` / `addFile` | `tcc_add_library_path` / `tcc_add_library` / `tcc_add_file` |
| `defineSymbol` / `undefineSymbol` | `tcc_define_symbol` / `tcc_undefine_symbol` |
| `addSymbol` | `tcc_add_symbol` |
| `setErrorHandler` / `setIO` | `tcc_set_error_func` |
| `TinyC.runCli(io, ...)` | `tinyc_main_io` (io composed into a TCCIO via `tcc_io_callback`) |
| `TinyC.runCli(...)` | `tinyc_main` |
| `compileString` | `tcc_compile_string` |
| `compileFile` | `tcc_add_file` (libtcc.h has no `tcc_compile_file`) |
| `setOutputType` | `tcc_set_output_type` (constants `TinyC.TCC_OUTPUT_*`) |
| `outputFile` | `tcc_output_file` |
| `run` | `tcc_run` (the target program runs inside the host process; its exit and crashes belong to the host) |
| `relocate` / `getSymbol` | `tcc_relocate` / `tcc_get_symbol` |

## IO

`TccIO` is a three-channel interface: `writeOut`, `writeErr`,
`readIn`, with data passed as bytes. Ready-made implementations:
`TccIO.stdio()` (the System streams), `TccIO.of(out, err, in)`
(stream adapter; null falls back to the System streams),
`TccIO.capturing()` (accumulates in memory, out/err retrieved
separately).

Current wiring:

- Error channel: after `TccSession.setIO(io)` or
  `setErrorHandler(handler)`, libtcc's error text is handed to
  `writeErr` as bytes through a Panama upcall trampoline.
- Session IO: `TccSession.setIO(io)` also composes the three
  channels into a callback TCCIO and installs it on the session's
  own state with the kernel's `tcc_set_io`; the session's
  preprocessor output and the stdio of programs it runs go through
  that IO, independently of other sessions.
- All three channels: `TinyC.runCli(io, args...)` composes the
  `writeOut` / `writeErr` / `readIn` trampolines into a callback
  TCCIO via the kernel's `tcc_io_callback`, passes it to
  `tinyc_main_io`, and it is installed on the state main creates
  for that call: the compiler's own output (version, preprocessor
  results, and the like) goes to `writeOut`, diagnostics go to
  `writeErr`, and `readIn` is passed to the kernel along with the
  TCCIO. `runCli(args...)` without io goes through `tinyc_main`,
  with output on the process's real stdio, matching the command
  line.
- The trampolines obey four rules: exceptions never cross the C
  boundary (they are caught and stored, then rethrown as
  `TccException` after the native call returns); bytes are passed
  without assuming a `\0` terminator; the Arena holding a stub is
  pinned until the call ends (a session error callback is pinned
  until `close()`); callbacks execute synchronously on the calling
  thread.
- By default no callback is set: libtcc's errors go to the process's
  real stderr, matching the command line.
- When `-run` runs with IO, the target program's stdin is fed by
  `readIn` while it runs, and its stdout and stderr are delivered
  through `writeOut` and `writeErr` respectively; without IO, the
  target program uses the process's real stdio.

## SIMD intrinsics

The bundled headers include SIMD intrinsic headers for every
target, in the sysroot's `lib/tcc/include/` — the compiler's
default include path, so there is nothing extra to install and
no `-I` to add:

- AArch64 targets: `#include <arm_neon.h>` — the ACLE NEON
  surface; the dot-product family needs `-march=armv8.2-a+dotprod`
  or a later baseline.
- x86-64 targets: `#include <immintrin.h>` — SSE, SSE2, SSSE3,
  SSE4, AVX, AVX2, FMA, F16C and the AVX-512 subsets, gated by
  `-march` (`x86-64-v3` for AVX2 level, `x86-64-v4` for AVX-512).
- riscv64 target: `#include <riscv_vector.h>` — the RVV 1.0 API
  in fixed-length form; requires `-march` with the V extension,
  e.g. `rv64gcv_zvl256b`.

The root README's "SIMD intrinsics" section lists, per header,
which instructions are covered and which extensions are not
provided.

## Building and tests

```sh
bash build.sh        # produces dist/tinyc.jar and compiles the test classes
bash run-tests.sh    # three passes: A system properties pointing at local files / B Jar extraction distribution mode / C host simulation
```

Host simulation (C) is driven by this package's manifest: if the
simulated combination is in the package, it is resolved and the full
suite runs; if not, the test asserts the error names that host and
lists every combination in the package. A 32-bit host has a separate
assertion rejecting it.

`build.sh` and `run-tests.sh` locate the JDK by the same rule:
candidates are checked one by one for their major version and the
first one ≥ 22 is taken, so compilation and running come from the
same JDK.
