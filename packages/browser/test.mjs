// Glue-logic tests for tinyc-web, run in Node against an injected
// mock native module that implements the libtcc export names.
// Run: npm test   (node --test test.mjs)

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readdirSync, readFileSync } from 'node:fs';

import {
  loadTinyc,
  VirtualFS,
  TinycMissingExportError,
  TinycRunNotSupportedError,
  VIRTUAL_BIN_PATH,
  sysrootManifest,
  DEFAULT_TARGET,
  DEFAULT_RUN_OUTPUT,
  SUPPORTED_TARGETS,
  isRunnableTarget,
  targetModuleFiles,
  loadSysrootManifest,
  getLoadedTargets,
  clearTargetModuleCache,
  runWasmModule,
  runWasmModuleSync,
  WasiExit,
  TinycMissingImportError,
  registerGlobalShim,
  registerGlobalShims,
  unregisterGlobalShim,
  writeModuleData,
  readModuleData,
  readModuleCString,
} from './index.mjs';
import { TinycWorker } from './worker.mjs';

// ---------------------------------------------------------------------
// Mock native module: same export names as libtcc.h, JS values pass
// through as-is (mock mode in the bridge). Records every call.
// ---------------------------------------------------------------------
function makeMock(overrides = {}) {
  const calls = [];
  const mock = {
    calls,
    errorFunc: null,
    main(argc, argv) { calls.push(['main', argc, argv]); return 3; },
    tinyc_main(argc, argv) { calls.push(['tinyc_main', argc, argv]); return 7; },
    tcc_new() { calls.push(['tcc_new']); return { fakeState: true }; },
    tcc_delete(h) { calls.push(['tcc_delete', h]); },
    tcc_set_lib_path(h, p) { calls.push(['tcc_set_lib_path', h, p]); },
    tcc_set_error_func(h, opaque, fn) {
      calls.push(['tcc_set_error_func', h]);
      mock.errorFunc = fn;
    },
    tcc_set_options(h, s) { calls.push(['tcc_set_options', h, s]); return 0; },
    tcc_add_include_path(h, p) { calls.push(['tcc_add_include_path', h, p]); return 0; },
    tcc_add_sysinclude_path(h, p) { calls.push(['tcc_add_sysinclude_path', h, p]); return 0; },
    tcc_define_symbol(h, s, v) { calls.push(['tcc_define_symbol', h, s, v]); },
    tcc_undefine_symbol(h, s) { calls.push(['tcc_undefine_symbol', h, s]); },
    tcc_add_file(h, f) { calls.push(['tcc_add_file', h, f]); return 0; },
    tcc_compile_string(h, src) { calls.push(['tcc_compile_string', h, src]); return 0; },
    tcc_set_output_type(h, t) { calls.push(['tcc_set_output_type', h, t]); return 0; },
    tcc_add_library_path(h, p) { calls.push(['tcc_add_library_path', h, p]); return 0; },
    tcc_add_library(h, l) { calls.push(['tcc_add_library', h, l]); return 0; },
    tcc_output_file(h, f) { calls.push(['tcc_output_file', h, f]); return 0; },
    tcc_relocate(h) { calls.push(['tcc_relocate', h]); return 0; },
    tcc_get_symbol(h, n) { calls.push(['tcc_get_symbol', h, n]); return 0; },
  };
  return Object.assign(mock, overrides);
}

const callsNamed = (mock, name) => mock.calls.filter((c) => c[0] === name);

// ---------------------------------------------------------------------
// Virtual FS: three intake sources
// ---------------------------------------------------------------------

test('vfs: string and bytes sources round-trip', async () => {
  const fs = new VirtualFS();
  await fs.addFile('/src/main.c', 'int main() { return 0; }');
  fs.addFileSync('src/util.c', new TextEncoder().encode('int util;'));
  assert.equal(fs.readTextFile('/src/main.c'), 'int main() { return 0; }');
  assert.equal(fs.readTextFile('/src/util.c'), 'int util;');
  assert.ok(fs.hasFile('/src/main.c'));
  assert.deepEqual(fs.listFiles('/src'), ['/src/main.c', '/src/util.c']);
});

test('vfs: File and Blob sources', async () => {
  const fs = new VirtualFS();
  const file = new File(['#define FROM_FILE 1\n'], 'defs.h', { type: 'text/plain' });
  const blob = new Blob(['#define FROM_BLOB 2\n']);
  await fs.addFile('/inc/defs.h', file);
  await fs.addFile('/inc/blob.h', blob);
  assert.equal(fs.readTextFile('/inc/defs.h'), '#define FROM_FILE 1\n');
  assert.equal(fs.readTextFile('/inc/blob.h'), '#define FROM_BLOB 2\n');
});

test('vfs: URL source via injected fetch', async () => {
  const fs = new VirtualFS();
  const body = '#define FROM_URL 3\n';
  const fetchImpl = async (url) => {
    assert.equal(url, 'https://example.com/net.h');
    return {
      ok: true,
      status: 200,
      arrayBuffer: async () => new TextEncoder().encode(body).buffer,
    };
  };
  await fs.addFileFromUrl('/net/net.h', 'https://example.com/net.h', { fetchImpl });
  assert.equal(fs.readTextFile('/net/net.h'), body);
});

test('vfs: URL source reports HTTP failure', async () => {
  const fs = new VirtualFS();
  const fetchImpl = async () => ({ ok: false, status: 404, arrayBuffer: async () => new ArrayBuffer(0) });
  await assert.rejects(
    fs.addFileFromUrl('/x.h', 'https://example.com/x.h', { fetchImpl }),
    /404/
  );
});

// ---------------------------------------------------------------------
// Include dirs and state wiring
// ---------------------------------------------------------------------

test('registerIncludeDir reaches tcc_add_include_path on createState', async () => {
  const mock = makeMock();
  const tinyc = await loadTinyc({ wasmExports: mock });
  tinyc.registerIncludeDir('/deps/include');
  tinyc.registerIncludeDir('/deps/include'); // duplicate is a no-op
  const state = tinyc.createState();
  const inc = callsNamed(mock, 'tcc_add_include_path');
  assert.equal(inc.length, 1);
  assert.equal(inc[0][2], '/deps/include');
  state.compileString('int x;');
  const compiled = callsNamed(mock, 'tcc_compile_string');
  assert.equal(compiled.length, 1);
  assert.equal(compiled[0][2], 'int x;');
  state.delete();
  assert.equal(callsNamed(mock, 'tcc_delete').length, 1);
});

test('compileFile feeds virtual FS content with a #line marker', async () => {
  const mock = makeMock();
  const tinyc = await loadTinyc({ wasmExports: mock });
  tinyc.addFileSync('/src/a.c', 'int a;\n');
  const state = tinyc.createState();
  state.compileFile('/src/a.c');
  const src = callsNamed(mock, 'tcc_compile_string')[0][2];
  assert.ok(src.startsWith('#line 1 "/src/a.c"\n'));
  assert.ok(src.includes('int a;'));
  state.delete();
});

// ---------------------------------------------------------------------
// IO routing
// ---------------------------------------------------------------------

test('io: Emscripten print/printErr wiring routes to custom io', async () => {
  const mock = makeMock();
  let captured;
  const outChunks = [];
  const errChunks = [];
  await loadTinyc({
    createModule: async (opts) => { captured = opts; return mock; },
    io: { out: (t) => outChunks.push(t), err: (t) => errChunks.push(t) },
  });
  captured.print('hello');
  captured.printErr('oops');
  assert.deepEqual(outChunks, ['hello\n']);
  assert.deepEqual(errChunks, ['oops\n']);
});

test('io: default wiring lands on console', async () => {
  const mock = makeMock();
  let captured;
  await loadTinyc({ createModule: async (opts) => { captured = opts; return mock; } });
  const logged = [];
  const origLog = console.log;
  console.log = (line) => logged.push(line);
  try {
    captured.print('to-console');
  } finally {
    console.log = origLog;
  }
  assert.deepEqual(logged, ['to-console']);
});

test('io: state error callback routes to err io', async () => {
  const mock = makeMock();
  const errChunks = [];
  const tinyc = await loadTinyc({
    wasmExports: mock,
    io: { err: (t) => errChunks.push(t) },
  });
  const state = tinyc.createState();
  assert.equal(typeof mock.errorFunc, 'function');
  mock.errorFunc(null, 'something broke');
  assert.deepEqual(errChunks, ['something broke\n']);
  state.delete();
});

// ---------------------------------------------------------------------
// callMain argv assembly
// ---------------------------------------------------------------------

test('callMain: argv assembled with virtual bin path, tinyc_main preferred', async () => {
  const mock = makeMock();
  const tinyc = await loadTinyc({ wasmExports: mock });
  const rc = tinyc.callMain(['-o', 'a.out', 'a.c']);
  assert.equal(rc, 7); // tinyc_main's return, not main's
  assert.equal(callsNamed(mock, 'main').length, 0);
  const entry = callsNamed(mock, 'tinyc_main')[0];
  assert.equal(entry[1], 4); // argc includes argv[0]
  assert.deepEqual(entry[2], [VIRTUAL_BIN_PATH, '-o', 'a.out', 'a.c']);
});

test('callMain: falls back to main when tinyc_main is absent', async () => {
  const mock = makeMock({ tinyc_main: undefined });
  const tinyc = await loadTinyc({ wasmExports: mock });
  const rc = tinyc.callMain(['-v']);
  assert.equal(rc, 3);
  const entry = callsNamed(mock, 'main')[0];
  assert.deepEqual(entry[2], [VIRTUAL_BIN_PATH, '-v']);
});

test('callMain: missing main export throws a named error', async () => {
  const mock = makeMock({ tinyc_main: undefined, main: undefined });
  const tinyc = await loadTinyc({ wasmExports: mock });
  assert.throws(
    () => tinyc.callMain([]),
    (err) => err instanceof TinycMissingExportError && err.exportName === 'main'
  );
});

// ---------------------------------------------------------------------
// Missing exports on the fine-grained API
// ---------------------------------------------------------------------

test('missing tcc_* export throws TinycMissingExportError naming it', async () => {
  const mock = makeMock({ tcc_compile_string: undefined });
  const tinyc = await loadTinyc({ wasmExports: mock });
  const state = tinyc.createState();
  assert.throws(
    () => state.compileString('int x;'),
    (err) => err instanceof TinycMissingExportError &&
      err.exportName === 'tcc_compile_string' &&
      /tcc_compile_string/.test(err.message)
  );
  state.delete();
});

// ---------------------------------------------------------------------
// Sysroot manifest vs upstream sources
// ---------------------------------------------------------------------

test('sysroot manifest lists the real upstream include files', () => {
  const upstreamInclude = new URL('../../vendor-tinycc/include/', import.meta.url);
  const onDisk = readdirSync(upstreamInclude).sort();
  const inManifest = sysrootManifest.includeFiles
    .map((e) => e.source)
    .filter((s) => s.startsWith('include/'))
    .map((s) => s.slice('include/'.length))
    .sort();
  assert.deepEqual(inManifest, onDisk);
  const paths = sysrootManifest.includeFiles.map((e) => e.path);
  assert.ok(paths.includes('/sysroot/include/tcclib.h'));
  assert.ok(paths.every((p) => p.startsWith(sysrootManifest.includeDir + '/')));
});

// ---------------------------------------------------------------------
// Worker module surface (import must be side-effect free off-worker)
// ---------------------------------------------------------------------

test('worker module imports cleanly and exposes TinycWorker', () => {
  assert.equal(typeof TinycWorker, 'function');
});

// ---------------------------------------------------------------------
// Multi-target registry
// ---------------------------------------------------------------------

test('targets: registry lists the supported targets, wasm32-wasi is the default', () => {
  assert.equal(DEFAULT_TARGET, 'wasm32-wasi');
  assert.deepEqual([...SUPPORTED_TARGETS], [
    'wasm32-wasi',
    'x86_64-linux-gnu',
    'aarch64-linux-gnu',
    'riscv64-linux-gnu',
    'x86_64-windows',
    'aarch64-linux-android',
    'x86_64-linux-android',
    'arm64-apple-darwin',
    'x86_64-apple-darwin',
  ]);
  assert.equal(isRunnableTarget('wasm32-wasi'), true);
  for (const t of SUPPORTED_TARGETS.slice(1)) {
    assert.equal(isRunnableTarget(t), false, t);
  }
  assert.deepEqual(targetModuleFiles('aarch64-linux-gnu'), {
    wasmFile: 'tcc-aarch64-linux-gnu.wasm',
    glueFile: 'tcc-aarch64-linux-gnu.mjs',
  });
});

test('targets: unknown target is rejected with the supported list', async () => {
  await assert.rejects(
    loadTinyc({ target: 'sparc-unknown', wasmExports: makeMock() }),
    (err) => /unknown target "sparc-unknown"/.test(err.message) &&
      err.message.includes('wasm32-wasi')
  );
});

test('targets: two instances of different targets coexist, virtual FS isolated', async () => {
  const mockA = makeMock();
  const mockB = makeMock();
  const a = await loadTinyc({ target: 'x86_64-linux-gnu', wasmExports: mockA });
  const b = await loadTinyc({ target: 'wasm32-wasi', wasmExports: mockB });
  assert.equal(a.target, 'x86_64-linux-gnu');
  assert.equal(b.target, 'wasm32-wasi');
  assert.equal(a.canRun, false);
  assert.equal(b.canRun, true);

  a.addFileSync('/src/only-a.c', 'int only_a;\n');
  assert.ok(a.fs.hasFile('/src/only-a.c'));
  assert.equal(b.fs.hasFile('/src/only-a.c'), false);

  const stateA = a.createState();
  stateA.compileString('int from_a;');
  assert.equal(callsNamed(mockA, 'tcc_compile_string').length, 1);
  assert.equal(callsNamed(mockB, 'tcc_compile_string').length, 0);
  stateA.delete();

  const stateB = b.createState();
  stateB.compileString('int from_b;');
  assert.equal(callsNamed(mockB, 'tcc_compile_string').length, 1);
  stateB.delete();
});

test('targets: module files load lazily, per target, at most once', async () => {
  clearTargetModuleCache();
  const loads = [];
  const loadModule = async (target, files) => {
    loads.push([target, files.wasmFile, files.glueFile]);
    return makeMock();
  };
  const one = await loadTinyc({ target: 'x86_64-windows', loadModule });
  assert.equal(one.target, 'x86_64-windows');
  assert.deepEqual(loads, [[
    'x86_64-windows', 'tcc-x86_64-windows.wasm', 'tcc-x86_64-windows.mjs',
  ]]);
  // Same target again: served from the per-target cache, no reload.
  await loadTinyc({ target: 'x86_64-windows', loadModule });
  assert.equal(loads.length, 1);
  // A different target loads only its own module file.
  await loadTinyc({ target: 'riscv64-linux-gnu', loadModule });
  assert.deepEqual(loads.map((l) => l[0]), ['x86_64-windows', 'riscv64-linux-gnu']);
  assert.deepEqual(getLoadedTargets().sort(), ['riscv64-linux-gnu', 'x86_64-windows']);
  clearTargetModuleCache();
});

// ---------------------------------------------------------------------
// Hand-assembled wasm modules for the run tests (no wasm toolchain
// in this environment; each array is the byte encoding of the WAT
// described above it, validated with WebAssembly.validate).
// ---------------------------------------------------------------------

// (module (func (export "add") (param i32 i32) (result i32)
//   local.get 0 local.get 1 i32.add))
const WASM_ADD = new Uint8Array([
  0x00,0x61,0x73,0x6d, 0x01,0x00,0x00,0x00,
  0x01,0x07,0x01, 0x60,0x02,0x7f,0x7f,0x01,0x7f,
  0x03,0x02,0x01,0x00,
  0x07,0x07,0x01, 0x03,0x61,0x64,0x64, 0x00,0x00,
  0x0a,0x09,0x01, 0x07,0x00, 0x20,0x00,0x20,0x01,0x6a,0x0b,
]);

const WASI_NAME = [0x77,0x61,0x73,0x69,0x5f,0x73,0x6e,0x61,0x70,0x73,0x68,0x6f,0x74,0x5f,0x70,0x72,0x65,0x76,0x69,0x65,0x77,0x31]; // "wasi_snapshot_preview1"

// (module
//   (import "wasi_snapshot_preview1" "fd_write" (func (param i32 i32 i32 i32) (result i32)))
//   (import "wasi_snapshot_preview1" "proc_exit" (func (param i32)))
//   (memory (export "memory") 1)
//   (func (export "_start")
//     ;; iovec at 0 = { ptr: 1024, len: 16 }; fd_write(1, 0, 1, 8); proc_exit(7)
//     ...)
//   (data (i32.const 1024) "hello from wasm\n"))
const WASM_HELLO = new Uint8Array([
  0x00,0x61,0x73,0x6d, 0x01,0x00,0x00,0x00,
  0x01,0x10,0x03, 0x60,0x04,0x7f,0x7f,0x7f,0x7f,0x01,0x7f, 0x60,0x01,0x7f,0x00, 0x60,0x00,0x00,
  0x02,0x46,0x02,
    0x16,...WASI_NAME, 0x08,0x66,0x64,0x5f,0x77,0x72,0x69,0x74,0x65, 0x00,0x00,
    0x16,...WASI_NAME, 0x09,0x70,0x72,0x6f,0x63,0x5f,0x65,0x78,0x69,0x74, 0x00,0x01,
  0x03,0x02,0x01,0x02,
  0x05,0x03,0x01,0x00,0x01,
  0x07,0x13,0x02, 0x06,0x6d,0x65,0x6d,0x6f,0x72,0x79,0x02,0x00, 0x06,0x5f,0x73,0x74,0x61,0x72,0x74,0x00,0x02,
  0x0a,0x22,0x01,0x20,
    0x00,
    0x41,0x00, 0x41,0x80,0x08, 0x36,0x02,0x00,
    0x41,0x04, 0x41,0x10, 0x36,0x02,0x00,
    0x41,0x01, 0x41,0x00, 0x41,0x01, 0x41,0x08, 0x10,0x00, 0x1a,
    0x41,0x07, 0x10,0x01,
    0x0b,
  0x0b,0x17,0x01, 0x00, 0x41,0x80,0x08,0x0b, 0x10,
    0x68,0x65,0x6c,0x6c,0x6f,0x20,0x66,0x72,0x6f,0x6d,0x20,0x77,0x61,0x73,0x6d,0x0a,
]);

// (module
//   (import "wasi_snapshot_preview1" "args_sizes_get" (func (param i32 i32) (result i32)))
//   (import "wasi_snapshot_preview1" "proc_exit" (func (param i32)))
//   (memory (export "memory") 1)
//   (func (export "_start")
//     ;; args_sizes_get(0, 4); proc_exit(mem[0])  -- exits with argc
//     ...))
const WASM_ARGC = new Uint8Array([
  0x00,0x61,0x73,0x6d, 0x01,0x00,0x00,0x00,
  0x01,0x0e,0x03, 0x60,0x02,0x7f,0x7f,0x01,0x7f, 0x60,0x01,0x7f,0x00, 0x60,0x00,0x00,
  0x02,0x4c,0x02,
    0x16,...WASI_NAME, 0x0e,0x61,0x72,0x67,0x73,0x5f,0x73,0x69,0x7a,0x65,0x73,0x5f,0x67,0x65,0x74, 0x00,0x00,
    0x16,...WASI_NAME, 0x09,0x70,0x72,0x6f,0x63,0x5f,0x65,0x78,0x69,0x74, 0x00,0x01,
  0x03,0x02,0x01,0x02,
  0x05,0x03,0x01,0x00,0x01,
  0x07,0x13,0x02, 0x06,0x6d,0x65,0x6d,0x6f,0x72,0x79,0x02,0x00, 0x06,0x5f,0x73,0x74,0x61,0x72,0x74,0x00,0x02,
  0x0a,0x12,0x01,0x10,
    0x00,
    0x41,0x00, 0x41,0x04, 0x10,0x00, 0x1a,
    0x41,0x00, 0x28,0x02,0x00, 0x10,0x01,
    0x0b,
]);

// (module
//   (import "env" "printf" (func (param i32 i32) (result i32)))
//   (import "env" "memcpy" (func (param i32 i32 i32) (result i32)))
//   (import "env" "malloc" (func (param i32) (result i32)))
//   (import "env" "free" (func (param i32)))
//   (memory (export "memory") 2)
//   (func (export "_start")
//     ;; buf = malloc(32); buf = "ABCDEFGH"; dest = malloc(32);
//     ;; memcpy(dest, buf, 9); free(buf); buf2 = malloc(32);
//     ;; printf("val=%d str=%s\n", 42, "hi");
//     ;; printf("copy=%s reuse=%d\n", dest, buf2 == buf)
//     ...)
//   (data (i32.const 1024) "val=%d str=%s\n\00")
//   (data (i32.const 2048) "hi\00")
//   (data (i32.const 3072) "copy=%s reuse=%d\n\00"))
const WASM_SHIMTEST = new Uint8Array([
  0x00,0x61,0x73,0x6d,0x01,0x00,0x00,0x00,0x01,0x1a,0x05,0x60,
  0x02,0x7f,0x7f,0x01,0x7f,0x60,0x03,0x7f,0x7f,0x7f,0x01,0x7f,
  0x60,0x01,0x7f,0x01,0x7f,0x60,0x01,0x7f,0x00,0x60,0x00,0x00,
  0x02,0x33,0x04,0x03,0x65,0x6e,0x76,0x06,0x70,0x72,0x69,0x6e,
  0x74,0x66,0x00,0x00,0x03,0x65,0x6e,0x76,0x06,0x6d,0x65,0x6d,
  0x63,0x70,0x79,0x00,0x01,0x03,0x65,0x6e,0x76,0x06,0x6d,0x61,
  0x6c,0x6c,0x6f,0x63,0x00,0x02,0x03,0x65,0x6e,0x76,0x04,0x66,
  0x72,0x65,0x65,0x00,0x03,0x03,0x02,0x01,0x04,0x05,0x03,0x01,
  0x00,0x02,0x07,0x13,0x02,0x06,0x6d,0x65,0x6d,0x6f,0x72,0x79,
  0x02,0x00,0x06,0x5f,0x73,0x74,0x61,0x72,0x74,0x00,0x04,0x0a,
  0xb9,0x01,0x01,0xb6,0x01,0x00,0x41,0x80,0x20,0x41,0x20,0x10,
  0x02,0x36,0x02,0x00,0x41,0x80,0x20,0x28,0x02,0x00,0x41,0xc1,
  0x84,0x8d,0xa2,0x04,0x36,0x02,0x00,0x41,0x80,0x20,0x28,0x02,
  0x00,0x41,0xc5,0x8c,0x9d,0xc2,0x04,0x36,0x02,0x04,0x41,0x80,
  0x20,0x28,0x02,0x00,0x41,0x00,0x36,0x02,0x08,0x41,0x84,0x20,
  0x41,0x20,0x10,0x02,0x36,0x02,0x00,0x41,0x84,0x20,0x28,0x02,
  0x00,0x41,0x80,0x20,0x28,0x02,0x00,0x41,0x09,0x10,0x01,0x1a,
  0x41,0x80,0x20,0x28,0x02,0x00,0x10,0x03,0x41,0x88,0x20,0x41,
  0x20,0x10,0x02,0x36,0x02,0x00,0x41,0x8c,0x20,0x41,0x88,0x20,
  0x28,0x02,0x00,0x41,0x80,0x20,0x28,0x02,0x00,0x46,0x36,0x02,
  0x00,0x41,0x80,0xc0,0x00,0x41,0x2a,0x36,0x02,0x00,0x41,0x84,
  0xc0,0x00,0x41,0x80,0x10,0x36,0x02,0x00,0x41,0x80,0x08,0x41,
  0x80,0xc0,0x00,0x10,0x00,0x1a,0x41,0x90,0xc0,0x00,0x41,0x84,
  0x20,0x28,0x02,0x00,0x36,0x02,0x00,0x41,0x94,0xc0,0x00,0x41,
  0x8c,0x20,0x28,0x02,0x00,0x36,0x02,0x00,0x41,0x80,0x18,0x41,
  0x90,0xc0,0x00,0x10,0x00,0x1a,0x0b,0x0b,0x37,0x03,0x00,0x41,
  0x80,0x08,0x0b,0x0f,0x76,0x61,0x6c,0x3d,0x25,0x64,0x20,0x73,
  0x74,0x72,0x3d,0x25,0x73,0x0a,0x00,0x00,0x41,0x80,0x10,0x0b,
  0x03,0x68,0x69,0x00,0x00,0x41,0x80,0x18,0x0b,0x12,0x63,0x6f,
  0x70,0x79,0x3d,0x25,0x73,0x20,0x72,0x65,0x75,0x73,0x65,0x3d,
  0x25,0x64,0x0a,0x00,
]);

// (module
//   (import "env" "my_custom_add" (func (param i32 i32) (result i32)))
//   (import "env" "host_log" (func (param i32)))
//   (memory (export "memory") 1)
//   (func (export "run") (result i32)
//     (call 1 (i32.const 1024))          ;; host_log("custom says hi")
//     (call 0 (i32.const 19) (i32.const 23)))
//   (data (i32.const 1024) "custom says hi\00"))
// Neither import is a default shim: this module drives the
// missing-import diagnostic and the shim registration tests.
const WASM_CUSTOM = new Uint8Array([
  0x00,0x61,0x73,0x6d,0x01,0x00,0x00,0x00,0x01,0x0f,0x03,0x60,
  0x02,0x7f,0x7f,0x01,0x7f,0x60,0x01,0x7f,0x00,0x60,0x00,0x01,
  0x7f,0x02,0x24,0x02,0x03,0x65,0x6e,0x76,0x0d,0x6d,0x79,0x5f,
  0x63,0x75,0x73,0x74,0x6f,0x6d,0x5f,0x61,0x64,0x64,0x00,0x00,
  0x03,0x65,0x6e,0x76,0x08,0x68,0x6f,0x73,0x74,0x5f,0x6c,0x6f,
  0x67,0x00,0x01,0x03,0x02,0x01,0x02,0x05,0x03,0x01,0x00,0x01,
  0x07,0x10,0x02,0x06,0x6d,0x65,0x6d,0x6f,0x72,0x79,0x02,0x00,
  0x03,0x72,0x75,0x6e,0x00,0x02,0x0a,0x0f,0x01,0x0d,0x00,0x41,
  0x80,0x08,0x10,0x01,0x41,0x13,0x41,0x17,0x10,0x00,0x0b,0x0b,
  0x16,0x01,0x00,0x41,0x80,0x08,0x0b,0x0f,0x63,0x75,0x73,0x74,
  0x6f,0x6d,0x20,0x73,0x61,0x79,0x73,0x20,0x68,0x69,0x00,
]);

// (module
//   (import "env" "host_fill" (func (param i32) (result i32)))
//   (import "env" "host_log" (func (param i32)))
//   (memory (export "memory") 1)
//   (func (export "run") (result i32) (local $n i32)
//     (local.set $n (call 0 (i32.const 1024)))  ;; host_fill(buf)
//     (call 1 (i32.const 1024))                 ;; host_log(buf)
//     (local.get $n)))
const WASM_WRITE = new Uint8Array([
  0x00,0x61,0x73,0x6d,0x01,0x00,0x00,0x00,0x01,0x0e,0x03,0x60,
  0x01,0x7f,0x01,0x7f,0x60,0x01,0x7f,0x00,0x60,0x00,0x01,0x7f,
  0x02,0x20,0x02,0x03,0x65,0x6e,0x76,0x09,0x68,0x6f,0x73,0x74,
  0x5f,0x66,0x69,0x6c,0x6c,0x00,0x00,0x03,0x65,0x6e,0x76,0x08,
  0x68,0x6f,0x73,0x74,0x5f,0x6c,0x6f,0x67,0x00,0x01,0x03,0x02,
  0x01,0x02,0x05,0x03,0x01,0x00,0x01,0x07,0x10,0x02,0x06,0x6d,
  0x65,0x6d,0x6f,0x72,0x79,0x02,0x00,0x03,0x72,0x75,0x6e,0x00,
  0x02,0x0a,0x14,0x01,0x12,0x01,0x01,0x7f,0x41,0x80,0x08,0x10,
  0x00,0x21,0x00,0x41,0x80,0x08,0x10,0x01,0x20,0x00,0x0b,
]);

// (module
//   (import "env" "my_custom_add" (func (param i32 i32) (result i32)))
//   (import "wasi_snapshot_preview1" "proc_exit" (func (param i32)))
//   (memory (export "memory") 1)
//   (func (export "_start")
//     (call 1 (call 0 (i32.const 19) (i32.const 23)))))  ;; proc_exit(add(19,23))
const WASM_CUSTOM_START = new Uint8Array([
  0x00,0x61,0x73,0x6d,0x01,0x00,0x00,0x00,0x01,0x0e,0x03,0x60,
  0x02,0x7f,0x7f,0x01,0x7f,0x60,0x01,0x7f,0x00,0x60,0x00,0x00,
  0x02,0x38,0x02,0x03,0x65,0x6e,0x76,0x0d,0x6d,0x79,0x5f,0x63,
  0x75,0x73,0x74,0x6f,0x6d,0x5f,0x61,0x64,0x64,0x00,0x00,0x16,
  0x77,0x61,0x73,0x69,0x5f,0x73,0x6e,0x61,0x70,0x73,0x68,0x6f,
  0x74,0x5f,0x70,0x72,0x65,0x76,0x69,0x65,0x77,0x31,0x09,0x70,
  0x72,0x6f,0x63,0x5f,0x65,0x78,0x69,0x74,0x00,0x01,0x03,0x02,
  0x01,0x02,0x05,0x03,0x01,0x00,0x01,0x07,0x13,0x02,0x06,0x6d,
  0x65,0x6d,0x6f,0x72,0x79,0x02,0x00,0x06,0x5f,0x73,0x74,0x61,
  0x72,0x74,0x00,0x02,0x0a,0x0c,0x01,0x0a,0x00,0x41,0x13,0x41,
  0x17,0x10,0x00,0x10,0x01,0x0b,
]);

// ---------------------------------------------------------------------
// Run guard: only the wasm32 target runs in the browser
// ---------------------------------------------------------------------

test('run guard: non-wasm target rejects -run in callMain before reaching native', async () => {
  const mock = makeMock();
  const tinyc = await loadTinyc({ target: 'aarch64-linux-gnu', wasmExports: mock });
  assert.throws(
    () => tinyc.callMain(['-run', '/src/a.c']),
    (err) => err instanceof TinycRunNotSupportedError &&
      err.target === 'aarch64-linux-gnu' &&
      /cannot be run in the browser/.test(err.message) &&
      /only be compiled to a file/.test(err.message)
  );
  // The guard fired in the glue layer: native main was never called.
  assert.equal(callsNamed(mock, 'tinyc_main').length, 0);
  assert.equal(callsNamed(mock, 'main').length, 0);
  // Compiling to a file on the same target is not affected.
  const rc = tinyc.callMain(['-c', '/src/a.c', '-o', '/out/a.o']);
  assert.equal(rc, 7);
  assert.equal(callsNamed(mock, 'tinyc_main').length, 1);
});

test('run guard: darwin target rejects -run and state.run before reaching native', async () => {
  const runCalls = [];
  const mock = makeMock({
    tcc_run(h, argc, argv) { runCalls.push([h, argc, argv]); return 42; },
  });
  const tinyc = await loadTinyc({ target: 'arm64-apple-darwin', wasmExports: mock });
  assert.equal(tinyc.canRun, false);
  assert.throws(
    () => tinyc.callMain(['-run', '/src/a.c']),
    (err) => err instanceof TinycRunNotSupportedError &&
      err.target === 'arm64-apple-darwin' &&
      /cannot be run in the browser/.test(err.message)
  );
  const state = tinyc.createState();
  assert.throws(
    () => state.run([]),
    (err) => err instanceof TinycRunNotSupportedError && err.target === 'arm64-apple-darwin'
  );
  // Neither path reached the native module.
  assert.equal(callsNamed(mock, 'tinyc_main').length, 0);
  assert.equal(callsNamed(mock, 'main').length, 0);
  assert.equal(runCalls.length, 0);
  state.delete();
});

test('run guard: non-wasm target rejects state.run even when tcc_run exists', async () => {
  const runCalls = [];
  const mock = makeMock({
    tcc_run(h, argc, argv) { runCalls.push([h, argc, argv]); return 42; },
  });
  const tinyc = await loadTinyc({ target: 'riscv64-linux-gnu', wasmExports: mock });
  const state = tinyc.createState();
  assert.throws(
    () => state.run([]),
    (err) => err instanceof TinycRunNotSupportedError && err.target === 'riscv64-linux-gnu'
  );
  assert.equal(runCalls.length, 0);
  state.delete();
});

test('run: wasm32 target state.run compiles to a file and instantiates it in the host', async () => {
  const runCalls = [];
  const mock = makeMock({
    tcc_run(h, argc, argv) { runCalls.push([h, argc, argv]); return 42; },
  });
  const outChunks = [];
  const tinyc = await loadTinyc({
    wasmExports: mock,
    io: { out: (t) => outChunks.push(t), err: () => {} },
  });
  assert.equal(tinyc.target, 'wasm32-wasi');
  // The mock "compiler" produces its product by having the bytes
  // ready in the virtual FS at the default run output path.
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_HELLO);
  const state = tinyc.createState();
  const rc = state.run([]);
  assert.equal(rc, 7); // the product's proc_exit(7), not tcc_run's 42
  assert.equal(runCalls.length, 0); // tcc_run is never used
  const outputs = callsNamed(mock, 'tcc_output_file');
  assert.equal(outputs.length, 1);
  assert.equal(outputs[0][2], DEFAULT_RUN_OUTPUT);
  assert.equal(outChunks.join(''), 'hello from wasm\n');
  state.delete();
});

test('run: wasm32 target callMain -run compiles then instantiates; native main never sees -run', async () => {
  const mock = makeMock({
    tinyc_main(argc, argv) {
      mock.calls.push(['tinyc_main', argc, argv]);
      return 0; // compilation succeeds
    },
  });
  const outChunks = [];
  const tinyc = await loadTinyc({
    wasmExports: mock,
    io: { out: (t) => outChunks.push(t), err: () => {} },
  });
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_HELLO);
  const rc = tinyc.callMain(['-run', '/src/prog.c', 'x']);
  assert.equal(rc, 7); // the product's proc_exit(7)
  const entries = callsNamed(mock, 'tinyc_main');
  assert.equal(entries.length, 1); // exactly the compile invocation
  assert.deepEqual(entries[0][2], [VIRTUAL_BIN_PATH, '-o', DEFAULT_RUN_OUTPUT, '/src/prog.c']);
  assert.ok(!entries[0][2].includes('-run'));
  assert.equal(outChunks.join(''), 'hello from wasm\n');
});

// ---------------------------------------------------------------------
// runWasmModule: real modules executed by the host
// ---------------------------------------------------------------------

test('runWasmModule: function-style entry returns the export result (async and sync)', async () => {
  assert.equal(await runWasmModule(WASM_ADD, { entry: 'add', args: [19, 23] }), 42);
  assert.equal(runWasmModuleSync(WASM_ADD, { entry: 'add', args: [19, 23] }), 42);
  assert.equal(runWasmModuleSync(WASM_ADD, { entry: 'add', callArgs: [1, 2] }), 3);
});

test('runWasmModule: _start writes through the WASI fd_write shim; proc_exit becomes the exit code', async () => {
  for (const run of [(b, o) => runWasmModuleSync(b, o), (b, o) => runWasmModule(b, o)]) {
    const dec = new TextDecoder();
    let text = '';
    const io = {
      out: (b) => { text += dec.decode(b, { stream: true }); },
      err: (b) => { text += dec.decode(b, { stream: true }); },
    };
    const result = await run(WASM_HELLO, { io });
    assert.deepEqual(result, { exitCode: 7 }); // the module's proc_exit(7)
    assert.equal(text, 'hello from wasm\n');
  }
});

test('runWasmModule: the args shim feeds _start the WASI argv', () => {
  assert.deepEqual(runWasmModuleSync(WASM_ARGC, { args: ['prog', 'a', 'b'] }), { exitCode: 3 });
  assert.deepEqual(runWasmModuleSync(WASM_ARGC, { args: ['prog'] }), { exitCode: 1 });
});

test('memory helpers: writeModuleData/readModuleData/readModuleCString at a caller offset', () => {
  const instance = new WebAssembly.Instance(new WebAssembly.Module(WASM_HELLO), {
    wasi_snapshot_preview1: { fd_write: () => 0, proc_exit: () => {} },
  });
  const { ptr, length } = writeModuleData(instance, 'tinyc', { offset: 2048 });
  assert.equal(ptr, 2048);
  assert.equal(length, 5);
  assert.equal(new TextDecoder().decode(readModuleData(instance, ptr, length)), 'tinyc');
  writeModuleData(instance, new Uint8Array([0x68, 0x69, 0x00]), { offset: 4096 });
  assert.equal(readModuleCString(instance, 4096), 'hi');
  // This module exports no malloc, and no offset was given: refuse.
  assert.throws(() => writeModuleData(instance, 'x'), /exported malloc or options\.offset/);
});

// ---------------------------------------------------------------------
// Default libc shims (runtime-shims.mjs)
// ---------------------------------------------------------------------

test('default shims: printf formats into io, memcpy copies, malloc/free reuse the heap', () => {
  const dec = new TextDecoder();
  let text = '';
  const io = {
    out: (b) => { text += dec.decode(b, { stream: true }); },
    err: (b) => { text += dec.decode(b, { stream: true }); },
  };
  const result = runWasmModuleSync(WASM_SHIMTEST, { io });
  assert.deepEqual(result, { exitCode: 0 });
  // "copy=ABCDEFGH" proves memcpy; "reuse=1" proves the block freed
  // by free(buf) was handed out again by the next malloc(32).
  assert.equal(text, 'val=42 str=hi\ncopy=ABCDEFGH reuse=1\n');
});

test('default shims: a same-named options.imports entry replaces the default shim', () => {
  const dec = new TextDecoder();
  let text = '';
  const io = {
    out: (b) => { text += dec.decode(b, { stream: true }); },
    err: () => {},
  };
  const result = runWasmModuleSync(WASM_SHIMTEST, {
    io,
    imports: { env: { printf: () => 0 } },
  });
  assert.deepEqual(result, { exitCode: 0 }); // memcpy/malloc/free still ran
  assert.equal(text, ''); // both printf calls went to the replacement
});

// ---------------------------------------------------------------------
// Custom shim registration and the missing-import diagnostic
// ---------------------------------------------------------------------

test('custom shims: unsatisfiable imports throw TinycMissingImportError naming every missing import', async () => {
  let caught;
  try {
    runWasmModuleSync(WASM_CUSTOM, { entry: 'run' });
  } catch (err) {
    caught = err;
  }
  assert.ok(caught instanceof TinycMissingImportError);
  assert.deepEqual(caught.missing, ['env.my_custom_add', 'env.host_log']);
  assert.match(caught.message, /env\.my_custom_add, env\.host_log/);
  assert.match(caught.message, /registerShim/);
  await assert.rejects(
    runWasmModule(WASM_CUSTOM, { entry: 'run' }),
    (err) => err instanceof TinycMissingImportError
  );
});

test('custom shims: instance-level registration serves state.run; ctx reads module memory', async () => {
  const tinyc = await loadTinyc({ wasmExports: makeMock() });
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_CUSTOM);
  const seen = [];
  const logs = [];
  tinyc.registerShim('my_custom_add', (ctx, a, b) => { seen.push([a, b]); return a + b; });
  tinyc.registerShims({ host_log: (ctx, ptr) => { logs.push(ctx.readCString(ptr)); } });
  const state = tinyc.createState();
  const rc = state.run([], { entry: 'run' });
  assert.equal(rc, 42);
  assert.deepEqual(seen, [[19, 23]]); // the C arguments arrived as numbers
  assert.deepEqual(logs, ['custom says hi']); // read from the module's memory via ctx
  state.delete();
});

test('custom shims: ctx.writeCString writes a NUL-terminated string read back via ctx.readCString', async () => {
  const tinyc = await loadTinyc({ wasmExports: makeMock() });
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_WRITE);
  const logs = [];
  let written = -1;
  tinyc.registerShim('host_fill', (ctx, ptr) => {
    written = ctx.writeCString(ptr, 'filled by shim');
    return written;
  });
  tinyc.registerShim('host_log', (ctx, ptr) => { logs.push(ctx.readCString(ptr)); });
  const state = tinyc.createState();
  const rc = state.run([], { entry: 'run' });
  assert.equal(rc, 15); // 'filled by shim' is 14 chars + the NUL terminator
  assert.equal(written, 15);
  assert.deepEqual(logs, ['filled by shim']); // the module's buffer now holds the string
  state.delete();
});

test('custom shims: per-run shims override instance registrations for that run only', async () => {
  const tinyc = await loadTinyc({ wasmExports: makeMock() });
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_CUSTOM);
  tinyc.registerShim('my_custom_add', (ctx, a, b) => a + b);
  tinyc.registerShim('host_log', () => {});
  const state = tinyc.createState();
  assert.equal(state.run([], { entry: 'run' }), 42);
  assert.equal(state.run([], { entry: 'run', shims: { my_custom_add: () => 7 } }), 7);
  // The per-run override does not disturb the instance registration.
  assert.equal(state.run([], { entry: 'run' }), 42);
  state.delete();
});

test('custom shims: unregisterShim removes an instance registration', async () => {
  const tinyc = await loadTinyc({ wasmExports: makeMock() });
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_CUSTOM);
  tinyc.registerShims({
    my_custom_add: (ctx, a, b) => a + b,
    host_log: () => {},
  });
  const state = tinyc.createState();
  assert.equal(state.run([], { entry: 'run' }), 42);
  tinyc.unregisterShim('my_custom_add');
  assert.throws(
    () => state.run([], { entry: 'run' }),
    (err) => err instanceof TinycMissingImportError &&
      err.missing.includes('env.my_custom_add') &&
      !err.missing.includes('env.host_log')
  );
  state.delete();
});

test('custom shims: callMain -run forwards per-run shims to the execution', async () => {
  const mock = makeMock({
    tinyc_main(argc, argv) {
      mock.calls.push(['tinyc_main', argc, argv]);
      return 0; // compilation succeeds
    },
  });
  const tinyc = await loadTinyc({ wasmExports: mock });
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_CUSTOM_START);
  // Without the shim, the missing-import diagnostic fires.
  assert.throws(
    () => tinyc.callMain(['-run', '/src/p.c']),
    (err) => err instanceof TinycMissingImportError &&
      err.missing.includes('env.my_custom_add')
  );
  const rc = tinyc.callMain(['-run', '/src/p.c'], {
    shims: { my_custom_add: (ctx, a, b) => a + b },
  });
  assert.equal(rc, 42); // proc_exit(my_custom_add(19, 23))
});

test('custom shims: package-global registration serves new instances and bare runWasmModule calls', async () => {
  registerGlobalShims({
    my_custom_add: (ctx, a, b) => a + b,
    host_log: () => {},
  });
  try {
    assert.equal(runWasmModuleSync(WASM_CUSTOM, { entry: 'run' }), 42);
    const tinyc = await loadTinyc({ wasmExports: makeMock() });
    tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_CUSTOM);
    const state = tinyc.createState();
    assert.equal(state.run([], { entry: 'run' }), 42);
    state.delete();
  } finally {
    unregisterGlobalShim('my_custom_add');
    unregisterGlobalShim('host_log');
  }
  // After unregistering, the diagnostic is back.
  assert.throws(
    () => runWasmModuleSync(WASM_CUSTOM, { entry: 'run' }),
    TinycMissingImportError
  );
});

test('worker: TinycWorker.registerShim throws and points at the shimModules option', () => {
  const worker = Object.create(TinycWorker.prototype);
  assert.throws(() => worker.registerShim('x', () => {}), /postMessage/);
  assert.throws(() => worker.registerShim('x', () => {}), /shimModules/);
  assert.throws(() => worker.registerShims({}), /shimModules/);
});

// ---------------------------------------------------------------------
// Compiled products leave through exportFile / exportBlob
// ---------------------------------------------------------------------

test('export: products come out of the virtual FS as bytes and as a Blob', async () => {
  const tinyc = await loadTinyc({ target: 'x86_64-windows', wasmExports: makeMock() });
  const peBytes = new Uint8Array([0x4d, 0x5a, 0x90, 0x00, 0x03, 0x00]); // "MZ" header
  tinyc.addFileSync('/out/prog.exe', peBytes);
  const out = tinyc.exportFile('/out/prog.exe');
  assert.deepEqual(out, peBytes);
  const blob = tinyc.exportBlob('/out/prog.exe');
  assert.equal(blob.size, peBytes.length);
  assert.deepEqual(new Uint8Array(await blob.arrayBuffer()), peBytes);
  assert.throws(() => tinyc.exportFile('/out/missing.o'), /no such file in virtual FS/);
});

// ---------------------------------------------------------------------
// Per-target sysroot manifests
// ---------------------------------------------------------------------

test('sysroot: every supported target has a manifest with its own lib source', () => {
  for (const target of SUPPORTED_TARGETS) {
    const url = new URL(`./sysroot-manifests/${target}.json`, import.meta.url);
    const manifest = JSON.parse(readFileSync(url, 'utf8'));
    assert.equal(manifest.target, target);
    assert.deepEqual(manifest.includeFiles, sysrootManifest.includeFiles);
    assert.equal(manifest.includeDir, sysrootManifest.includeDir);
    assert.equal(manifest.libFiles.length, 1);
    assert.equal(manifest.libFiles[0].source, `lib/${target}/libtcc1.a`);
    assert.equal(manifest.libFiles[0].path, '/sysroot/lib/libtcc1.a');
  }
});

test('sysroot: loadSysrootManifest returns the per-target manifest', async () => {
  const m = await loadSysrootManifest('riscv64-linux-gnu');
  assert.equal(m.target, 'riscv64-linux-gnu');
  assert.equal(m.libFiles[0].source, 'lib/riscv64-linux-gnu/libtcc1.a');
  const d = await loadSysrootManifest('wasm32-wasi');
  assert.equal(d, sysrootManifest);
});

test('sysroot: android and darwin manifests resolve through loadSysrootManifest', async () => {
  const android = await loadSysrootManifest('aarch64-linux-android');
  assert.equal(android.target, 'aarch64-linux-android');
  assert.equal(android.libc, 'bionic');
  assert.equal(android.androidApi, 30);
  assert.equal(android.dynamicLinker, '/system/bin/linker64');
  assert.equal(android.libFiles[0].source, 'lib/aarch64-linux-android/libtcc1.a');
  const androidX64 = await loadSysrootManifest('x86_64-linux-android');
  assert.equal(androidX64.libc, 'bionic');
  assert.equal(androidX64.dynamicLinker, '/system/bin/linker64');
  // Darwin manifests carry TCC's own headers only; system headers
  // come from the user's local SDK, marked in the manifest.
  const darwin = await loadSysrootManifest('arm64-apple-darwin');
  assert.equal(darwin.target, 'arm64-apple-darwin');
  assert.equal(darwin.systemHeaders, 'user-sdk');
  assert.deepEqual(darwin.includeFiles, sysrootManifest.includeFiles);
  const darwinX64 = await loadSysrootManifest('x86_64-apple-darwin');
  assert.equal(darwinX64.systemHeaders, 'user-sdk');
});

test('sysroot: outputs field matches the target format family', () => {
  const ELF = {
    exe: { format: 'ELF executable', ext: '' },
    obj: { format: 'ELF relocatable', ext: '.o' },
    shared: { format: 'ELF shared object', ext: '.so' },
    ar: { format: 'ar archive', ext: '.a' },
  };
  const MACHO = {
    exe: { format: 'Mach-O executable', ext: '' },
    obj: { format: 'Mach-O object', ext: '.o' },
    shared: { format: 'Mach-O dylib', ext: '.dylib' },
    ar: { format: 'ar archive', ext: '.a' },
  };
  const expected = {
    'wasm32-wasi': { module: { format: 'WebAssembly', ext: '.wasm' } },
    'x86_64-linux-gnu': ELF,
    'aarch64-linux-gnu': ELF,
    'riscv64-linux-gnu': ELF,
    'aarch64-linux-android': ELF,
    'x86_64-linux-android': ELF,
    'x86_64-windows': {
      exe: { format: 'PE executable', ext: '.exe' },
      obj: { format: 'COFF object', ext: '.o' },
      shared: { format: 'PE DLL', ext: '.dll' },
      ar: { format: 'ar archive', ext: '.a' },
    },
    'arm64-apple-darwin': MACHO,
    'x86_64-apple-darwin': MACHO,
  };
  for (const target of SUPPORTED_TARGETS) {
    const url = new URL(`./sysroot-manifests/${target}.json`, import.meta.url);
    const manifest = JSON.parse(readFileSync(url, 'utf8'));
    assert.deepEqual(manifest.outputs, expected[target], target);
  }
});

test('sysroot: loading a target sysroot fetches that target\'s lib and registers its include dir', async () => {
  const fetched = [];
  const fetchImpl = async (url) => {
    fetched.push(String(url));
    return {
      ok: true,
      status: 200,
      arrayBuffer: async () => new TextEncoder().encode('asset-bytes').buffer,
    };
  };
  const mock = makeMock();
  const tinyc = await loadTinyc({
    target: 'x86_64-windows',
    wasmExports: mock,
    sysroot: { fetchImpl, includeBinaries: true },
  });
  assert.ok(fetched.some((u) => u.endsWith('/sysroot/include/stddef.h')), fetched.join(','));
  assert.ok(
    fetched.some((u) => u.endsWith('/sysroot/lib/x86_64-windows/libtcc1.a')),
    fetched.join(',')
  );
  assert.ok(tinyc.fs.hasFile('/sysroot/lib/libtcc1.a'));
  assert.ok(tinyc.fs.hasFile('/sysroot/include/stddef.h'));
  const state = tinyc.createState();
  const inc = callsNamed(mock, 'tcc_add_include_path').map((c) => c[2]);
  assert.ok(inc.includes('/sysroot/include'));
  state.delete();
});
