// Glue-logic tests for tinyc-web, run in Node against a mock
// native module that implements the libtcc export names, supplied
// through the internal loadTinycWithNative test seam.
// Run: npm test   (node --test test.mjs)

import { test } from 'node:test';
import assert from 'node:assert/strict';
import { existsSync, readdirSync, readFileSync } from 'node:fs';
import { createHash } from 'node:crypto';

import {
  loadTinyc,
  loadTinycWithNative,
  VirtualFS,
  TinycProcess,
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

/** The asset envelope file's text form, exactly as
 *  build-asset-envelopes.mjs writes it. */
function envelopeText(key, payload) {
  return (
    'globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null);\n' +
    `globalThis.__TINYC_ASSET_REGISTRY__[${JSON.stringify(key)}] = ` +
    `${JSON.stringify(payload)};\n`
  );
}

/** A fetch stub serving one sysroot envelope whose every manifest
 *  entry carries the bytes 'asset-bytes'. Records requested URLs
 *  in `fetched` when given. */
async function sysrootEnvelopeFetch(target, fetched) {
  const manifest = await loadSysrootManifest(target);
  const b64 = Buffer.from('asset-bytes').toString('base64');
  const files = {};
  for (const e of [...(manifest.includeFiles ?? []), ...(manifest.libFiles ?? [])]) {
    files[e.source] = b64;
  }
  return async (url) => {
    if (fetched) fetched.push(String(url));
    return {
      ok: true,
      status: 200,
      text: async () => envelopeText(`sysroot-${target}`, { files }),
    };
  };
}

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

test('vfs: addFile stages string/bytes content synchronously', async () => {
  const fs = new VirtualFS();
  const native = new Map();
  fs.mountTo({
    mkdirTree() {},
    writeFile(p, bytes) { native.set(p, bytes); },
    unlink(p) { if (!native.delete(p)) throw new Error('missing'); },
  });
  // No await before these assertions: for content whose bytes are
  // at hand, the file is staged — index, content cache and the
  // native mirror — in the same turn, so a synchronous compile or
  // read issued right after the call (the way the browser consumer
  // test drives the package) already sees it. Only the store write
  // is still in flight; the returned promise settles it.
  const pending = fs.addFile('/src/sync.c', 'int sync_stage;');
  assert.ok(fs.hasFile('/src/sync.c'));
  assert.equal(fs.readTextFile('/src/sync.c'), 'int sync_stage;');
  assert.ok(native.has('/src/sync.c'));
  await pending;
  const pendingBytes = fs.addFile('/src/sync2.c', new Uint8Array([65]));
  assert.ok(fs.hasFile('/src/sync2.c'));
  assert.ok(native.has('/src/sync2.c'));
  await pendingBytes;
});

test('vfs: removeFile deletes the map entry and the mounted native mirror', async () => {
  const fs = new VirtualFS();
  const native = new Map();
  fs.mountTo({
    mkdirTree() {},
    writeFile(p, bytes) { native.set(p, bytes); },
    unlink(p) { if (!native.delete(p)) throw new Error('missing'); },
  });
  await fs.addFile('/out/prog.wasm', new Uint8Array([1, 2, 3]));
  assert.ok(native.has('/out/prog.wasm')); // write was mirrored
  assert.equal(fs.removeFile('/out/prog.wasm'), true);
  assert.equal(fs.hasFile('/out/prog.wasm'), false);
  assert.equal(native.has('/out/prog.wasm'), false); // mirror unlinked too
  assert.equal(fs.removeFile('/out/prog.wasm'), false); // already gone
  // A file that only exists natively is still unlinked by path.
  native.set('/out/native-only.o', new Uint8Array([9]));
  assert.equal(fs.removeFile('/out/native-only.o'), true);
  assert.equal(native.has('/out/native-only.o'), false);
});

test('vfs: clear removes everything except the sysroot', async () => {
  const fs = new VirtualFS();
  fs.addFileSync('/sysroot/include/stdio.h', '#pragma once');
  fs.addFileSync('/src/main.c', 'int main() {}');
  fs.addFileSync('/out/main.wasm', new Uint8Array([0]));
  assert.equal(fs.clear(), 2);
  assert.deepEqual(fs.listFiles(), ['/sysroot/include/stdio.h']);
});

test('vfs: Tinyc instance shortcuts cover write/read/list/remove/clear', async () => {
  const tinyc = await loadTinycWithNative(makeMock());
  await tinyc.addFile('/src/a.c', 'int a;');
  tinyc.addFileSync('/out/a.wasm', new Uint8Array([7]));
  assert.equal(tinyc.readTextFile('/src/a.c'), 'int a;');
  assert.deepEqual(tinyc.readFile('/out/a.wasm'), new Uint8Array([7]));
  assert.deepEqual(tinyc.listFiles('/out'), ['/out/a.wasm']);
  assert.equal(tinyc.removeFile('/out/a.wasm'), true);
  assert.equal(tinyc.hasFile('/out/a.wasm'), false);
  assert.equal(tinyc.clearFiles(), 1);
  assert.deepEqual(tinyc.listFiles(), []);
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
// Virtual FS: persistent store (vfs-store.mjs)
//
// Fake store with IndexedDB semantics, no third-party fake: records
// live in a backing Map shared by every store instance over it (the
// "database"), open() rebuilds the index from the records, and puts
// and deletes land asynchronously through a timer queue — so a
// synchronous write is only durable once the queue has been awaited
// (fs/instance whenIdle()).
// ---------------------------------------------------------------------
function makeFakeFsStore(backing = new Map(), { delayMs = 1 } = {}) {
  const store = {
    kind: 'fake',
    index: new Map(),
    async open() {
      store.index = new Map();
      for (const [p, r] of backing) {
        store.index.set(p, { size: r.size, mtime: r.mtime });
      }
      return store.index;
    },
    async get(path) {
      const r = backing.get(path);
      return r ? { bytes: r.bytes, size: r.size, mtime: r.mtime } : null;
    },
    put(path, record) {
      return new Promise((resolve) => setTimeout(() => {
        backing.set(path, { bytes: record.bytes, size: record.size, mtime: record.mtime });
        resolve();
      }, delayMs));
    },
    delete(path) {
      return new Promise((resolve) => setTimeout(() => {
        backing.delete(path);
        resolve();
      }, delayMs));
    },
    async close() {},
  };
  return store;
}

test('vfs store: files persist across instances of the same target', async () => {
  const backing = new Map();
  const first = await loadTinycWithNative(makeMock(), { fsStore: makeFakeFsStore(backing) });
  await first.addFile('/src/keep.c', 'int keep;');
  first.addFileSync('/src/sync.c', 'int sync;');
  // The synchronous write is staged in memory immediately but only
  // queued for the store: the backing record is not there yet.
  assert.equal(backing.has('/src/sync.c'), false);
  await first.whenIdle();
  assert.equal(backing.has('/src/sync.c'), true);

  const second = await loadTinycWithNative(makeMock(), { fsStore: makeFakeFsStore(backing) });
  assert.deepEqual(second.listFiles('/src'), ['/src/keep.c', '/src/sync.c']);
  assert.equal(await second.readTextFileAsync('/src/keep.c'), 'int keep;');
  assert.equal(await second.readTextFileAsync('/src/sync.c'), 'int sync;');
  assert.deepEqual(
    await second.exportFileAsync('/src/keep.c'),
    new TextEncoder().encode('int keep;')
  );
});

test('vfs store: removeFile and clear stay deleted across instances', async () => {
  const backing = new Map();
  const first = await loadTinycWithNative(makeMock(), { fsStore: makeFakeFsStore(backing) });
  await first.addFile('/src/gone.c', 'x');
  await first.addFile('/src/cleared.c', 'y');
  await first.addFile('/sysroot/include/kept.h', 'z');
  assert.equal(first.removeFile('/src/gone.c'), true);
  assert.equal(first.clearFiles(), 1); // only /src/cleared.c; sysroot is kept
  await first.whenIdle();

  const second = await loadTinycWithNative(makeMock(), { fsStore: makeFakeFsStore(backing) });
  assert.deepEqual(second.listFiles(), ['/sysroot/include/kept.h']);
  assert.equal(second.hasFile('/src/gone.c'), false);
  assert.equal(second.hasFile('/src/cleared.c'), false);
});

test('vfs store: a second load of the same target sysroot fetches nothing', async () => {
  const backing = new Map();
  let fetches = 0;
  const serve = await sysrootEnvelopeFetch('x86_64-windows');
  const fetchImpl = async (url) => {
    fetches += 1;
    return serve(url);
  };
  const opts = () => ({
    target: 'x86_64-windows',
    fsStore: makeFakeFsStore(backing),
    sysroot: { fetchImpl, includeBinaries: true },
  });
  const first = await loadTinycWithNative(makeMock(), opts());
  assert.ok(fetches > 0);
  assert.ok(first.fs.hasFile('/sysroot/include/stddef.h'));
  const afterFirst = fetches;

  const second = await loadTinycWithNative(makeMock(), opts());
  assert.equal(fetches, afterFirst); // every entry was a store hit
  assert.ok(second.fs.hasFile('/sysroot/include/stddef.h'));
  assert.ok(second.fs.hasFile('/sysroot/lib/libtcc1.a'));
  delete globalThis.__TINYC_ASSET_REGISTRY__;
});

test('vfs store: sync read of a restored-but-uncached file names the async read', async () => {
  const backing = new Map();
  const first = await loadTinycWithNative(makeMock(), { fsStore: makeFakeFsStore(backing) });
  await first.addFile('/src/cold.c', 'int cold;');

  const second = await loadTinycWithNative(makeMock(), { fsStore: makeFakeFsStore(backing) });
  assert.throws(() => second.readFile('/src/cold.c'), /readFileAsync/);
  assert.throws(() => second.exportFile('/src/cold.c'), /exportFileAsync/);
  assert.equal(await second.readTextFileAsync('/src/cold.c'), 'int cold;');
  // The async read warmed the cache: the sync read now succeeds.
  assert.equal(second.readTextFile('/src/cold.c'), 'int cold;');
});

test('vfs store: storeFactory injection receives the target', async () => {
  const backing = new Map();
  const seen = [];
  const tinyc = await loadTinycWithNative(makeMock(), {
    target: 'aarch64-linux-gnu',
    storeFactory: (target) => {
      seen.push(target);
      return makeFakeFsStore(backing);
    },
  });
  assert.deepEqual(seen, ['aarch64-linux-gnu']);
  await tinyc.addFile('/x.c', 'int x;');
  assert.ok(backing.has('/x.c'));
});

// Minimal hand-rolled IndexedDB stub — just enough of the API for
// IndexedDbFsStore (open with upgrade, transactions, get/put/delete,
// a cursor), so the real IndexedDB code path runs in this suite too.
// Records live in a Map per database name, shared across opens.
function makeIndexedDbStub() {
  const databases = new Map();
  const later = (fn) => queueMicrotask(fn);
  function request(exec, onSettled) {
    const req = { result: undefined, error: null, onsuccess: null, onerror: null };
    later(() => {
      try {
        req.result = exec();
      } catch (err) {
        req.error = err;
      }
      if (req.error) req.onerror?.();
      else req.onsuccess?.();
      onSettled?.();
    });
    return req;
  }
  function makeDb(records) {
    return {
      objectStoreNames: { contains: () => true },
      createObjectStore() {},
      transaction() {
        const tx = { error: null, oncomplete: null, onerror: null, onabort: null };
        const settle = () => later(() => tx.oncomplete?.());
        tx.objectStore = () => ({
          get: (key) => request(() => records.get(key)),
          put: (value, key) => request(() => { records.set(key, value); }, settle),
          delete: (key) => request(() => { records.delete(key); }, settle),
          openCursor: () => {
            const entries = [...records.entries()];
            let i = 0;
            const req = { result: null, error: null, onsuccess: null, onerror: null };
            const step = () => later(() => {
              if (i < entries.length) {
                const [key, value] = entries[i++];
                req.result = { key, value, continue: step };
              } else {
                req.result = null;
              }
              req.onsuccess?.();
            });
            step();
            return req;
          },
        });
        return tx;
      },
      close() {},
    };
  }
  return {
    open(name) {
      if (!databases.has(name)) databases.set(name, new Map());
      const req = { result: null, error: null, onsuccess: null, onerror: null, onupgradeneeded: null };
      later(() => {
        req.result = makeDb(databases.get(name));
        req.onupgradeneeded?.();
        req.onsuccess?.();
      });
      return req;
    },
    _databases: databases,
  };
}

test('vfs store: IndexedDB backend persists per target when the environment has it', async () => {
  const stub = makeIndexedDbStub();
  globalThis.indexedDB = stub;
  try {
    const first = await loadTinycWithNative(makeMock());
    assert.equal(first.fs.storeKind, 'indexeddb');
    assert.ok(stub._databases.has('tinyc-web-fs-wasm32-wasi'));
    await first.addFile('/src/idb.c', 'int idb;');
    await first.addFile('/src/drop.c', 'int drop;');
    assert.equal(first.removeFile('/src/drop.c'), true);
    await first.whenIdle();

    const second = await loadTinycWithNative(makeMock());
    assert.deepEqual(second.listFiles('/src'), ['/src/idb.c']);
    assert.equal(await second.readTextFileAsync('/src/idb.c'), 'int idb;');

    // A different target gets its own database and sees nothing.
    const other = await loadTinycWithNative(makeMock(), { target: 'riscv64-linux-gnu' });
    assert.equal(other.hasFile('/src/idb.c'), false);
    assert.ok(stub._databases.has('tinyc-web-fs-riscv64-linux-gnu'));
  } finally {
    delete globalThis.indexedDB;
  }
});

test('vfs store: without IndexedDB the default backend is memory and works', async () => {
  // This suite runs in Node, which has no IndexedDB, so loadTinyc
  // falls back to the in-memory backend.
  const tinyc = await loadTinycWithNative(makeMock());
  assert.equal(tinyc.fs.storeKind, 'memory');
  await tinyc.addFile('/src/m.c', 'int m;');
  tinyc.addFileSync('/src/n.c', 'int n;');
  await tinyc.whenIdle();
  assert.equal(tinyc.readTextFile('/src/m.c'), 'int m;');
  assert.equal(tinyc.readTextFile('/src/n.c'), 'int n;');
});

// ---------------------------------------------------------------------
// Module FS backend: the WasmFS OPFS mount attempt (fsBackend)
// ---------------------------------------------------------------------

// Run `fn` with globals that make the environment look like a Web
// Worker (the OPFS mount attempt is gated on a worker context).
async function withWorkerGlobals(fn) {
  class FakeWorkerGlobalScope {}
  const hadWGS = 'WorkerGlobalScope' in globalThis;
  const hadSelf = 'self' in globalThis;
  const savedWGS = globalThis.WorkerGlobalScope;
  const savedSelf = globalThis.self;
  globalThis.WorkerGlobalScope = FakeWorkerGlobalScope;
  globalThis.self = Object.create(FakeWorkerGlobalScope.prototype);
  try {
    return await fn();
  } finally {
    if (hadWGS) globalThis.WorkerGlobalScope = savedWGS;
    else delete globalThis.WorkerGlobalScope;
    if (hadSelf) globalThis.self = savedSelf;
    else delete globalThis.self;
  }
}

// An Emscripten-shaped native module: ccall present (so the bridge
// is in emscripten mode) plus a recording fake FS.
function makeFakeEmscripten(overrides = {}) {
  const nativeFiles = new Map();
  const native = {
    nativeFiles,
    FS: {
      mkdirTree() {},
      writeFile(p, bytes) { nativeFiles.set(p, bytes); },
      readFile(p) {
        if (!nativeFiles.has(p)) throw new Error(`no such file: ${p}`);
        return nativeFiles.get(p);
      },
      unlink(p) { nativeFiles.delete(p); },
    },
    ccall() { throw new Error('ccall must not be reached'); },
  };
  return Object.assign(native, overrides);
}

test('fsBackend: mock instances report memory', async () => {
  const tinyc = await loadTinycWithNative(makeMock());
  assert.equal(tinyc.fsBackend, 'memory');
});

test('fsBackend: emscripten module without the wasmfs exports falls back, mirroring intact', async () => {
  const fake = makeFakeEmscripten();
  const tinyc = await loadTinycWithNative(fake);
  assert.equal(tinyc.nativeMode, 'emscripten');
  assert.equal(tinyc.fsBackend, 'memory');
  // The failed attempt does not disturb the FS mirroring.
  tinyc.addFileSync('/src/main.c', 'int main() {}');
  assert.ok(fake.nativeFiles.has('/src/main.c'));
});

test('fsBackend: a worker-context module with working wasmfs exports mounts OPFS once', async () => {
  await withWorkerGlobals(async () => {
    const ccalls = [];
    const fake = makeFakeEmscripten({
      _wasmfs_create_opfs_backend() {},
      _wasmfs_create_directory() {},
      async ccall(name, _returnType, _argTypes, args) {
        ccalls.push([name, args]);
        return name === 'wasmfs_create_opfs_backend' ? 4242 : 0;
      },
    });
    const tinyc = await loadTinycWithNative(fake);
    assert.equal(tinyc.fsBackend, 'opfs');
    assert.deepEqual(
      ccalls.map((c) => c[0]),
      ['wasmfs_create_opfs_backend', 'wasmfs_create_directory']
    );
    assert.deepEqual(ccalls[1][1], ['/opfs', 0o777, 4242]);
    // A second load of the same module reuses the outcome and does
    // not mount again.
    const again = await loadTinycWithNative(fake);
    assert.equal(again.fsBackend, 'opfs');
    assert.equal(ccalls.length, 2);
  });
});

test('fsBackend: a failing OPFS mount attempt falls back to memory without throwing', async () => {
  await withWorkerGlobals(async () => {
    const throwing = makeFakeEmscripten({
      _wasmfs_create_opfs_backend() {},
      _wasmfs_create_directory() {},
      async ccall(name) {
        if (name === 'wasmfs_create_opfs_backend') throw new Error('no OPFS');
        return 0;
      },
    });
    const tinyc = await loadTinycWithNative(throwing);
    assert.equal(tinyc.fsBackend, 'memory');

    const denied = makeFakeEmscripten({
      _wasmfs_create_opfs_backend() {},
      _wasmfs_create_directory() {},
      async ccall(name) {
        return name === 'wasmfs_create_opfs_backend' ? 7 : -13;
      },
    });
    const tinyc2 = await loadTinycWithNative(denied);
    assert.equal(tinyc2.fsBackend, 'memory');
  });
});

test('fsBackend: a backend that traps on first use falls back to memory', async () => {
  await withWorkerGlobals(async () => {
    // The mount itself succeeds; the backend's operations throw on
    // first use, like the WasmFS OPFS backend of a build whose
    // async operations have nothing to suspend into. The probe in
    // the mount attempt must absorb that and leave the instance on
    // the memory backend, with mirroring rooted at the module FS
    // root exactly as if no mount had happened.
    const fake = makeFakeEmscripten({
      _wasmfs_create_opfs_backend() {},
      _wasmfs_create_directory() {},
      async ccall(name) {
        return name === 'wasmfs_create_opfs_backend' ? 4242 : 0;
      },
    });
    const serve = fake.FS.writeFile;
    fake.FS.writeFile = (p, bytes) => {
      if (p.startsWith('/opfs/')) throw new Error('RuntimeError: unreachable');
      serve(p, bytes);
    };
    fake.FS.readFile = (p) => {
      if (p.startsWith('/opfs/')) throw new Error('RuntimeError: unreachable');
      if (!fake.nativeFiles.has(p)) throw new Error(`no such file: ${p}`);
      return fake.nativeFiles.get(p);
    };
    const tinyc = await loadTinycWithNative(fake);
    assert.equal(tinyc.fsBackend, 'memory');
    tinyc.addFileSync('/src/main.c', 'int main() {}');
    assert.ok(fake.nativeFiles.has('/src/main.c'));
    assert.ok(![...fake.nativeFiles.keys()].some((p) => p.startsWith('/opfs/')));
  });
});

// ---------------------------------------------------------------------
// OPFS path translation: with the OPFS backend mounted, every path
// that crosses into the native module is rooted at /opfs
// ---------------------------------------------------------------------

// An Emscripten-shaped module whose WasmFS OPFS mount succeeds:
// the wasmfs exports are present, ccall serves the mount and the
// tcc_* calls, the fake FS records files by their module-side path,
// _tinyc_main decodes argv out of the fake heap, and tcc_output_file
// "writes" the product (native.productBytes) at the path it is
// given, like TCC would.
function makeOpfsEmscripten(overrides = {}) {
  const nativeFiles = new Map();
  const ccalls = [];
  const heap = new Uint8Array(1 << 16);
  let heapTop = 8;
  const native = {
    nativeFiles,
    ccalls,
    mainArgv: null,
    productBytes: new Uint8Array([0xde, 0xad]),
    HEAPU8: heap,
    FS: {
      mkdirTree() {},
      writeFile(p, bytes) { nativeFiles.set(p, bytes); },
      readFile(p) {
        if (!nativeFiles.has(p)) throw new Error(`no such file: ${p}`);
        return nativeFiles.get(p);
      },
      unlink(p) { if (!nativeFiles.delete(p)) throw new Error(`no such file: ${p}`); },
    },
    _malloc(n) { const p = heapTop; heapTop += n; return p; },
    _free() {},
    _tinyc_main(argc, argvPtr) {
      const view = new DataView(heap.buffer);
      const dec = new TextDecoder();
      const argv = [];
      for (let i = 0; i < argc; i++) {
        const ptr = view.getUint32(argvPtr + i * 4, true);
        let end = ptr;
        while (heap[end] !== 0) end++;
        argv.push(dec.decode(heap.subarray(ptr, end)));
      }
      native.mainArgv = argv;
      return 0;
    },
    _wasmfs_create_opfs_backend() {},
    _wasmfs_create_directory() {},
    _tcc_new() {},
    _tcc_delete() {},
    _tcc_set_lib_path() {},
    _tcc_set_options() {},
    _tcc_add_include_path() {},
    _tcc_add_sysinclude_path() {},
    _tcc_add_library_path() {},
    _tcc_add_file() {},
    _tcc_compile_string() {},
    _tcc_set_output_type() {},
    _tcc_output_file() {},
    ccall(name, _returnType, _argTypes, args) {
      ccalls.push([name, args]);
      if (name === 'wasmfs_create_opfs_backend') return 4242;
      if (name === 'wasmfs_create_directory') return 0;
      if (name === 'tcc_new') return 111;
      if (name === 'tcc_output_file') nativeFiles.set(args[1], native.productBytes);
      return 0;
    },
  };
  return Object.assign(native, overrides);
}

test('opfs paths: mirror, state paths, product read-back and unlink all land under /opfs', async () => {
  await withWorkerGlobals(async () => {
    const fake = makeOpfsEmscripten();
    const tinyc = await loadTinycWithNative(fake);
    assert.equal(tinyc.fsBackend, 'opfs');
    const ccallsNamed = (name) => fake.ccalls.filter((c) => c[0] === name);

    // Mirroring writes under the prefix, never at the root path.
    tinyc.addFileSync('/src/main.c', 'int main() {}');
    assert.ok(fake.nativeFiles.has('/opfs/src/main.c'));
    assert.equal(fake.nativeFiles.has('/src/main.c'), false);

    // Registered include dirs reach tcc_add_include_path relocated.
    tinyc.registerIncludeDir('/deps');
    const state = tinyc.createState();
    assert.deepEqual(
      ccallsNamed('tcc_add_include_path').map((c) => c[1]),
      [[111, '/opfs/deps']]
    );

    // The path handed to tcc_add_file is the module-side location.
    state.addFile('/src/main.c');
    assert.deepEqual(
      ccallsNamed('tcc_add_file').map((c) => c[1]),
      [[111, '/opfs/src/main.c']]
    );

    // The other path registrations relocate the same way.
    state.addSysincludePath('/sysinc');
    state.addLibraryPath('/libs');
    state.setLibPath('/sysroot');
    assert.deepEqual(ccallsNamed('tcc_add_sysinclude_path')[0][1], [111, '/opfs/sysinc']);
    assert.deepEqual(ccallsNamed('tcc_add_library_path')[0][1], [111, '/opfs/libs']);
    assert.deepEqual(ccallsNamed('tcc_set_lib_path')[0][1], [111, '/opfs/sysroot']);

    // TCC writes the product under the prefix; exportFile reads it
    // back from there and files it under the canonical path.
    state.outputFile('/out/prog.o');
    assert.deepEqual(ccallsNamed('tcc_output_file')[0][1], [111, '/opfs/out/prog.o']);
    assert.deepEqual(tinyc.exportFile('/out/prog.o'), new Uint8Array([0xde, 0xad]));
    assert.ok(tinyc.hasFile('/out/prog.o'));

    // exportFileAsync takes the same translated native read for a
    // product that exists only inside the module.
    fake.nativeFiles.set('/opfs/out/async.o', new Uint8Array([1, 2]));
    assert.deepEqual(await tinyc.exportFileAsync('/out/async.o'), new Uint8Array([1, 2]));
    assert.ok(tinyc.hasFile('/out/async.o'));

    // removeFile unlinks the prefixed mirror.
    assert.equal(tinyc.removeFile('/src/main.c'), true);
    assert.equal(fake.nativeFiles.has('/opfs/src/main.c'), false);
    state.delete();
  });
});

test('opfs paths: state.run reads the product back from under the prefix', async () => {
  await withWorkerGlobals(async () => {
    const fake = makeOpfsEmscripten({ productBytes: WASM_HELLO });
    const outChunks = [];
    const tinyc = await loadTinycWithNative(fake, {
      io: { out: (t) => outChunks.push(t), err: () => {} },
    });
    assert.equal(tinyc.fsBackend, 'opfs');
    const state = tinyc.createState();
    const rc = state.run([]);
    assert.equal(rc, 7); // the product's proc_exit(7)
    assert.equal(outChunks.join(''), 'hello from wasm\n');
    state.delete();
  });
});

test('opfs paths: callMain argv is relocated under /opfs', async () => {
  await withWorkerGlobals(async () => {
    const fake = makeOpfsEmscripten();
    const tinyc = await loadTinycWithNative(fake);
    const rc = tinyc.callMain(
      ['-c', '/src/main.c', '-o', '/out/main.o', '-I/inc', '-L', '/libs', '-lm']
    );
    assert.equal(rc, 0);
    assert.deepEqual(fake.mainArgv, [
      '/opfs' + VIRTUAL_BIN_PATH,
      '-c', '/opfs/src/main.c',
      '-o', '/opfs/out/main.o',
      '-I/opfs/inc',
      '-L', '/opfs/libs',
      '-lm', // a library name, not a path: untouched
    ]);
    // Joined -o and relative paths relocate through normalization.
    tinyc.callMain(['-oout/rel.o', 'src/rel.c']);
    assert.deepEqual(fake.mainArgv, [
      '/opfs' + VIRTUAL_BIN_PATH,
      '-o/opfs/out/rel.o',
      '/opfs/src/rel.c',
    ]);
  });
});

test('opfs paths: compileFile #line keeps the canonical virtual path', async () => {
  await withWorkerGlobals(async () => {
    const fake = makeOpfsEmscripten();
    const tinyc = await loadTinycWithNative(fake);
    tinyc.addFileSync('/src/a.c', 'int a;\n');
    const state = tinyc.createState();
    state.compileFile('/src/a.c');
    const src = fake.ccalls.filter((c) => c[0] === 'tcc_compile_string')[0][1][1];
    assert.ok(src.startsWith('#line 1 "/src/a.c"\n'));
    state.delete();
  });
});

test('opfs paths: memory backend keeps module-side paths byte-identical', async () => {
  // Same module shape but no worker globals: the OPFS mount is not
  // attempted, the backend stays memory, and nothing is relocated.
  const fake = makeOpfsEmscripten();
  const tinyc = await loadTinycWithNative(fake);
  assert.equal(tinyc.fsBackend, 'memory');
  tinyc.addFileSync('/src/main.c', 'int main() {}');
  assert.ok(fake.nativeFiles.has('/src/main.c'));
  const rc = tinyc.callMain(['-o', 'a.out', 'a.c']);
  assert.equal(rc, 0);
  assert.deepEqual(fake.mainArgv, [VIRTUAL_BIN_PATH, '-o', 'a.out', 'a.c']);
});

// ---------------------------------------------------------------------
// Include dirs and state wiring
// ---------------------------------------------------------------------

test('registerIncludeDir reaches tcc_add_include_path on createState', async () => {
  const mock = makeMock();
  const tinyc = await loadTinycWithNative(mock);
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
  const tinyc = await loadTinycWithNative(mock);
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

/** Load through the envelope path with a stub fetch serving a
 *  compiler envelope whose glue factory records the opts it is
 *  called with and returns `mock`. Resolves to the factory opts. */
async function loadViaEnvelopeCapturingFactory(mock, loadOpts = {}) {
  const glue = `export default function factory(opts) {
    globalThis.__IO_FACTORY_OPTS__ = opts;
    return globalThis.__IO_MOCK__;
  }`;
  globalThis.__IO_MOCK__ = mock;
  const prevFetch = globalThis.fetch;
  globalThis.fetch = async () => ({
    ok: true,
    status: 200,
    text: async () => envelopeText('tcc-riscv64-linux-gnu', {
      glue,
      wasm: Buffer.from([0, 97, 115, 109]).toString('base64'),
    }),
  });
  try {
    await loadTinyc({
      target: 'riscv64-linux-gnu',
      transport: 'fetch',
      fsStore: makeFakeFsStore(new Map()),
      ...loadOpts,
    });
    return globalThis.__IO_FACTORY_OPTS__;
  } finally {
    globalThis.fetch = prevFetch;
    delete globalThis.__IO_MOCK__;
    delete globalThis.__IO_FACTORY_OPTS__;
    clearTargetModuleCache();
  }
}

test('io: Emscripten print/printErr wiring routes to custom io', async () => {
  const mock = makeMock();
  const outChunks = [];
  const errChunks = [];
  const captured = await loadViaEnvelopeCapturingFactory(mock, {
    io: { out: (t) => outChunks.push(t), err: (t) => errChunks.push(t) },
  });
  captured.print('hello');
  captured.printErr('oops');
  assert.deepEqual(outChunks, ['hello\n']);
  assert.deepEqual(errChunks, ['oops\n']);
});

test('io: default wiring lands on console', async () => {
  const mock = makeMock();
  const captured = await loadViaEnvelopeCapturingFactory(mock);
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

/** The envelope content a loadModule hook returns for `target`:
 *  a compiler envelope whose glue factory hands back `mock`,
 *  stashed on globalThis under `stashKey` (the same capture
 *  technique as loadViaEnvelopeCapturingFactory). */
function hookEnvelope(target, mock, stashKey) {
  globalThis[stashKey] = mock;
  return envelopeText(`tcc-${target}`, {
    glue: `export default function factory() { return globalThis[${JSON.stringify(stashKey)}]; }`,
    wasm: Buffer.from([0, 97, 115, 109]).toString('base64'),
  });
}

test('loadTinyc: module-supplying options are not read; the module is built from the envelope the loadModule hook returns', async () => {
  const mock = makeMock();
  const boobyTrap = () => { throw new Error('a module-supplying option was read'); };
  try {
    const tinyc = await loadTinyc({
      loadModule: async (target) => hookEnvelope(target, mock, '__HOOK_MOCK__'),
      createModule: boobyTrap,
      wasmExports: new Proxy({}, { get: boobyTrap }),
      module: new Proxy({}, { get: boobyTrap }),
      wasmUrl: 'https://example.invalid/unused.wasm',
      fsStore: makeFakeFsStore(new Map()),
    });
    assert.equal(tinyc.target, DEFAULT_TARGET);
    const state = tinyc.createState();
    state.compileString('int x;');
    assert.equal(callsNamed(mock, 'tcc_compile_string').length, 1);
    state.delete();
  } finally {
    delete globalThis.__HOOK_MOCK__;
    clearTargetModuleCache();
  }
});

test('loadTinyc: a loadModule hook returning a module object is rejected by name', async () => {
  const mock = makeMock();
  try {
    await assert.rejects(
      loadTinyc({
        loadModule: async () => mock,
        fsStore: makeFakeFsStore(new Map()),
      }),
      /loadModule hook .* must return the asset envelope's content/);
    clearTargetModuleCache();
    // Content that is not an envelope is a named parse failure,
    // never a fallback to some other load path.
    await assert.rejects(
      loadTinyc({
        loadModule: async () => 'this is not an envelope',
        fsStore: makeFakeFsStore(new Map()),
      }),
      /asset envelope .* is not in the envelope form/);
  } finally {
    clearTargetModuleCache();
  }
});

test('loadTinyc: the loadModule hook may return the envelope as bytes', async () => {
  const mock = makeMock();
  const text = hookEnvelope(DEFAULT_TARGET, mock, '__HOOK_MOCK__');
  try {
    for (const content of [
      new TextEncoder().encode(text),
      new TextEncoder().encode(text).buffer,
    ]) {
      clearTargetModuleCache();
      const tinyc = await loadTinyc({
        loadModule: async () => content,
        fsStore: makeFakeFsStore(new Map()),
      });
      assert.equal(tinyc.target, DEFAULT_TARGET);
    }
    clearTargetModuleCache();
    const tinyc = await loadTinyc({
      loadModule: async () => new TextEncoder().encode(text),
      fsStore: makeFakeFsStore(new Map()),
    });
    const state = tinyc.createState();
    state.compileString('int x;');
    assert.equal(callsNamed(mock, 'tcc_compile_string').length, 1);
    state.delete();
  } finally {
    delete globalThis.__HOOK_MOCK__;
    clearTargetModuleCache();
  }
});

test('io: state error callback routes to err io', async () => {
  const mock = makeMock();
  const errChunks = [];
  const tinyc = await loadTinycWithNative(mock, {
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
  const tinyc = await loadTinycWithNative(mock);
  const rc = tinyc.callMain(['-o', 'a.out', 'a.c']);
  assert.equal(rc, 7); // tinyc_main's return, not main's
  assert.equal(callsNamed(mock, 'main').length, 0);
  const entry = callsNamed(mock, 'tinyc_main')[0];
  assert.equal(entry[1], 4); // argc includes argv[0]
  assert.deepEqual(entry[2], [VIRTUAL_BIN_PATH, '-o', 'a.out', 'a.c']);
});

test('callMain: falls back to main when tinyc_main is absent', async () => {
  const mock = makeMock({ tinyc_main: undefined });
  const tinyc = await loadTinycWithNative(mock);
  const rc = tinyc.callMain(['-v']);
  assert.equal(rc, 3);
  const entry = callsNamed(mock, 'main')[0];
  assert.deepEqual(entry[2], [VIRTUAL_BIN_PATH, '-v']);
});

test('callMain: missing main export throws a named error', async () => {
  const mock = makeMock({ tinyc_main: undefined, main: undefined });
  const tinyc = await loadTinycWithNative(mock);
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
  const tinyc = await loadTinycWithNative(mock);
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
  // Manifest sources resolve through the envelope lookup order:
  // wasm-backend/include first, vendor-tinycc/include second, and
  // tcclib.h is vendor-tinycc/tcclib.h.  Every listed source must
  // name a real file under that order.
  const lookupDirs = [
    new URL('../../wasm-backend/include/', import.meta.url),
    new URL('../../vendor-tinycc/include/', import.meta.url),
  ];
  for (const entry of sysrootManifest.includeFiles) {
    if (entry.source === 'tcclib.h') {
      assert.ok(
        existsSync(new URL('../../vendor-tinycc/tcclib.h', import.meta.url)),
        entry.source);
      continue;
    }
    assert.ok(entry.source.startsWith('include/'), entry.source);
    const name = entry.source.slice('include/'.length);
    assert.ok(
      lookupDirs.some((dir) => existsSync(new URL(name, dir))),
      `${entry.source} does not resolve in the lookup order`);
  }
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
    envelopeFile: 'tcc-aarch64-linux-gnu.tinyc.js',
  });
});

test('targets: unknown target is rejected with the supported list', async () => {
  await assert.rejects(
    loadTinyc({ target: 'sparc-unknown' }),
    (err) => /unknown target "sparc-unknown"/.test(err.message) &&
      err.message.includes('wasm32-wasi')
  );
});

test('targets: two instances of different targets coexist, virtual FS isolated', async () => {
  const mockA = makeMock();
  const mockB = makeMock();
  const a = await loadTinycWithNative(mockA, { target: 'x86_64-linux-gnu' });
  const b = await loadTinycWithNative(mockB, { target: 'wasm32-wasi' });
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
    loads.push([target, files.envelopeFile]);
    return hookEnvelope(target, makeMock(), `__LAZY_MOCK_${target}`);
  };
  const one = await loadTinyc({ target: 'x86_64-windows', loadModule });
  assert.equal(one.target, 'x86_64-windows');
  assert.deepEqual(loads, [[
    'x86_64-windows', 'tcc-x86_64-windows.tinyc.js',
  ]]);
  // Same target again: served from the per-target cache, no reload.
  await loadTinyc({ target: 'x86_64-windows', loadModule });
  assert.equal(loads.length, 1);
  // A different target loads only its own module file.
  await loadTinyc({ target: 'riscv64-linux-gnu', loadModule });
  assert.deepEqual(loads.map((l) => l[0]), ['x86_64-windows', 'riscv64-linux-gnu']);
  assert.deepEqual(getLoadedTargets().sort(), ['riscv64-linux-gnu', 'x86_64-windows']);
  delete globalThis['__LAZY_MOCK_x86_64-windows'];
  delete globalThis['__LAZY_MOCK_riscv64-linux-gnu'];
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
  const tinyc = await loadTinycWithNative(mock, { target: 'aarch64-linux-gnu' });
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
  const tinyc = await loadTinycWithNative(mock, { target: 'arm64-apple-darwin' });
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
  const tinyc = await loadTinycWithNative(mock, { target: 'riscv64-linux-gnu' });
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
  const tinyc = await loadTinycWithNative(mock, {
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
  const tinyc = await loadTinycWithNative(mock, {
    io: { out: (t) => outChunks.push(t), err: () => {} },
  });
  tinyc.addFileSync(DEFAULT_RUN_OUTPUT, WASM_HELLO);
  const rc = tinyc.callMain(['-run', '/src/prog.c', 'x']);
  assert.equal(rc, 7); // the product's proc_exit(7)
  const entries = callsNamed(mock, 'tinyc_main');
  assert.equal(entries.length, 1); // exactly the compile invocation
  assert.deepEqual(entries[0][2], [VIRTUAL_BIN_PATH, '-wasm-dylink=main', '-o', DEFAULT_RUN_OUTPUT, '/src/prog.c']);
  assert.ok(!entries[0][2].includes('-run'));
  assert.equal(outChunks.join(''), 'hello from wasm\n');
});

test('runWasmFile: executes a product stored in the virtual FS without recompiling', async () => {
  const mock = makeMock({});
  const outChunks = [];
  const tinyc = await loadTinycWithNative(mock, {
    io: { out: (t) => outChunks.push(t), err: () => {} },
  });
  tinyc.addFileSync('/out/prog.wasm', WASM_HELLO);
  const rc = tinyc.runWasmFile('/out/prog.wasm');
  assert.equal(rc, 7); // the product's proc_exit(7)
  assert.equal(outChunks.join(''), 'hello from wasm\n');
  assert.equal(callsNamed(mock, 'tinyc_main').length, 0); // nothing recompiled
});

test('runWasmFile: function-style entry and instance shims apply, same stack as -run', async () => {
  const mock = makeMock({});
  const tinyc = await loadTinycWithNative(mock);
  tinyc.addFileSync('/out/add.wasm', WASM_ADD);
  assert.equal(tinyc.runWasmFile('/out/add.wasm', { entry: 'add', callArgs: [19, 23] }), 42);
  // WASM_CUSTOM imports env.my_custom_add and env.host_log; the
  // instance-level shims must satisfy them here exactly as they do
  // on the state.run / "-run" paths.
  tinyc.addFileSync('/out/custom.wasm', WASM_CUSTOM);
  const seen = [];
  tinyc.registerShim('my_custom_add', (ctx, a, b) => { seen.push([a, b]); return a + b; });
  tinyc.registerShims({ host_log: (ctx, ptr) => { ctx.readCString(ptr); } });
  const rc = tinyc.runWasmFile('/out/custom.wasm', { entry: 'run' });
  assert.deepEqual(seen, [[19, 23]]);
  assert.equal(rc, 42);
});

test('runWasmFile: unsatisfiable imports throw TinycMissingImportError naming every missing import', async () => {
  const tinyc = await loadTinycWithNative(makeMock());
  tinyc.addFileSync('/out/custom.wasm', WASM_CUSTOM);
  let caught;
  try {
    tinyc.runWasmFile('/out/custom.wasm', { entry: 'run' });
  } catch (err) {
    caught = err;
  }
  assert.ok(caught instanceof TinycMissingImportError);
  assert.deepEqual(caught.missing, ['env.my_custom_add', 'env.host_log']);
  assert.match(caught.message, /env\.my_custom_add, env\.host_log/);
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
  const tinyc = await loadTinycWithNative(makeMock());
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
  const tinyc = await loadTinycWithNative(makeMock());
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
  const tinyc = await loadTinycWithNative(makeMock());
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
  const tinyc = await loadTinycWithNative(makeMock());
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
  const tinyc = await loadTinycWithNative(mock);
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
    const tinyc = await loadTinycWithNative(makeMock());
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
  const tinyc = await loadTinycWithNative(makeMock(), { target: 'x86_64-windows' });
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

// The wasm32-wasi sysroot additionally carries the headers whose
// content serves the wasm backend alone: the four SIMD
// compatibility headers (their intrinsics map onto wasm SIMD128
// builtins) and dlfcn.h / pthread.h, whose own text restricts them
// to the wasm target (the process loader and scheduler are host
// shims there; other targets take these headers from their own
// SDK). stdio.h, string.h, stdlib.h and ctype.h are in this set
// for the same reason: their text restricts them to the wasm
// target, and the surface they declare is the host shim layer's
// supply — the standard streams as fixed handle values, files
// over the process's file space, no errno surface — which only
// the wasm32 target runs against; the other targets take these
// headers from their own SDK. Every other target ships the
// shared freestanding set.
const WASM_ONLY_INCLUDE_PATHS = new Set([
  '/sysroot/include/arm_neon.h',
  '/sysroot/include/ctype.h',
  '/sysroot/include/dlfcn.h',
  '/sysroot/include/emmintrin.h',
  '/sysroot/include/pthread.h',
  '/sysroot/include/stdio.h',
  '/sysroot/include/stdlib.h',
  '/sysroot/include/string.h',
  '/sysroot/include/wasm_simd128.h',
  '/sysroot/include/xmmintrin.h',
]);
const sharedIncludeFiles = sysrootManifest.includeFiles.filter(
  (e) => !WASM_ONLY_INCLUDE_PATHS.has(e.path));

// math.h, tgmath.h and tcclib.h are not part of the x86_64-windows
// sysroot: the facilities they declare beyond the C89 core (the
// C99 math set, the long double forms, snprintf, the dl* quartet)
// are not exports of msvcrt.dll, the runtime a TCC PE link binds,
// so the headers cannot close there.  Every other target's runtime
// (glibc, bionic, libSystem, the wasm shims) exports the full set.
const WINDOWS_DROPPED_INCLUDE_PATHS = new Set([
  '/sysroot/include/math.h',
  '/sysroot/include/tgmath.h',
  '/sysroot/include/tcclib.h',
]);
const windowsIncludeFiles = sharedIncludeFiles.filter(
  (e) => !WINDOWS_DROPPED_INCLUDE_PATHS.has(e.path));

// The link inputs each target's sysroot carries next to
// libtcc1.a, in manifest order: crt objects and stub libraries
// for the ELF targets, the import definition files for Windows,
// the libSystem text stub for Darwin.  wasm32-wasi carries none:
// its runtime is the shim layer.
const EXPECTED_LIB_SOURCES = {
  'wasm32-wasi': [],
  'x86_64-linux-gnu': ['crt1.o', 'crti.o', 'crtn.o', 'libc.so', 'libm.so'],
  'aarch64-linux-gnu': ['crt1.o', 'crti.o', 'crtn.o', 'libc.so', 'libm.so'],
  'riscv64-linux-gnu': ['crt1.o', 'crti.o', 'crtn.o', 'libc.so', 'libm.so'],
  'aarch64-linux-android': ['crt1.o', 'crti.o', 'crtn.o', 'libc.so', 'libm.so', 'libdl.so'],
  'x86_64-linux-android': ['crt1.o', 'crti.o', 'crtn.o', 'libc.so', 'libm.so', 'libdl.so'],
  'x86_64-windows': ['msvcrt.def', 'kernel32.def'],
  'arm64-apple-darwin': ['libc.tbd'],
  'x86_64-apple-darwin': ['libc.tbd'],
};

test('sysroot: every supported target has a manifest with its own lib source', () => {
  for (const target of SUPPORTED_TARGETS) {
    const url = new URL(`./sysroot-manifests/${target}.json`, import.meta.url);
    const manifest = JSON.parse(readFileSync(url, 'utf8'));
    assert.equal(manifest.target, target);
    assert.deepEqual(manifest.includeFiles,
      target === 'wasm32-wasi' ? sysrootManifest.includeFiles
        : target === 'x86_64-windows' ? windowsIncludeFiles
        : sharedIncludeFiles);
    assert.equal(manifest.includeDir, sysrootManifest.includeDir);
    assert.deepEqual(manifest.libFiles.map((e) => e.source), [
      `lib/${target}/libtcc1.a`,
      ...EXPECTED_LIB_SOURCES[target].map((n) => `lib/${target}/${n}`),
    ]);
    assert.equal(manifest.libFiles[0].path, '/sysroot/lib/libtcc1.a');
    for (const e of manifest.libFiles) {
      assert.equal(e.path, `/sysroot/lib/${e.source.split('/').pop()}`);
      assert.equal(e.binary, true);
    }
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
  assert.deepEqual(darwin.includeFiles, sharedIncludeFiles);
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

test('sysroot: loading a target sysroot retrieves its envelope and registers its include dir', async () => {
  const fetched = [];
  const fetchImpl = await sysrootEnvelopeFetch('x86_64-windows', fetched);
  const mock = makeMock();
  const tinyc = await loadTinycWithNative(mock, {
    target: 'x86_64-windows',
    sysroot: { fetchImpl, includeBinaries: true },
  });
  // The whole sysroot arrives as one envelope file, retrieved once.
  assert.deepEqual(fetched.length, 1);
  assert.ok(fetched[0].endsWith('/sysroot/sysroot-x86_64-windows.tinyc.js'),
    fetched.join(','));
  assert.ok(tinyc.fs.hasFile('/sysroot/lib/libtcc1.a'));
  assert.ok(tinyc.fs.hasFile('/sysroot/include/stddef.h'));
  const state = tinyc.createState();
  const inc = callsNamed(mock, 'tcc_add_include_path').map((c) => c[2]);
  assert.ok(inc.includes('/sysroot/include'));
  state.delete();
  delete globalThis.__TINYC_ASSET_REGISTRY__;
});

// ---------------------------------------------------------------------
// Compiler asset envelopes (design §14): the compiler module and
// the sysroot ship only as envelope files that register their
// payload in __TINYC_ASSET_REGISTRY__. The script transport injects
// them; the fetch transport reads the same files as text and parses
// the payload out without executing it.
// ---------------------------------------------------------------------

/** A document stub whose script elements "load" by running the
 *  given register callback (the envelope file's side effect) and
 *  then firing onload. */
function stubScriptDocument(register) {
  const doc = {
    createElement: () => ({}),
    head: {
      appendChild: (el) => {
        register(el.src);
        queueMicrotask(() => el.onload && el.onload());
        el.remove = () => {};
      },
    },
  };
  return doc;
}

test('assets: sysroot loads from its envelope under the script transport', async () => {
  const headerBytes = new TextEncoder().encode('#define FROM_ENVELOPE 1\n');
  const b64 = Buffer.from(headerBytes).toString('base64');
  const prevDocument = globalThis.document;
  globalThis.document = stubScriptDocument(() => {
    const registry = (globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null));
    registry['sysroot-wasm32-wasi'] = { files: { 'include/x.h': b64 } };
  });
  try {
    const tinyc = await loadTinycWithNative(makeMock(), {
      fsStore: makeFakeFsStore(new Map()),
      sysroot: {
        transport: 'script',
        manifest: {
          includeDir: '/sysroot/include',
          includeFiles: [{ path: '/sysroot/include/x.h', source: 'include/x.h' }],
          libFiles: [],
        },
      },
    });
    assert.equal(
      tinyc.fs.readTextFile('/sysroot/include/x.h'),
      '#define FROM_ENVELOPE 1\n');
  } finally {
    globalThis.document = prevDocument;
    delete globalThis.__TINYC_ASSET_REGISTRY__;
  }
});

test('assets: a sysroot file missing from the envelope is a named load failure', async () => {
  const prevDocument = globalThis.document;
  globalThis.document = stubScriptDocument(() => {
    const registry = (globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null));
    registry['sysroot-wasm32-wasi'] = { files: {} };
  });
  try {
    await assert.rejects(
      loadTinycWithNative(makeMock(), {
        fsStore: makeFakeFsStore(new Map()),
        sysroot: {
          transport: 'script',
          manifest: {
            includeDir: '/sysroot/include',
            includeFiles: [{ path: '/sysroot/include/x.h', source: 'include/x.h' }],
            libFiles: [],
          },
        },
      }),
      /sysroot file "include\/x\.h" is missing from the asset envelope/);
  } finally {
    globalThis.document = prevDocument;
    delete globalThis.__TINYC_ASSET_REGISTRY__;
  }
});

test('assets: the compiler module loads from its envelope under the script transport', async () => {
  const mock = makeMock();
  let factoryOpts = null;
  const glue = `export default function factory(opts) {
    globalThis.__ENVELOPE_FACTORY_OPTS__ = opts;
    return globalThis.__ENVELOPE_MOCK__;
  }`;
  globalThis.__ENVELOPE_MOCK__ = mock;
  const prevDocument = globalThis.document;
  globalThis.document = stubScriptDocument(() => {
    const registry = (globalThis.__TINYC_ASSET_REGISTRY__ ??= Object.create(null));
    registry['tcc-riscv64-linux-gnu'] = {
      glue,
      wasm: Buffer.from([0, 97, 115, 109]).toString('base64'),
    };
  });
  try {
    const tinyc = await loadTinyc({
      target: 'riscv64-linux-gnu',
      transport: 'script',
      fsStore: makeFakeFsStore(new Map()),
    });
    assert.equal(tinyc.target, 'riscv64-linux-gnu');
    factoryOpts = globalThis.__ENVELOPE_FACTORY_OPTS__;
    assert.equal(typeof factoryOpts.instantiateWasm, 'function');
    assert.equal(typeof factoryOpts.print, 'function');
  } finally {
    globalThis.document = prevDocument;
    delete globalThis.__TINYC_ASSET_REGISTRY__;
    delete globalThis.__ENVELOPE_MOCK__;
    delete globalThis.__ENVELOPE_FACTORY_OPTS__;
    clearTargetModuleCache();
  }
});

test('assets: sysroot loads from its envelope under the fetch transport', async () => {
  const headerBytes = new TextEncoder().encode('#define FROM_ENVELOPE 1\n');
  const b64 = Buffer.from(headerBytes).toString('base64');
  const fetched = [];
  const fetchImpl = async (url) => {
    fetched.push(String(url));
    return {
      ok: true,
      status: 200,
      text: async () => envelopeText(
        'sysroot-wasm32-wasi', { files: { 'include/x.h': b64 } }),
    };
  };
  try {
    const tinyc = await loadTinycWithNative(makeMock(), {
      fsStore: makeFakeFsStore(new Map()),
      sysroot: {
        fetchImpl,
        manifest: {
          includeDir: '/sysroot/include',
          includeFiles: [{ path: '/sysroot/include/x.h', source: 'include/x.h' }],
          libFiles: [],
        },
      },
    });
    assert.equal(
      tinyc.fs.readTextFile('/sysroot/include/x.h'),
      '#define FROM_ENVELOPE 1\n');
    // One request, for the envelope itself — never one per file.
    assert.equal(fetched.length, 1);
    assert.ok(fetched[0].endsWith('sysroot/sysroot-wasm32-wasi.tinyc.js'),
      fetched[0]);
  } finally {
    delete globalThis.__TINYC_ASSET_REGISTRY__;
  }
});

test('assets: the compiler module loads from its envelope under the fetch transport', async () => {
  const mock = makeMock();
  const glue = `export default function factory(opts) {
    globalThis.__ENVELOPE_FACTORY_OPTS__ = opts;
    return globalThis.__ENVELOPE_MOCK__;
  }`;
  globalThis.__ENVELOPE_MOCK__ = mock;
  const fetched = [];
  const prevFetch = globalThis.fetch;
  globalThis.fetch = async (url) => {
    fetched.push(String(url));
    return {
      ok: true,
      status: 200,
      text: async () => envelopeText('tcc-riscv64-linux-gnu', {
        glue,
        wasm: Buffer.from([0, 97, 115, 109]).toString('base64'),
      }),
    };
  };
  try {
    const tinyc = await loadTinyc({
      target: 'riscv64-linux-gnu',
      transport: 'fetch',
      fsStore: makeFakeFsStore(new Map()),
    });
    assert.equal(tinyc.target, 'riscv64-linux-gnu');
    const factoryOpts = globalThis.__ENVELOPE_FACTORY_OPTS__;
    assert.equal(typeof factoryOpts.instantiateWasm, 'function');
    assert.equal(typeof factoryOpts.print, 'function');
    assert.equal(fetched.length, 1);
    assert.ok(fetched[0].endsWith('tcc-riscv64-linux-gnu.tinyc.js'),
      fetched[0]);
  } finally {
    globalThis.fetch = prevFetch;
    delete globalThis.__TINYC_ASSET_REGISTRY__;
    delete globalThis.__ENVELOPE_MOCK__;
    delete globalThis.__ENVELOPE_FACTORY_OPTS__;
    clearTargetModuleCache();
  }
});

// ---------------------------------------------------------------------
// Dylink loader: memory cache for units whose write-back failed
//
// The loader's resolution is TinycProcess._prefetchClosure (the
// first step of loadModule): local sources, then retrieval, then
// a best-effort write-back into the attached instance's virtual
// FS. The harness attaches a fake instance whose FS is a Map with
// a switchable write failure, and stubs the global fetch the
// transport reads, counting every retrieval.
// ---------------------------------------------------------------------

/** A dylink unit manifest over arbitrary body bytes, in the form
 *  the loader validates: version 1, base64 body, its SHA-256. */
function unitManifest(bodyBytes) {
  return {
    version: 1,
    flavor: 0,
    body: Buffer.from(bodyBytes).toString('base64'),
    hash: createHash('sha256').update(bodyBytes).digest('hex'),
    mem_size: 1024,
    mem_align: 16,
    bss_off: 1024,
    bss_size: 0,
    table_size: 0,
    needed: [],
    exports: [],
  };
}

/** The unit file's envelope text: the form the write-back stores
 *  and the envelope parser reads back. */
function unitEnvelopeText(canonical, manifest) {
  return (
    'globalThis.__TINYC_DYLINK_REGISTRY__ ??= Object.create(null);\n' +
    `globalThis.__TINYC_DYLINK_REGISTRY__[${JSON.stringify(canonical)}] = ` +
    `${JSON.stringify(manifest)};\n`
  );
}

function makeLoaderHarness(canonical, bodyBytes) {
  const files = new Map();
  const fs = {
    failWrites: false,
    hasFile: (p) => files.has(p),
    readFile: (p) => files.get(p) ?? null,
    addFileSync: (p, content) => {
      if (fs.failWrites) throw new Error('tinyc test: store write failed');
      files.set(p, typeof content === 'string'
        ? new TextEncoder().encode(content) : content);
      return p;
    },
    removeFile: (p) => files.delete(p),
  };
  const fetched = [];
  const envelope = unitEnvelopeText(canonical, unitManifest(bodyBytes));
  const fetchStub = async (url) => {
    fetched.push(String(url));
    return { ok: true, status: 200, text: async () => envelope };
  };
  const tinyc = { fs }; // one attached instance shared by every process
  const newProcess = () => new TinycProcess({
    tinyc, baseUrl: 'https://libs.example.test/', transport: 'fetch',
  });
  return { files, fs, fetched, fetchStub, newProcess };
}

async function withLoaderFetch(stub, fn) {
  const prevFetch = globalThis.fetch;
  globalThis.fetch = stub;
  try {
    return await fn();
  } finally {
    globalThis.fetch = prevFetch;
  }
}

test('loader cache: a failed write-back is served from memory, not refetched', async () => {
  const canonical = '/sysroot/libmem-a.tinyc.js';
  const body = new Uint8Array([1, 2, 3, 4, 5]);
  const h = makeLoaderHarness(canonical, body);
  h.fs.failWrites = true;
  await withLoaderFetch(h.fetchStub, async () => {
    await h.newProcess()._prefetchClosure(canonical);
    assert.equal(h.fetched.length, 1);
    const proc2 = h.newProcess();
    await proc2._prefetchClosure(canonical);
    assert.equal(h.fetched.length, 1); // memory hit: no second fetch
    assert.deepEqual(proc2._libraryLocal(canonical).bodyBytes, body);
  });
});

test('loader cache: a successful write-back leaves no memory copy', async () => {
  const canonical = '/sysroot/libmem-b.tinyc.js';
  const h = makeLoaderHarness(canonical, new Uint8Array([9, 8, 7]));
  await withLoaderFetch(h.fetchStub, async () => {
    await h.newProcess()._prefetchClosure(canonical);
    assert.equal(h.fetched.length, 1);
    assert.equal(h.fs.hasFile(canonical), true);
    h.fs.removeFile(canonical); // the sysroot copy is deleted
    await h.newProcess()._prefetchClosure(canonical);
    assert.equal(h.fetched.length, 2); // no memory copy: fetched again
  });
});

test('loader cache: after a failed write-back the unit loads with no sysroot copy', async () => {
  const canonical = '/sysroot/libmem-c.tinyc.js';
  const body = new Uint8Array([3, 1, 4]);
  const h = makeLoaderHarness(canonical, body);
  h.fs.failWrites = true;
  await withLoaderFetch(h.fetchStub, async () => {
    const proc = h.newProcess();
    await proc._prefetchClosure(canonical); // the failed write is not a load failure
    assert.equal(h.fs.hasFile(canonical), false); // the entry is memory-only
    assert.deepEqual(proc._libraryLocal(canonical).bodyBytes, body);
  });
});

test('loader cache: a memory hit retries the write-back and evicts on success', async () => {
  const canonical = '/sysroot/libmem-d.tinyc.js';
  const h = makeLoaderHarness(canonical, new Uint8Array([4, 2]));
  await withLoaderFetch(h.fetchStub, async () => {
    h.fs.failWrites = true;
    await h.newProcess()._prefetchClosure(canonical);
    assert.equal(h.fetched.length, 1);
    assert.equal(h.fs.hasFile(canonical), false);
    h.fs.failWrites = false; // writes recover
    await h.newProcess()._prefetchClosure(canonical); // memory hit + retry
    assert.equal(h.fetched.length, 1); // served from memory
    assert.equal(h.fs.hasFile(canonical), true); // the retry landed it
    h.fs.removeFile(canonical); // the sysroot copy is deleted
    await h.newProcess()._prefetchClosure(canonical);
    assert.equal(h.fetched.length, 2); // memory was evicted: fetched again
  });
});

test('loader cache: a memory hit whose retry write still fails keeps the entry', async () => {
  const canonical = '/sysroot/libmem-e.tinyc.js';
  const h = makeLoaderHarness(canonical, new Uint8Array([6, 6, 6]));
  h.fs.failWrites = true;
  await withLoaderFetch(h.fetchStub, async () => {
    await h.newProcess()._prefetchClosure(canonical);
    await h.newProcess()._prefetchClosure(canonical); // hit, retry fails, kept
    await h.newProcess()._prefetchClosure(canonical); // hit again
    assert.equal(h.fetched.length, 1);
    assert.equal(h.fs.hasFile(canonical), false);
  });
});

// ---------------------------------------------------------------------
// dylink name resolution (search_dirs): needed records names
// only; a name resolves through the declaring manifest's
// search_dirs with staged name forms — first among the modules
// already loaded, by file name, then on disk — and retrieval
// happens only when nothing is local. The units below carry a
// minimal hand-built wasm body: no imports; the exports are
// the dylink init/fini hooks (init returns 0) and, for a main
// module, a main returning 42 — so loads and runs are real.
// ---------------------------------------------------------------------

/** A minimal loadable dylink body (see the section comment). */
function synthUnitBody(withMain) {
  const bytes = [
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    // type section: type0 ()->(), type1 ()->i32, type2 (i32)->i32
    0x01, 0x0d, 0x03, 0x60, 0x00, 0x00, 0x60, 0x00, 0x01, 0x7f,
    0x60, 0x01, 0x7f, 0x01, 0x7f,
    // function section: init=type2, fini=type0, main=type1
    ...(withMain
      ? [0x03, 0x04, 0x03, 0x02, 0x00, 0x01]
      : [0x03, 0x03, 0x02, 0x02, 0x00]),
  ];
  const name = (s) => [s.length, ...[...s].map((c) => c.charCodeAt(0))];
  const exportsList = [
    ...name('__tinyc_dylink_init'), 0x00, 0x00,
    ...name('__tinyc_dylink_fini'), 0x00, 0x01,
    ...(withMain ? [...name('main'), 0x00, 0x02] : []),
  ];
  const exportPayload = [withMain ? 3 : 2, ...exportsList];
  bytes.push(0x07, exportPayload.length, ...exportPayload);
  const initBody = [0x04, 0x00, 0x41, 0x00, 0x0b]; // return 0
  const emptyBody = [0x02, 0x00, 0x0b];
  const mainBody = [0x04, 0x00, 0x41, 0x2a, 0x0b]; // return 42
  const codePayload = withMain
    ? [0x03, ...initBody, ...emptyBody, ...mainBody]
    : [0x02, ...initBody, ...emptyBody];
  bytes.push(0x0a, codePayload.length, ...codePayload);
  return new Uint8Array(bytes);
}

/** A unit manifest in the current format over a synth body. */
function synthManifest(withMain, extra = {}) {
  return {
    ...unitManifest(synthUnitBody(withMain)),
    flavor: 2, mem_size: 16, bss_off: 16,
    ...extra,
  };
}

function makeSearchHarness() {
  const files = new Map();
  const fs = {
    hasFile: (p) => files.has(p),
    readFile: (p) => files.get(p) ?? null,
    addFileSync: (p, content) => {
      files.set(p, typeof content === 'string'
        ? new TextEncoder().encode(content) : content);
      return p;
    },
    removeFile: (p) => files.delete(p),
  };
  const tinyc = { fs };
  const newProcess = () => new TinycProcess({
    tinyc, baseUrl: 'https://libs.example.test/', transport: 'fetch',
  });
  const place = (canonical, manifest) => {
    files.set(canonical,
      new TextEncoder().encode(unitEnvelopeText(canonical, manifest)));
  };
  const unitBytes = (canonical, manifest) =>
    new TextEncoder().encode(unitEnvelopeText(canonical, manifest));
  return { files, fs, newProcess, place, unitBytes };
}

const DEFAULT_DIRS = ['$ORIGIN', 'lib', '.'];
const loadedPaths = (proc) => [...proc._modules.keys()];

/** Run a synth main module placed at /app/main.tinyc.js. */
function runSynthMain(h, { needed = [], searchDirs } = {}) {
  const proc = h.newProcess();
  const manifest = synthManifest(true, {
    needed,
    ...(searchDirs !== undefined ? { search_dirs: searchDirs } : {}),
  });
  const result = proc.runMainModule(
    h.unitBytes('/app/main.tinyc.js', manifest),
    { path: '/app/main.tinyc.js' });
  return { proc, result };
}

test('search: a needed name is found in the declaring module\'s own directory ($ORIGIN)', () => {
  const h = makeSearchHarness();
  h.place('/app/libside.tinyc.js', synthManifest(false));
  const { proc, result } = runSynthMain(h, {
    needed: ['libside.tinyc.js'], searchDirs: DEFAULT_DIRS,
  });
  assert.equal(result.exitCode, 42);
  assert.ok(loadedPaths(proc).includes('/app/libside.tinyc.js'));
});

test('search: a needed name is found in the sysroot lib directory', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  const { proc, result } = runSynthMain(h, {
    needed: ['libside.tinyc.js'], searchDirs: DEFAULT_DIRS,
  });
  assert.equal(result.exitCode, 42);
  assert.ok(loadedPaths(proc).includes('/sysroot/lib/libside.tinyc.js'));
});

test('search: a needed name is found at the sysroot root (the "." entry)', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/libside.tinyc.js', synthManifest(false));
  const { proc, result } = runSynthMain(h, {
    needed: ['libside.tinyc.js'], searchDirs: DEFAULT_DIRS,
  });
  assert.equal(result.exitCode, 42);
  assert.ok(loadedPaths(proc).includes('/sysroot/libside.tinyc.js'));
});

test('search: a needed name found nowhere locally is retrieved under the same layout, in candidate order', async () => {
  const h = makeSearchHarness();
  const served = new Map([
    ['/sysroot/libside.tinyc.js', synthManifest(false)],
  ]);
  const fetched = [];
  const stub = async (url) => {
    fetched.push(String(url));
    const canonical =
      `/sysroot/${String(url).slice('https://libs.example.test/'.length)}`;
    const manifest = served.get(canonical);
    if (!manifest) return { ok: false, status: 404, text: async () => '' };
    return {
      ok: true, status: 200,
      text: async () => unitEnvelopeText(canonical, manifest),
    };
  };
  await withLoaderFetch(stub, async () => {
    const proc = h.newProcess();
    const mainManifest = synthManifest(true, {
      needed: ['libside.tinyc.js'], search_dirs: DEFAULT_DIRS,
    });
    h.place('/app/main.tinyc.js', mainManifest);
    await proc._prefetchClosure('/app/main.tinyc.js');
    const result = proc.runMainModule(
      h.unitBytes('/app/main.tinyc.js', mainManifest),
      { path: '/app/main.tinyc.js' });
    assert.equal(result.exitCode, 42);
    assert.deepEqual(fetched, [
      'https://libs.example.test/app/libside.tinyc.js',
      'https://libs.example.test/lib/libside.tinyc.js',
      'https://libs.example.test/libside.tinyc.js',
    ]);
    assert.ok(loadedPaths(proc).includes('/sysroot/libside.tinyc.js'));
    assert.ok(h.fs.hasFile('/sysroot/libside.tinyc.js')); // written back
  });
});

test('search: editing search_dirs in the manifest changes what is found', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  const ok = runSynthMain(h, {
    needed: ['libside.tinyc.js'], searchDirs: DEFAULT_DIRS,
  });
  assert.equal(ok.result.exitCode, 42);
  // The same unit with 'lib' edited out of its manifest's
  // search_dirs no longer finds the library there.
  assert.throws(
    () => runSynthMain(h, {
      needed: ['libside.tinyc.js'], searchDirs: ['$ORIGIN', '.'],
    }),
    /needed library "libside\.tinyc\.js" is not present locally \(searched: \/app\/libside\.tinyc\.js, \/sysroot\/libside\.tinyc\.js;/);
});

test('search: the search_dirs order decides between same-named units in different directories', () => {
  const h = makeSearchHarness();
  h.place('/app/libside.tinyc.js', synthManifest(false));
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  const a = runSynthMain(h, {
    needed: ['libside.tinyc.js'], searchDirs: ['$ORIGIN', 'lib', '.'],
  });
  assert.ok(loadedPaths(a.proc).includes('/app/libside.tinyc.js'));
  assert.ok(!loadedPaths(a.proc).includes('/sysroot/lib/libside.tinyc.js'));
  const b = runSynthMain(h, {
    needed: ['libside.tinyc.js'], searchDirs: ['lib', '$ORIGIN', '.'],
  });
  assert.ok(loadedPaths(b.proc).includes('/sysroot/lib/libside.tinyc.js'));
  assert.ok(!loadedPaths(b.proc).includes('/app/libside.tinyc.js'));
});

test('search: the name as spelled is tried through every directory before any completion', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/side', synthManifest(false)); // original form, last directory
  h.place('/app/libside.tinyc.js', synthManifest(false)); // completed form, first directory
  const { proc, result } = runSynthMain(h, {
    needed: ['side'], searchDirs: DEFAULT_DIRS,
  });
  assert.equal(result.exitCode, 42);
  assert.ok(loadedPaths(proc).includes('/sysroot/side'));
  assert.ok(!loadedPaths(proc).includes('/app/libside.tinyc.js'));
});

test('search: the extension completion is tried through every directory before the lib prefix', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/side.tinyc.js', synthManifest(false)); // stage 2, last directory
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false)); // stage 3, earlier directory
  const proc = h.newProcess();
  assert.ok(proc.dlopen('side') > 0);
  assert.ok(loadedPaths(proc).includes('/sysroot/side.tinyc.js'));
  assert.ok(!loadedPaths(proc).includes('/sysroot/lib/libside.tinyc.js'));
});

test('search: a name already carrying the lib prefix gains only the extension', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  const proc = h.newProcess();
  assert.ok(proc.dlopen('libside') > 0);
  assert.ok(loadedPaths(proc).includes('/sysroot/lib/libside.tinyc.js'));
  // The prefix is never doubled: with only a liblib* unit
  // present, the same request finds nothing.
  const h2 = makeSearchHarness();
  h2.place('/sysroot/lib/liblibside.tinyc.js', synthManifest(false));
  assert.throws(() => h2.newProcess().dlopen('libside'),
    /is not present locally/);
});

test('search: dlopen takes the full name and the short name; needed takes the short name', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  assert.ok(h.newProcess().dlopen('libside.tinyc.js') > 0);
  assert.ok(h.newProcess().dlopen('side') > 0);
  const { result } = runSynthMain(h, {
    needed: ['side'], searchDirs: DEFAULT_DIRS,
  });
  assert.equal(result.exitCode, 42);
});

test('search: dlopen rejects a path spelling by name', () => {
  const proc = makeSearchHarness().newProcess();
  assert.throws(() => proc.dlopen('/sysroot/lib/libside.tinyc.js'),
    /dlopen takes a library name, not a path/);
  assert.throws(() => proc.dlopen('lib/libside.tinyc.js'),
    /dlopen takes a library name, not a path/);
});

test('search: an old-format manifest whose needed entry is an absolute path still loads', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  const { result } = runSynthMain(h, {
    needed: ['/sysroot/lib/libside.tinyc.js'],
  });
  assert.equal(result.exitCode, 42);
});

test('search: an old-format manifest whose needed entry is a relative path resolves against its own directory', () => {
  const h = makeSearchHarness();
  h.place('/app/sub/libside.tinyc.js', synthManifest(false));
  const { proc, result } = runSynthMain(h, {
    needed: ['sub/libside.tinyc.js'],
  });
  assert.equal(result.exitCode, 42);
  assert.ok(loadedPaths(proc).includes('/app/sub/libside.tinyc.js'));
});

test('search: a manifest without search_dirs resolves with the toolchain default', () => {
  const h = makeSearchHarness();
  h.place('/sysroot/libside.tinyc.js', synthManifest(false));
  const { result } = runSynthMain(h, { needed: ['libside.tinyc.js'] });
  assert.equal(result.exitCode, 42);
});

test('search: a module loaded from outside the search directories is found by name', async () => {
  const h = makeSearchHarness();
  h.place('/elsewhere/libside.tinyc.js', synthManifest(false));
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  const proc = h.newProcess();
  const placed = await proc.loadModule('/elsewhere/libside.tinyc.js');
  assert.ok(placed > 0);
  // The short name reaches the loaded module by name before
  // the disk copy of the completed form is even considered.
  assert.equal(proc.dlopen('side'), placed);
  assert.equal(proc.moduleHandle('side'), placed);
  assert.ok(!loadedPaths(proc).includes('/sysroot/lib/libside.tinyc.js'));
});

test('search: an earlier name form found loaded beats a later form found on disk', async () => {
  const h = makeSearchHarness();
  h.place('/elsewhere/side', synthManifest(false));
  h.place('/sysroot/lib/libside.tinyc.js', synthManifest(false));
  const proc = h.newProcess();
  const placed = await proc.loadModule('/elsewhere/side');
  assert.equal(proc.dlopen('side'), placed);
  assert.ok(!loadedPaths(proc).includes('/sysroot/lib/libside.tinyc.js'));
});

test('search: a needed short name binds a module already loaded from outside the search directories', async () => {
  const h = makeSearchHarness();
  h.place('/elsewhere/libside.tinyc.js', synthManifest(false));
  h.place('/app/main.tinyc.js', synthManifest(true, {
    needed: ['side'], search_dirs: DEFAULT_DIRS,
  }));
  const proc = h.newProcess();
  const placed = await proc.loadModule('/elsewhere/libside.tinyc.js');
  await proc.loadModule('/app/main.tinyc.js');
  assert.equal(proc.moduleHandle('libside.tinyc.js'), placed);
  assert.ok(loadedPaths(proc).includes('/app/main.tinyc.js'));
});
