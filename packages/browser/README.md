# tinyc-web

TinyC (TCC) for the browser: the compiler built to WebAssembly, with a JavaScript glue layer, a virtual filesystem, and a Web Worker interface.

## Build prerequisite

Producing the WASM modules requires an activated Emscripten SDK (`emcc` on `PATH`). `./build.sh` checks for `emcc` and stops with an error if it is missing; it installs nothing.

A TCC build has exactly one code generation target, so the package carries one WASM module per target, built by one `build.sh` run per target:

```
TARGET=x86_64-linux-gnu ./build.sh   # emits tcc-x86_64-linux-gnu.mjs + tcc-x86_64-linux-gnu.wasm
TARGET=aarch64-linux-gnu ./build.sh  # emits tcc-aarch64-linux-gnu.mjs + tcc-aarch64-linux-gnu.wasm
```

Each module exports the libtcc `tcc_*` API plus `main`. The `wasm32-wasi` module additionally needs a wasm code generation backend in TCC, which does not exist yet; `build.sh` stops with that message for `TARGET=wasm32-wasi`. The two Darwin branches of `build.sh` follow upstream TCC's Makefile (`-DTCC_TARGET_MACHO` plus `tccmacho.c`); they have not been built or verified in this environment.

The glue layer (`index.mjs`, `worker.mjs`) is complete without the WASM build: `loadTinyc` also accepts an injected module (`module`, `createModule`, `wasmExports`, or an async `loadModule(target, files)` loader), which is how the test suite runs it.

## Targets

```js
import { loadTinyc } from 'tinyc-web';

const tinyc = await loadTinyc({ target: 'x86_64-linux-gnu', sysroot: true });
```

Supported targets — the same set as the native target matrix, plus `wasm32-wasi`, the browser's own host format. There are no browser-only targets and no native-only targets:

| target | output | run in browser |
|---|---|---|
| `wasm32-wasi` (default) | WebAssembly (WASI) | yes |
| `x86_64-linux-gnu` | Linux x86-64 ELF | no |
| `aarch64-linux-gnu` | Linux AArch64 ELF | no |
| `riscv64-linux-gnu` | Linux RISC-V 64 ELF | no |
| `x86_64-windows` | Windows x86-64 PE | no |
| `aarch64-linux-android` | Android AArch64 ELF (bionic) | no |
| `x86_64-linux-android` | Android x86-64 ELF (bionic) | no |
| `arm64-apple-darwin` | macOS arm64 Mach-O | no |
| `x86_64-apple-darwin` | macOS x86-64 Mach-O | no |

The target's module (`tcc-<target>.wasm` / `tcc-<target>.mjs`) is loaded lazily — only the requested target's module is fetched, and at most once per page. Instances of different targets coexist; each instance has its own virtual filesystem and its own sysroot manifest (`sysroot-manifests/<target>.json`; the top-level `sysroot-manifest.json` is the default target's manifest). An unknown target is rejected with the supported list.

`sysroot: true` loads the target's bundled header set (TCC's own headers: `float.h`, `stdarg.h`, `stddef.h`, `stdbool.h`, `tcclib.h`, and the rest of that target's manifest) into the virtual filesystem under `/sysroot/include` and registers it as an include directory. A custom manifest can be passed as `{ manifest, baseUrl }`.

Target-specific manifest facts:

- The Android manifests describe the bionic sysroot layout of the native Android builds: `libc: "bionic"`, `androidApi: 30`, and `dynamicLinker: "/system/bin/linker64"` (the interpreter of 64-bit Android ELF products).
- The Darwin manifests contain TCC's own headers only. macOS system SDK headers are license-restricted and are not distributed with the package; the manifest marks this as `systemHeaders: "user-sdk"`, and system headers are supplied by pointing the dynamic-add mechanism at a local SDK path. Darwin targets compile to files only; a Mach-O product needs a code signature to run on Apple Silicon — at least an ad-hoc signature — wherever it was compiled.

## Output formats

The output file formats a target can produce are the same in every package and on every platform. Each target's manifest carries them in its `outputs` field; the full table:

| family (targets) | exe | obj | shared | ar |
|---|---|---|---|---|
| ELF (`*-linux-gnu`, `*-linux-android`) | ELF executable, no extension | ELF relocatable `.o` | ELF shared object `.so` | ar archive `.a` |
| PE (`x86_64-windows`) | PE executable `.exe` | COFF object `.o` | PE DLL `.dll` | ar archive `.a` |
| Mach-O (`*-apple-darwin`) | Mach-O executable, no extension | Mach-O object `.o` | Mach-O dylib `.dylib` | ar archive `.a` |
| WebAssembly (`wasm32-wasi`) | — | — | — | module: WebAssembly `.wasm` |

## Running: wasm32 only

Compiling to a file works for every target. Running does not: the compiler's host platform in a browser is WebAssembly itself, so only the `wasm32-wasi` target's output can run here — the produced module is instantiated and executed by the browser.

For every other target, the glue layer rejects run requests up front with `TinycRunNotSupportedError`, before anything reaches the native module:

- `tinyc.callMain(['-run', ...])` throws;
- `state.run(...)` throws.

`instance.canRun` reports whether the instance's target can run.

### How a wasm32 run works

A WebAssembly module cannot instantiate another WebAssembly module from inside itself — instantiation is a host capability. So running never goes through the compiler module (native TCC provides no `tcc_run` for the wasm target). The glue layer takes the compiled product's bytes and instantiates them itself with the WebAssembly API:

- `state.run(args, options)` writes the product to a file in the virtual filesystem (`options.outputPath`, default `/a.out`), reads the bytes back and runs them; the WASI argv is `[outputPath, ...args]`.
- `tinyc.callMain(['-run', src, ...progArgs])` first compiles `src` to `/a.out` through the exported main (the native main never sees `-run`), then runs the product with WASI argv `[src, ...progArgs]`.
- `runWasmModule(bytes, options)` / `runWasmModuleSync(bytes, options)` run product bytes directly (async `WebAssembly.compile` variant, and a synchronous `WebAssembly.Module`/`Instance` variant used inside workers and by the two paths above).

Entry conventions: with no `options.entry`, an exported `_start` is called (command style) and the result is `{ exitCode }`; with `options.entry`, that export is called with `options.callArgs` (default `options.args`) and its return value is returned as-is (function style). `state.run` returns the exit code, or the entry's return value.

The instance gets a minimal WASI shim (`wasi_snapshot_preview1`): `fd_write`, `fd_read`, `fd_close`, `fd_seek` (the stdio descriptors are not seekable: `ESPIPE`), `fd_fdstat_get`, `args_get`, `args_sizes_get`, `environ_get`, `environ_sizes_get`, `clock_time_get`, `random_get`, `sched_yield`, and `proc_exit`. `fd_write` output on fd 1/2 is routed to the run's IO as `Uint8Array` chunks. `proc_exit` never terminates the host: the shim throws, the runner catches it, and the code becomes the run's exit code.

Passing data into a module instance: `writeModuleData(instance, data, options)` writes a string/`Uint8Array` into the instance's exported memory — through the module's exported `malloc` when it has one (the result carries a `free()`), otherwise at `options.offset` (a number, or `(byteLength) => address`). `readModuleData` / `readModuleCString` read back.

## Runtime shims: the default libc

A C program compiled to wasm32 imports the libc functions it uses, and there is no libc archive to link in this package. Every run therefore also gets the default shim library (`runtime-shims.mjs`) as imports under the module name `env`, implemented in JS against the instance's linear memory:

- memory/string: `memcpy`, `memmove`, `memset`, `memcmp`, `strlen`, `strcpy`, `strncpy`, `strcat`, `strncat`, `strcmp`, `strncmp`, `strchr`, `strrchr`, `strstr`, `strdup`
- allocation: `malloc`, `calloc`, `realloc`, `free`
- stdio: `printf`, `fprintf`, `vprintf`, `vfprintf`, `sprintf`, `vsprintf`, `snprintf`, `vsnprintf`, `puts`, `putchar`, `fputs`, `putc`, `fputc`, `fwrite`, `fread`, `fgetc`, `getchar`, `ungetc`
- stdlib: `exit`, `abort`, `atexit`, `atoi`, `atol`, `atoll`, `atof`, `strtol`, `strtoul`, `strtoll`, `strtoull`, `strtod`, `strtof`, `abs`, `labs`, `llabs`, `rand`, `srand`, `getenv` (returns NULL), `system` (returns -1)
- ctype: `isalpha`, `isdigit`, `isspace`, `isalnum`, `isupper`, `islower`, `ispunct`, `iscntrl`, `isxdigit`, `isblank`, `isprint`, `isgraph`, `tolower`, `toupper`
- math: `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`, `exp`, `log`, `log10`, `pow`, `sqrt`, `floor`, `ceil`, `fabs`, `round`, `trunc`, `fmod` (the JS `Math` functions)
- time: `time` (seconds from `Date.now()`), `clock`, `difftime`

Details that matter:

- **Timing.** Import functions are created before the instance exists, so they never capture the memory: they read it through a holder on every call. The glue fills the holder right after instantiation, before any entry point runs, so the memory is always in place when a shim is called.
- **Allocator.** `malloc` and friends are a first-fit free-list allocator (with block splitting) over the region starting at `heapBase` (default 65536, overridable per run); it grows the memory when it runs out. If the module exports its own `malloc`, the shim forwards allocation calls to the module's allocator instead and the internal one never touches the heap — exactly one allocator is ever in play.
- **stdio.** The printf family parses the format string in JS, reading it and the arguments from linear memory. Variadic arguments are read from the region the `va_list` points to, in order, each at its natural alignment: i32/pointers in 4-byte slots, i64/f64 in 8-byte slots aligned to 8. Supported: flags `- + space # 0`, width and precision (including `*`), length modifiers `hh h l ll z j t`, conversions `d i u x X o c s p f F e E g G a A n %`. Output is routed to the run's IO as bytes; a `FILE*` stream value of 2 goes to `err`, any other value to `out`. `fread`/`fgetc` read the run's stdin (`options.stdin`, then `io.in()` chunks).
- **exit.** `exit(code)` runs `atexit` handlers when the module exports its indirect function table (`__indirect_function_table`) — handlers are table indices and only callable through it — then unwinds as `{ exitCode: code }`, like `proc_exit`. `abort()` unwinds with exit code 134.

### libtcc1 helpers

TCC's runtime support library (`vendor-tinycc/lib/`) was checked symbol by symbol against what a wasm32 target can emit. No JS shims are provided for it, for these reasons:

- `libtcc1.c` — `__divdi3`, `__moddi3`, `__udivdi3`, `__umoddi3`, `__ashrdi3`, `__lshrdi3`, `__ashldi3`, and the float conversions `__floatundisf/df/xf`, `__fixunssfdi`, `__fixsfdi`, `__fixunsdfdi`, `__fixdfdi`, `__fixunsxfdi`, `__fixxfdi`: 64-bit integer arithmetic and float↔integer conversion helpers for 32-bit targets. wasm32 has native i64 arithmetic and native float↔i64 conversion instructions (and long double is f64 on this target), so the code generator emits instructions, not calls. This version's `lib/` tree contains no `__tcc_cvt_*` symbols; the conversion helpers are the `__fix*`/`__float*` family above.
- `builtin.c` — the `__tcc_builtin_*` family (`ffs`, `clz`, `ctz`, `popcount`, `parity`, …): wasm has native `clz`/`ctz`/`popcnt` instructions; code generation handles them.
- `va_list.c` — `__va_arg`: on the wasm target a va_list is a pointer into a memory argument area and `va_arg` is pointer advancement done by the code generator.
- `alloca.S` — `alloca`/`__alloca`: wasm alloca adjusts the stack-pointer global in generated code.
- `stdatomic.c` / `atomic.S` — the `__atomic_*` family: the C source is compiled by the wasm backend like any other C file when a program uses `<stdatomic.h>`; in single-threaded wasm the operations reduce to plain loads and stores. No host imports are involved.
- `bcheck.c` — the `__bound_*` family: emitted only in `-b` bounds-checking mode, and the implementation depends on mmap, signals and pthreads. Not provided.
- `runmain.c` — `_runmain`, `__run_on_exit`, `on_exit`, `atexit`, `exit`: the native `-run` support file. In the browser path the glue invokes `_start` directly, and `exit`/`atexit` semantics come from the libc shims above.
- `armeabi.c` (`__aeabi_*`), `lib-arm64.c`, `tcov.c`: ARM-target helpers and the coverage runtime; not applicable to wasm.

## Custom shims

Programs that import symbols outside the WASI shim and the default libc — a host API, a peripheral, anything — get them from JS functions registered as shims. A shim is called as `fn(ctx, ...cArgs)`:

- `ctx.memory()` / `ctx.u8()` — the instance's linear memory and a `Uint8Array` view of it;
- `ctx.readCString(ptr)`, `ctx.readBytes(ptr, len)`, `ctx.writeBytes(ptr, bytes)` — memory helpers;
- `ctx.io` — the run's byte IO (`out`/`err` taking `Uint8Array`, optional `in()`).

C arguments arrive in their wasm types: i32/f32/f64 as numbers, i64 as `BigInt`; the return value goes back the same way, so an i64 result must be returned as a `BigInt`. The JS function declares no signature — the engine coerces the arguments and the return value to the import type the module itself declares.

Registration exists at three levels. For the same import, the higher level wins; within one level, a later registration replaces an earlier one:

1. **Per-run** — `runWasmModule(bytes, { shims: { name: fn } })`; `state.run(args, { shims })` and `callMain(['-run', ...], { shims })` forward it.
2. **Instance** — `tinyc.registerShim(name, fn)`, `tinyc.registerShims({ ... })`, `tinyc.unregisterShim(name)`; applies to every later run through that instance.
3. **Package-global** — `registerGlobalShim(name, fn)`, `registerGlobalShims({ ... })`, `unregisterGlobalShim(name)`; applies to every later instance and `runWasmModule` call.

Below all three sit the default libc shims and the WASI shim. Above all three sits `options.imports`, merged into the import object as-is (the raw escape hatch — it can also supply non-function imports such as a memory). A registration key is a bare name (module `env`) or `"module.name"` for any other import module — including `wasi_snapshot_preview1`, whose built-in entries can be overridden the same way.

**Missing imports.** Before instantiation, the runner lists the module's required imports (`WebAssembly.Module.imports`) and diffs them against the assembled import object. Anything missing throws `TinycMissingImportError` — with the full `module.name` list on `err.missing` and the registration routes in the message — instead of the engine's raw `LinkError`. Registering the named symbols is the fix.

## Getting compiled products out

Compilation products are files in the virtual filesystem, for every target (object files, executables, libraries, Windows PE files):

```js
const bytes = tinyc.exportFile('/out/prog.exe');  // Uint8Array
const blob = tinyc.exportBlob('/out/prog.exe');   // Blob, e.g. for a download link
```

## Usage

### Command-line mode

```js
const exitCode = tinyc.callMain(['-o', '/out/a.o', '-c', '/src/main.c']);
```

`callMain` fills `argv[0]` with the virtual binary path (`/sysroot/bin/tinyc`) and calls the exported `tinyc_main` (falling back to `main`). It returns the exit code. The exported main is not reentrant; a nested `callMain` throws.

### Fine-grained API

```js
const state = tinyc.createState();
state.setOutputType(1); // TCC_OUTPUT.MEMORY
state.addIncludePath('/deps/include');
state.compileString('int answer() { return 42; }');
state.delete();
```

State methods mirror the libtcc functions they call (`tcc_compile_string`, `tcc_add_include_path`, `tcc_add_file`, …). A method whose export is missing from the native module throws `TinycMissingExportError` naming that export.

### Virtual filesystem and adding files

There is no real filesystem for TCC to point at in a browser, so user files enter the virtual filesystem by content. This is the browser form of the "dynamically add local files" rule:

```js
await tinyc.addFile('/src/main.c', 'int main() { return 0; }');   // string / Uint8Array / ArrayBuffer
await tinyc.addFile('/src/util.h', fileFromAnInputElement);       // File or Blob
await tinyc.addFileFromUrl('/deps/sqlite3.h', 'https://example.com/sqlite3.h');
tinyc.registerIncludeDir('/deps');                                // -> tcc_add_include_path on new states
```

Files are held in a `Map` in `index.mjs` (`tinyc.fs`). When the native module is an Emscripten build, the map is mirrored into the module's `FS`; that mirroring is confined to `VirtualFS.mountTo`/`syncFile`, the Emscripten FS adaptation point.

`compileFile(path)` compiles a virtual-FS source through `tcc_compile_string` with a `#line` marker carrying the path; `state.addFile(path)` passes the path to `tcc_add_file` for sources, objects and archives.

### IO

IO is plain JS functions — this package needs no trampoline layer:

```js
const lines = [];
tinyc.setIO({
  out: (text) => lines.push(text),
  err: (text) => console.error(text),
  in: () => null, // EOF
});
```

Defaults: `out`/`err` are line-buffered writers onto `console.log`/`console.error`, `in` returns EOF. Per-state `state.setIO(...)` reroutes that state's error channel through `tcc_set_error_func`; native stdout routing is per-module and follows the instance IO.

### Web Worker

```js
import { TinycWorker } from 'tinyc-web/worker';

const worker = new TinycWorker({ target: 'x86_64-linux-gnu', sysroot: true });
await worker.compileString('int f() { return 1; }');
const exitCode = await worker.runCli(['-c', '/src/main.c', '-o', '/out/a.o']);
worker.terminate();
```

`TinycWorker` spawns `worker.mjs` as a module worker and returns promises for `compileString`, `compileFile`, `addFile`, `registerIncludeDir` and `runCli`. The `target` option selects the per-target module inside the worker (default `wasm32-wasi`); the run guard applies there too, so `runCli(['-run', ...])` rejects for non-wasm32 targets. Native output inside the worker is forwarded to the main thread and dispatched to the `io` option.

Shim functions cannot be registered on a `TinycWorker` from the main thread — a JS function cannot cross `postMessage`, and `worker.registerShim(...)` / `worker.registerShims(...)` throw saying so. The worker-side route is the constructor's `shimModules` option: an array of module URLs. The worker imports each module and registers its default export — a shim map object (`{ name: fn }`) — on its instance:

```js
const worker = new TinycWorker({ shimModules: ['./my-shims.mjs'] });
// my-shims.mjs:  export default { host_log: (ctx, ptr) => { ... } };
```

## Tests

```
npm test        # node --test test.mjs
```

The suite runs the glue logic in Node against an injected mock module implementing the `tcc_*` names: virtual FS intake from string, `Blob`/`File` and URL sources, include-dir registration reaching `tcc_add_include_path`, IO routing at the Emscripten wiring point and through the error callback, `callMain` argv assembly, the missing-export error, the target registry (two targets coexisting with isolated virtual filesystems, per-target lazy loading), the run guard on non-wasm32 targets and the wasm32 run path, product export, and the per-target sysroot manifests. The run paths are additionally tested with real hand-assembled wasm modules (byte arrays in `test.mjs`, each with its WAT described): a function-style `add` export, a `_start` module writing through the WASI `fd_write` shim and exiting via `proc_exit`, the WASI argv shim, the default libc shims (`printf` formatting into IO, `memcpy`, `malloc`/`free` heap reuse, `options.imports` overriding a default shim), and the custom-shim surface — the missing-import diagnostic with its exact list, instance-level registration with `ctx.readCString`, per-run override of an instance registration, `unregisterShim`, `callMain -run` shim forwarding, package-global registration, and the `TinycWorker.registerShim` boundary error.
