# tinyc-embedded

**English** | [简体中文](README.zh-CN.md)

TinyC (TCC 0.9.28rc) packaged as an embedded compiler: the compiler
itself plus its headers and libraries, distributed as one dual-form
native file per target and as per-ecosystem packages for Node.js,
Python, Java, Android, and the browser.

## The dual-form file

`build/dist/x86_64-linux-gnu/bin/tinyc` is a single file that is both:

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

Per platform, the same idea takes the form the platform allows:

- **Linux** — the PIE file above: executable and `dlopen`-able as-is.
- **Windows** — an EXE with an export table. `LoadLibrary` can load
  it; the host glue then calls the exported `tinyc_init()` once
  before using `tinyc_main` / `tcc_*`.
- **macOS** — a PIE executable cannot be `dlopen`ed (Mach-O `MH_PIE`
  and `MH_DYLIB` are mutually exclusive), so the file carries a
  prebuilt dylib segment that the package glue extracts to a cache
  directory and `dlopen`s. To the user it is still one file.

## Two entry styles

Every package exposes the same two entry styles over the one file:

- **Command-line style** — pass an argument array, get an exit code.
  The glue fills `argv[0]` with the native file's own path so the
  compiler finds its bundled headers and libraries. `tinyc_main`
  keeps process-global state: calls are serialized by the glue and
  must not run concurrently or reentrantly.
- **Session style** — hold a state object (wrapping `tcc_new` /
  `TCCState`) and drive the `tcc_*` API step by step: add paths,
  compile strings or files, produce an output file, or relocate into
  memory and call compiled functions directly.

## IO

Compiler-owned output goes through the `TCCIO` interface
(`core/tinyc_io.h`): `write_out` / `write_err` / `read_in` / `flush`
plus a context pointer. Each TCCState carries its own IO, installed
with `tcc_set_io`; there is no process-wide current IO, and a state
with none installed uses the default implementation, which writes
directly to the process's real stdio. Ready-made implementations:
`tcc_io_stdio`, `tcc_io_buffer` (separate or merged channels),
`tcc_io_file`, `tcc_io_fd`, `tcc_io_FILE`, `tcc_io_callback`,
`tcc_io_null`, `tcc_io_tee`. The compiler's error path, help,
version, search-path output and `-E` preprocessor output are routed
through the owning state's IO; the host process's fd 0/1/2 are never
replaced. While a compiled program runs in-process (see the run
rule below), its stdin/stdout/stderr are connected to the running
state's IO for the duration of that run. Each package lets you pass
your own callbacks, streams, or file objects instead — see the
package sections.

## Hosts and targets

A **host** is where TCC itself runs; a **target** is what it
compiles for. One binary serves one target; a package carries one
binary per target next to that target's sysroot. Hosts and targets
are 64-bit only — on a 32-bit host the loaders report the host as
unsupported instead of falling back to another platform's files.

Hosts:

| Host triple | Form |
|---|---|
| `x86_64-linux-gnu` | native packages (glibc) |
| `aarch64-linux-gnu` | native packages (glibc) |
| `x86_64-w64-mingw32` | native packages (Windows x86-64) |
| `arm64-apple-darwin` | native packages (macOS) |
| `x86_64-apple-darwin` | native packages (macOS) |
| `aarch64-linux-android`, `x86_64-linux-android` | Android AAR; host and target are the device |

Targets (9): `x86_64-linux-gnu`, `aarch64-linux-gnu`,
`riscv64-linux-gnu`, `x86_64-w64-mingw32`, `aarch64-linux-android`,
`x86_64-linux-android`, `arm64-apple-darwin`, `x86_64-apple-darwin`,
`wasm32-wasi`. The output formats each target can produce (ELF /
PE / Mach-O / WebAssembly, with file extensions) are recorded in
that target's `manifest.json` `outputs` field and are the same in
every package.

- `riscv64-linux-gnu` is a compile target only; there is no
  riscv64 host package.
- iOS has no native package: code signing and the ban on executing
  runtime-generated code leave no path for runtime-delivered native
  binaries. On iOS, use the browser package.

**Run rule.** Compiling to a file works for every (host, target)
combination. Running a compiled program inside the host process
(`tcc_run`, `-run`) works only when the target is the host's own
platform; cross combinations refuse at the call site with an error
naming the combination. The browser is the same rule with the
browser as host: only `wasm32-wasi` output can run there. Note that
an in-process run executes the compiled program inside your own
process — its `exit()` and its crashes land on the host.

## Adding your own headers and libraries

Headers and libraries beyond the bundled set are given as local
real filesystem paths at use time and used as-is — nothing is
copied or packaged:

- directories via `-I` / `-L` on the command line, or
  `tcc_add_include_path` / `tcc_add_library_path` in the API;
- a single header file by registering its parent directory;
- a single library file via `tcc_add_file`.

User paths are registered before the bundled sysroot and shadow it.
Each compilation assembles a fresh state, so a changed path list
takes effect on the next compile without restarting the process.
(In the browser package there is no real filesystem to point at;
files enter its virtual filesystem by content instead — see its
section below.)

## Packages

The npm, Python, and Java packages share one internal layout:

```
<package root>/
├─ packaging-manifest.json     the (host, target) combinations inside
├─ natives/<host-triple>/toolchain/<target-triple>/tinyc[.exe]
└─ sysroot/<target-triple>/    one sysroot per target, shared by all hosts
```

The loaders resolve in the same order: detect the host triple
(`TINYC_HOST_TRIPLE` overrides detection), default the target to
the host (`TINYC_TARGET_TRIPLE` selects a cross target), then look
the combination up in `packaging-manifest.json`. A combination the
package does not contain fails with an error naming the host, the
target, and the combinations available.

### npm — `@xnetx/tinyc`

```
npm install @xnetx/tinyc
```

The native files ship in a per-host subpackage,
`@xnetx/tinyc-native-<host-triple>`, which npm installs
automatically as an optional dependency (the main package also
works standalone when it carries its own `natives/`). The binding
uses koffi.

```js
const tcc = require('@xnetx/tinyc');

const code = tcc.runCli(['hello.c', '-o', 'hello']);   // exit code
const [code2, output] = tcc.runCliCapture(['--version']);

const s = new tcc.State();
s.addIncludePath('/path/to/headers');     // local paths, used as-is
s.addFile('/path/to/libfoo.a');
s.compileString('int add(int a, int b){ return a + b; }');
const add = s.function('add', 'int', ['int', 'int']);
console.log(add(19, 23)); // 42
s.close();
```

IO options: `tcc.runCli(args, { io })` accepts `'capture'`,
`tcc.IO.file(path)`, `tcc.IO.callback({ out: fn })` (fn receives a
Buffer), or any Node `Writable`. Compile errors throw
`tcc.TinyCError` and are collected in `s.errors`.

### Python — `tinyc`

```
pip install tinyc
```

Wheels are published per host platform tag (`manylinux` x86_64 /
aarch64, `win_amd64`, `macosx` arm64 / x86_64); each wheel carries
that host's target variants. The binding is `ctypes` only — no
extra native components.

```python
import ctypes, tinyc

code = tinyc.run_cli(["hello.c", "-o", "hello"])      # exit code
code, output = tinyc.run_cli_capture(["--version"])   # captured output

with tinyc.State() as s:
    s.add_include_path("/path/to/headers")     # local paths, used as-is
    s.add_file("/path/to/libfoo.a")
    s.compile_string("int add(int a, int b){ return a + b; }")
    add = s.function("add", ctypes.c_int, ctypes.c_int, ctypes.c_int)
    assert add(19, 23) == 42
```

IO options: `tinyc.run_cli(args, io=...)` accepts `"capture"`,
`tinyc.IO.file(path)`, `tinyc.IO.callback(out=my_fn)` (my_fn
receives `bytes`), or any binary file object. Compile errors raise
`tinyc.TinyCError` and are collected in `s.errors`.

### Java — Jar

The server-side Java package is a single Jar using the Panama FFM
API (JDK 22 or newer; run with `--enable-native-access=ALL-UNNAMED`).
No JNI and no extra native libraries — the binding is pure Java.
On first use the Jar extracts its native files and sysroots to
`~/.cache/tinyc-java/<version>/` and reuses that directory
afterwards. Classes live in the `tinyc` package.

```java
try (TccSession s = TinyC.newSession()) {
    s.setOutputType(TinyC.TCC_OUTPUT_MEMORY);
    if (s.compileString("int add(int a, int b) { return a + b; }") != 0) {
        throw new IllegalStateException("compile failed");
    }
    s.relocate();
    MemorySegment fn = s.getSymbol("add");
    // call it through a MethodHandle from TinyC.functionHandle(...)
}

int rc = TinyC.runCli("-o", "hello", "hello.c");   // exit code
```

IO is the `TccIO` interface (`writeOut` / `writeErr` / `readIn`,
byte-oriented), with ready-made stdio, stream-adapting, and
capturing implementations.

### Android — AAR

The Android package is an AAR carrying `libtinyc.so` (the dual-form
kernel with a JNI entry) per ABI — `arm64-v8a` ↔
`aarch64-linux-android`, `x86_64` ↔ `x86_64-linux-android` — plus
the toolchain and bionic sysroot as assets. On the device, host and
target are the same Android platform.

```gradle
dependencies {
    implementation files('libs/tinyc-android-release.aar')
}
```

```java
TinycAssets.Paths paths = TinycAssets.init(context);  // once: extracts assets to filesDir

try (Tinyc t = Tinyc.create(paths)) {
    t.setOutputType(Tinyc.OUTPUT_OBJ);
    t.addIncludePath("/data/user/0/com.example/files/mylib/include");
    t.compileString("int answer(void) { return 42; }");
    t.outputFile(new File(dir, "answer.o").getAbsolutePath());
}

int rc = Tinyc.runCli("-o", outPath, "-I", includeDir, sourcePath);
```

IO is `TinycIO` (streams or byte-chunk callbacks). Dynamic addition
works as everywhere else, with paths pointing at real files inside
the app sandbox.

### Browser — `tinyc-web`

The compiler built to WebAssembly, one module per target, with a
JavaScript glue layer and a virtual filesystem. The host is the
browser itself.

```
npm install tinyc-web
```

```js
import { loadTinyc } from 'tinyc-web';

const tinyc = await loadTinyc({ target: 'x86_64-linux-gnu', sysroot: true });
await tinyc.addFile('/src/main.c', 'int main(void) { return 0; }');
tinyc.callMain(['-c', '/src/main.c', '-o', '/out/main.o']);
const bytes = tinyc.exportFile('/out/main.o');   // Uint8Array
```

It also ships as a single classic script, `tinyc-web.js`, defining
the global `TinycWeb` with the same API — usable straight from a
CDN:

```html
<script src="https://cdn.jsdelivr.net/npm/tinyc-web@0.1.0/tinyc-web.js"></script>
```

Only the requested target's module is fetched, lazily. Files are
managed by content: write with `addFile` / `addFileSync` /
`addFileFromUrl`, read with `readFile` / `readTextFile`
(`exportFile` / `exportBlob` for compiled products), list with
`listFiles(prefix)`, delete with `removeFile` / `clearFiles()`;
`await tinyc.whenIdle()` is the point where queued writes have
been persisted. File contents persist in IndexedDB (one database
per target); only a path index and a small content cache stay in
memory. The compiler module's own filesystem is WasmFS: inside a
Web Worker its working set is mounted on OPFS, the browser's
on-disk origin-private filesystem (`tinyc.fsBackend === 'opfs'`);
in a page context it stays in memory (`'memory'`).

Running follows the run rule with the browser as host: only the
default target, `wasm32-wasi`, produces output the browser can
execute. `tinyc.runWasmFile(path, options)` runs a product already
in the virtual filesystem; `state.run(...)` and
`callMain(['-run', ...])` go through the same runner. All other
targets compile to files, and run requests for them are rejected
with `TinycRunNotSupportedError`; `tinyc.canRun` reports which
case an instance is in. Programs that import symbols beyond the
built-in WASI and libc shims receive them from JavaScript functions
registered as shims (`registerShim` / `registerShims`, per run,
per instance, or package-global). A `TinycWorker` interface
(`tinyc-web/worker`) runs an instance inside a Web Worker.

## Known limitations

- **macOS, dynamically added libraries:** TCC can only load object
  files and archives it produced itself. A `.o` produced by clang /
  Xcode is rejected with `unrecognized file type`, and the BSD
  archive index (`__.SYMDEF`) in a clang-produced `.a` is not
  understood, so its members are not pulled in. System libraries
  are not affected.
- **Browser, page context:** the module filesystem backend is
  memory there; the OPFS on-disk backend is available in Web
  Workers only. (The package's own virtual filesystem persists to
  IndexedDB in both contexts.)
- **Standard input as source:** naming `-` as the input file reads
  the process's real stdin; that path does not go through TCCIO's
  `read_in`.

## Building from source

```
bash build.sh            # full target matrix -> build/dist/<target>/
bash build.sh host       # host variants (target == host) -> build/dist-host/
bash tests/run_all.sh    # kernel, IO, matrix, and package tests
```

Each target lands in `build/dist/<target>/` with this layout:

```
bin/tinyc          dual-form file (bin/tcc is a symlink to it)
include/           libtcc.h, tinyc_io.h
lib/tcc/           libtcc1.a and TCC's builtin headers (lib/tcc/include/)
manifest.json      target, ISA baseline, outputs, file locations
```

Each package has its own build entry and README under
`packages/<name>/`. Building the browser package additionally
requires an activated Emscripten SDK (`emcc` on `PATH`).

## License

LGPL-2.1-or-later, the license of TCC itself.
