# tinyc for Node.js

**English** | [简体中文](README.zh-CN.md)

TinyC (TCC) as an embedded compiler in Node.js. The package bundles
one dual-form native file per target — the same file is the `tcc`
command and the libtcc library — together with the headers and
libraries it needs, and drives it with koffi.

## Install

```
npm install @xnetx/tinyc
```

The native files for your host ship in a per-host subpackage,
`@xnetx/tinyc-native-<host-triple>`, which npm installs
automatically as an optional dependency. The main package also
works standalone when it carries its own staged files. Either way
the layout under the package root is the unified one shared by all
the native packages:

```
<package root>/
├─ packaging-manifest.json     the (host, target) combinations inside
├─ natives/<host-triple>/toolchain/<target-triple>/tinyc[.exe]
└─ sysroot/<target-triple>/    one sysroot per target, shared by all hosts
```

Roots are searched in order: `$TINYC_DIST`, the installed
subpackage, then this package itself. The host triple is detected
from the platform (`$TINYC_HOST_TRIPLE` overrides detection,
`$TINYC_TRIPLE` is an alias); the target defaults to the host
(`$TINYC_TARGET_TRIPLE` selects a cross target). `$TINYC_NATIVE`
points at the dual-form file itself, bypassing the manifest
lookup. A combination the package does not contain fails with an
error naming the host, the target, and the combinations available.
Hosts and targets are 64-bit only.

The host/target matrix and the run rule (a compiled program runs
in-process only when the target is the host's own platform; cross
combinations compile to files) are in the
[repository README](../../README.md).

## Command-line style

```js
const tcc = require('@xnetx/tinyc');

const code = tcc.runCli(['hello.c', '-o', 'hello']);  // exit code
const [code2, output] = tcc.runCliCapture(['--version']);
```

`runCli` fills in argv[0] with the native file's own path, so the
compiler finds its bundled headers and libraries. The host process is
never exited; the return value is the compiler's exit code.

## API style

```js
const s = new tcc.State();
s.addIncludePath('/path/to/headers');     // local paths, used as-is
s.addFile('/path/to/libfoo.a');
s.compileString('int add(int a, int b){ return a + b; }');
const add = s.function('add', 'int', ['int', 'int']);
console.log(add(19, 23)); // 42
s.close();
```

Compile errors throw `tcc.TinyCError` and are also collected in
`s.errors`; they are not printed.

## Output (IO)

Compiler output goes through the TCCIO interface. Default is the
process's real stdout/stderr. Alternatives:

```js
tcc.runCli(args, { io: 'capture' });                    // via runCliCapture
tcc.runCli(args, { io: tcc.IO.file('build.log') });     // to a file
tcc.runCli(args, { io: tcc.IO.callback({ out: fn }) }); // fn(Buffer)
tcc.runCli(args, { io: process.stdout });               // any Writable
```

IO callbacks run synchronously on the calling thread, matching the
compiler's synchronous IO calls.

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

## Tests

```
node bundle_native.js   # after ../../build.sh
npm test
```
