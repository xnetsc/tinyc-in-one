// tinyc-web: TinyC (TCC) WebAssembly glue layer.
//
// A TCC build has exactly one code generation target, so the native
// side is one WASM module per target (see build.sh), shipped as one
// asset envelope file tcc-<target>.tinyc.js (the Emscripten glue
// source plus the module bytes as base64; see the transport section
// below). loadTinyc() picks the
// module for the requested target from a registry and loads it
// lazily; instances of different targets coexist. This file provides,
// in plain JS:
//   - loadTinyc()          instantiate the WASM module
//   - instance.callMain()  command-line mode through the exported main
//   - TinycState           fine-grained libtcc API (tcc_* exports)
//   - runWasmModule()      execute a compiled wasm32 product: a WASM
//                          module cannot instantiate another module
//                          from inside itself, so running is the
//                          host's (this glue layer's) job — it takes
//                          the product bytes, instantiates them with
//                          the WebAssembly API plus a minimal WASI
//                          shim, and calls _start or a named export.
//                          Products also get default JS libc shims
//                          (runtime-shims.mjs, module "env"); users
//                          can register their own shims at package,
//                          instance and per-run level, and imports
//                          no layer satisfies are reported by name
//                          (TinycMissingImportError) up front
//   - VirtualFS            in-page filesystem; user files enter from
//                          content strings, File/Blob objects or URLs
//   - IO                   JS callbacks directly, no trampoline needed:
//                          this package's host language is JavaScript.
//
// Native module resolution in loadTinyc(), per target: the compiler
// module comes from the per-target asset envelope
// tcc-<target>.tinyc.js built by build.sh, retrieved through the
// §14 transport (fetch reads it as text, script injects it).
// options.loadModule may replace the retrieval step: an async
// loader called with (target, files) that returns that envelope's
// content — its text or its bytes; the package parses the envelope
// and builds the module from it. The built module is cached per
// target.

import manifestWasm32Wasi from './sysroot-manifests/wasm32-wasi.json' with { type: 'json' };
import manifestX86_64LinuxGnu from './sysroot-manifests/x86_64-linux-gnu.json' with { type: 'json' };
import manifestAarch64LinuxGnu from './sysroot-manifests/aarch64-linux-gnu.json' with { type: 'json' };
import manifestRiscv64LinuxGnu from './sysroot-manifests/riscv64-linux-gnu.json' with { type: 'json' };
import manifestX86_64Windows from './sysroot-manifests/x86_64-windows.json' with { type: 'json' };
import manifestAarch64LinuxAndroid from './sysroot-manifests/aarch64-linux-android.json' with { type: 'json' };
import manifestX86_64LinuxAndroid from './sysroot-manifests/x86_64-linux-android.json' with { type: 'json' };
import manifestArm64AppleDarwin from './sysroot-manifests/arm64-apple-darwin.json' with { type: 'json' };
import manifestX86_64AppleDarwin from './sysroot-manifests/x86_64-apple-darwin.json' with { type: 'json' };
import { createDefaultShims, ShimExit } from './runtime-shims.mjs';
import {
  Scheduler, ThreadRecord, createSimPthreadShims, ThreadExitSignal,
  HDR_TID_COUNTER, createCoordinatorPthreadShims,
} from './threads.mjs';
import { MemoryFsStore, openFsStore } from './vfs-store.mjs';

export { MemoryFsStore, IndexedDbFsStore, createFsStore } from './vfs-store.mjs';

// Helpers the thread-worker runtime (threads.mjs, entered from
// worker.mjs) builds replicas with. Not part of the public
// embedding surface.
export {
  manifestToLibrary, libraryFromBytes, parseImportTypes,
  makeWrapperFunction, createWasiShim, createShimContext,
  canonicalModulePath,
};

/** The default target's sysroot manifest (sysroot-manifests/wasm32-wasi.json). */
export const sysrootManifest = manifestWasm32Wasi;

// Every target's sysroot manifest, statically imported so the data is
// part of the module graph: the classic-script bundle (tinyc-web.js)
// inlines all of them, and loadSysrootManifest never fetches JSON at
// runtime.
const SYSROOT_MANIFESTS = Object.freeze({
  'wasm32-wasi': manifestWasm32Wasi,
  'x86_64-linux-gnu': manifestX86_64LinuxGnu,
  'aarch64-linux-gnu': manifestAarch64LinuxGnu,
  'riscv64-linux-gnu': manifestRiscv64LinuxGnu,
  'x86_64-windows': manifestX86_64Windows,
  'aarch64-linux-android': manifestAarch64LinuxAndroid,
  'x86_64-linux-android': manifestX86_64LinuxAndroid,
  'arm64-apple-darwin': manifestArm64AppleDarwin,
  'x86_64-apple-darwin': manifestX86_64AppleDarwin,
});

// ---------------------------------------------------------------------
// Asset base URL
//
// The per-target envelope files and the sysroot files resolve
// against a base captured for the environment this code runs in:
//   - ES module: this module's own URL (import.meta.url), so assets
//     sitting next to index.mjs are found;
//   - classic-script bundle (tinyc-web.js): the directory of the
//     <script> element that loaded the bundle, captured below from
//     document.currentScript while the bundle executes, so assets
//     sitting next to tinyc-web.js are found; when no script URL is
//     available, the page's own directory (location.href) is used.
// loadTinyc's baseUrl option overrides the base per call.
// ---------------------------------------------------------------------

const SCRIPT_DIR_HREF = (() => {
  try {
    const script = typeof document !== 'undefined' ? document.currentScript : null;
    if (script && script.src) return new URL('.', script.src).href;
  } catch { /* no usable script URL */ }
  return null;
})();

const MODULE_URL = (() => {
  try {
    return import.meta.url || null;
  } catch {
    return null;
  }
})();

const PAGE_DIR_HREF = (() => {
  try {
    if (typeof location !== 'undefined' && location.href) {
      return new URL('.', location.href).href;
    }
  } catch { /* no page URL */ }
  return null;
})();

/** baseUrl values are directory URLs: a missing trailing slash is
 *  added, so "https://host/libs" names the directory, not a file. */
function dirHref(url) {
  return url.endsWith('/') ? url : url + '/';
}

/** Absolute URL of an asset file: resolved against baseUrl when
 *  given, else against the captured environment base. */
function assetUrl(name, baseUrl) {
  const base = baseUrl
    ? dirHref(baseUrl)
    : SCRIPT_DIR_HREF ?? MODULE_URL ?? PAGE_DIR_HREF;
  return base ? new URL(name, base).href : name;
}

/** Virtual path placed in argv[0] when calling the exported main. TCC
 *  derives its library path from argv[0]; the sysroot is mounted at
 *  /sysroot in the virtual filesystem, so argv[0] points inside it. */
export const VIRTUAL_BIN_PATH = sysrootManifest.binPath;

export class TinycError extends Error {
  constructor(message, code) {
    super(message);
    this.name = 'TinycError';
    if (code !== undefined) this.code = code;
  }
}

/** Thrown when a required tcc_* / main export is not present in the
 *  loaded native module. The message always names the missing export. */
export class TinycMissingExportError extends TinycError {
  constructor(exportName) {
    super(
      `tinyc: native module does not export "${exportName}". ` +
      `The WASM build must list it in EXPORTED_FUNCTIONS (see build.sh).`
    );
    this.name = 'TinycMissingExportError';
    this.exportName = exportName;
  }
}

// ---------------------------------------------------------------------
// Targets
//
// A TCC binary has exactly one code generation target, fixed when it
// is built; upstream ships parallel cross-compiler variants instead
// of one multi-target binary. This package does the same: one WASM
// module per target, shipped as one asset envelope file
// tcc-<target>.tinyc.js, chosen through the registry below and
// loaded lazily — only the requested target's module is ever
// fetched. Instances of different
// targets coexist in one page; each has its own virtual FS and its
// own sysroot manifest (sysroot-manifests/<target>.json).
//
// Compiling and running are different things here. The compiler's
// host platform in a browser is WebAssembly itself, so only the
// wasm32 target's output can run in the browser (the produced module
// is instantiated and executed). Every other target compiles to
// files only; the glue layer rejects run requests for them up front
// (TinycRunNotSupportedError) instead of passing them to the native
// module to fail there.
// ---------------------------------------------------------------------

export const DEFAULT_TARGET = 'wasm32-wasi';

export const TARGETS = Object.freeze({
  'wasm32-wasi': { runnable: true, description: 'WebAssembly (WASI); output runs in the browser' },
  'x86_64-linux-gnu': { runnable: false, description: 'Linux x86-64 ELF' },
  'aarch64-linux-gnu': { runnable: false, description: 'Linux AArch64 ELF' },
  'riscv64-linux-gnu': { runnable: false, description: 'Linux RISC-V 64 ELF' },
  'x86_64-windows': { runnable: false, description: 'Windows x86-64 PE' },
  'aarch64-linux-android': { runnable: false, description: 'Android AArch64 ELF (bionic)' },
  'x86_64-linux-android': { runnable: false, description: 'Android x86-64 ELF (bionic)' },
  'arm64-apple-darwin': { runnable: false, description: 'macOS arm64 Mach-O' },
  'x86_64-apple-darwin': { runnable: false, description: 'macOS x86-64 Mach-O' },
});

export const SUPPORTED_TARGETS = Object.freeze(Object.keys(TARGETS));

/** The ELF targets: their sysroots carry the platform's crt
 *  objects and stub shared libraries, and a hosted link is a
 *  dynamic link against the platform's own libc/libm. */
const ELF_TARGETS = new Set([
  'x86_64-linux-gnu',
  'aarch64-linux-gnu',
  'riscv64-linux-gnu',
  'aarch64-linux-android',
  'x86_64-linux-android',
]);

/** The Android targets (bionic): the dl* functions live in libdl
 *  there, not in libc. */
const ANDROID_TARGETS = new Set([
  'aarch64-linux-android',
  'x86_64-linux-android',
]);

export function isRunnableTarget(target) {
  return Boolean(TARGETS[target]?.runnable);
}

export function targetModuleFiles(target) {
  return { envelopeFile: `tcc-${target}.tinyc.js` };
}

function assertKnownTarget(target) {
  if (!Object.hasOwn(TARGETS, target)) {
    throw new TinycError(
      `tinyc: unknown target "${target}". Supported targets: ${SUPPORTED_TARGETS.join(', ')}`
    );
  }
}

/** Thrown by the glue layer — before anything reaches the native
 *  module — when code tries to run the output of a target that
 *  cannot execute in the browser. */
export class TinycRunNotSupportedError extends TinycError {
  constructor(target) {
    super(
      `tinyc: target "${target}" cannot be run in the browser: in the browser it ` +
      `can only be compiled to a file. Running compiled output is only possible ` +
      `for the "${DEFAULT_TARGET}" target, whose output is WebAssembly and is ` +
      `instantiated and executed by the browser itself.`
    );
    this.name = 'TinycRunNotSupportedError';
    this.target = target;
  }
}

// ---------------------------------------------------------------------
// IO
//
// An IO object is { out, err, in }:
//   out(text) / err(text)  receive decoded text chunks written by TCC
//   in()                   returns the next stdin chunk (string) or
//                          null at EOF; may also return a Promise
// Defaults: out/err are line-buffered writers onto console.log /
// console.error, in returns null (immediate EOF). No fd juggling and
// no host stdio is ever touched: this is the TCCIO interface of the
// native core with JS functions plugged straight in.
// ---------------------------------------------------------------------

function lineBuffered(sink) {
  let pending = '';
  const write = (text) => {
    pending += text;
    let idx;
    while ((idx = pending.indexOf('\n')) >= 0) {
      sink(pending.slice(0, idx));
      pending = pending.slice(idx + 1);
    }
  };
  write.flush = () => {
    if (pending.length > 0) {
      sink(pending);
      pending = '';
    }
  };
  return write;
}

export function createDefaultIO() {
  return {
    out: lineBuffered((line) => console.log(line)),
    err: lineBuffered((line) => console.error(line)),
    in: () => null,
  };
}

export function normalizeIO(partial, base) {
  const io = { ...(base ?? createDefaultIO()) };
  if (partial) {
    for (const key of ['out', 'err', 'in']) {
      if (partial[key] !== undefined) {
        if (typeof partial[key] !== 'function') {
          throw new TinycError(`tinyc: io.${key} must be a function`);
        }
        io[key] = partial[key];
      }
    }
  }
  return io;
}

export function flushIO(io) {
  for (const key of ['out', 'err']) {
    if (typeof io[key]?.flush === 'function') io[key].flush();
  }
}

// ---------------------------------------------------------------------
// Virtual filesystem
//
// File contents are persisted in a store (vfs-store.mjs): one
// IndexedDB database per target where IndexedDB exists, an in-memory
// backend where it does not. What stays in memory here is only the
// path index (path -> { size, mtime }) plus a small LRU content
// cache; the bytes themselves live in the store. Paths are
// POSIX-style and normalized to absolute form ("/src/main.c"). This
// is where the "dynamically add local files" rule of the native
// packages lands in a browser, where there is no real filesystem to
// point TCC at: a local file enters as its content, as a File/Blob
// from an <input> or drag-and-drop, or fetched from a URL.
//
// Synchronous reads (readFile / readTextFile) are served from the
// content cache — everything written or read during this session is
// cached — and throw for a persisted file that is not cached,
// naming readFileAsync / readTextFileAsync as the alternative.
// addFile awaits its store write; addFileSync stays synchronous and
// queues the write instead — await whenIdle() (on the FS or the
// instance) to know queued writes and deletes have landed.
//
// === Emscripten FS adaptation point ===
// When the native module is an Emscripten build, mountTo() copies
// every virtual file into the module's FS (FS.mkdirTree + FS.writeFile)
// and syncFile() keeps single files in sync after each write. The
// compile working set has to live in that FS while TCC runs, so it
// is mirrored there in full — under the bridge's moduleFsPrefix
// ("/opfs", the WasmFS OPFS backend loadTinyc mounts before
// mirroring, when that mount succeeds; the module FS root
// otherwise). Emscripten-FS knowledge in this package is confined
// to those two methods, to removeFile's unlink, and to the OPFS
// mount attempt itself (tryMountOpfsBackend).
// ---------------------------------------------------------------------

/** Content cache bounds for the virtual FS: at most this many
 *  files, and at most this many bytes in total, are held in memory
 *  for synchronous reads. A single file larger than the byte bound
 *  is still cached (the bound is soft for one entry). */
export const VFS_CACHE_MAX_ENTRIES = 32;
export const VFS_CACHE_MAX_BYTES = 16 * 1024 * 1024;

const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder();

export function normalizeVfsPath(path) {
  if (typeof path !== 'string' || path.length === 0) {
    throw new TinycError('tinyc: vfs path must be a non-empty string');
  }
  const parts = [];
  for (const seg of path.split('/')) {
    if (seg === '' || seg === '.') continue;
    if (seg === '..') parts.pop();
    else parts.push(seg);
  }
  return '/' + parts.join('/');
}

/** Bytes for the content kinds whose bytes are at hand synchronously
 *  (string, Uint8Array, ArrayBuffer); null for Blob/File content,
 *  whose bytes only an asynchronous read can produce. Any other
 *  content is rejected with the addFile content error. */
function contentToBytesSync(content) {
  if (typeof content === 'string') return textEncoder.encode(content);
  if (content instanceof Uint8Array) return new Uint8Array(content);
  if (content instanceof ArrayBuffer) return new Uint8Array(content);
  if (typeof Blob !== 'undefined' && content instanceof Blob) return null;
  throw new TinycError(
    'tinyc: addFile content must be a string, Uint8Array, ArrayBuffer, File or Blob'
  );
}

async function contentToBytes(content) {
  const syncBytes = contentToBytesSync(content);
  if (syncBytes) return syncBytes;
  // File is a Blob subclass; both are read the same way.
  return new Uint8Array(await content.arrayBuffer());
}

export class VirtualFS {
  /** `store` is an opened FsStore (see vfs-store.mjs); its persisted
   *  index becomes this filesystem's initial file set. With no store
   *  argument, a volatile in-memory store is used. loadTinyc opens
   *  the per-target store (IndexedDB by default) before building
   *  the instance's VirtualFS. */
  constructor(store = null) {
    this._store = store ?? new MemoryFsStore();
    this.index = new Map(this._store.index ?? []);
    this.includeDirs = [];
    this._emscriptenFS = null;
    this._moduleFsPrefix = '';
    this._cache = new Map();
    this._cacheBytes = 0;
    this._pending = new Set();
    this._idleError = null;
  }

  /** The kind of the backing store: 'indexeddb' or 'memory'. */
  get storeKind() {
    return this._store.kind;
  }

  _cacheGet(path) {
    const bytes = this._cache.get(path);
    if (bytes) {
      // Refresh recency: re-insert at the newest end.
      this._cache.delete(path);
      this._cache.set(path, bytes);
    }
    return bytes;
  }

  _cachePut(path, bytes) {
    const prev = this._cache.get(path);
    if (prev) {
      this._cache.delete(path);
      this._cacheBytes -= prev.byteLength;
    }
    this._cache.set(path, bytes);
    this._cacheBytes += bytes.byteLength;
    while (
      this._cache.size > 1 &&
      (this._cache.size > VFS_CACHE_MAX_ENTRIES ||
        this._cacheBytes > VFS_CACHE_MAX_BYTES)
    ) {
      const oldest = this._cache.keys().next().value;
      this._cacheBytes -= this._cache.get(oldest).byteLength;
      this._cache.delete(oldest);
    }
  }

  _cacheDelete(path) {
    const prev = this._cache.get(path);
    if (prev) {
      this._cache.delete(path);
      this._cacheBytes -= prev.byteLength;
    }
  }

  /** Queue a store operation kicked off by a synchronous method
   *  (addFileSync / removeFile / clear). Tracked so whenIdle() can
   *  await the queue; the first failure is remembered and rethrown
   *  by whenIdle() instead of surfacing as an unhandled rejection. */
  _queueStoreOp(op) {
    let base;
    try {
      base = Promise.resolve(op());
    } catch (err) {
      base = Promise.reject(err);
    }
    const tracked = base.then(
      () => { this._pending.delete(tracked); },
      (err) => {
        this._pending.delete(tracked);
        if (this._idleError == null) this._idleError = err;
      }
    );
    this._pending.add(tracked);
  }

  /** Resolve once every queued store write/delete has landed.
   *  Rethrows the first queued operation's failure, if any. */
  async whenIdle() {
    while (this._pending.size > 0) {
      await Promise.all([...this._pending]);
    }
    if (this._idleError != null) {
      const err = this._idleError;
      this._idleError = null;
      throw err;
    }
  }

  /** Synchronous half of a write: index, content cache and the
   *  Emscripten FS mirror. Returns the normalized path and the
   *  record to persist. */
  _stage(path, bytes) {
    const p = normalizeVfsPath(path);
    const buf = bytes instanceof Uint8Array ? bytes : new Uint8Array(bytes);
    const meta = { size: buf.byteLength, mtime: Date.now() };
    this.index.set(p, meta);
    this._cachePut(p, buf);
    this.syncFile(p);
    return { p, record: { bytes: buf, size: meta.size, mtime: meta.mtime } };
  }

  /** Add or replace a file. `content` may be a string, Uint8Array,
   *  ArrayBuffer, File or Blob. For the content kinds whose bytes
   *  are at hand (string, Uint8Array, ArrayBuffer) the file is
   *  staged — index, content cache and the Emscripten FS mirror —
   *  synchronously, before this call returns its promise, so a
   *  compile or read issued immediately afterwards, without
   *  awaiting, already sees the file; Blob and File content stages
   *  once its bytes have been read. The returned promise resolves
   *  once the content is persisted in the store. */
  async addFile(path, content) {
    const bytes = contentToBytesSync(content) ?? (await contentToBytes(content));
    const { p, record } = this._stage(path, bytes);
    await this._store.put(p, record);
  }

  /** Synchronous write: index, cache and the Emscripten mirror are
   *  updated immediately (so the file can be compiled and read back
   *  synchronously); the store write is queued behind any other
   *  queued operations — await whenIdle() for it to land. */
  addFileSync(path, content) {
    const bytes =
      typeof content === 'string' ? textEncoder.encode(content) : content;
    const { p, record } = this._stage(path, bytes);
    this._queueStoreOp(() => this._store.put(p, record));
    return p;
  }

  /** Fetch a file from a URL into the virtual filesystem.
   *  `fetchImpl` is injectable for tests and non-window contexts. */
  async addFileFromUrl(path, url, { fetchImpl = globalThis.fetch } = {}) {
    if (typeof fetchImpl !== 'function') {
      throw new TinycError('tinyc: addFileFromUrl needs a fetch implementation');
    }
    const res = await fetchImpl(url);
    if (res && res.ok === false) {
      throw new TinycError(`tinyc: fetching ${url} failed with status ${res.status}`);
    }
    const buf = await res.arrayBuffer();
    const bytes = new Uint8Array(buf);
    const { p, record } = this._stage(path, bytes);
    await this._store.put(p, record);
    return p;
  }

  /** Register a directory as a header search directory. Every state
   *  created afterwards gets it via tcc_add_include_path, in
   *  registration order, ahead of the sysroot. */
  registerIncludeDir(path) {
    const p = normalizeVfsPath(path);
    if (!this.includeDirs.includes(p)) this.includeDirs.push(p);
    return p;
  }

  getIncludeDirs() {
    return [...this.includeDirs];
  }

  hasFile(path) {
    return this.index.has(normalizeVfsPath(path));
  }

  /** Synchronous read, served from the content cache: files written
   *  or read during this session are cached, so this covers the
   *  compile/read-back paths. A persisted file that is not cached
   *  throws, naming the async alternative — use readFileAsync. */
  readFile(path) {
    const p = normalizeVfsPath(path);
    if (!this.index.has(p)) {
      throw new TinycError(`tinyc: no such file in virtual FS: ${path}`);
    }
    const bytes = this._cacheGet(p);
    if (!bytes) {
      throw new TinycError(
        `tinyc: ${p} is persisted in the virtual FS store but is not in ` +
        `the in-memory cache; read it with readFileAsync() instead`
      );
    }
    return bytes;
  }

  readTextFile(path) {
    return textDecoder.decode(this.readFile(path));
  }

  /** Read a file's content from the store, warming the content
   *  cache, so later synchronous reads of the same file succeed. */
  async readFileAsync(path) {
    const p = normalizeVfsPath(path);
    if (!this.index.has(p)) {
      throw new TinycError(`tinyc: no such file in virtual FS: ${path}`);
    }
    const hit = this._cacheGet(p);
    if (hit) return hit;
    const rec = await this._store.get(p);
    if (!rec) {
      throw new TinycError(`tinyc: no such file in virtual FS: ${path}`);
    }
    this._cachePut(p, rec.bytes);
    return rec.bytes;
  }

  async readTextFileAsync(path) {
    return textDecoder.decode(await this.readFileAsync(path));
  }

  /** Delete one file. Every copy goes: the index entry and cached
   *  content, the persisted record (queued — see whenIdle), and,
   *  when an Emscripten FS is mounted, the mirrored file inside the
   *  native module (an unlink is attempted even for a path missing
   *  from the index, e.g. a file the compiler created natively).
   *  Returns true when the index or the native copy existed. */
  removeFile(path) {
    const p = normalizeVfsPath(path);
    const had = this.index.delete(p);
    this._cacheDelete(p);
    this._queueStoreOp(() => this._store.delete(p));
    let unlinked = false;
    const FS = this._emscriptenFS;
    if (FS && typeof FS.unlink === 'function') {
      try { FS.unlink(this._nativePath(p)); unlinked = true; } catch { /* not present natively */ }
    }
    return had || unlinked;
  }

  /** Delete every file tracked by this virtual FS except the
   *  mounted sysroot (paths under /sysroot/), which stays so the
   *  instance can keep compiling. Store records are deleted (queued)
   *  and mirrors in the native module FS are unlinked as well.
   *  Returns the number of files removed. */
  clear() {
    let removed = 0;
    for (const p of [...this.index.keys()]) {
      if (p.startsWith('/sysroot/')) continue;
      if (this.removeFile(p)) removed += 1;
    }
    return removed;
  }

  listFiles(prefix = '/') {
    const p = normalizeVfsPath(prefix);
    return [...this.index.keys()].filter((f) => f.startsWith(p)).sort();
  }

  // === Emscripten FS adaptation point (see header note) ===
  /** Mirror the whole virtual FS into the native module's FS. The
   *  mounted FS is registered synchronously; copying the persisted
   *  files is asynchronous (their bytes may have to come out of the
   *  store), so this returns a promise loadTinyc awaits before the
   *  instance is handed out. Callers that mount an empty or
   *  cache-resident FS can ignore the promise.
   *  `moduleFsPrefix` is the directory inside the module FS the
   *  mirror lands under — the bridge's moduleFsPrefix ("/opfs" when
   *  the WasmFS OPFS backend is mounted there, "" for the memory
   *  root). It is remembered here so syncFile() and removeFile()
   *  hit the same locations; the virtual FS's own paths, index and
   *  store are unaffected by it. */
  async mountTo(emscriptenFS, moduleFsPrefix = '') {
    this._emscriptenFS = emscriptenFS ?? null;
    this._moduleFsPrefix = moduleFsPrefix ?? '';
    if (!this._emscriptenFS) return;
    for (const path of [...this.index.keys()]) {
      let bytes = this._cacheGet(path);
      if (!bytes) {
        const rec = await this._store.get(path);
        if (rec) {
          bytes = rec.bytes;
          this._cachePut(path, bytes);
        }
      }
      if (bytes) this._writeNative(path, bytes);
    }
  }

  /** Mirror one file into the native FS from the content cache.
   *  Files written through this FS are synced as they are written;
   *  a file whose content is neither cached nor already mirrored
   *  cannot be synced synchronously and is left to mountTo. */
  syncFile(path) {
    const FS = this._emscriptenFS;
    if (!FS) return;
    const bytes = this._cacheGet(path);
    if (!bytes) return;
    this._writeNative(path, bytes);
  }

  /** The location of a (normalized) virtual path inside the
   *  native module's filesystem: under the mount prefix when one
   *  was given to mountTo, at the module FS root otherwise. */
  _nativePath(path) {
    return this._moduleFsPrefix ? this._moduleFsPrefix + path : path;
  }

  _writeNative(path, bytes) {
    const FS = this._emscriptenFS;
    const nativePath = this._nativePath(path);
    const dir = nativePath.slice(0, nativePath.lastIndexOf('/')) || '/';
    if (typeof FS.mkdirTree === 'function') FS.mkdirTree(dir);
    // The mirror write is a replace: the native copy must end up
    // holding exactly `bytes`. The WasmFS FS.writeFile appends to a
    // file that already exists (it writes at the file's current
    // size), so an existing native file is unlinked first and the
    // write lands as a fresh file with exactly the mirrored content.
    if (typeof FS.unlink === 'function') {
      try { FS.unlink(nativePath); } catch { /* not present natively */ }
    }
    FS.writeFile(nativePath, bytes);
  }
}

// ---------------------------------------------------------------------
// Native bridge
//
// Normalizes the three native shapes onto one call interface whose
// function names are exactly the libtcc export names (tcc_new,
// tcc_compile_string, ..., plus main / tinyc_main):
//   'emscripten'  module object with ccall (build.sh output)
//   'raw'         WebAssembly.Instance exports with a memory
//   'mock'        plain object of JS functions with the same names;
//                 strings and handles pass through as JS values.
//                 This is the shape the tests' mock modules take;
//                 they reach the bridge through the internal
//                 loadTinycWithNative seam below.
// ---------------------------------------------------------------------

class NativeBridge {
  constructor(native, io) {
    this.native = native;
    this.io = io;
    /** Directory inside the native module's filesystem under which
     *  the virtual FS is mirrored and native-side paths are rooted:
     *  "/opfs" when loadTinyc mounted the WasmFS OPFS backend there,
     *  "" (the module FS root, the memory backend) otherwise. Set by
     *  loadTinyc once the mount attempt has decided; every path that
     *  crosses into the native module — mirrored files, tcc_add_file
     *  and path registrations, callMain argv, product read-back —
     *  goes through modulePath() so the two stay consistent. */
    this.moduleFsPrefix = '';
    if (native && typeof native.ccall === 'function') this.mode = 'emscripten';
    else if (native && native.memory instanceof WebAssembly.Memory) this.mode = 'raw';
    else if (native && typeof native === 'object') this.mode = 'mock';
    else {
      throw new TinycError(
        'tinyc: unsupported native module shape; pass an Emscripten module, ' +
        'WebAssembly exports, or a mock object implementing the tcc_* functions'
      );
    }
  }

  /** Translate a virtual FS path to the path the native module
   *  knows it by: the normalized path under moduleFsPrefix. With no
   *  prefix (memory backend) the argument passes through exactly as
   *  given, so native calls are unchanged in that mode. */
  modulePath(path) {
    if (!this.moduleFsPrefix) return path;
    return this.moduleFsPrefix + normalizeVfsPath(path);
  }

  has(name) {
    const n = this.native;
    switch (this.mode) {
      case 'emscripten':
        return typeof n['_' + name] === 'function' || typeof n[name] === 'function';
      case 'raw':
        return typeof n[name] === 'function' || typeof n['_' + name] === 'function';
      default:
        return typeof n[name] === 'function';
    }
  }

  requireExport(name) {
    if (!this.has(name)) throw new TinycMissingExportError(name);
  }

  _rawFn(name) {
    return this.native[name] ?? this.native['_' + name];
  }

  /** Call a tcc_* style export. argTypes entries: 'number' | 'string'.
   *  In mock mode arguments pass through as JS values untouched. */
  call(name, returnType, argTypes, args) {
    this.requireExport(name);
    if (this.mode === 'mock') return this.native[name](...args);
    if (this.mode === 'emscripten') {
      return this.native.ccall(name, returnType, argTypes, args);
    }
    return this._callRaw(name, argTypes, args);
  }

  _mem() {
    return new Uint8Array(this.native.memory.buffer);
  }

  _alloc(bytes) {
    const malloc = this._rawFn('malloc');
    if (typeof malloc !== 'function') {
      throw new TinycMissingExportError('malloc (needed to pass strings to the raw WASM module)');
    }
    const ptr = malloc(bytes);
    if (!ptr) throw new TinycError('tinyc: WASM malloc returned null');
    return ptr;
  }

  _free(ptr) {
    const free = this._rawFn('free');
    if (typeof free === 'function' && ptr) free(ptr);
  }

  _allocCString(str) {
    const bytes = textEncoder.encode(str + '\0');
    const ptr = this._alloc(bytes.length);
    this._mem().set(bytes, ptr);
    return ptr;
  }

  _callRaw(name, argTypes, args) {
    const fn = this._rawFn(name);
    const ptrs = [];
    try {
      const conv = args.map((arg, i) => {
        if (argTypes[i] === 'string') {
          const p = this._allocCString(arg);
          ptrs.push(p);
          return p;
        }
        return arg;
      });
      return fn(...conv);
    } finally {
      for (const p of ptrs) this._free(p);
    }
  }

  /** Call the exported main / tinyc_main with a fully assembled argv.
   *  argvStrings[0] is the program name slot, already filled by the
   *  caller. tinyc_main is preferred when both are exported. */
  callMain(argvStrings) {
    const entry = this.has('tinyc_main') ? 'tinyc_main' : 'main';
    this.requireExport(entry);
    const argc = argvStrings.length;
    if (this.mode === 'mock') return this.native[entry](argc, [...argvStrings]);
    if (this.mode === 'emscripten') return this._callMainEmscripten(entry, argvStrings);
    return this._callMainRaw(entry, argvStrings);
  }

  _callMainRaw(entry, argvStrings) {
    const fn = this._rawFn(entry);
    const strPtrs = argvStrings.map((s) => this._allocCString(s));
    const argvPtr = this._alloc((argvStrings.length + 1) * 4);
    try {
      const view = new DataView(this.native.memory.buffer);
      strPtrs.forEach((p, i) => view.setUint32(argvPtr + i * 4, p, true));
      view.setUint32(argvPtr + argvStrings.length * 4, 0, true);
      return fn(argvStrings.length, argvPtr);
    } finally {
      for (const p of strPtrs) this._free(p);
      this._free(argvPtr);
    }
  }

  _callMainEmscripten(entry, argvStrings) {
    const n = this.native;
    if (typeof n._malloc !== 'function' || !n.HEAPU8) {
      throw new TinycError(
        'tinyc: Emscripten module must export _malloc and HEAPU8 to assemble argv'
      );
    }
    const strPtrs = argvStrings.map((s) => {
      const bytes = textEncoder.encode(s + '\0');
      const p = n._malloc(bytes.length);
      n.HEAPU8.set(bytes, p);
      return p;
    });
    const argvPtr = n._malloc((argvStrings.length + 1) * 4);
    try {
      const view = new DataView(n.HEAPU8.buffer);
      strPtrs.forEach((p, i) => view.setUint32(argvPtr + i * 4, p, true));
      view.setUint32(argvPtr + argvStrings.length * 4, 0, true);
      const fn = n['_' + entry];
      return fn(argvStrings.length, argvPtr);
    } finally {
      for (const p of strPtrs) n._free(p);
      n._free(argvPtr);
    }
  }
}

// ---------------------------------------------------------------------
// Running compiled wasm32 products
//
// A WebAssembly module cannot instantiate another WebAssembly module
// from inside itself — instantiation is a host capability. So for
// the wasm32 target, "run" does not go through the compiler module
// at all (native TCC provides no tcc_run for the wasm target): this
// glue layer takes the compiled product's bytes, instantiates them
// with the WebAssembly API, and calls the entry point. The instance
// gets a minimal WASI shim (wasi_snapshot_preview1) assembled here;
// proc_exit is turned into a WasiExit exception that the runner
// catches and reports as an exit code — it never terminates the host.
//
// runWasmModule(bytes, options) is async (WebAssembly.compile);
// runWasmModuleSync(bytes, options) uses WebAssembly.Module /
// Instance directly and is the variant used inside Workers and by
// the synchronous Tinyc / TinycState run paths below.
//
// options:
//   args      program arguments. For _start modules this is the WASI
//             argv (args[0] is the program name slot); for a named
//             entry it is the default argument list (see callArgs).
//   callArgs  arguments for a named entry, overriding args.
//   entry     export name to call instead of _start. Its return
//             value is returned as-is (function style).
//   env       object of environment variables for environ_get.
//   imports   extra import objects, merged as-is over the WASI shim
//             (a same-named entry replaces the shim's).
//   io        { out, err } receiving Uint8Array chunks from fd_write
//             on fd 1 / 2. Defaults decode to console.log/error.
//   stdin     Uint8Array (or string) served by fd_read on fd 0.
//   shims     per-run custom shims: { name: fn } (module "env") or
//             { "module.name": fn } for any other import module.
//   heapBase  heap start for the default shims' allocator.
// Without options.entry: if the module exports _start it is called
// (command style) and the result is { exitCode }; a normal return
// from _start means exit code 0.
//
// Imports are assembled in layers; for the same module.name, a later
// layer replaces an earlier one:
//   1. built-in WASI shim (wasi_snapshot_preview1)
//   2. default libc shims (runtime-shims.mjs, module "env")
//   3. package-global shims (registerGlobalShim / registerGlobalShims)
//   4. instance shims (Tinyc.registerShim; only for runs made
//      through an instance)
//   5. per-run options.shims
//   6. options.imports, merged as-is (raw escape hatch)
// A registered shim is called as fn(ctx, ...cArgs). ctx exposes the
// running instance: { memory(), u8(), readCString(ptr),
// writeCString(ptr, str), readBytes(ptr, len),
// writeBytes(ptr, bytes), io }, where io is
// the run's byte IO. C arguments arrive in their wasm types (i32 /
// f32 / f64 as numbers, i64 as BigInt) and the return value goes
// back the same way — an i64 result must be returned as a BigInt.
// The JS function declares no signature: the engine coerces the
// arguments and the return value to the import type the module
// itself declares.
//
// Before instantiation, the module's required imports
// (WebAssembly.Module.imports) are diffed against the assembled
// importObject. Anything missing throws TinycMissingImportError
// listing every missing module.name, instead of surfacing the
// engine's raw LinkError.
// ---------------------------------------------------------------------

export class WasiExit extends Error {
  constructor(code) {
    super(`tinyc: wasi proc_exit(${code})`);
    this.name = 'WasiExit';
    this.exitCode = code;
  }
}

const WASI_ERRNO = {
  SUCCESS: 0, BADF: 8, FAULT: 21, INVAL: 28, NOSYS: 52, SPIPE: 70,
};
const WASI_RIGHTS_FD_READ = 1n << 1n;
const WASI_RIGHTS_FD_WRITE = 1n << 6n;

function createDefaultByteIO() {
  const outDec = new TextDecoder();
  const errDec = new TextDecoder();
  return {
    out: (bytes) => console.log(outDec.decode(bytes, { stream: true })),
    err: (bytes) => console.error(errDec.decode(bytes, { stream: true })),
  };
}

function createWasiShim(options, holder, io) {
  const argv = (options.args ?? []).map(String);
  const envPairs = Object.entries(options.env ?? {}).map(([k, v]) => `${k}=${v}`);
  let stdin = options.stdin ?? new Uint8Array(0);
  if (typeof stdin === 'string') stdin = textEncoder.encode(stdin);
  let stdinPos = 0;

  const mem = () => holder.memory;
  const view = () => new DataView(mem().buffer);
  const bytes = () => new Uint8Array(mem().buffer);

  // Write a (char**) array at ptrAddr plus the NUL-terminated strings
  // themselves at bufAddr; returns total string bytes written.
  function writeStringTable(ptrAddr, bufAddr, strings) {
    const v = view();
    const m = bytes();
    let offset = 0;
    strings.forEach((s, i) => {
      const encoded = textEncoder.encode(s + '\0');
      v.setUint32(ptrAddr + i * 4, bufAddr + offset, true);
      m.set(encoded, bufAddr + offset);
      offset += encoded.length;
    });
    return offset;
  }
  const stringTableSize = (strings) =>
    strings.reduce((n, s) => n + textEncoder.encode(s).length + 1, 0);

  return {
    fd_write(fd, iovs, iovsLen, nwrittenPtr) {
      if (fd !== 1 && fd !== 2) return WASI_ERRNO.BADF;
      if (!mem()) return WASI_ERRNO.FAULT;
      const v = view();
      const m = bytes();
      const chunks = [];
      let total = 0;
      for (let i = 0; i < iovsLen; i++) {
        const ptr = v.getUint32(iovs + i * 8, true);
        const len = v.getUint32(iovs + i * 8 + 4, true);
        chunks.push(m.slice(ptr, ptr + len)); // copy: memory may move
        total += len;
      }
      const merged = new Uint8Array(total);
      let at = 0;
      for (const c of chunks) { merged.set(c, at); at += c.length; }
      (fd === 1 ? io.out : io.err)(merged);
      v.setUint32(nwrittenPtr, total, true);
      return WASI_ERRNO.SUCCESS;
    },

    fd_read(fd, iovs, iovsLen, nreadPtr) {
      if (fd !== 0) return WASI_ERRNO.BADF;
      if (!mem()) return WASI_ERRNO.FAULT;
      const v = view();
      const m = bytes();
      let total = 0;
      for (let i = 0; i < iovsLen && stdinPos < stdin.length; i++) {
        const ptr = v.getUint32(iovs + i * 8, true);
        const len = v.getUint32(iovs + i * 8 + 4, true);
        const n = Math.min(len, stdin.length - stdinPos);
        m.set(stdin.subarray(stdinPos, stdinPos + n), ptr);
        stdinPos += n;
        total += n;
      }
      v.setUint32(nreadPtr, total, true);
      return WASI_ERRNO.SUCCESS;
    },

    fd_close(fd) {
      return fd >= 0 && fd <= 2 ? WASI_ERRNO.SUCCESS : WASI_ERRNO.BADF;
    },

    fd_seek(fd, _offset, _whence, newoffsetPtr) {
      if (fd < 0 || fd > 2) return WASI_ERRNO.BADF;
      if (!mem()) return WASI_ERRNO.FAULT;
      view().setBigUint64(newoffsetPtr, 0n, true);
      return WASI_ERRNO.SPIPE; // stdio descriptors are not seekable
    },

    fd_fdstat_get(fd, ptr) {
      if (fd < 0 || fd > 2) return WASI_ERRNO.BADF;
      if (!mem()) return WASI_ERRNO.FAULT;
      const m = bytes();
      m.fill(0, ptr, ptr + 24);
      const v = view();
      m[ptr] = 2; // fs_filetype: character device
      v.setBigUint64(ptr + 8, fd === 0 ? WASI_RIGHTS_FD_READ : WASI_RIGHTS_FD_WRITE, true);
      v.setBigUint64(ptr + 16, 0n, true);
      return WASI_ERRNO.SUCCESS;
    },

    proc_exit(code) {
      throw new WasiExit(code);
    },

    args_sizes_get(argcPtr, argvBufSizePtr) {
      if (!mem()) return WASI_ERRNO.FAULT;
      const v = view();
      v.setUint32(argcPtr, argv.length, true);
      v.setUint32(argvBufSizePtr, stringTableSize(argv), true);
      return WASI_ERRNO.SUCCESS;
    },

    args_get(argvPtr, bufPtr) {
      if (!mem()) return WASI_ERRNO.FAULT;
      writeStringTable(argvPtr, bufPtr, argv);
      return WASI_ERRNO.SUCCESS;
    },

    environ_sizes_get(countPtr, sizePtr) {
      if (!mem()) return WASI_ERRNO.FAULT;
      const v = view();
      v.setUint32(countPtr, envPairs.length, true);
      v.setUint32(sizePtr, stringTableSize(envPairs), true);
      return WASI_ERRNO.SUCCESS;
    },

    environ_get(envPtr, bufPtr) {
      if (!mem()) return WASI_ERRNO.FAULT;
      writeStringTable(envPtr, bufPtr, envPairs);
      return WASI_ERRNO.SUCCESS;
    },

    clock_time_get(id, _precision, resultPtr) {
      if (!mem()) return WASI_ERRNO.FAULT;
      const nowNs = id === 0
        ? BigInt(Date.now()) * 1_000_000n
        : BigInt(Math.round(performance.now() * 1_000_000));
      view().setBigUint64(resultPtr, nowNs, true);
      return WASI_ERRNO.SUCCESS;
    },

    random_get(bufPtr, len) {
      if (!mem()) return WASI_ERRNO.FAULT;
      const m = bytes();
      const chunk = new Uint8Array(len);
      if (typeof globalThis.crypto?.getRandomValues === 'function') {
        for (let at = 0; at < len; at += 65536) {
          globalThis.crypto.getRandomValues(
            chunk.subarray(at, Math.min(at + 65536, len)));
        }
      } else {
        for (let i = 0; i < len; i++) chunk[i] = Math.floor(Math.random() * 256);
      }
      m.set(chunk, bufPtr);
      return WASI_ERRNO.SUCCESS;
    },

    sched_yield() {
      return WASI_ERRNO.SUCCESS;
    },
  };
}

// ---------------------------------------------------------------------
// Custom shim registration
//
// A shim key is a bare name ("printf", served from the "env"
// module) or "module.name" for any other import module — including
// wasi_snapshot_preview1, whose built-in entries can be overridden
// the same way. Registrations live in Maps keyed by the canonical
// "module.name" form; registering the same key again replaces the
// previous function.
// ---------------------------------------------------------------------

const globalShimRegistry = new Map();

function splitShimKey(key) {
  const dot = key.indexOf('.');
  if (dot < 0) return { module: 'env', name: key };
  return { module: key.slice(0, dot), name: key.slice(dot + 1) };
}

function canonicalShimKey(key) {
  const { module, name } = splitShimKey(key);
  return `${module}.${name}`;
}

function setShimEntry(map, name, fn) {
  if (typeof name !== 'string' || name.length === 0) {
    throw new TinycError('tinyc: shim name must be a non-empty string');
  }
  if (typeof fn !== 'function') {
    throw new TinycError(`tinyc: shim "${name}" must be a function`);
  }
  map.set(canonicalShimKey(name), fn);
}

function shimEntriesOf(layer) {
  if (!layer) return [];
  if (layer instanceof Map) return [...layer.entries()];
  return Object.entries(layer);
}

/** Register a package-global shim: it applies to every later
 *  runWasmModule call and every instance run, until unregistered.
 *  Only the default shims sit below this layer. */
export function registerGlobalShim(name, fn) {
  setShimEntry(globalShimRegistry, name, fn);
}

/** Bulk form of registerGlobalShim: { name: fn, ... }. */
export function registerGlobalShims(shims) {
  for (const [name, fn] of shimEntriesOf(shims)) {
    setShimEntry(globalShimRegistry, name, fn);
  }
}

export function unregisterGlobalShim(name) {
  globalShimRegistry.delete(canonicalShimKey(name));
}

/** The ctx handed to every registered shim as its first argument.
 *  Memory access goes through the holder, which the runner fills
 *  after instantiation and before the entry point runs — so a shim
 *  can only ever be called with the memory in place. */
function createShimContext(holder, byteIO) {
  return {
    memory: () => holder.memory,
    u8: () => new Uint8Array(holder.memory.buffer),
    readCString(ptr, maxLength = 1 << 16) {
      const m = new Uint8Array(holder.memory.buffer);
      let end = ptr;
      while (end < m.length && end - ptr < maxLength && m[end] !== 0) end++;
      // Copy before decoding: a shared memory's views are
      // refused by TextDecoder (real-tier processes).
      return textDecoder.decode(m.slice(ptr, end));
    },
    writeCString(ptr, str) {
      const bytes = textEncoder.encode(str);
      const m = new Uint8Array(holder.memory.buffer);
      m.set(bytes, ptr);
      m[ptr + bytes.length] = 0;
      return bytes.length + 1;
    },
    readBytes(ptr, length) {
      return new Uint8Array(holder.memory.buffer).slice(ptr, ptr + length);
    },
    writeBytes(ptr, data) {
      new Uint8Array(holder.memory.buffer).set(data, ptr);
    },
    io: byteIO,
  };
}

function buildWasmImports(options, holder) {
  const defaults = createDefaultByteIO();
  const byteIO = {
    out: options.io?.out ?? defaults.out,
    err: options.io?.err ?? defaults.err,
    in: options.io?.in,
  };
  const imports = {};
  const put = (moduleName, name, value) => {
    (imports[moduleName] ??= {})[name] = value;
  };
  // Layer 1: built-in WASI shim.
  const wasi = createWasiShim(options, holder, byteIO);
  for (const [name, fn] of Object.entries(wasi)) {
    put('wasi_snapshot_preview1', name, fn);
  }
  // Layer 2: default libc shims (module "env").
  const defaultShims = createDefaultShims({
    io: byteIO,
    holder,
    args: options.args,
    env: options.env,
    stdin: options.stdin,
    heapBase: options.heapBase,
  });
  for (const [moduleName, fns] of Object.entries(defaultShims)) {
    for (const [name, fn] of Object.entries(fns)) put(moduleName, name, fn);
  }
  // Layers 3-5: registered shims — package-global, then instance,
  // then per-run. Each is wrapped so it is called as fn(ctx, ...).
  const ctx = createShimContext(holder, byteIO);
  for (const layer of [globalShimRegistry, options._instanceShims, options.shims]) {
    for (const [key, fn] of shimEntriesOf(layer)) {
      const { module: moduleName, name } = splitShimKey(key);
      put(moduleName, name, (...cArgs) => fn(ctx, ...cArgs));
    }
  }
  // Layer 6: raw imports, as-is.
  for (const [moduleName, fns] of Object.entries(options.imports ?? {})) {
    for (const [name, value] of Object.entries(fns)) put(moduleName, name, value);
  }
  return imports;
}

/** Thrown before instantiation when a wasm product imports symbols
 *  that no import layer provides. `missing` is the full list of
 *  "module.name" strings; the message names the registration routes
 *  that can supply them. */
export class TinycMissingImportError extends TinycError {
  constructor(missing) {
    const list = missing.map((m) => `${m.module}.${m.name}`);
    super(
      `tinyc: wasm module imports are not satisfied: ${list.join(', ')}. ` +
      `Provide them as shims: runWasmModule(bytes, { shims: { name: fn } }), ` +
      `instance.registerShim(name, fn) / instance.registerShims({ ... }), or ` +
      `registerGlobalShim(name, fn); use a "module.name" key for imports ` +
      `outside the default "env" module.`
    );
    this.name = 'TinycMissingImportError';
    this.missing = list;
  }
}

function assertAllImportsSatisfied(module, imports) {
  const missing = [];
  for (const imp of WebAssembly.Module.imports(module)) {
    const provided = imports[imp.module]?.[imp.name];
    const ok =
      imp.kind === 'function'
        ? typeof provided === 'function'
        : provided !== undefined;
    if (!ok) missing.push(imp);
  }
  if (missing.length > 0) throw new TinycMissingImportError(missing);
}

function runWasmInstance(instance, options, holder) {
  holder.memory = instance.exports.memory ?? null;
  holder.instance = instance;
  const exports = instance.exports;
  try {
    if (options.entry !== undefined) {
      const fn = exports[options.entry];
      if (typeof fn !== 'function') {
        throw new TinycError(`tinyc: wasm module does not export "${options.entry}"`);
      }
      return fn(...(options.callArgs ?? options.args ?? []));
    }
    if (typeof exports._start === 'function') {
      exports._start();
      return { exitCode: 0 };
    }
    throw new TinycError(
      'tinyc: wasm module exports no _start; pass options.entry to call an exported function'
    );
  } catch (err) {
    if (err instanceof WasiExit) return { exitCode: err.exitCode };
    if (err instanceof ShimExit) return { exitCode: err.exitCode };
    throw err;
  }
}

/** Instantiate and run a compiled wasm32 product (async variant,
 *  built on WebAssembly.compile). See the section header above for
 *  the options and the entry conventions. */
export async function runWasmModule(bytes, options = {}) {
  const holder = { memory: null, instance: null };
  const imports = buildWasmImports(options, holder);
  const module = bytes instanceof WebAssembly.Module
    ? bytes
    : await WebAssembly.compile(bytes);
  assertAllImportsSatisfied(module, imports);
  const instance = await WebAssembly.instantiate(module, imports);
  return runWasmInstance(instance, options, holder);
}

/** Synchronous variant of runWasmModule (WebAssembly.Module +
 *  WebAssembly.Instance). This is the variant used inside Workers
 *  and by TinycState.run / Tinyc.callMain below. */
export function runWasmModuleSync(bytes, options = {}) {
  const holder = { memory: null, instance: null };
  const imports = buildWasmImports(options, holder);
  const module = bytes instanceof WebAssembly.Module
    ? bytes
    : new WebAssembly.Module(bytes);
  assertAllImportsSatisfied(module, imports);
  const instance = new WebAssembly.Instance(module, imports);
  return runWasmInstance(instance, options, holder);
}

/** Where run paths put the compiled product in the virtual FS when
 *  no output path is given: TCC's own default output name, at the
 *  virtual FS root. */
export const DEFAULT_RUN_OUTPUT = '/a.out';

// Adapt an instance text IO ({ out, err } receiving strings) to the
// byte IO runWasmModule expects, decoding each fd_write chunk.
function textIOToByteIO(io) {
  const outDec = new TextDecoder();
  const errDec = new TextDecoder();
  return {
    out: (chunk) => io.out(outDec.decode(chunk, { stream: true })),
    err: (chunk) => io.err(errDec.decode(chunk, { stream: true })),
  };
}

// ---------------------------------------------------------------------
// Module memory helpers
//
// Passing strings/bytes into a module instance: if the module
// exports malloc (and free), writeModuleData allocates through it
// and the returned free() releases the block. Otherwise the caller
// supplies the write address with options.offset — a number, or a
// function (byteLength) => address for bump-allocator style layouts.
// ---------------------------------------------------------------------

export function writeModuleData(instance, data, options = {}) {
  const memory = instance.exports.memory;
  if (!memory) throw new TinycError('tinyc: writeModuleData: module exports no memory');
  const chunk = typeof data === 'string' ? textEncoder.encode(data) : data;
  const malloc = instance.exports.malloc ?? instance.exports._malloc;
  let ptr;
  let free = null;
  if (typeof malloc === 'function' && options.offset === undefined) {
    ptr = malloc(chunk.length);
    if (!ptr && chunk.length > 0) throw new TinycError('tinyc: writeModuleData: malloc returned null');
    const freeFn = instance.exports.free ?? instance.exports._free;
    if (typeof freeFn === 'function') free = () => freeFn(ptr);
  } else if (options.offset !== undefined) {
    ptr = typeof options.offset === 'function' ? options.offset(chunk.length) : options.offset;
  } else {
    throw new TinycError(
      'tinyc: writeModuleData needs an exported malloc or options.offset'
    );
  }
  new Uint8Array(memory.buffer).set(chunk, ptr);
  return { ptr, length: chunk.length, free };
}

export function readModuleData(instance, ptr, length) {
  const memory = instance.exports.memory;
  if (!memory) throw new TinycError('tinyc: readModuleData: module exports no memory');
  return new Uint8Array(memory.buffer).slice(ptr, ptr + length);
}

export function readModuleCString(instance, ptr, maxLength = 1 << 16) {
  const memory = instance.exports.memory;
  if (!memory) throw new TinycError('tinyc: readModuleCString: module exports no memory');
  const m = new Uint8Array(memory.buffer);
  let end = ptr;
  while (end < m.length && end - ptr < maxLength && m[end] !== 0) end++;
  return textDecoder.decode(m.subarray(ptr, end));
}

// ---------------------------------------------------------------------
// Fine-grained API: one TCCState wrapped as a JS object.
// Method names mirror the libtcc functions they call. A method whose
// export is absent from the native module throws
// TinycMissingExportError naming that export.
// ---------------------------------------------------------------------

export const TCC_OUTPUT = {
  MEMORY: 1,
  EXE: 2,
  OBJ: 3,
  DLL: 4,
  PREPROCESS: 5,
};

export class TinycState {
  constructor(instance, handle) {
    this._instance = instance;
    this._bridge = instance._bridge;
    this.handle = handle;
    this._io = null;
    this._deleted = false;
    this._outputTypeSet = false;
    // Compilation operations recorded for run()'s replay (see
    // _record): only successful operations, and only once
    // createState has finished its own setup.
    this._ops = [];
    this._recording = false;
  }

  /** Record a compilation operation for run()'s replay. run()
   *  re-compiles on a fresh state with the dylink flavor set
   *  before compiling, because the flavor is a
   *  code-generation-time property the original state (compiled
   *  standalone) cannot gain afterwards. Only successful
   *  operations are recorded; createState's own setup (-nostdlib,
   *  include directories) is not, because createState redoes it
   *  on the replay state. */
  _record(op) {
    if (this._recording) this._ops.push(op);
  }

  /** Replay one recorded operation on this state. A recorded
   *  -wasm-dylink option is dropped: the flavor is run()'s to
   *  control, and a stale one must not override it. */
  _replayOp(op) {
    switch (op.kind) {
      case 'options': {
        const kept = op.value.split(/\s+/)
          .filter((t) => t && !t.startsWith('-wasm-dylink')).join(' ');
        if (kept) this.setOptions(kept);
        break;
      }
      case 'includePath': this.addIncludePath(op.value); break;
      case 'sysincludePath': this.addSysincludePath(op.value); break;
      case 'libraryPath': this.addLibraryPath(op.value); break;
      case 'library': this.addLibrary(op.value); break;
      case 'libPath': this.setLibPath(op.value); break;
      case 'define': this.defineSymbol(op.sym, op.value); break;
      case 'undefine': this.undefineSymbol(op.sym); break;
      case 'addFile': this.addFile(op.value); break;
      case 'compileString': this.compileString(op.value); break;
      case 'compileFile': this.compileFile(op.value); break;
      default:
        throw new TinycError(`tinyc: unknown recorded operation ${op.kind}`);
    }
  }

  _call(name, returnType, argTypes, args) {
    if (this._deleted) throw new TinycError('tinyc: state already deleted');
    // The handle goes in as the first argument AND as a leading
    // 'number' in argTypes: the bridge converts args[i] through
    // argTypes[i], so prepending the handle to the values alone
    // shifts every converter one slot left (a 'string' converter
    // lands on the numeric handle, and the real string argument
    // reaches the module unconverted, as 0).
    return this._bridge.call(name, returnType,
      ['number', ...argTypes], [this.handle, ...args]);
  }

  _checkRc(name, rc) {
    if (typeof rc === 'number' && rc < 0) {
      throw new TinycError(`tinyc: ${name} failed with code ${rc}`, rc);
    }
    return rc;
  }

  /** Per-state IO. The error channel takes effect immediately through
   *  tcc_set_error_func; out/in are kept for JS-side helpers because
   *  native stdout routing is per-module (see Tinyc.setIO). */
  setIO(partial) {
    this._io = normalizeIO(partial, this._io ?? this._instance.io);
    this._installErrorFunc();
    return this;
  }

  get io() {
    return this._io ?? this._instance.io;
  }

  _installErrorFunc() {
    if (!this._bridge.has('tcc_set_error_func')) return;
    const io = this.io;
    const errorFunc = (opaque, msg) => {
      io.err(String(msg) + '\n');
    };
    this._errorFunc = errorFunc; // pinned: must outlive the state
    if (this._bridge.mode === 'mock') {
      this._bridge.native.tcc_set_error_func(this.handle, null, errorFunc);
    } else if (this._bridge.mode === 'emscripten') {
      const n = this._bridge.native;
      if (typeof n.addFunction === 'function') {
        const ptr = n.addFunction((opaque, msgPtr) => {
          errorFunc(opaque, n.UTF8ToString(msgPtr));
        }, 'vii');
        this._errorFuncPtr = ptr;
        n.ccall('tcc_set_error_func', null, ['number', 'number', 'number'],
          [this.handle, 0, ptr]);
      }
    }
    // raw mode: function-pointer upcalls need a wasm table slot; the
    // native default error writer already routes through module IO.
  }

  setLibPath(path) {
    this._call('tcc_set_lib_path', null, ['string'], [this._bridge.modulePath(path)]);
    this._record({ kind: 'libPath', value: path });
    return this;
  }

  setOptions(str) {
    const rc = this._checkRc('tcc_set_options',
      this._call('tcc_set_options', 'number', ['string'], [str]));
    this._record({ kind: 'options', value: str });
    return rc;
  }

  addIncludePath(path) {
    const rc = this._checkRc('tcc_add_include_path',
      this._call('tcc_add_include_path', 'number', ['string'], [this._bridge.modulePath(path)]));
    this._record({ kind: 'includePath', value: path });
    return rc;
  }

  addSysincludePath(path) {
    const rc = this._checkRc('tcc_add_sysinclude_path',
      this._call('tcc_add_sysinclude_path', 'number', ['string'], [this._bridge.modulePath(path)]));
    this._record({ kind: 'sysincludePath', value: path });
    return rc;
  }

  addLibraryPath(path) {
    const rc = this._checkRc('tcc_add_library_path',
      this._call('tcc_add_library_path', 'number', ['string'], [this._bridge.modulePath(path)]));
    this._record({ kind: 'libraryPath', value: path });
    return rc;
  }

  addLibrary(name) {
    const rc = this._checkRc('tcc_add_library',
      this._call('tcc_add_library', 'number', ['string'], [name]));
    this._record({ kind: 'library', value: name });
    return rc;
  }

  defineSymbol(sym, value = '1') {
    this._call('tcc_define_symbol', null, ['string', 'string'], [sym, value]);
    this._record({ kind: 'define', sym, value });
    return this;
  }

  undefineSymbol(sym) {
    this._call('tcc_undefine_symbol', null, ['string'], [sym]);
    this._record({ kind: 'undefine', sym });
    return this;
  }

  setOutputType(type) {
    const rc = this._checkRc('tcc_set_output_type',
      this._call('tcc_set_output_type', 'number', ['number'], [type]));
    this._outputTypeSet = true;
    this._outputType = type;
    return rc;
  }

  /* tcc_set_output_type is not a plain setter in libtcc: it also
   * creates the state's ELF sections (tccelf_new), and upstream's
   * contract is that it MUST be called before any compilation —
   * compiling without it runs tccelf_end_file over sections that
   * do not exist and traps inside put_elf_sym. Establish the
   * MEMORY default lazily, exactly once, at the first compiling
   * entry point; an explicit setOutputType beforehand suppresses
   * the default, and tccelf_new is never run twice on one state. */
  _ensureOutputType() {
    if (!this._outputTypeSet) this.setOutputType(TCC_OUTPUT.MEMORY);
  }

  /** Add a file (source, object or archive) by virtual path. The file
   *  must exist in the instance's virtual FS; it is synced to the
   *  native filesystem first when an adapter is mounted. The path
   *  handed to tcc_add_file is the file's location inside the
   *  module FS (bridge.modulePath), which differs from the virtual
   *  path when the OPFS backend is mounted. */
  addFile(path) {
    const p = normalizeVfsPath(path);
    if (this._bridge.mode !== 'mock' && !this._instance.fs.hasFile(p)) {
      throw new TinycError(`tinyc: addFile: ${p} is not in the virtual FS`);
    }
    this._instance.fs.syncFile(p);
    this._ensureOutputType();
    const rc = this._checkRc('tcc_add_file',
      this._call('tcc_add_file', 'number', ['string'], [this._bridge.modulePath(p)]));
    this._record({ kind: 'addFile', value: p });
    return rc;
  }

  compileString(source) {
    this._ensureOutputType();
    const rc = this._checkRc('tcc_compile_string',
      this._call('tcc_compile_string', 'number', ['string'], [source]));
    this._record({ kind: 'compileString', value: source });
    return rc;
  }

  /** Compile a file from the virtual FS: its content is fed through
   *  tcc_compile_string with a #line marker carrying the path, so no
   *  native filesystem access is needed for plain sources. The
   *  marker carries the virtual path exactly as given — it is what
   *  diagnostics show the user — never the file's location inside
   *  the module FS. */
  compileFile(path) {
    const p = normalizeVfsPath(path);
    const src = this._instance.fs.readTextFile(p);
    const marked = `#line 1 "${p}"\n${src}`;
    this._ensureOutputType();
    const rc = this._checkRc('tcc_compile_string',
      this._call('tcc_compile_string', 'number', ['string'], [marked]));
    this._record({ kind: 'compileFile', value: p });
    return rc;
  }

  /** Write the compiled product to a virtual path. TCC writes it
   *  inside the module FS (at bridge.modulePath(path)); exportFile
   *  reads it back from there into the virtual FS. */
  outputFile(path) {
    // A hosted ELF link resolves the C library through the
    // runtime's own -lc addition at output time; libm is the one
    // library TCC leaves to the driver, and the sysroot's stub
    // carries the full math surface, so an executable or shared
    // library link adds it here -- at output time, when the
    // state's library paths are populated and the sysroot's files
    // are in place. Android keeps the dl* functions in a separate
    // libdl, also stubbed in the sysroot; glibc targets resolve
    // them from libc itself.
    if (!this._driverLibsAdded &&
        ELF_TARGETS.has(this._instance.target) &&
        (this._outputType === TCC_OUTPUT.EXE ||
         this._outputType === TCC_OUTPUT.DLL)) {
      this._driverLibsAdded = true;
      this.addLibrary('m');
      if (ANDROID_TARGETS.has(this._instance.target)) {
        this.addLibrary('dl');
      }
    }
    return this._checkRc('tcc_output_file',
      this._call('tcc_output_file', 'number', ['string'],
        [this._bridge.modulePath(normalizeVfsPath(path))]));
  }

  relocate() {
    return this._checkRc('tcc_relocate',
      this._call('tcc_relocate', 'number', [], []));
  }

  getSymbol(name) {
    return this._call('tcc_get_symbol', 'number', ['string'], [name]);
  }

  /** Run the compiled program. Only on the wasm32 target, and not
   *  through the native module: a WASM module cannot instantiate
   *  another module from inside itself, and native TCC provides no
   *  tcc_run for the wasm target. Instead the glue layer writes the
   *  product to a file in the virtual FS (options.outputPath,
   *  default DEFAULT_RUN_OUTPUT), reads its bytes back, and
   *  instantiates it in the host with runWasmModuleSync — the WASI
   *  argv is [outputPath, ...args]. Returns the program's exit code
   *  for a _start module, or the named entry's return value when
   *  options.entry is given. On every other target the glue layer
   *  throws TinycRunNotSupportedError before anything is passed to
   *  the native module — those targets compile to files only.
   *  options: { outputPath, entry, callArgs, env, imports, shims,
   *  stdin, heapBase, io } — entry/callArgs/env/imports/shims/stdin/
   *  heapBase forward to runWasmModuleSync (and entry/callArgs to
   *  the dylink process's runMainModule), and the instance's
   *  registered shims apply automatically; io is a text IO partial
   *  (default: this state's IO) whose out/err receive the program's
   *  fd_write output, decoded. A dylink product runs through a
   *  threaded process, so for it run returns a promise of the
   *  exit code or entry return value (threads design §2.3); a
   *  standalone product returns the value synchronously. */
  run(args = [], options = {}) {
    if (!isRunnableTarget(this._instance.target)) {
      throw new TinycRunNotSupportedError(this._instance.target);
    }
    const outputPath = options.outputPath ?? DEFAULT_RUN_OUTPUT;
    // When the compiler accepts it, the product is built as a
    // dylink main module and run as a process's main module (its
    // exports enter the global symbol table; it can dlopen further
    // modules). A product that is not a dylink unit file runs in
    // its standalone form, as before.
    // The dylink flavor is a code-generation-time property: it
    // must be set before compiling, and this state was compiled
    // standalone, so setting it here would emit a module whose
    // bodies write the immutable __memory_base import. Instead
    // re-compile the recorded operations on a fresh state with
    // the flavor set first and EXE output type set once; the
    // original state is untouched, so a later outputFile on it
    // still produces its standalone module. When the compiler
    // does not accept the option, keep the old behavior.
    if (this._instance._dylinkOptionOk()) {
      const replay = this._instance.createState();
      try {
        replay.setOptions('-wasm-dylink=main');
        replay.setOutputType(TCC_OUTPUT.EXE);
        for (const op of this._ops) replay._replayOp(op);
        replay.outputFile(outputPath);
      } finally {
        replay.delete();
      }
    } else {
      this.setOutputType(TCC_OUTPUT.EXE);
      this.outputFile(outputPath);
    }
    const bytes = this._instance.exportFile(outputPath);
    const io = normalizeIO(options.io, this.io);
    if (isDylinkUnit(bytes)) {
      // The run paths always create threaded processes
      // (threads design §2.1); runMainModule therefore returns
      // a promise, which run propagates.
      const proc = this._instance.createProcess({
        io,
        args: [outputPath, ...args.map(String)],
        env: options.env,
        stdin: options.stdin,
        shims: options.shims,
        imports: options.imports,
        threads: true,
      });
      return Promise.resolve(proc.runMainModule(bytes, {
        path: outputPath,
        entry: options.entry,
        callArgs: options.callArgs,
      })).then((result) => {
        flushIO(io);
        if (result && typeof result === 'object' && 'exitCode' in result) {
          return result.exitCode;
        }
        return result;
      });
    }
    const result = runWasmModuleSync(bytes, {
      args: [outputPath, ...args.map(String)],
      entry: options.entry,
      callArgs: options.callArgs,
      env: options.env,
      imports: options.imports,
      shims: options.shims,
      stdin: options.stdin,
      heapBase: options.heapBase,
      _instanceShims: this._instance._shims,
      io: textIOToByteIO(io),
    });
    flushIO(io);
    if (result && typeof result === 'object' && 'exitCode' in result) {
      return result.exitCode;
    }
    return result;
  }

  delete() {
    if (this._deleted) return;
    this._deleted = true;
    if (this._bridge.has('tcc_delete')) {
      this._bridge.call('tcc_delete', null, [], [this.handle]);
    }
    if (this._errorFuncPtr && typeof this._bridge.native.removeFunction === 'function') {
      this._bridge.native.removeFunction(this._errorFuncPtr);
      this._errorFuncPtr = null;
    }
    this._instance._forgetState(this);
  }

  [Symbol.dispose]() {
    this.delete();
  }
}

// Compiler options whose value is a filesystem path, in the
// separate form ("-o file") and in the joined form ("-I/dir").
// Used only by Tinyc._moduleArgv to relocate path arguments into
// the module FS when the OPFS backend is mounted; option values
// that are not paths (-l names, -D macros, -x languages) are never
// in these tables and pass through untouched.
const PATH_VALUE_OPTIONS = new Set([
  '-o', '-I', '-L', '-B', '-MF',
  '-include', '-isystem', '-idirafter', '-iquote',
  '-iprefix', '-iwithprefix', '-iwithprefixbefore',
]);
const PATH_JOINED_OPTIONS = [
  '-iwithprefixbefore', '-iwithprefix', '-idirafter', '-isystem',
  '-iprefix', '-iquote', '-include', '-I', '-L', '-B', '-MF', '-o',
];

// ---------------------------------------------------------------------
// Instance
// ---------------------------------------------------------------------

export class Tinyc {
  constructor(bridge, { fs, io, target = DEFAULT_TARGET }) {
    this._bridge = bridge;
    this.fs = fs;
    // Session memory cache of dylink library units whose
    // write-back into the virtual FS failed: canonical path ->
    // unit-file envelope text. Every process of this instance
    // shares it (see TinycProcess._libraryMemCache); an entry
    // exists only while the virtual FS does not hold the unit.
    this._libraryMemCache = new Map();
    this._io = io;
    this.target = target;
    this._states = new Set();
    this._mainRunning = false;
    this._shims = new Map();
    this._fsBackend = 'memory';
  }

  get io() {
    return this._io;
  }

  /** True only for the wasm32 target: its compiled output is
   *  WebAssembly, which the browser itself instantiates and runs.
   *  All other targets compile to files only. */
  get canRun() {
    return isRunnableTarget(this.target);
  }

  get nativeMode() {
    return this._bridge.mode;
  }

  /** The backend mounted at "/opfs" in the native module's
   *  filesystem: 'opfs' when loadTinyc mounted the WasmFS OPFS
   *  backend there, 'memory' when it did not — the browser main
   *  thread, a browser without OPFS, and every non-Emscripten
   *  module land on 'memory'. Decided once per module by loadTinyc
   *  (see tryMountOpfsBackend). The module filesystem's root is the
   *  WasmFS memory backend in both cases; with 'opfs', the mirrored
   *  compile working set and every path handed to the native module
   *  are rooted at "/opfs" (bridge.moduleFsPrefix), so the working
   *  set lives on disk. */
  get fsBackend() {
    return this._fsBackend;
  }

  /** Replace the instance IO. out/err are module-level on the native
   *  side (Emscripten print/printErr were bound at load time to the
   *  functions this object delegates to), so swapping the delegates
   *  here reroutes native output for everything created afterwards
   *  and for callMain. */
  setIO(partial) {
    this._io = normalizeIO(partial, this._io);
    return this;
  }

  /** Register a custom shim on this instance: a JS function served
   *  as a wasm import for every later run made through this
   *  instance (state.run, callMain "-run"). `name` is a bare
   *  function name (module "env") or "module.name" for any other
   *  import module — including wasi_snapshot_preview1 entries. The
   *  function is called as fn(ctx, ...cArgs); see the runWasmModule
   *  section for the ctx convention and the layer priority.
   *  Registering the same name again replaces the previous shim. */
  registerShim(name, fn) {
    setShimEntry(this._shims, name, fn);
    return this;
  }

  /** Bulk form of registerShim: { name: fn, ... }. */
  registerShims(shims) {
    for (const [name, fn] of shimEntriesOf(shims)) {
      setShimEntry(this._shims, name, fn);
    }
    return this;
  }

  /** Remove an instance shim previously added with registerShim. */
  unregisterShim(name) {
    this._shims.delete(canonicalShimKey(name));
    return this;
  }

  // -- virtual FS shortcuts (full API on instance.fs) --
  addFile(path, content) { return this.fs.addFile(path, content); }
  addFileSync(path, content) { return this.fs.addFileSync(path, content); }
  addFileFromUrl(path, url, opts) { return this.fs.addFileFromUrl(path, url, opts); }
  registerIncludeDir(path) { return this.fs.registerIncludeDir(path); }
  hasFile(path) { return this.fs.hasFile(path); }
  readFile(path) { return this.fs.readFile(path); }
  readTextFile(path) { return this.fs.readTextFile(path); }
  readFileAsync(path) { return this.fs.readFileAsync(path); }
  readTextFileAsync(path) { return this.fs.readTextFileAsync(path); }
  removeFile(path) { return this.fs.removeFile(path); }
  clearFiles() { return this.fs.clear(); }
  listFiles(prefix) { return this.fs.listFiles(prefix); }

  /** Resolve once the virtual FS's queued store writes and deletes
   *  (from addFileSync / removeFile / clearFiles) have landed in the
   *  persistent store. */
  whenIdle() { return this.fs.whenIdle(); }

  /** Read a compiled product (object file, executable, library —
   *  including a Windows PE for the x86_64-windows target) out of
   *  the virtual FS as bytes. This is how compile-to-file results
   *  leave the browser package, for every target. With an Emscripten
   *  module, a file that only exists in the module FS is read from
   *  there and mirrored into the virtual FS. A file restored from
   *  the persistent store that is not in the content cache falls
   *  back to the module FS copy; when there is none either, the
   *  read fails naming exportFileAsync as the alternative. */
  exportFile(path) {
    const p = normalizeVfsPath(path);
    // The module FS copy is authoritative: it is where TCC writes,
    // and a compile can rewrite a path the virtual FS already holds
    // a mirror of (TinycState.run writes the fixed DEFAULT_RUN_OUTPUT
    // on every call). Read it first and refresh the mirror; only a
    // path the module FS does not have falls back to the mirror.
    const FS = this._bridge.native?.FS;
    if (FS && typeof FS.readFile === 'function') {
      try {
        const bytes = FS.readFile(this._bridge.modulePath(p));
        this.fs.addFileSync(p, bytes);
        return this.fs.readFile(p);
      } catch {
        // not in the module FS: fall through to the virtual FS copy
      }
    }
    if (this.fs.hasFile(p)) {
      try {
        return this.fs.readFile(p);
      } catch {
        throw new TinycError(
          `tinyc: ${p} is persisted in the virtual FS store but is not in ` +
          `the in-memory cache; read it with exportFileAsync() instead`
        );
      }
    }
    throw new TinycError(`tinyc: no such file in virtual FS: ${p}`);
  }

  /** exportFile, reading the content from the persistent store
   *  when it is not in the content cache. */
  async exportFileAsync(path) {
    const p = normalizeVfsPath(path);
    const FS = this._bridge.native?.FS;
    if (FS && typeof FS.readFile === 'function') {
      try {
        const bytes = FS.readFile(this._bridge.modulePath(p));
        this.fs.addFileSync(p, bytes);
        return bytes;
      } catch {
        // not in the module FS: fall through to the virtual FS copy
      }
    }
    if (this.fs.hasFile(p)) return this.fs.readFileAsync(p);
    throw new TinycError(`tinyc: no such file in virtual FS: ${p}`);
  }

  /** exportFile as a Blob, e.g. for a download link or a File
   *  handed to another tool. */
  exportBlob(path, type = 'application/octet-stream') {
    return new Blob([this.exportFile(path)], { type });
  }

  /** Execute a wasm32 product already sitting in the virtual FS —
   *  a file produced by an earlier compile-to-file callMain, or one
   *  placed there with addFile — without recompiling. The bytes are
   *  read with exportFile and instantiated with runWasmModuleSync,
   *  so the shim stack is exactly the one "-run" uses: default libc
   *  shims, package-global shims, this instance's registered shims,
   *  then options.shims and options.imports. options: { args, entry,
   *  callArgs, env, imports, shims, stdin, heapBase, io } — args
   *  defaults to [path]; io is a text IO partial (default: this
   *  instance's IO). Returns the exit code for a _start module, or
   *  the named entry's return value. */
  runWasmFile(path, options = {}) {
    const p = normalizeVfsPath(path);
    const bytes = this.exportFile(p);
    const io = normalizeIO(options.io, this._io);
    const result = runWasmModuleSync(bytes, {
      args: options.args ?? [p],
      entry: options.entry,
      callArgs: options.callArgs,
      env: options.env,
      shims: options.shims,
      imports: options.imports,
      stdin: options.stdin,
      heapBase: options.heapBase,
      _instanceShims: this._shims,
      io: textIOToByteIO(io),
    });
    flushIO(io);
    if (typeof result === 'number') return result;
    return result?.exitCode ?? 0;
  }

  /** Command-line mode: assemble argv exactly like a shell would
   *  (argv[0] filled with the virtual binary path) and call the
   *  exported main / tinyc_main. When the OPFS backend is mounted,
   *  path arguments are relocated under "/opfs" first (see
   *  _moduleArgv). Returns the exit code. main is not
   *  reentrant; a nested call (e.g. from an IO callback) throws.
   *  A "-run" argument on a non-wasm32 target is rejected by the
   *  glue layer with TinycRunNotSupportedError before the native
   *  module is called: that target can only compile to a file here.
   *  On the wasm32 target, "-run" is handled by the glue layer
   *  itself (see _callMainRun): the exported main never sees it.
   *  For "-run", the optional second argument forwards run options
   *  ({ shims, imports, stdin, heapBase }) to that execution. */
  callMain(args = [], options = {}) {
    if (!Array.isArray(args)) throw new TinycError('tinyc: callMain expects an array of arguments');
    if (args.some((a) => a === '-run')) {
      if (!this.canRun) throw new TinycRunNotSupportedError(this.target);
      return this._callMainRun(args.map(String), options);
    }
    if (this._mainRunning) {
      throw new TinycError('tinyc: exported main is not reentrant (callMain already running)');
    }
    const argv = this._moduleArgv(args.map(String));
    this._mainRunning = true;
    try {
      const rc = this._bridge.callMain(argv);
      flushIO(this._io);
      return rc;
    } finally {
      this._mainRunning = false;
    }
  }

  /** Assemble the argv handed to the exported main. argv[0] is the
   *  virtual binary path. When the module FS has no prefix (memory
   *  backend) the arguments pass through exactly as given. When the
   *  OPFS backend is mounted (bridge.moduleFsPrefix set), every
   *  argument that names a file inside the module FS — argv[0], the
   *  values of path options in separate and joined form, --sysroot=
   *  values, and bare input-file arguments — is relocated under the
   *  prefix (bridge.modulePath), where the mirrored files actually
   *  live; all other arguments pass through untouched. */
  _moduleArgv(args) {
    const bridge = this._bridge;
    if (!bridge.moduleFsPrefix) return [VIRTUAL_BIN_PATH, ...args];
    const out = [bridge.modulePath(VIRTUAL_BIN_PATH)];
    let pathValueNext = false;
    for (const arg of args) {
      if (pathValueNext) {
        pathValueNext = false;
        out.push(arg.startsWith('-') ? arg : bridge.modulePath(arg));
        continue;
      }
      if (PATH_VALUE_OPTIONS.has(arg)) {
        pathValueNext = true;
        out.push(arg);
        continue;
      }
      if (arg.startsWith('--sysroot=')) {
        out.push('--sysroot=' + bridge.modulePath(arg.slice('--sysroot='.length)));
        continue;
      }
      const joined = PATH_JOINED_OPTIONS.find(
        (opt) => arg.length > opt.length && arg.startsWith(opt)
      );
      if (joined) {
        out.push(joined + bridge.modulePath(arg.slice(joined.length)));
        continue;
      }
      // A bare argument in compiler mode is an input file.
      if (!arg.startsWith('-') && arg.length > 0) {
        out.push(bridge.modulePath(arg));
        continue;
      }
      out.push(arg);
    }
    return out;
  }

  /** wasm32 "-run": the arguments before "-run" are compiler
   *  options, the one after it is the source file, and the rest are
   *  the program's arguments. The source is compiled to
   *  DEFAULT_RUN_OUTPUT through the exported main (without "-run"),
   *  then the product bytes are instantiated and executed in the
   *  host by runWasmModuleSync, with the program arguments fed to
   *  _start through the WASI args shim. The instance's registered
   *  shims apply; options.shims / options.imports / options.stdin /
   *  options.heapBase forward per-run. Returns the compiler's exit
   *  code when compilation fails, otherwise the program's. */
  _callMainRun(args, options = {}) {
    const runIdx = args.indexOf('-run');
    const compileArgs = args.slice(0, runIdx);
    const [src, ...progArgs] = args.slice(runIdx + 1);
    if (!src) {
      throw new TinycError('tinyc: callMain: "-run" must be followed by a source file');
    }
    const dylink = this._dylinkOptionOk();
    const rc = this.callMain([
      ...compileArgs,
      ...(dylink ? ['-wasm-dylink=main'] : []),
      '-o', DEFAULT_RUN_OUTPUT, src,
    ]);
    if (rc !== 0) return rc;
    const bytes = this.exportFile(DEFAULT_RUN_OUTPUT);
    if (dylink && isDylinkUnit(bytes)) {
      // The run paths always create threaded processes
      // (threads design §2.1); runMainModule therefore returns
      // a promise, which callMain propagates.
      const proc = this.createProcess({
        io: this._io,
        args: [src, ...progArgs],
        shims: options.shims,
        imports: options.imports,
        stdin: options.stdin,
        threads: true,
      });
      return Promise.resolve(
        proc.runMainModule(bytes, { path: DEFAULT_RUN_OUTPUT })).then((result) => {
          flushIO(this._io);
          if (typeof result === 'number') return result;
          return result?.exitCode ?? 0;
        });
    }
    const result = runWasmModuleSync(bytes, {
      args: [src, ...progArgs],
      shims: options.shims,
      imports: options.imports,
      stdin: options.stdin,
      heapBase: options.heapBase,
      _instanceShims: this._shims,
      io: textIOToByteIO(this._io),
    });
    flushIO(this._io);
    if (typeof result === 'number') return result;
    return result?.exitCode ?? 0;
  }

  /** True when this instance's compiler accepts the
   *  -wasm-dylink=main option: probed once per instance on a
   *  throwaway state. A compiler without the option rejects it;
   *  the run paths then keep their standalone form. A product is
   *  only run as a dylink main module when it actually is a
   *  dylink unit file, which the run paths check on the bytes
   *  themselves. */
  _dylinkOptionOk() {
    if (this._dylinkOk !== undefined) return this._dylinkOk;
    let ok = false;
    try {
      const probe = this.createState();
      try {
        probe.setOptions('-wasm-dylink=main');
        ok = true;
      } catch {
        ok = false;
      }
      probe.delete();
    } catch {
      ok = false;
    }
    this._dylinkOk = ok;
    return ok;
  }

  /** Create a dylink process attached to this instance: libraries
   *  resolve in this instance's virtual FS and the instance's
   *  registered shims apply to its modules. */
  createProcess(options = {}) {
    return new TinycProcess({ ...options, tinyc: this });
  }

  /** Fine-grained mode: create a TCCState, pre-loaded with the
   *  virtual FS include directories (registration order, ahead of
   *  the sysroot) and the instance error routing. */
  createState() {
    this._bridge.requireExport('tcc_new');
    const handle = this._bridge.call('tcc_new', 'number', [], []);
    if (!handle && this._bridge.mode !== 'mock') {
      throw new TinycError('tinyc: tcc_new returned null');
    }
    const state = new TinycState(this, handle ?? {});
    this._states.add(state);
    state._installErrorFunc();
    if (isRunnableTarget(this.target)) {
      // The wasm target is freestanding: no crt1.o/crti.o exist for
      // it anywhere, and this package's run model enters the product
      // through a named export, never through a C runtime startup.
      // Without -nostdlib, tcc_set_output_type for any non-MEMORY
      // type fails inside tccelf_add_crtbegin ('crt1.o not found')
      // and poisons the state's error count, which also makes the
      // next compile report failure. The backend's own test suite
      // drives the target with -nostdlib throughout; mirror that as
      // the state default here. Other targets keep the hosted
      // default: their EXE output does link the C runtime.
      state.setOptions('-nostdlib');
    }
    if (this._bridge.has('tcc_add_include_path')) {
      for (const dir of this.fs.getIncludeDirs()) state.addIncludePath(dir);
    }
    // Setup is done; compilation operations from here on are
    // recorded for run()'s replay.
    state._recording = true;
    return state;
  }

  _forgetState(state) {
    this._states.delete(state);
  }

  /** Convenience: compile one source string in a throwaway state. */
  compileString(source, { outputType = TCC_OUTPUT.MEMORY } = {}) {
    const state = this.createState();
    try {
      state.setOutputType(outputType);
      return state.compileString(source);
    } finally {
      state.delete();
    }
  }
}

// ---------------------------------------------------------------------
// Dylink processes
//
// A process is the unit a set of dylink modules runs in (design:
// wasm-dylink-design.md): one shared linear memory, one shared
// function table, a module registry, a global symbol table and one
// GOT global per cross-module symbol. Modules are base-relative —
// they import their memory base and table base — so the linker
// assigns both at load time. A module's instance, memory region and
// table slots are retained for the process's whole life; dlclose is
// bookkeeping, never unloading.
//
// A library is resolved per library (design §13): its unit file —
// the §3.4 metadata plus the module body as a base64 field, in the
// §14 envelope — is one file at the library's canonical location
// (/sysroot/<path> for a relative path), whatever produced it.
// Local copies live in the attached instance's virtual FS (or come
// from a standalone process's readModule callback); a miss is
// retrieved through the process's transport: the fetch transport
// reads the unit file as text and parses it, the script transport
// — file: pages — injects it as a script element and takes the
// registered manifest object.
// ---------------------------------------------------------------------

export class TinycDylinkError extends TinycError {
  constructor(message) {
    super(message);
    this.name = 'TinycDylinkError';
  }
}

/** The dylink ABI's own RTLD_* values (dlfcn.h). */
export const RTLD_LAZY = 1;
export const RTLD_NOW = 2;
export const RTLD_GLOBAL = 4;
export const RTLD_LOCAL = 8;
const RTLD_KNOWN = RTLD_LAZY | RTLD_NOW | RTLD_GLOBAL | RTLD_LOCAL;

// SHA-256 over a Uint8Array, as lowercase hex. Synchronous on
// purpose: manifest hash verification also runs on the guest
// dlopen path, which cannot await SubtleCrypto.
const SHA256_K = [
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
];
function sha256Hex(bytes) {
  const rotr = (x, n) => (x >>> n) | (x << (32 - n));
  let h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a;
  let h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;
  const bitLen = bytes.length * 8;
  const padded = new Uint8Array((((bytes.length + 9) + 63) >> 6) << 6);
  padded.set(bytes);
  padded[bytes.length] = 0x80;
  const pv = new DataView(padded.buffer);
  pv.setUint32(padded.length - 4, bitLen >>> 0);
  pv.setUint32(padded.length - 8, Math.floor(bitLen / 2 ** 32));
  const w = new Int32Array(64);
  for (let block = 0; block < padded.length; block += 64) {
    for (let t = 0; t < 16; t++) w[t] = pv.getInt32(block + t * 4);
    for (let t = 16; t < 64; t++) {
      const s0 = rotr(w[t - 15], 7) ^ rotr(w[t - 15], 18) ^ (w[t - 15] >>> 3);
      const s1 = rotr(w[t - 2], 17) ^ rotr(w[t - 2], 19) ^ (w[t - 2] >>> 10);
      w[t] = (w[t - 16] + s0 + w[t - 7] + s1) | 0;
    }
    let a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;
    for (let t = 0; t < 64; t++) {
      const s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const ch = (e & f) ^ (~e & g);
      const t1 = (h + s1 + ch + SHA256_K[t] + w[t]) | 0;
      const s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const maj = (a & b) ^ (a & c) ^ (b & c);
      const t2 = (s0 + maj) | 0;
      h = g; g = f; f = e; e = (d + t1) | 0;
      d = c; c = b; b = a; a = (t1 + t2) | 0;
    }
    h0 = (h0 + a) | 0; h1 = (h1 + b) | 0; h2 = (h2 + c) | 0; h3 = (h3 + d) | 0;
    h4 = (h4 + e) | 0; h5 = (h5 + f) | 0; h6 = (h6 + g) | 0; h7 = (h7 + h) | 0;
  }
  return [h0, h1, h2, h3, h4, h5, h6, h7]
    .map((x) => (x >>> 0).toString(16).padStart(8, '0')).join('');
}

function base64ToBytes(b64) {
  const bin = atob(String(b64).replace(/\s+/g, ''));
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out;
}

function bytesToBase64(bytes) {
  let bin = '';
  const CHUNK = 0x8000;
  for (let i = 0; i < bytes.length; i += CHUNK) {
    bin += String.fromCharCode(...bytes.subarray(i, i + CHUNK));
  }
  return btoa(bin);
}

// The manifest envelope (design §14): a manifest file is a text
// file whose fixed form both serves over http(s) and executes as a
// classic script, registering the manifest object under the
// library's canonical path.
const DYLINK_REGISTRY = '__TINYC_DYLINK_REGISTRY__';

function parseManifestEnvelope(text, where) {
  const start = text.indexOf('{');
  const end = text.lastIndexOf('}');
  if (start < 0 || end <= start) {
    throw new TinycDylinkError(
      `tinyc: manifest at ${where} is not in the envelope form`);
  }
  try {
    return JSON.parse(text.slice(start, end + 1));
  } catch (err) {
    throw new TinycDylinkError(
      `tinyc: manifest at ${where} does not parse: ${err.message}`);
  }
}

function manifestEnvelopeText(canonical, manifest) {
  return `globalThis.${DYLINK_REGISTRY} ??= Object.create(null);\n` +
    `globalThis.${DYLINK_REGISTRY}[${JSON.stringify(canonical)}] = ${JSON.stringify(manifest)};\n`;
}

/** Validate a manifest object, decode its body and verify the
 *  recorded hash over the decoded bytes. Returns the library in the
 *  loader's internal shape: { meta, bodyBytes }. */
function manifestToLibrary(manifest, canonical) {
  if (!manifest || typeof manifest !== 'object' || Array.isArray(manifest)) {
    throw new TinycDylinkError(`tinyc: manifest for "${canonical}" is not an object`);
  }
  if (manifest.version !== 1) {
    throw new TinycDylinkError(
      `tinyc: manifest for "${canonical}" has unsupported version ${manifest.version}`);
  }
  if (typeof manifest.body !== 'string') {
    throw new TinycDylinkError(`tinyc: manifest for "${canonical}" carries no body`);
  }
  const bodyBytes = base64ToBytes(manifest.body);
  if (typeof manifest.hash === 'string' && sha256Hex(bodyBytes) !== manifest.hash) {
    throw new TinycDylinkError(
      `tinyc: manifest for "${canonical}": body fails its recorded SHA-256 hash`);
  }
  const meta = {
    version: 1,
    flavor: manifest.flavor | 0,
    memSize: manifest.mem_size | 0,
    memAlign: manifest.mem_align || 16,
    bssOff: manifest.bss_off | 0,
    bssSize: manifest.bss_size | 0,
    tableSize: manifest.table_size | 0,
    // The threaded revision's fields (threads design §3.2,
    // §7.2): stackSize is undefined on a pre-revision module,
    // which a threaded process refuses by name; the tls fields
    // are present only when the module has __thread variables.
    stackSize: typeof manifest.stack_size === 'number'
      ? manifest.stack_size | 0 : undefined,
    tlsSize: manifest.tls_size | 0,
    tlsAlign: manifest.tls_align || 4,
    tlsTemplate: typeof manifest.tls_template === 'string'
      ? base64ToBytes(manifest.tls_template) : null,
    needed: Array.isArray(manifest.needed) ? manifest.needed.map(String) : [],
    dataGotFunc: Array.isArray(manifest.data_got_func)
      ? manifest.data_got_func.map(String) : [],
    exports: (Array.isArray(manifest.exports) ? manifest.exports : [])
      .map((e) => ({
        name: String(e.name), kind: e.kind | 0, value: e.value | 0,
        tls: e.tls ? 1 : 0,
      })),
  };
  if (!(meta.memSize > 0)) {
    throw new TinycDylinkError(
      `tinyc: manifest for "${canonical}" has an invalid layout (mem_size)`);
  }
  return { meta, bodyBytes };
}

/** A library from the bytes of its unit file (the manifest
 *  envelope text). Every loadable library — compiler-produced or
 *  translated — has this one form. Throws TinycDylinkError naming
 *  the reason when the bytes are not a valid unit file. */
function libraryFromBytes(canonical, bytes) {
  const manifest = parseManifestEnvelope(textDecoder.decode(bytes), `"${canonical}"`);
  const lib = manifestToLibrary(manifest, canonical);
  lib.manifest = manifest;
  return lib;
}

/** True when the bytes are a valid dylink unit file. */
function isDylinkUnit(bytes) {
  try {
    libraryFromBytes('/probe', bytes);
    return true;
  } catch {
    return false;
  }
}

/** The transport selection rule (design §14): a file: context uses
 *  the script transport, anything else uses fetch. An explicit
 *  "fetch" / "script" option overrides. */
function createTransport(option) {
  let kind = option ?? 'auto';
  if (kind === 'auto') {
    kind = 'fetch';
    try {
      if (typeof location !== 'undefined' && location.protocol === 'file:' &&
          typeof document !== 'undefined') {
        kind = 'script';
      }
    } catch { /* no location: fetch */ }
  }
  if (kind !== 'fetch' && kind !== 'script') {
    throw new TinycError(
      `tinyc: transport must be "auto", "fetch" or "script", not "${option}"`);
  }
  return {
    kind,
    /** Retrieve the manifest object served at `url` for the library
     *  whose canonical path (and registry key) is `canonical`. */
    async retrieveManifest(url, canonical) {
      if (kind === 'script') {
        const reg = globalThis[DYLINK_REGISTRY];
        if (reg && reg[canonical] !== undefined) return reg[canonical];
        if (typeof document === 'undefined' || !document.head) {
          throw new TinycDylinkError(
            `tinyc: the script transport needs a page main thread; cannot retrieve ${url}`);
        }
        const before = new Set(Object.keys(globalThis[DYLINK_REGISTRY] ?? {}));
        await new Promise((resolve, reject) => {
          const el = document.createElement('script');
          el.src = url;
          el.async = true;
          el.onload = () => { el.remove(); resolve(); };
          el.onerror = () => {
            el.remove();
            reject(new TinycDylinkError(
              `tinyc: cannot load library "${canonical}" from ${url} (script transport)`));
          };
          document.head.appendChild(el);
        });
        const registry = globalThis[DYLINK_REGISTRY] ?? {};
        if (registry[canonical] !== undefined) return registry[canonical];
        // A unit file registers under the name its producer gave
        // it (a compiler names its output file); the registration
        // the injected file just made is the manifest for this
        // library, and is recorded under the canonical path.
        const fresh = Object.keys(registry).filter((k) => !before.has(k));
        if (fresh.length === 1) {
          registry[canonical] = registry[fresh[0]];
          return registry[canonical];
        }
        throw new TinycDylinkError(
          `tinyc: manifest at ${url} did not register "${canonical}"`);
      }
      let text;
      try {
        const res = await fetch(url);
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        text = await res.text();
      } catch (err) {
        throw new TinycDylinkError(
          `tinyc: cannot load library "${canonical}" from ${url}: ${err.message}`);
      }
      return parseManifestEnvelope(text, url);
    },
  };
}

/* The compiler's own assets travel through the same transport as
 * library unit files (design §14): every asset arrives as an
 * envelope file — a classic script that registers its payload in
 * this registry — the compiler envelope under the key `tcc-<target>`
 * (the glue source text plus the module bytes as base64) and the
 * sysroot envelope under `sysroot-<target>` (the sysroot files'
 * bytes as base64, keyed by manifest source).
 * build-asset-envelopes.mjs generates both. The fetch transport
 * reads the envelope as text and parses the payload out of it
 * without executing it; the script transport injects it and reads
 * the registry. */
const ASSET_REGISTRY = '__TINYC_ASSET_REGISTRY__';

function assetRegistry() {
  return (globalThis[ASSET_REGISTRY] ??= Object.create(null));
}

/** Parse an asset envelope's payload out of its file text: the
 *  envelope is a classic script registering a JSON payload; the
 *  payload is the text between the first "{" and the last "}",
 *  read without executing the file. The one parse path, shared by
 *  the transport retrieval and the loadModule hook. */
function parseAssetEnvelopeText(text, url) {
  const start = text.indexOf('{');
  const end = text.lastIndexOf('}');
  if (start < 0 || end <= start) {
    throw new TinycError(
      `tinyc: asset envelope at ${url} is not in the envelope form`);
  }
  try {
    return JSON.parse(text.slice(start, end + 1));
  } catch (err) {
    throw new TinycError(
      `tinyc: asset envelope at ${url} does not parse: ${err.message}`);
  }
}

/** Retrieve one registered asset payload through the transport. The
 *  fetch side reads the envelope file as text and parses the
 *  registered object out of it without executing it, the same rule
 *  as for library manifests; the script side injects the envelope
 *  as a classic script and reads the registry. Returns null when
 *  the envelope carries no entry under `key`. A caller-supplied
 *  fetchImpl, when given, performs the fetch side's request. */
async function retrieveAsset(transport, url, key, fetchImpl) {
  const registry = assetRegistry();
  if (registry[key] !== undefined) return registry[key];
  if (transport.kind === 'fetch') {
    let text;
    try {
      const response = await (fetchImpl ?? globalThis.fetch)(url);
      if (!response.ok) {
        throw new TinycError(
          `tinyc: cannot load asset "${key}" from ${url}: HTTP ${response.status}`);
      }
      text = await response.text();
    } catch (err) {
      if (err instanceof TinycError) throw err;
      throw new TinycError(
        `tinyc: cannot load asset "${key}" from ${url}: ${err.message}`);
    }
    const payload = parseAssetEnvelopeText(text, url);
    registry[key] = payload;
    return payload;
  }
  const doc = globalThis.document;
  if (!doc?.createElement || typeof doc.createElement !== 'function') {
    throw new TinycError(
      `tinyc: the script transport needs a page main thread; cannot retrieve ${url}`);
  }
  await new Promise((resolve, reject) => {
    const el = doc.createElement('script');
    el.src = url;
    el.onload = () => { el.remove(); resolve(); };
    el.onerror = () => {
      el.remove();
      reject(new TinycError(
        `tinyc: cannot load asset "${key}" from ${url} (script transport)`));
    };
    doc.head.appendChild(el);
  });
  return registry[key] ?? null;
}

/** A module path's canonical virtual-FS location: an absolute path
 *  is used as given; a relative path resolves against `baseDir`
 *  (/sysroot for dlopen and host loads, the naming manifest's
 *  directory for needed entries). */
function canonicalModulePath(path, baseDir = '/sysroot') {
  if (typeof path !== 'string' || path.length === 0) {
    throw new TinycDylinkError('tinyc: module path must be a non-empty string');
  }
  return path.startsWith('/')
    ? normalizeVfsPath(path)
    : normalizeVfsPath(`${baseDir}/${path}`);
}

function vfsDirname(p) {
  const i = p.lastIndexOf('/');
  return i <= 0 ? '/' : p.slice(0, i);
}

// A funcref table holds only wasm functions — a JS function cannot
// be stored in one. A JS function that has to be reachable through
// the table (a shim whose address is taken, a trap stub) therefore
// goes through a generated wasm wrapper of the function's type:
// a one-function module that imports the JS function and re-exports
// it. The type comes from the import entry of a module that
// imports the symbol, parsed from the module's own type and
// import sections; address-taken uses in a module share the C
// signature of its import of the same name.
const VALTYPE_NAMES = {
  0x7f: 'i32', 0x7e: 'i64', 0x7d: 'f32', 0x7c: 'f64',
  0x7b: 'v128', 0x70: 'funcref', 0x6f: 'externref',
};

function parseImportTypes(bytes) {
  const out = new Map();
  if (bytes.length < 8) return out;
  let p = 8;
  const uleb = () => {
    let v = 0, s = 0;
    for (;;) {
      const c = bytes[p++];
      v += (c & 0x7f) * 2 ** s;
      if (!(c & 0x80)) return v;
      s += 7;
    }
  };
  const str = () => {
    const n = uleb();
    const t = textDecoder.decode(bytes.subarray(p, p + n));
    p += n;
    return t;
  };
  let types = [];
  while (p < bytes.length) {
    const id = bytes[p++];
    const size = uleb();
    const end = p + size;
    if (id === 1) {
      const n = uleb();
      for (let i = 0; i < n; i++) {
        p++; // 0x60
        const np = uleb();
        const params = [];
        for (let j = 0; j < np; j++) params.push(VALTYPE_NAMES[bytes[p++]] ?? 'i32');
        const nr = uleb();
        const results = [];
        for (let j = 0; j < nr; j++) results.push(VALTYPE_NAMES[bytes[p++]] ?? 'i32');
        types.push({ params, results });
      }
    } else if (id === 2) {
      const n = uleb();
      for (let i = 0; i < n; i++) {
        const m = str();
        const name = str();
        const kind = bytes[p++];
        if (kind === 0) {
          const t = types[uleb()];
          if (t) out.set(`${m}.${name}`, t);
        } else if (kind === 1 || kind === 3) {
          p++; // limits flag (+ memory/table descriptor byte)
          if (kind === 3) p++;
        } else if (kind === 2) {
          p++;
          if (bytes[p - 1] & 1) p++;
        }
      }
    }
    p = end;
  }
  return out;
}

const VALTYPE_BYTES = { i32: 0x7f, i64: 0x7e, f32: 0x7d, f64: 0x7c, v128: 0x7b, funcref: 0x70, externref: 0x6f };
const DEFAULT_CALL_TYPE = { params: ['i32', 'i32', 'i32', 'i32'], results: ['i32'] };

function ulebBytes(v) {
  const out = [];
  do {
    let c = v % 128;
    v = Math.floor(v / 128);
    if (v) c |= 0x80;
    out.push(c);
  } while (v);
  return out;
}

/** Widen a module's memory import declaration to shared, for
 *  instantiation into a shared-memory (real-tier) process.
 *  Module bytes do not record the tier (threads design §2.1),
 *  and the backend declares the dylink memory import
 *  non-shared; the import's limits are rewritten here — flags
 *  gain the shared bit and a maximum of 65536 pages, matching
 *  the process memory — and every other byte is preserved.
 *  Returns the input unchanged when there is no memory import
 *  or it is already shared. */
function patchSharedMemoryImport(bytes) {
  const readUleb = (p) => {
    let v = 0, s = 0;
    for (;;) {
      const c = bytes[p++];
      v += (c & 0x7f) * 2 ** s;
      if (!(c & 0x80)) return [v, p];
      s += 7;
    }
  };
  let p = 8; // magic + version
  while (p < bytes.length) {
    const id = bytes[p];
    const [size, bodyStart] = readUleb(p + 1);
    if (id !== 2) { p = bodyStart + size; continue; }
    // Import section: find the memory import.
    let q = bodyStart;
    const [count, afterCount] = readUleb(q);
    q = afterCount;
    for (let i = 0; i < count; i++) {
      const [mlen, mp] = readUleb(q);
      const mod = textDecoder.decode(bytes.subarray(mp, mp + mlen));
      const [nlen, np] = readUleb(mp + mlen);
      const name = textDecoder.decode(bytes.subarray(np, np + nlen));
      const kindAt = np + nlen;
      const kind = bytes[kindAt];
      if (kind === 0x02 && name === 'memory') {
        const [flags, minAt] = readUleb(kindAt + 1);
        if (flags & 0x02) return bytes; // already shared
        const [min, afterMin] = readUleb(minAt);
        let entryEnd = afterMin;
        if (flags & 0x01) entryEnd = readUleb(afterMin)[1];
        const newEntry = [
          ...ulebBytes(mod.length), ...textEncoder.encode(mod),
          ...ulebBytes(name.length), ...textEncoder.encode(name),
          0x02, 0x03, ...ulebBytes(min), ...ulebBytes(65536),
        ];
        const sectionBody = [
          ...bytes.subarray(bodyStart, q),
          ...newEntry,
          ...bytes.subarray(entryEnd, bodyStart + size),
        ];
        return new Uint8Array([
          ...bytes.subarray(0, p),
          2, ...ulebBytes(sectionBody.length), ...sectionBody,
          ...bytes.subarray(bodyStart + size),
        ]);
      }
      // Skip this entry.
      let e = kindAt + 1;
      if (kind === 0x00) {
        e = readUleb(e)[1];                    // function: type index
      } else if (kind === 0x01) {
        e += 1;                                // table: element type
        const [f, m1] = readUleb(e);           //   then limits
        e = readUleb(m1)[1];
        if (f & 0x01) e = readUleb(e)[1];
      } else if (kind === 0x02) {
        const [f, m1] = readUleb(e);           // memory: limits
        e = readUleb(m1)[1];
        if (f & 0x01) e = readUleb(e)[1];
      } else if (kind === 0x03) {
        e += 2;                                // global: type + mutability
      }
      q = e;
    }
    return bytes; // no memory import
  }
  return bytes;
}

function makeWrapperFunction(jsFn, type) {
  const section = (id, body) => [id, ...ulebBytes(body.length), ...body];
  const vec = (items) => [...ulebBytes(items.length), ...items.flat()];
  const typeBody = [
    0x60,
    ...vec(type.params.map((t) => [VALTYPE_BYTES[t] ?? 0x7f])),
    ...vec(type.results.map((t) => [VALTYPE_BYTES[t] ?? 0x7f])),
  ];
  const name = (s) => [...ulebBytes(s.length), ...[...s].map((c) => c.charCodeAt(0))];
  const importBody = [...name('t'), ...name('f'), 0x00, 0x00];
  const codeBody = [
    0x00, // no locals
    ...type.params.flatMap((_, i) => [0x20, ...ulebBytes(i)]),
    0x10, 0x00, // call 0
    0x0b,
  ];
  const bytes = new Uint8Array([
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    ...section(1, vec([typeBody])),
    ...section(2, vec([importBody])),
    ...section(3, vec([[0x00]])),
    ...section(7, vec([[...name('w'), 0x00, 0x01]])),
    ...section(10, vec([[...ulebBytes(codeBody.length), ...codeBody]])),
  ]);
  const inst = new WebAssembly.Instance(new WebAssembly.Module(bytes), { t: { f: jsFn } });
  return inst.exports.w;
}

// The table's slot 1: a plain wasm trap for calls through a null
// function pointer.
const NULL_STUB_FN = (() => {
  const bytes = new Uint8Array([
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
    0x03, 0x02, 0x01, 0x00,
    0x07, 0x05, 0x01, 0x01, 0x74, 0x00, 0x00,
    0x0a, 0x05, 0x01, 0x03, 0x00, 0x00, 0x0b,
  ]);
  return new WebAssembly.Instance(new WebAssembly.Module(bytes), {}).exports.t;
})();

/** The real-tier probe (threads design §2.1): SharedArrayBuffer
 *  available, a shared memory actually constructible (the
 *  authoritative test), and a Worker constructor to host the
 *  threads. A file: context fails the first two by construction
 *  and is therefore always simulated — a consequence of the
 *  probe, not a special case. */
function probeRealTier() {
  if (typeof SharedArrayBuffer !== 'function') {
    return { ok: false, reason: 'SharedArrayBuffer is not available in this context' };
  }
  try {
    new WebAssembly.Memory({ initial: 1, maximum: 65536, shared: true });
  } catch (err) {
    return {
      ok: false,
      reason: `a shared WebAssembly.Memory cannot be constructed (${err.message})`,
    };
  }
  if (typeof Worker !== 'function') {
    return { ok: false, reason: 'the Worker constructor is not available in this context' };
  }
  return { ok: true };
}

/** Real-tier shared-memory control block word offsets (bytes)
 *  and thread-record word offsets (threads design §5). */
const CB_LOAD_SEQ = 0, CB_ACKS_DONE = 4, CB_LOAD_DONE = 8,
  CB_TASK_ADDR = 12, CB_CALL_SEQ = 16, CB_CALL_TASK_ADDR = 20,
  CB_CALL_DONE = 24, CB_STDIN_CURSOR = 28, CB_STDIN_ADDR = 32,
  CB_STDIN_LEN = 36, CB_LOAD_FAILED = 40;
const TR_STATE = 0, TR_EXIT = 4, TR_ACK = 8, TR_DETACHED = 12;
/** The thread-record directory (threads design §6.6): a
 *  block table of DIR_BLOCKS pointers; each block holds
 *  DIR_BLOCK_TIDS record addresses (32 KiB), reserved, zeroed
 *  and published by CAS on first use. The arithmetic cap is
 *  DIR_BLOCKS * DIR_BLOCK_TIDS = 33,554,432 cumulative
 *  thread ids. */
const DIR_BLOCK_TIDS = 8192, DIR_BLOCKS = 4096;
/** The host-call proxy block (threads design §5.7), one per
 *  thread in shared memory; the layout is shared with the
 *  worker-side stub in threads.mjs. */
const PX_STATE = 0, PX_GEN = 4, PX_STATUS = 8, PX_NARGS = 12,
  PX_RESTYPE = 16, PX_ARGTYPES = 20, PX_NAMELEN = 36,
  PX_NAME = 40, PX_ERRLEN = 168, PX_ERR = 172, PX_ARGS = 688,
  PX_RESULT = 816;
const PX_ERR_MAX = 512;
const PX_DONE = 2, PX_OK = 0, PX_ERROR = 1;
/** The thread-region allocator (threads design §3.1):
 *  exact-size free lists over a class table in shared
 *  memory, operated under the allocator spin lock — the
 *  header word at [520, 524) (§5.6). */
const REGION_CLASSES = 64;
const REGION_TABLE_SIZE = 16 + REGION_CLASSES * 16;
const REGION_ZERO_MAX = 4096;
const ALLOC_LOCK_WORD = 520;

/** The pthread shims that can block, supplied to a threaded
 *  simulated-tier process's modules wrapped in
 *  WebAssembly.Suspending (threads design §4.2). */
const SIM_BLOCKING_SHIMS = new Set([
  'pthread_join', 'pthread_mutex_lock', 'pthread_mutex_timedlock',
  'pthread_cond_wait', 'pthread_cond_timedwait', 'pthread_once',
  'pthread_spin_lock', 'sched_yield',
]);

/** A dylink process. Created attached to a Tinyc instance
 *  (instance.createProcess: libraries resolve in the instance's
 *  virtual FS, retrievals write back into it, the instance's
 *  registered shims apply) or standalone (createProcess: libraries
 *  resolve through options.readModule(canonicalPath) -> bytes and
 *  through retrieval from options.baseUrl).
 *
 *  options: { io, args, env, stdin, shims, imports, baseUrl,
 *  transport, readModule, threads, threadTier } — io is a text
 *  IO partial like the instance IO; args/env/stdin seed the
 *  process's WASI and libc layers; shims/imports are extra
 *  import layers with the runWasmModule priority (after the
 *  built-in layers, before symbol resolution); transport is
 *  "auto" | "fetch" | "script". threads defaults to false (the
 *  0.4.0 embedding contract: synchronous, creating-context
 *  execution); the run paths pass threads: true. threadTier
 *  may force 'simulated', or 'real' where the probe passes
 *  (threads design §2.1). */
export class TinycProcess {
  constructor(options = {}) {
    this._tinyc = options.tinyc ?? null;
    this._readModule = typeof options.readModule === 'function'
      ? options.readModule : null;
    // -- threading (threads design §2) --
    this._threaded = options.threads === true;
    this._jspi = typeof WebAssembly.promising === 'function' &&
      typeof WebAssembly.Suspending === 'function';
    this.threadTier = null;
    this._scheduler = null;
    if (this._threaded) {
      const probe = probeRealTier();
      const forced = options.threadTier ?? null;
      if (forced === 'real' && !probe.ok) {
        throw new TinycDylinkError(
          `tinyc: cannot create a real-tier threaded process: ${probe.reason}`);
      }
      if (forced !== null && forced !== 'real' && forced !== 'simulated') {
        throw new TinycDylinkError(
          `tinyc: threadTier must be "real" or "simulated", got "${forced}"`);
      }
      this.threadTier = forced ?? (probe.ok ? 'real' : 'simulated');
    }
    // A relative baseUrl resolves against the current context's
    // directory, like the package's other asset URLs.
    this._baseUrl = options.baseUrl ?? null;
    if (this._baseUrl !== null && !/^[a-zA-Z][a-zA-Z0-9+.-]*:/.test(this._baseUrl)) {
      const contextBase = SCRIPT_DIR_HREF ?? MODULE_URL ?? PAGE_DIR_HREF;
      if (contextBase) {
        try {
          this._baseUrl = new URL(dirHref(this._baseUrl), contextBase).href;
        } catch { /* keep as given; retrieval will name the failure */ }
      }
    }
    this._transport = createTransport(options.transport);
    this._args = (options.args ?? []).map(String);
    // A real-tier process's memory is shared, with the wasm32
    // maximum named (a shared memory must name one; §2.2).
    this.memory = this.threadTier === 'real'
      ? new WebAssembly.Memory({ initial: 1, maximum: 65536, shared: true })
      : new WebAssembly.Memory({ initial: 1 });
    this.table = new WebAssembly.Table({ element: 'anyfunc', initial: 2 });
    // Slot 0 stays null (the engine traps calls through it); slot 1
    // is the linker's stub for a null function pointer.
    this.table.set(1, NULL_STUB_FN);
    this._tableFree = 2;
    this._holder = { memory: this.memory, instance: null };
    // The shim allocator's frontier starts past the reserved
    // [0, 1024) span; module static regions are reserved from it.
    // The state itself lives in the memory's header words (threads
    // design §5.6), shared with every worker's shim set in the
    // real thread tier; this object is the accessor view over it.
    this._io = options.io ? normalizeIO(options.io) : null;
    this._byteIO = this._io ? textIOToByteIO(this._io) : createDefaultByteIO();
    this._wasi = createWasiShim(
      { args: this._args, env: options.env ?? {}, stdin: options.stdin },
      this._holder, this._byteIO);
    const def = createDefaultShims({
      io: this._byteIO, holder: this._holder, stdin: options.stdin,
      heapBase: 1024, memoryAllocator: true,
      processTable: this.table,
      // A process attached to an instance shares the instance's
      // virtual FS as its stdio file space: guest fopen reads and
      // writes land in the same files the host stages and reads
      // back. A standalone process gets the shim layer's private
      // in-memory file space instead.
      fileSystem: this._tinyc ? {
        read: (p) => {
          try {
            return this._tinyc.fs.hasFile(p)
              ? this._tinyc.fs.readFile(p) : null;
          } catch {
            return null;
          }
        },
        write: (p, bytes) => { this._tinyc.fs.addFileSync(p, bytes); },
        remove: (p) => this._tinyc.fs.removeFile(p),
      } : undefined,
    });
    this._defaultEnv = def.env;
    this._allocatorState = def.allocatorState;
    this._reserveStatic = def.reserveStatic;
    this._shimsOption = options.shims ?? null;
    this._importsOption = options.imports ?? null;
    // Host-supplied functions by "module.name" key, as bound
    // for this process's own instantiation (§5.7): in the
    // real tier the workers reach exactly these through the
    // proxy. Rebuilt by every _buildImports.
    this._hostProvidedFns = new Map();
    this._regionTableAddr = 0;  // §3.1 class table, on demand
    this._modules = new Map();      // canonical path -> record
    this._handles = new Map();      // handle id -> record
    this._nextHandle = 1;
    this._globalSymbols = new Map(); // name -> { kind, value, record }
    this._got = new Map();          // "func:name"/"mem:name" -> entry
    this._stubSlots = new Map();    // name -> table slot of its trap stub
    this._shimSlots = new Map();    // name -> table slot of a shim function
    this._wrapperSlots = new Map(); // JS function -> Map<typeKey, table slot>
    this._libraries = new Map();    // canonical path -> resolved library
    this._libraryErrors = new Map(); // canonical path -> why local bytes failed
    this._loadOrder = [];           // records in load-completion order
    this._main = null;
    this._dlerror = null;
    this._ended = false;
    // -- thread state (threads design §2.2, §6.6) --
    if (this._threaded) {
      // The main thread is id 1 and exists from creation; the
      // header counters start from it (§5.6).
      this._writeI32(HDR_TID_COUNTER, 1);
      if (this.threadTier === 'simulated') {
        this._scheduler = new Scheduler(this);
        this._scheduler.createMainThread();
        this._pthreadEnv = createSimPthreadShims(this, this._scheduler);
      } else {
        this._pthreadEnv = createCoordinatorPthreadShims();
        // Real tier: this process is the coordinator (§5.1).
        // Its shared-memory control block, key table and stdin
        // placement are set up lazily at the first run or load
        // that needs them — before the main module loads, the
        // allocator frontier is still inside the span the main
        // module's data image lands on.
        this._workerUrlOption = options.workerUrl ?? null;
        this._stdinOption = options.stdin ?? null;
        this._real = {
          workers: new Map(),   // tid -> { worker, recordAddr, alive, isMain }
          mainRun: null,        // { resolve } while a run is driven
          mainExited: false,    // main thread pthread_exited
          loadsInFlight: 0,
          loadQueue: [],        // pending load requests (serialized)
          spawnQueue: [],       // spawns gated on a load (§5.4)
          nextLoadSeq: 1,
          lastTaskIndex: 0,     // _loadOrder length at the last task
          callWaiters: new Map(), // call seq -> { resolve, reject }
          envMap: options.env ?? {},
          nextCallSeq: 1,
          control: 0,           // control block address (0 until setup)
          keyTableAddr: 0,
          dirAddr: 0,           // thread record directory
        };
        this._slotDesc = new Map(); // table slot -> descriptor, for replicas
      }
    }
  }

  /** The process's main module record, or null. */
  get mainModule() { return this._main; }

  /** The transport kind in use: "fetch" or "script". */
  get transportKind() { return this._transport.kind; }

  // -- memory / table frontiers -------------------------------------

  _ensureMemory(end) {
    const have = this.memory.buffer.byteLength;
    if (have < end) this.memory.grow(Math.ceil((end - have) / 65536));
  }

  _allocTable(n) {
    const base = this._tableFree;
    if (this.table.length < base + n) this.table.grow(base + n - this.table.length);
    this._tableFree = base + n;
    return base;
  }

  _allocTableSlot(fn) {
    const slot = this._allocTable(1);
    this.table.set(slot, fn);
    return slot;
  }

  _reserveForModule(meta, isMain) {
    if (isMain) {
      // The main module's base is architecturally 0; its span
      // includes the reserved [0, 1024) area.
      this._ensureMemory(meta.memSize);
      if (this._allocatorState.heapTop < meta.memSize) {
        this._allocatorState.heapTop = meta.memSize;
      }
      return 0;
    }
    return this._reserveStatic(meta.memSize, meta.memAlign || 16);
  }

  // -- memory words ---------------------------------------------------

  _readI32(addr) {
    return new DataView(this.memory.buffer).getInt32(addr, true);
  }

  _writeI32(addr, value) {
    new DataView(this.memory.buffer).setInt32(addr, value | 0, true);
  }

  // -- per-(thread, module) state (threads design §3.1, §7.2) ---------

  /** The thread's TLS area for a module, allocated and
   *  initialized (template bytes, zeroed tail) on first use —
   *  the later of thread creation and module load is when the
   *  callers invoke this. Areas come from the thread-region
   *  allocator (§3.1) and return to it when the thread is
   *  reaped. */
  _tlsAreaFor(tid, record) {
    let addr = record.tlsAreas.get(tid);
    if (addr === undefined) {
      addr = this._regionAlloc(record.meta.tlsSize, record.meta.tlsAlign || 4);
      const m = new Uint8Array(this.memory.buffer);
      m.fill(0, addr, addr + record.meta.tlsSize);
      if (record.meta.tlsTemplate) {
        m.set(record.meta.tlsTemplate.subarray(
          0, Math.min(record.meta.tlsTemplate.length, record.meta.tlsSize)),
          addr);
      }
      record.tlsAreas.set(tid, addr);
    }
    return addr;
  }

  /** A created thread's stack region for a module (§3.1): the
   *  main thread uses the module's baked span and has no
   *  region. Regions come from the thread-region allocator
   *  and return to it when the thread is reaped. */
  _stackRegionFor(tid, record) {
    let base = record.stackRegions.get(tid);
    if (base === undefined) {
      base = this._regionAlloc(record.meta.stackSize, 16);
      record.stackRegions.set(tid, base);
    }
    return base;
  }

  /** Point every GOT.tls global at the given thread's areas:
   *  (the defining module's area for the thread) + (the
   *  symbol's offset in the definer's span). */
  _refreshGotTls(tid) {
    for (const [key, e] of this._got) {
      if (!key.startsWith('tls:') || !e.definer) continue;
      e.global.value = this._tlsAreaFor(tid, e.definer) + e.offset;
    }
  }

  /** The address a symbol-table entry names for the given
   *  thread: a TLS variable lives in the definer's per-thread
   *  area at its span offset; anything else is its recorded
   *  value. */
  _symbolAddress(entry, tid) {
    if (entry.tls) {
      const area = entry.record.tlsAreas.get(tid);
      return (area ?? 0) + entry.value;
    }
    return entry.value;
  }

  /** The thread id a host-side lookup acts for: the scheduler's
   *  context owner in a threaded process, else the single
   *  thread. */
  _actingTid() {
    return this._scheduler?.contextOwner?.id ?? 1;
  }

  /** Make a thread's execution state current (§4.3): for every
   *  loaded module, swap the instance's stack pointer to the
   *  thread's saved value (capturing the outgoing thread's),
   *  install its TLS base, and refresh the GOT.tls globals. */
  _installThreadContext(thread) {
    const sched = this._scheduler;
    if (!sched || sched.contextOwner === thread) return;
    const prev = sched.contextOwner;
    for (const record of this._loadOrder) {
      const inst = record.instance;
      if (!inst) continue;
      const swap = inst.exports.__tinyc_dylink_swap_sp;
      if (typeof swap === 'function') {
        let incoming = thread.savedSp.get(record);
        if (incoming === undefined && record.stackRegions.has(thread.id)) {
          incoming = record.stackRegions.get(thread.id) + record.meta.stackSize;
        }
        if (incoming !== undefined) {
          const prevSp = swap(incoming);
          if (prev && prev !== thread) prev.savedSp.set(record, prevSp);
        }
      }
      if (record.tlsBaseGlobal) {
        const area = record.tlsAreas.get(thread.id);
        if (area !== undefined) record.tlsBaseGlobal.value = area;
      }
    }
    this._refreshGotTls(thread.id);
    sched.contextOwner = thread;
  }

  /** Allocate a created thread's per-module state and queue it
   *  for the rotation (simulated tier). The thread's entry is
   *  the start routine reached through the process table. */
  _createThread(startSlot, arg, detached) {
    const sched = this._scheduler;
    const id = this._readI32(HDR_TID_COUNTER) + 1;
    this._writeI32(HDR_TID_COUNTER, id);
    const thread = new ThreadRecord(id);
    thread.detached = detached;
    thread.keyValuesAddr = this._regionAlloc(128 * 4, 4);
    for (const record of this._loadOrder) {
      if (record.meta.stackSize !== undefined && id !== 1) {
        this._stackRegionFor(id, record);
      }
      if (record.meta.tlsSize > 0) this._tlsAreaFor(id, record);
    }
    thread.invoke = () => {
      const fn = this.table.get(startSlot);
      if (typeof fn !== 'function') {
        throw new TinycDylinkError(
          `tinyc: pthread_create: table slot ${startSlot} does not ` +
          `hold a function`);
      }
      if (this._jspi) return WebAssembly.promising(fn)(arg);
      return Promise.resolve(fn(arg));
    };
    sched.threads.set(id, thread);
    sched.runnable.push(thread);
    sched.kick();
    sched._nudge();
    return id;
  }

  // -- dlerror --------------------------------------------------------

  _setDlerror(message) {
    this._dlerror = String(message);
    // The fixed per-process buffer at [256, 512): inside the
    // reserved span, below every module's data and below the shim
    // allocator's frontier.
    const bytes = textEncoder.encode(this._dlerror.slice(0, 253));
    const m = new Uint8Array(this.memory.buffer);
    m.set(bytes, 256);
    m[256 + bytes.length] = 0;
  }

  _takeDlerrorPtr() {
    if (this._dlerror == null) return 0;
    this._dlerror = null;
    return 256;
  }

  _readCString(ptr, max = 1 << 16) {
    const m = new Uint8Array(this.memory.buffer);
    let end = ptr;
    while (end < m.length && end - ptr < max && m[end] !== 0) end++;
    return textDecoder.decode(m.slice(ptr, end));
  }

  // -- library resolution (design §13) --------------------------------

  /** The session memory cache of retrieved library units whose
   *  write-back into the virtual FS failed: canonical path ->
   *  unit-file envelope text (the exact text the write-back
   *  tried to store; its body was hash-verified at retrieval).
   *  The cache lives on the attached instance — the session
   *  owning the virtual FS — so all of the instance's processes
   *  share it. A standalone process has no virtual FS and never
   *  writes back, so it has no cache either. */
  _libraryMemCache() {
    const t = this._tinyc;
    if (!t) return null;
    return (t._libraryMemCache ??= new Map());
  }

  /** A library from local sources only — the resolved cache, the
   *  script registry, the attached instance's virtual FS, the
   *  readModule callback, the session memory cache of units
   *  whose write-back failed. A local file that fails its
   *  integrity check counts as absent; the reason is remembered
   *  for error messages. Returns null when nothing usable is
   *  local. */
  _libraryLocal(canonical) {
    const cached = this._libraries.get(canonical);
    if (cached) return cached;
    let lib = null;
    const reg = globalThis[DYLINK_REGISTRY];
    if (reg && reg[canonical] !== undefined) {
      try {
        lib = manifestToLibrary(reg[canonical], canonical);
        lib.manifest = reg[canonical];
      } catch (err) {
        this._libraryErrors.set(canonical, err.message);
      }
    }
    if (!lib && this._tinyc?.fs.hasFile(canonical)) {
      let bytes = null;
      try { bytes = this._tinyc.fs.readFile(canonical); } catch { bytes = null; }
      if (bytes) {
        try {
          lib = libraryFromBytes(canonical, bytes);
        } catch (err) {
          this._libraryErrors.set(canonical, err.message);
        }
      }
    }
    if (!lib && this._readModule) {
      const bytes = this._readModule(canonical);
      if (bytes) {
        try {
          lib = libraryFromBytes(canonical, bytes);
        } catch (err) {
          this._libraryErrors.set(canonical, err.message);
        }
      }
    }
    if (!lib) {
      const memCache = this._libraryMemCache();
      const text = memCache?.get(canonical);
      if (text !== undefined) {
        try {
          lib = libraryFromBytes(canonical, textEncoder.encode(text));
        } catch (err) {
          this._libraryErrors.set(canonical, err.message);
        }
        if (lib && this._tinyc) {
          // A memory hit retries the write-back: the unit
          // belongs in the virtual FS, and a write that lands
          // now evicts the memory entry (later loads read the
          // FS copy); a write that still fails keeps it.
          try {
            this._tinyc.fs.addFileSync(canonical, text);
            memCache.delete(canonical);
          } catch { /* still unwritable: keep the memory entry */ }
        }
      }
    }
    if (lib) {
      this._libraries.set(canonical, lib);
      this._libraryErrors.delete(canonical);
    }
    return lib;
  }

  /** Whether a library and its whole needed closure are
   *  available from local sources — the guest dlopen fast path
   *  in a threaded process (§9.1). */
  _closureLocal(canonical, seen = new Set()) {
    if (seen.has(canonical)) return true;
    seen.add(canonical);
    const lib = this._libraryLocal(canonical);
    if (!lib) return false;
    return lib.meta.needed.every((n) =>
      this._closureLocal(canonicalModulePath(n, vfsDirname(canonical)), seen));
  }

  /** A library, retrieving its manifest through the transport when
   *  no usable copy is local; a retrieval is written back into the
   *  virtual FS, best-effort — a failed write-back leaves the unit
   *  in the session memory cache instead (see _libraryLocal). */
  async _libraryFetched(canonical) {
    const local = this._libraryLocal(canonical);
    if (local) return local;
    const origin = canonical.startsWith('/sysroot/')
      ? canonical.slice('/sysroot/'.length)
      : canonical.replace(/^\//, '');
    const url = assetUrl(origin, this._baseUrl ?? undefined);
    let manifest;
    try {
      manifest = await this._transport.retrieveManifest(url, canonical);
    } catch (err) {
      const why = this._libraryErrors.get(canonical);
      if (why) {
        throw new TinycDylinkError(`${err.message} (the local copy is unusable: ${why})`);
      }
      throw err;
    }
    const lib = manifestToLibrary(manifest, canonical);
    lib.manifest = manifest;
    this._libraries.set(canonical, lib);
    this._libraryErrors.delete(canonical);
    if (this._tinyc) {
      // Write-back is best-effort; never a load failure. A unit
      // that lands is served from the virtual FS afterwards, so
      // no memory copy is kept; a unit whose write fails goes to
      // the session memory cache instead and resolves from
      // memory next time, until a retried write lands and
      // evicts it.
      const text = manifestEnvelopeText(canonical, manifest);
      try {
        this._tinyc.fs.addFileSync(canonical, text);
        this._libraryMemCache()?.delete(canonical);
      } catch {
        this._libraryMemCache()?.set(canonical, text);
      }
    }
    return lib;
  }

  /** Resolve the whole needed closure of a root library through
   *  _libraryFetched, so the synchronous load below never waits. */
  async _prefetchClosure(rootCanonical) {
    const seen = new Set();
    const walk = async (canonical) => {
      if (seen.has(canonical)) return;
      seen.add(canonical);
      const lib = await this._libraryFetched(canonical);
      for (const need of lib.meta.needed) {
        await walk(canonicalModulePath(need, vfsDirname(canonical)));
      }
    };
    await walk(rootCanonical);
  }

  // -- symbols --------------------------------------------------------

  _gotEntry(kind, name) {
    const key = `${kind}:${name}`;
    let e = this._got.get(key);
    if (!e) {
      e = {
        global: new WebAssembly.Global({ value: 'i32', mutable: true }, 0),
        filled: false,
        definer: null,
      };
      if (kind === 'func') e.global.value = this._stubSlot(name);
      this._got.set(key, e);
    }
    return e;
  }

  /** A table slot holding a wasm wrapper of a JS function (the
   *  table accepts only wasm functions), typed for the calls the
   *  process's modules make through it. */
  _jsFunctionSlot(fn, type) {
    const typeKey = `${type.params.join(',')}->${type.results.join(',')}`;
    let byType = this._wrapperSlots.get(fn);
    if (!byType) {
      byType = new Map();
      this._wrapperSlots.set(fn, byType);
    }
    let slot = byType.get(typeKey);
    if (slot === undefined) {
      slot = this._allocTableSlot(makeWrapperFunction(fn, type));
      byType.set(typeKey, slot);
    }
    return slot;
  }

  _stubSlot(name) {
    let slot = this._stubSlots.get(name);
    if (slot === undefined) {
      const trap = () => {
        throw new Error(`tinyc: call to unresolved function "${name}"`);
      };
      slot = this._jsFunctionSlot(trap, DEFAULT_CALL_TYPE);
      this._stubSlots.set(name, slot);
      if (this._slotDesc) {
        this._slotDesc.set(slot, {
          kind: 'stub', name,
          params: [...DEFAULT_CALL_TYPE.params],
          results: [...DEFAULT_CALL_TYPE.results],
        });
      }
    }
    return slot;
  }

  _shimSlot(name, fn, requester = null, isHost = false) {
    let slot = this._shimSlots.get(name);
    if (slot === undefined) {
      let type = requester?.importTypes?.get(`env.${name}`) ?? null;
      if (!type) {
        for (const rec of this._modules.values()) {
          type = rec.importTypes?.get(`env.${name}`) ?? null;
          if (type) break;
        }
      }
      const t = type ?? DEFAULT_CALL_TYPE;
      slot = this._jsFunctionSlot(fn, t);
      this._shimSlots.set(name, slot);
      if (this._slotDesc) {
        this._slotDesc.set(slot, {
          kind: isHost ? 'host' : 'shim', name,
          params: [...t.params], results: [...t.results],
        });
      }
    }
    return slot;
  }

  /** Fill a GOT entry from a definition, first-wins: an entry that
   *  already holds a definition keeps it. */
  _fillGot(kind, name, value, definer) {
    const e = this._gotEntry(kind, name);
    if (!e.filled) {
      e.global.value = value;
      e.filled = true;
      e.definer = definer;
    }
  }

  _registerGlobals(record) {
    for (const [name, e] of record.exports) {
      if (!this._globalSymbols.has(name)) {
        this._globalSymbols.set(name, {
          kind: e.kind, value: e.value, record, tls: e.tls ? 1 : 0,
        });
      }
    }
    record.global = true;
  }

  _withdrawGlobals(record) {
    for (const [name, e] of this._globalSymbols) {
      if (e.record === record) this._globalSymbols.delete(name);
    }
    for (const [name, e] of record.exports) {
      const entry = this._got.get(
        `${e.tls ? 'tls' : e.kind === 0 ? 'func' : 'mem'}:${name}`);
      if (entry && entry.definer === record) {
        entry.global.value = e.kind === 0 && !e.tls ? this._stubSlot(name) : 0;
        entry.filled = false;
        entry.definer = null;
      }
    }
    record.global = false;
  }

  /** Modules a requester has opened and not yet closed. */
  _openedBy(requester) {
    const out = [];
    for (const rec of this._modules.values()) {
      if ((rec.handleHolders.get(requester) ?? 0) > 0) out.push(rec);
    }
    return out;
  }

  /** A record's visibility closure (design §9.4): itself, its
   *  needed closure transitively, and — for a module loaded by a
   *  runtime dlopen — its requester's exports, the requester's
   *  closure and the modules the requester has open. */
  _closureRecords(record) {
    const out = [];
    const seen = new Set();
    const visit = (rec) => {
      if (!rec || seen.has(rec)) return;
      seen.add(rec);
      out.push(rec);
      for (const dep of rec.neededRecords) visit(dep);
      if (rec.scopeFrom) {
        visit(rec.scopeFrom);
        for (const dep of this._openedBy(rec.scopeFrom)) visit(dep);
      }
    };
    visit(record);
    return out;
  }

  _lookupClosure(name, record, includeSelf) {
    const records = this._closureRecords(record);
    for (const rec of records) {
      if (!includeSelf && rec === record) continue;
      const e = rec.exports.get(name);
      if (e) return { record: rec, entry: e };
    }
    return null;
  }

  /** The callable for a function symbol provided by a module:
   *  the exported function itself when the definer is
   *  instantiated, a call-time forwarding stub while it is still
   *  in flight (a dependency cycle). */
  _functionValue(record, name, requester) {
    const fn = record.instance?.exports?.[name];
    if (typeof fn === 'function') return fn;
    return (...args) => {
      const f = record.instance?.exports?.[name];
      if (typeof f !== 'function') {
        throw new Error(
          `tinyc: call to "${name}" of "${record.canonical}", which is ` +
          `not instantiated yet (imported by "${requester.canonical}")`);
      }
      return f(...args);
    };
  }

  // -- import assembly --------------------------------------------------

  _buildImports(record, wasmModule) {
    const imports = {};
    const provided = new Map();
    const put = (m, n, v) => {
      (imports[m] ??= {})[n] = v;
      provided.set(`${m}.${n}`, v);
    };
    for (const [name, fn] of Object.entries(this._wasi)) {
      put('wasi_snapshot_preview1', name, fn);
    }
    if (this._pthreadEnv && typeof this._pthreadEnv.sched_yield === 'function') {
      // The wasm backend emits sched_yield as a WASI import
      // (it is on the WASI name list), not an env import. In a
      // threaded process the yield must reach the scheduler,
      // so the WASI name is routed to the pthread shim too.
      put('wasi_snapshot_preview1', 'sched_yield',
        SIM_BLOCKING_SHIMS.has('sched_yield') && this._scheduler &&
        this._jspi
          ? new WebAssembly.Suspending(this._pthreadEnv.sched_yield)
          : this._pthreadEnv.sched_yield);
    }
    for (const [name, fn] of Object.entries(this._defaultEnv)) {
      put('env', name, fn);
    }
    const dl = this._dlfcnFor(record);
    for (const [name, fn] of Object.entries(dl)) {
      // In a threaded simulated-tier process the guest dlopen
      // can suspend on a retrieval (§9.1): it is supplied
      // wrapped in WebAssembly.Suspending, like the blocking
      // pthread shims (§4.2).
      if (name === 'dlopen' && this._scheduler && this._jspi) {
        put('env', name, new WebAssembly.Suspending(fn));
      } else {
        put('env', name, fn);
      }
    }
    if (this._pthreadEnv) {
      for (const [name, fn] of Object.entries(this._pthreadEnv)) {
        put('env', name, SIM_BLOCKING_SHIMS.has(name) && this._scheduler &&
          this._jspi ? new WebAssembly.Suspending(fn) : fn);
      }
    }
    const ctx = createShimContext(this._holder, this._byteIO);
    // Provenance of the final binding, per "module.name" key:
    // a function supplied by a host layer (the package-global
    // registry, the instance's shims, the process's shims or
    // raw imports) exists only in this context. In the real
    // tier the workers classify it 'host' and reach it
    // through the §5.7 proxy; the package's own layers above
    // are 'shim' and exist in every worker.
    const hostFns = new Map();
    for (const layer of [globalShimRegistry, this._tinyc?._shims, this._shimsOption]) {
      if (!layer) continue;
      for (const [key, fn] of shimEntriesOf(layer)) {
        const { module: mn, name } = splitShimKey(key);
        const bound = (...a) => fn(ctx, ...a);
        put(mn, name, bound);
        hostFns.set(`${mn}.${name}`, bound);
      }
    }
    for (const [mn, fns] of Object.entries(this._importsOption ?? {})) {
      for (const [name, v] of Object.entries(fns)) {
        put(mn, name, v);
        if (typeof v === 'function') hostFns.set(`${mn}.${name}`, v);
        else hostFns.delete(`${mn}.${name}`);
      }
    }
    this._hostProvidedFns = hostFns;
    // Structural imports: process-owned, not overridable.
    put('env', 'memory', this.memory);
    put('env', '__indirect_function_table', this.table);
    put('env', '__memory_base',
      new WebAssembly.Global({ value: 'i32', mutable: false }, record.memoryBase));
    put('env', '__table_base',
      new WebAssembly.Global({ value: 'i32', mutable: false }, record.tableBase));
    // env.__tls_base, when the module imports it: a mutable
    // global the loader holds and sets to the executing
    // thread's TLS area for this module (§7.2). Its initial
    // value is the loading context's area, so init — which
    // runs on that context — sees the right area.
    const moduleImports = WebAssembly.Module.imports(wasmModule);
    if (moduleImports.some((i) =>
      i.module === 'env' && i.name === '__tls_base' && i.kind === 'global')) {
      const g = new WebAssembly.Global({ value: 'i32', mutable: true },
        record.meta.tlsSize > 0
          ? this._tlsAreaFor(this._actingTid(), record) : 0);
      record.tlsBaseGlobal = g;
      put('env', '__tls_base', g);
    }
    const unresolvedFunctions = [];
    const unresolvedGotMem = [];
    const unresolvedGotTls = [];
    const unresolvedOther = [];
    for (const imp of moduleImports) {
      const key = `${imp.module}.${imp.name}`;
      if (imp.kind === 'global' &&
          (imp.module === 'GOT.func' || imp.module === 'GOT.mem' ||
           imp.module === 'GOT.tls')) {
        const kind = imp.module === 'GOT.func' ? 'func'
          : imp.module === 'GOT.mem' ? 'mem' : 'tls';
        const entry = this._gotEntry(kind, imp.name);
        if (!entry.filled) {
          if (kind === 'tls') {
            const hit = this._resolveGotTls(imp.name, record);
            if (hit) this._fillGotTls(imp.name, hit.definer, hit.offset);
            else unresolvedGotTls.push(imp.name);
          } else {
            const hit = this._resolveGot(kind, imp.name, record, provided, hostFns);
            if (hit) this._fillGot(kind, imp.name, hit.value, hit.definer);
            else if (kind === 'mem') unresolvedGotMem.push(imp.name);
          }
        }
        put(imp.module, imp.name, entry.global);
        continue;
      }
      if (provided.has(key)) continue;
      if (imp.kind === 'function') {
        const g = this._globalSymbols.get(imp.name);
        if (g && g.kind === 0) {
          put(imp.module, imp.name, this._functionValue(g.record, imp.name, record));
          continue;
        }
        const hit = this._lookupClosure(imp.name, record, false);
        if (hit && hit.entry.kind === 0) {
          put(imp.module, imp.name, this._functionValue(hit.record, imp.name, record));
          continue;
        }
        unresolvedFunctions.push(imp.name);
        put(imp.module, imp.name, () => {
          throw new Error(
            `tinyc: call to unresolved function "${imp.name}" ` +
            `imported by "${record.canonical}"`);
        });
        continue;
      }
      unresolvedOther.push(key);
    }
    // GOT.func entries that static data initializers reference
    // bind eagerly, like GOT.mem: the module's init copies the
    // GOT value into a data slot, and a definition arriving later
    // updates the GOT global but can never repair the copy.  An
    // entry still unfilled here has no resolution source in this
    // load, so the load must fail naming the symbol instead of
    // baking a stub slot (or worse, the previous silent 0) into
    // the module's data.
    const unresolvedDataGotFunc = [];
    for (const name of record.meta.dataGotFunc ?? []) {
      const entry = this._got.get(`func:${name}`);
      if (!entry || !entry.filled) unresolvedDataGotFunc.push(name);
    }
    return { imports, unresolvedFunctions, unresolvedGotMem, unresolvedOther,
             unresolvedDataGotFunc, unresolvedGotTls };
  }

  /** A GOT value for a not-yet-filled entry, in the §5 order:
   *  shim layer (functions only), global table, requester closure. */
  _resolveGot(kind, name, record, provided, hostFns) {
    const wantKind = kind === 'func' ? 0 : 1;
    if (kind === 'func') {
      const shim = provided.get(`env.${name}`);
      if (typeof shim === 'function') {
        return {
          value: this._shimSlot(name, shim, record,
            hostFns?.has(`env.${name}`) ?? false),
          definer: 'shim',
        };
      }
    }
    const g = this._globalSymbols.get(name);
    if (g && g.kind === wantKind) return { value: g.value, definer: g.record };
    const hit = this._lookupClosure(name, record, false);
    if (hit && hit.entry.kind === wantKind) {
      return { value: hit.entry.value, definer: hit.record };
    }
    return null;
  }

  // -- the load procedure (design §9) -----------------------------------

  /** Load (or re-reference) a module by canonical path.
   *  opts: { flags, requester, runtimeEdge, hostEdge, isMain, lib }.
   *  Reference accounting: the call itself is one reference — a
   *  needed edge, a dlopen handle reference, or the process's main
   *  reference. Throws TinycDylinkError on failure, with the
   *  partial load rolled back. */
  _loadModule(canonical, opts = {}) {
    const flags = opts.flags ?? RTLD_LOCAL;
    const requester = opts.requester ?? null;
    const holderKey = opts.runtimeEdge || opts.hostEdge
      ? (requester ?? 'host') : null;
    const existing = this._modules.get(canonical);
    if (existing) {
      existing.refcount++;
      if (holderKey !== null) {
        existing.handleHolders.set(
          holderKey, (existing.handleHolders.get(holderKey) ?? 0) + 1);
      }
      if ((flags & RTLD_GLOBAL) && !existing.global) {
        this._registerGlobals(existing);
      }
      return existing;
    }
    if (this._ended) {
      throw new TinycDylinkError('tinyc: the process has ended');
    }
    const lib = opts.lib ?? this._libraryLocal(canonical);
    if (!lib) {
      const why = this._libraryErrors.get(canonical);
      throw new TinycDylinkError(why
        ? `tinyc: cannot load "${canonical}": ${why}`
        : `tinyc: cannot load "${canonical}": not present locally ` +
          `(a host loadModule call retrieves it from the base URL first)`);
    }
    const meta = lib.meta;
    if (this._threaded && meta.stackSize === undefined) {
      // §3.3: a threaded process refuses a pre-revision module
      // — its active data segments make replica instantiation
      // unsafe. A non-threaded process accepts both forms.
      throw new TinycDylinkError(
        `tinyc: cannot load "${canonical}" into a threaded process: ` +
        `the module's metadata carries no stack_size; it predates ` +
        `the threaded module revision (threads design §3.3)`);
    }
    const record = {
      canonical, meta, lib,
      handle: this._nextHandle++,
      state: 'loading',
      memoryBase: 0, tableBase: 0,
      instance: null, wasmModule: null,
      exports: new Map(),
      neededRecords: [],
      handleHolders: new Map(),
      scopeFrom: opts.runtimeEdge ? requester : null,
      refcount: 1,
      global: false,
      isMain: Boolean(opts.isMain),
      tlsAreas: new Map(),      // thread id -> TLS area address
      stackRegions: new Map(),  // thread id -> stack region base
      tlsBaseGlobal: null,      // the env.__tls_base global, if imported
    };
    if (holderKey !== null) record.handleHolders.set(holderKey, 1);
    this._modules.set(canonical, record);
    this._handles.set(record.handle, record);
    try {
      // Allocate and register exports before the needed walk, so a
      // dependency cycle binds against them (design §9.3).
      record.memoryBase = this._reserveForModule(meta, record.isMain);
      record.tableBase = this._allocTable(meta.tableSize);
      for (const e of meta.exports) {
        if (record.exports.has(e.name)) continue;
        // A TLS export's recorded value is its offset in the
        // definer's TLS span, not a memory address (§7.2); the
        // address is per-thread and computed at lookup time.
        const value = e.kind === 0
          ? record.tableBase + e.value
          : e.tls ? e.value : record.memoryBase + e.value;
        record.exports.set(e.name, { kind: e.kind, value, tls: e.tls ? 1 : 0 });
      }
      for (const [name, e] of record.exports) {
        if (e.tls) this._fillGotTls(name, record, e.value);
        else this._fillGot(e.kind === 0 ? 'func' : 'mem', name, e.value, record);
      }
      if ((flags & RTLD_GLOBAL) || record.isMain) this._registerGlobals(record);
      // Needed walk: depth-first; LAZY/NOW propagate, GLOBAL does not.
      const childFlags = (flags & RTLD_NOW) ? RTLD_NOW : RTLD_LAZY;
      for (const need of meta.needed) {
        const depCanonical = canonicalModulePath(need, vfsDirname(canonical));
        const dep = this._loadModule(depCanonical, {
          flags: childFlags, requester: record,
        });
        record.neededRecords.push(dep);
      }
      // Instantiate. In a real-tier (shared-memory) process
      // the module's memory import declaration is widened to
      // shared first (see patchSharedMemoryImport); the
      // widened bytes are also what the tier's workers are
      // booted with, so every replica compiles the same form.
      const bodyBytes = this.threadTier === 'real'
        ? patchSharedMemoryImport(lib.bodyBytes) : lib.bodyBytes;
      record.bodyBytes = bodyBytes;
      const wasmModule = this.threadTier === 'real'
        ? new WebAssembly.Module(bodyBytes)
        : (lib.wasmModule ?? new WebAssembly.Module(bodyBytes));
      record.wasmModule = wasmModule;
      record.importTypes = parseImportTypes(bodyBytes);
      const { imports, unresolvedFunctions, unresolvedGotMem, unresolvedOther,
              unresolvedDataGotFunc, unresolvedGotTls } =
        this._buildImports(record, wasmModule);
      if (unresolvedGotMem.length > 0) {
        throw new TinycDylinkError(
          `tinyc: cannot load "${canonical}": unresolved variable(s) ` +
          `${unresolvedGotMem.map((n) => `"${n}"`).join(', ')} ` +
          `(GOT.mem references bind eagerly)`);
      }
      if (unresolvedGotTls.length > 0) {
        throw new TinycDylinkError(
          `tinyc: cannot load "${canonical}": unresolved TLS variable(s) ` +
          `${unresolvedGotTls.map((n) => `"${n}"`).join(', ')} ` +
          `(GOT.tls references bind eagerly)`);
      }
      if (unresolvedDataGotFunc.length > 0) {
        throw new TinycDylinkError(
          `tinyc: cannot load "${canonical}": unresolved function(s) ` +
          `${unresolvedDataGotFunc.map((n) => `"${n}"`).join(', ')} ` +
          `(GOT.func references from static data bind eagerly)`);
      }
      if ((flags & RTLD_NOW) && unresolvedFunctions.length > 0) {
        throw new TinycDylinkError(
          `tinyc: cannot load "${canonical}": unresolved function(s) ` +
          `${unresolvedFunctions.map((n) => `"${n}"`).join(', ')} under RTLD_NOW`);
      }
      if (unresolvedOther.length > 0) {
        throw new TinycDylinkError(
          `tinyc: cannot load "${canonical}": unsupported import(s) ` +
          unresolvedOther.join(', '));
      }
      let instance;
      try {
        instance = new WebAssembly.Instance(wasmModule, imports);
      } catch (err) {
        throw new TinycDylinkError(
          `tinyc: cannot instantiate "${canonical}": ${err.message}`);
      }
      record.instance = instance;
      if (typeof instance.exports.__tinyc_dylink_init !== 'function' ||
          typeof instance.exports.__tinyc_dylink_fini !== 'function') {
        throw new TinycDylinkError(
          `tinyc: module "${canonical}" does not export ` +
          `__tinyc_dylink_init/__tinyc_dylink_fini; it is not a loadable dylink module`);
      }
      if (this._threaded &&
          typeof instance.exports.__tinyc_dylink_swap_sp !== 'function') {
        throw new TinycDylinkError(
          `tinyc: module "${canonical}" does not export ` +
          `__tinyc_dylink_swap_sp; a threaded process requires it ` +
          `(threads design §3.1)`);
      }
      // Initialize (post-order: dependencies are loaded by now).
      // In the real thread tier the coordinator's instance is a
      // replica that never runs program code (§5.1): init is
      // deferred to the §5.4 protocol — the primary context
      // runs it once every context holds its replica.
      if (this.threadTier === 'real') {
        record.state = 'instantiated';
        this._loadOrder.push(record);
        this._afterModuleLoaded(record);
        return record;
      }
      this._initRecord(record);
      this._loadOrder.push(record);
      this._afterModuleLoaded(record);
      return record;
    } catch (err) {
      this._rollbackLoad(record);
      throw err;
    }
  }

  /** Run a record's init on this context's instance — the
   *  single-context path, and the coordinator's replica for a
   *  host-initiated load in the real tier (§5.4). */
  _initRecord(record) {
    record.state = 'initializing';
    let rc;
    try {
      rc = record.instance.exports.__tinyc_dylink_init(record.handle);
    } catch (err) {
      throw new TinycDylinkError(
        `tinyc: initialization of "${record.canonical}" trapped: ${err.message}`);
    }
    if (rc !== 0) {
      throw new TinycDylinkError(
        `tinyc: initialization of "${record.canonical}" failed: ` +
        `its DllMain returned FALSE`);
    }
    record.state = 'loaded';
  }

  /** Per-(thread, module) state for a freshly loaded module
   *  (threads design §3.1, §7.2): TLS areas for every live
   *  thread (the module-load side of the later-of-two-events
   *  rule), stack regions for live created threads, and the
   *  __tls_base installation for the current context. In a
   *  non-threaded process the same machinery runs with the
   *  single thread. */
  _afterModuleLoaded(record) {
    const meta = record.meta;
    const tids = this._scheduler ? [...this._scheduler.threads.keys()] : [1];
    if (meta.tlsSize > 0) {
      for (const tid of tids) this._tlsAreaFor(tid, record);
      if (record.tlsBaseGlobal) {
        const area = record.tlsAreas.get(this._actingTid());
        if (area !== undefined) record.tlsBaseGlobal.value = area;
      }
    }
    if (!this._scheduler) return;
    for (const tid of tids) {
      if (tid !== 1 && meta.stackSize !== undefined) {
        this._stackRegionFor(tid, record);
      }
    }
    // If a created thread performed this load, init ran on its
    // step and installed the module's baked stack top — the
    // span that is the main thread's. Swap in the loading
    // thread's own region top and record the baked top as the
    // main thread's saved value.
    const owner = this._scheduler.contextOwner;
    if (owner && owner.id !== 1 && meta.stackSize !== undefined &&
        !owner.savedSp.has(record)) {
      const swap = record.instance?.exports?.__tinyc_dylink_swap_sp;
      if (typeof swap === 'function') {
        const top = this._stackRegionFor(owner.id, record) + meta.stackSize;
        const baked = swap(top);
        this._scheduler.mainThread.savedSp.set(record, baked);
        owner.savedSp.set(record, top);
      }
    }
    this._refreshGotTls(this._actingTid());
  }

  /** Fill a GOT.tls entry from a TLS definition, first-wins.
   *  The entry records the definer and the span offset; its
   *  global's value is the acting thread's address and is
   *  refreshed at every context install (§7.2). */
  _fillGotTls(name, definer, offset) {
    const e = this._gotEntry('tls', name);
    if (!e.filled) {
      e.filled = true;
      e.definer = definer;
      e.offset = offset;
      e.global.value = this._tlsAreaFor(this._actingTid(), definer) + offset;
    }
  }

  /** A GOT.tls resolution: the global table, then the
   *  requester's closure — TLS entries only. */
  _resolveGotTls(name, record) {
    const g = this._globalSymbols.get(name);
    if (g && g.tls) return { definer: g.record, offset: g.value };
    const hit = this._lookupClosure(name, record, false);
    if (hit && hit.entry.tls) {
      return { definer: hit.record, offset: hit.entry.value };
    }
    return null;
  }

  /** Undo a failed load: release the references the record took,
   *  withdraw its registrations, discard the record. Its memory
   *  region and table slots are retired, never reused. */
  _rollbackLoad(record) {
    const deps = record.neededRecords;
    record.neededRecords = [];
    for (let i = deps.length - 1; i >= 0; i--) this._releaseRef(deps[i]);
    this._withdrawGlobals(record);
    this._modules.delete(record.canonical);
    this._handles.delete(record.handle);
    record.state = 'discarded';
  }

  _releaseRef(record) {
    record.refcount--;
    if (record.refcount > 0 || record.isMain) return;
    this._closeModule(record);
  }

  /** Close a module whose count reached zero (design §8/§9.5):
   *  finalize, withdraw its symbols, then release its own
   *  references — needed edges in reverse order, then the modules
   *  it had opened. The instance and its regions persist. */
  _closeModule(record) {
    if (record.state === 'loaded') {
      record.state = 'closed';
      try {
        record.instance.exports.__tinyc_dylink_fini(record.handle, 0);
      } catch (err) {
        this._setDlerror(
          `tinyc: finalizer of "${record.canonical}" trapped: ${err.message}`);
      }
    }
    this._withdrawGlobals(record);
    this._modules.delete(record.canonical);
    this._handles.delete(record.handle);
    const li = this._loadOrder.indexOf(record);
    if (li >= 0) this._loadOrder.splice(li, 1);
    const deps = record.neededRecords;
    record.neededRecords = [];
    for (let i = deps.length - 1; i >= 0; i--) this._releaseRef(deps[i]);
    for (const dep of this._openedBy(record)) {
      const n = dep.handleHolders.get(record) ?? 0;
      dep.handleHolders.delete(record);
      for (let i = 0; i < n; i++) this._releaseRef(dep);
    }
  }

  // -- dlfcn --------------------------------------------------------------

  _dlsym(handle, name) {
    const tid = this._actingTid();
    if (!handle) {
      const g = this._globalSymbols.get(name);
      if (g) return this._symbolAddress(g, tid);
      this._setDlerror(
        `tinyc: dlsym: symbol "${name}" not found in the global scope`);
      return 0;
    }
    const record = this._handles.get(handle);
    if (!record) {
      this._setDlerror(`tinyc: dlsym: unknown handle ${handle}`);
      return 0;
    }
    const own = record.exports.get(name);
    if (own) return this._symbolAddress({ ...own, record }, tid);
    const hit = this._lookupClosure(name, record, false);
    if (hit) return this._symbolAddress({ ...hit.entry, record: hit.record }, tid);
    this._setDlerror(
      `tinyc: dlsym: symbol "${name}" not found in "${record.canonical}" ` +
      `or its dependency closure`);
    return 0;
  }

  _consumeHolder(record, requester) {
    const holders = record.handleHolders;
    const key = requester ?? 'host';
    if ((holders.get(key) ?? 0) > 0) {
      const n = holders.get(key) - 1;
      if (n === 0) holders.delete(key); else holders.set(key, n);
      return true;
    }
    // A close of a handle another module opened consumes that
    // reference; handles are process-wide.
    for (const [k, n] of holders) {
      if (n > 0) {
        if (n === 1) holders.delete(k); else holders.set(k, n - 1);
        return true;
      }
    }
    return false;
  }

  _dlclose(handle, requester) {
    const record = this._handles.get(handle);
    if (!record) {
      this._setDlerror(`tinyc: dlclose: unknown handle ${handle}`);
      return -1;
    }
    if (record.isMain) {
      // Accepted and counted, but a main module's close takes no
      // effect while the process runs (design §8).
      this._consumeHolder(record, requester);
      return 0;
    }
    if (!this._consumeHolder(record, requester)) {
      this._setDlerror(
        `tinyc: dlclose: handle ${handle} ("${record.canonical}") has ` +
        `no outstanding dlopen reference`);
      return -1;
    }
    this._releaseRef(record);
    return 0;
  }

  /** The dlfcn shim set for one requesting module: its dlopen
   *  calls carry the requester for scoping and edge accounting. */
  _dlfcnFor(requester) {
    const proc = this;
    return {
      dlopen(pathPtr, flags) {
        flags = flags | 0;
        if (pathPtr === 0) {
          if (proc._main) return proc._main.handle;
          proc._setDlerror('tinyc: dlopen(NULL): the process has no main module');
          return 0;
        }
        const bad = flags & ~RTLD_KNOWN;
        if (bad) {
          proc._setDlerror(
            `tinyc: dlopen: unsupported flag bit(s) 0x${(bad >>> 0).toString(16)}`);
          return 0;
        }
        const path = proc._readCString(pathPtr);
        let canonical;
        try {
          canonical = canonicalModulePath(path, '/sysroot');
        } catch (err) {
          proc._setDlerror(err.message);
          return 0;
        }
        const load = () => proc._loadModule(canonical, {
          flags, requester, runtimeEdge: true,
        });
        if (proc._scheduler && !proc._closureLocal(canonical)) {
          // Threaded, simulated tier (§9.1): the closure is not
          // local, so park the calling thread on the retrieval —
          // other threads rotate meanwhile — and complete the
          // load synchronously once the closure is local. In
          // the real tier the guest dlopen is a coordination
          // operation (§5.4), not this path.
          if (!proc._jspi) {
            throw new Error(
              `tinyc: dlopen: loading "${canonical}" requires a ` +
              `retrieval, which suspends the calling thread; that ` +
              `needs WebAssembly.promising (JSPI), which this ` +
              `engine does not provide (threads design §4.2)`);
          }
          const thread = proc._scheduler.actingThread();
          return proc._scheduler.parkOnHost(thread,
            () => proc._prefetchClosure(canonical)).then(() => {
              try {
                return load().handle;
              } catch (err) {
                proc._setDlerror(err.message);
                return 0;
              }
            });
        }
        try {
          return load().handle;
        } catch (err) {
          proc._setDlerror(err.message);
          return 0;
        }
      },
      dlsym(handle, namePtr) {
        return proc._dlsym(handle | 0, proc._readCString(namePtr));
      },
      dlclose(handle) {
        return proc._dlclose(handle | 0, requester);
      },
      dlerror() {
        return proc._takeDlerrorPtr();
      },
    };
  }

  // -- host surface -------------------------------------------------------

  /** Load a library and its whole needed closure, retrieving
   *  manifests through the transport where no usable copy is
   *  local. Returns the module's handle. */
  async loadModule(path, flags = RTLD_LOCAL) {
    if (this._ended) throw new TinycDylinkError('tinyc: the process has ended');
    const canonical = canonicalModulePath(path, '/sysroot');
    await this._prefetchClosure(canonical);
    if (this._scheduler && this._scheduler.drivingRun &&
        !this._scheduler.hostDriving) {
      // A load's init runs guest code; mid-run it goes through
      // the rotation as a host operation (§4.1).
      return this._scheduler.enqueueHostOp(() => {
        return this._loadModule(canonical, { flags, hostEdge: true }).handle;
      });
    }
    if (this.threadTier === 'real') {
      // The §5.4 protocol with the coordinator as initiator:
      // replicas everywhere first, then init here.
      this._realSetup();
      const before = this._loadOrder.length;
      const rec = this._loadModule(canonical, { flags, hostEdge: true });
      const delta = this._loadOrder.slice(before);
      if (delta.length > 0) {
        if (this._real.workers.size > 0) {
          await this._realBroadcastLoad(delta, 'coordinator', 0);
        } else {
          for (const r of delta) this._initRecord(r);
        }
      }
      return rec.handle;
    }
    const rec = this._loadModule(canonical, { flags, hostEdge: true });
    return rec.handle;
  }

  /** Synchronous dlopen: the library and its closure must already
   *  be local (see loadModule for the retrieving form). Returns
   *  the handle; throws TinycDylinkError on failure. In a
   *  threaded process the synchronous form does not exist — a
   *  load can run constructors on another context and wait for
   *  replicas — and it fails by name (§2.3). */
  dlopen(path, flags = RTLD_LAZY) {
    if (this._threaded) {
      throw new TinycDylinkError(
        'tinyc: dlopen: a threaded process loads asynchronously; ' +
        'use loadModule (threads design §2.3)');
    }
    const canonical = canonicalModulePath(path, '/sysroot');
    return this._loadModule(canonical, { flags, hostEdge: true }).handle;
  }

  /** The handle of an already-loaded module by path, or 0. */
  moduleHandle(path) {
    const rec = this._modules.get(canonicalModulePath(path, '/sysroot'));
    return rec ? rec.handle : 0;
  }

  dlsym(handle, name) {
    const value = this._dlsym(handle, name);
    if (value === 0) {
      const msg = this._dlerror ?? 'tinyc: dlsym failed';
      this._dlerror = null;
      throw new TinycDylinkError(msg);
    }
    return value;
  }

  dlclose(handle) {
    const rc = this._dlclose(handle, null);
    if (rc !== 0) {
      const msg = this._dlerror ?? 'tinyc: dlclose failed';
      this._dlerror = null;
      throw new TinycDylinkError(msg);
    }
    return 0;
  }

  /** Call an exported function of a loaded module by handle and
   *  name — the host counterpart of a guest function pointer.
   *  In a threaded process the call executes guest code, so it
   *  returns a promise: through the scheduler as a host
   *  operation in the simulated tier, delegated to the main
   *  thread's worker in the real tier (§2.3). */
  callExport(handle, name, ...args) {
    const record = this._handles.get(handle);
    if (!record?.instance) {
      throw new TinycDylinkError(`tinyc: callExport: unknown handle ${handle}`);
    }
    const fn = record.instance.exports[name];
    if (typeof fn !== 'function') {
      throw new TinycDylinkError(
        `tinyc: module "${record.canonical}" does not export "${name}"`);
    }
    if (this._scheduler) {
      return this._scheduler.enqueueHostOp(async () => {
        if (this._jspi) return WebAssembly.promising(fn)(...args);
        return fn(...args);
      });
    }
    if (this._threaded) return this._realCallExport(record, name, args);
    return fn(...args);
  }

  /** Read a variable exported by a loaded module, as an i32.
   *  A TLS variable reads the acting thread's area (§7.2). */
  readExportI32(handle, name) {
    const record = this._handles.get(handle);
    if (!record) throw new TinycDylinkError(`tinyc: readExportI32: unknown handle ${handle}`);
    const e = record.exports.get(name);
    if (!e || e.kind !== 1) {
      throw new TinycDylinkError(
        `tinyc: module "${record.canonical}" does not export variable "${name}"`);
    }
    const addr = this._symbolAddress({ ...e, record }, this._actingTid());
    return new DataView(this.memory.buffer).getInt32(addr, true);
  }

  writeExportI32(handle, name, value) {
    const record = this._handles.get(handle);
    if (!record) throw new TinycDylinkError(`tinyc: writeExportI32: unknown handle ${handle}`);
    const e = record.exports.get(name);
    if (!e || e.kind !== 1) {
      throw new TinycDylinkError(
        `tinyc: module "${record.canonical}" does not export variable "${name}"`);
    }
    const addr = this._symbolAddress({ ...e, record }, this._actingTid());
    new DataView(this.memory.buffer).setInt32(addr, value | 0, true);
  }

  /** Load raw module bytes as the process's main module and run
   *  it: the product of a dylink-main compile, whose exports enter
   *  the global symbol table. The entry is options.entry when
   *  given, else _start, else main(argc, argv) with the process's
   *  args. Running the main module ends the process: every loaded
   *  module is finalized in reverse load order. Returns the
   *  entry's return value, or { exitCode } for a _start main.
   *  In a threaded process the result comes back as a promise
   *  for the same shape (§2.3). */
  runMainModule(bytes, options = {}) {
    if (this._threaded) {
      if (this.threadTier === 'real') {
        return this._runMainModuleReal(bytes, options);
      }
      return this._runMainModuleSim(bytes, options);
    }
    return this._runMainModuleSync(bytes, options);
  }

  /** The main thread's step in a simulated-tier process: load
   *  the main module and run its entry on a parkable stack
   *  (§4.2), so even load-time constructors may block or
   *  dlopen. Resolves with the runMainModule result shape. */
  _invokeMainThread(bytes, options) {
    if (this._main) {
      throw new TinycDylinkError('tinyc: this process already has a main module');
    }
    if (this._modules.size > 0) {
      throw new TinycDylinkError(
        'tinyc: a main module must be the first module loaded into a process');
    }
    const canonical = canonicalModulePath(options.path ?? '/a.out', '/');
    const lib = libraryFromBytes(canonical, bytes);
    this._libraries.set(canonical, lib);
    const record = this._loadModule(canonical, {
      flags: RTLD_GLOBAL | RTLD_NOW, isMain: true, lib,
    });
    this._main = record;
    this._holder.instance = record.instance;
    // The main thread's pthread_key value array is allocated
    // only now: before the main module's load, the allocator
    // frontier still starts at 1024, inside the span the main
    // module's data image is about to land on.
    const mainThread = this._scheduler?.mainThread;
    if (mainThread && !mainThread.keyValuesAddr) {
      mainThread.keyValuesAddr = this._regionAlloc(128 * 4, 4);
    }
    const exports = record.instance.exports;
    const invoke = (fn, args) => this._jspi
      ? WebAssembly.promising(fn)(...args)
      : Promise.resolve(fn(...args));
    if (options.entry !== undefined) {
      const fn = exports[options.entry];
      if (typeof fn !== 'function') {
        throw new TinycDylinkError(
          `tinyc: main module does not export "${options.entry}"`);
      }
      return invoke(fn, options.callArgs ?? []);
    }
    if (typeof exports._start === 'function') {
      return invoke(exports._start, []).then(() => ({ exitCode: 0 }));
    }
    if (typeof exports.main === 'function') {
      const { argc, argv } = this._mainArgv(options);
      return invoke(exports.main, [argc, argv])
        .then((rc) => ({ exitCode: rc | 0 }));
    }
    throw new TinycDylinkError(
      'tinyc: main module exports neither _start nor main');
  }

  /** Drive a simulated-tier run to process end (§2.3's three
   *  rules, classified by the scheduler), then finalize in
   *  reverse load order on the main thread's context. */
  async _runMainModuleSim(bytes, options) {
    const sched = this._scheduler;
    const main = sched.mainThread;
    main.mainEntry = true;
    main.invoke = () => this._invokeMainThread(bytes, options);
    sched.drivingRun = true;
    sched.runnable.push(main);
    const completion = sched.runCompletion();
    sched.kick();
    const end = await completion;
    sched.drivingRun = false;
    // Finalizers run with the main thread's context installed.
    sched.contextOwner = null;
    this._installThreadContext(main);
    this._teardown();
    if (this._io) flushIO(this._io);
    switch (end.type) {
      case 'exit': return { exitCode: end.code };
      case 'entry': return end.result;
      case 'all-exited': return { exitCode: 0 };
      case 'fault': throw end.error;
      case 'deadlock': throw end.error;
      default: throw new TinycDylinkError('tinyc: the run ended abnormally');
    }
  }

  // -- the real tier: coordinator (threads design §5) ------------------

  _workerUrl() {
    if (this._real.workerUrl) return this._real.workerUrl;
    this._real.workerUrl = this._workerUrlOption ??
      new URL('./worker.mjs', import.meta.url).href;
    return this._real.workerUrl;
  }

  /** Shared-memory setup for the real tier, done once, lazily:
   *  the control block, the pthread key destructor table
   *  (§7.1), the thread-record directory, and the stdin byte
   *  string with its atomic cursor (§5.6). */
  _realSetup() {
    const R = this._real;
    if (R.control) return;
    R.control = this._reserveStatic(64, 4);
    R.keyTableAddr = this._reserveStatic(128 * 4, 4);
    R.dirAddr = this._reserveStatic(DIR_BLOCKS * 4, 4);
    this._regionTable();
    const stdin = this._stdinOption;
    if (stdin !== null && stdin !== undefined) {
      const bytes = typeof stdin === 'string'
        ? textEncoder.encode(stdin) : stdin;
      const addr = this._reserveStatic(bytes.length, 4);
      new Uint8Array(this.memory.buffer).set(bytes, addr);
      this._writeI32(R.control + CB_STDIN_ADDR, addr);
      this._writeI32(R.control + CB_STDIN_LEN, bytes.length);
    }
    R.mainRecordAddr = this._allocThreadRecord(1);
  }

  _allocThreadRecord(tid) {
    const addr = this._reserveStatic(32, 4);
    const dv = new DataView(this.memory.buffer);
    for (let i = 0; i < 32; i += 4) dv.setInt32(addr + i, 0, true);
    this._dirSet(tid, addr);
    return addr;
  }

  /** The directory block covering `tid` (§6.6), or 0 when the
   *  tid is past the table's arithmetic cap. With `create`, a
   *  missing block is reserved, zeroed and published by CAS —
   *  the same protocol the workers use (threads.mjs), so a
   *  race between a worker registering its thread and the
   *  coordinator registering the main record has one winner
   *  and the loser discards its block. */
  _dirBlock(tid, create) {
    const idx = tid >> 13;
    if (idx < 0 || idx >= DIR_BLOCKS) return 0;
    const view = new Int32Array(this.memory.buffer);
    const at = this._real.dirAddr / 4 + idx;
    let block = Atomics.load(view, at);
    if (block || !create) return block;
    const fresh = this._reserveStatic(DIR_BLOCK_TIDS * 4, 4);
    new Uint8Array(this.memory.buffer)
      .fill(0, fresh, fresh + DIR_BLOCK_TIDS * 4);
    const prev = Atomics.compareExchange(view, at, 0, fresh);
    return prev === 0 ? fresh : prev;
  }

  _dirSet(tid, recordAddr) {
    const block = this._dirBlock(tid, true);
    if (!block) return;
    Atomics.store(new Int32Array(this.memory.buffer),
      block / 4 + (tid & (DIR_BLOCK_TIDS - 1)), recordAddr);
  }

  _dirClear(tid) {
    const block = this._dirBlock(tid, false);
    if (!block) return;
    Atomics.store(new Int32Array(this.memory.buffer),
      block / 4 + (tid & (DIR_BLOCK_TIDS - 1)), 0);
  }

  // -- the thread-region allocator (§3.1) ---------------------

  /** The shared class table for thread-region size classes,
   *  reserved once per threaded process. */
  _regionTable() {
    if (!this._regionTableAddr) {
      const addr = this._reserveStatic(REGION_TABLE_SIZE, 16);
      new Uint8Array(this.memory.buffer)
        .fill(0, addr, addr + REGION_TABLE_SIZE);
      this._regionTableAddr = addr;
    }
    return this._regionTableAddr;
  }

  _regionLock() {
    const view = new Int32Array(this.memory.buffer);
    while (Atomics.compareExchange(view, ALLOC_LOCK_WORD / 4, 0, 1) !== 0) {
      // Never held across a wait; a plain spin is brief.
    }
  }

  _regionUnlock() {
    const view = new Int32Array(this.memory.buffer);
    Atomics.store(view, ALLOC_LOCK_WORD / 4, 0);
    Atomics.notify(view, ALLOC_LOCK_WORD / 4, 1);
  }

  /** Find (or create) the class entry for (size, align); the
   *  caller holds the region lock. */
  _regionClass(size, align, create) {
    const table = this._regionTableAddr;
    const dv = new DataView(this.memory.buffer);
    const count = dv.getInt32(table, true);
    for (let i = 0; i < count; i++) {
      const at = table + 16 + i * 16;
      if (dv.getInt32(at, true) === size &&
          dv.getInt32(at + 4, true) === align) return at;
    }
    if (!create || count >= REGION_CLASSES) return 0;
    const at = table + 16 + count * 16;
    dv.setInt32(at, size, true);
    dv.setInt32(at + 4, align, true);
    dv.setInt32(at + 8, 0, true);
    dv.setInt32(at + 12, 0, true);
    dv.setInt32(table, count + 1, true);
    return at;
  }

  /** Allocate a thread region of an exact size class: a
   *  reaped thread's region of the same class is recycled
   *  before the bump frontier advances. In a non-threaded
   *  process there is nothing to reclaim — the single thread
   *  lives to process end — and the plain reserve serves, so
   *  the 0.4.0 layout is untouched. */
  _regionAlloc(size, align) {
    align = align || 4;
    if (!this._threaded) return this._reserveStatic(size, align);
    this._regionTable();
    let addr = 0;
    this._regionLock();
    try {
      const cls = this._regionClass(size, align, true);
      if (cls) {
        const head = this._readI32(cls + 8);
        if (head) {
          addr = head;
          this._writeI32(cls + 8, this._readI32(head));
        }
      }
    } finally {
      this._regionUnlock();
    }
    if (!addr) return this._reserveStatic(size, align);
    if (size <= REGION_ZERO_MAX) {
      new Uint8Array(this.memory.buffer).fill(0, addr, addr + size);
    }
    return addr;
  }

  /** Return a thread region to its class free list. */
  _regionFree(addr, size, align) {
    if (!addr || !this._threaded) return;
    align = align || 4;
    this._regionTable();
    this._regionLock();
    try {
      const cls = this._regionClass(size, align, true);
      if (cls) {
        this._writeI32(addr, this._readI32(cls + 8));
        this._writeI32(cls + 8, addr);
      }
    } finally {
      this._regionUnlock();
    }
  }

  _bootData(tid, entry, extra = {}) {
    const R = this._real;
    const modules = this._loadOrder.map((record) => ({
      canonical: record.canonical,
      memoryBase: record.memoryBase,
      tableBase: record.tableBase,
      handle: record.handle,
      global: record.global ? 1 : 0,
      manifest: record.lib.manifest,
      bodyBytes: record.bodyBytes ?? record.lib.bodyBytes,
      exports: [...record.exports].map(([name, e]) =>
        [name, { kind: e.kind, value: e.value, tls: e.tls ? 1 : 0 }]),
      needed: [...record.meta.needed],
    }));
    const got = [];
    for (const [key, e] of this._got) {
      if (!e.filled) continue;
      const [kind, name] = [key.slice(0, key.indexOf(':')), key.slice(key.indexOf(':') + 1)];
      got.push(kind === 'tls'
        ? { kind, name, definer: e.definer?.canonical ?? '', offset: e.offset }
        : { kind, name, value: e.global.value });
    }
    const slots = [...this._slotDesc].map(([slot, d]) => ({ slot, ...d }));
    return {
      type: 'tinyc-thread-boot',
      threadId: tid,
      isMain: tid === 1,
      memory: this.memory,
      controlAddr: R.control,
      keyTableAddr: R.keyTableAddr,
      dirAddr: R.dirAddr,
      regionTableAddr: this._regionTableAddr,
      hostImports: [...this._hostProvidedFns.keys()],
      recordAddr: extra.recordAddr ?? 0,
      keyArrayAddr: extra.keyArrayAddr ?? 0,
      args: [...this._args],
      env: { ...R.envMap },
      tableFree: this._tableFree,
      modules, got, slots,
      entry,
      regions: extra.regions ?? {},
    };
  }

  _spawnWorker(tid, boot, info = {}) {
    const worker = new Worker(this._workerUrl(), { type: 'module' });
    const entry = {
      worker, recordAddr: info.recordAddr ?? 0,
      alive: true, isMain: Boolean(info.isMain),
    };
    this._real.workers.set(tid, entry);
    worker.addEventListener('message', (ev) => this._onWorkerMessage(tid, ev.data));
    worker.addEventListener('error', (ev) => {
      this._settleMainRun({
        type: 'fault',
        error: new Error(
          `tinyc: thread ${tid}'s worker failed: ${ev.message ?? 'unknown error'}`),
      });
    });
    if (entry.recordAddr) {
      // A worker booted with the current layout has, by
      // construction, every replica the last task posted.
      this._writeI32(entry.recordAddr + TR_ACK, this._real.nextLoadSeq - 1);
    }
    worker.postMessage(boot);
    return entry;
  }

  _settleMainRun(end) {
    const R = this._real;
    if (!R || !R.mainRun) return;
    const { resolve } = R.mainRun;
    R.mainRun = null;
    resolve(end);
  }

  /** Drive a real-tier run: lay out the main module, boot the
   *  main thread's worker with it, and wait for the process
   *  end the workers report (§2.3). Finalization runs on the
   *  coordinator's replicas (§5.5). */
  async _runMainModuleReal(bytes, options) {
    if (this._main) {
      throw new TinycDylinkError('tinyc: this process already has a main module');
    }
    if (this._modules.size > 0) {
      throw new TinycDylinkError(
        'tinyc: a main module must be the first module loaded into a process');
    }
    const canonical = canonicalModulePath(options.path ?? '/a.out', '/');
    const lib = libraryFromBytes(canonical, bytes);
    this._libraries.set(canonical, lib);
    const record = this._loadModule(canonical, {
      flags: RTLD_GLOBAL | RTLD_NOW, isMain: true, lib,
    });
    this._main = record;
    this._holder.instance = record.instance;
    this._realSetup();
    const regions = {};
    for (const rec of this._loadOrder) {
      regions[rec.canonical] = {
        stackBase: null,
        tlsArea: rec.meta.tlsSize > 0 ? this._tlsAreaFor(1, rec) : 0,
      };
    }
    const boot = this._bootData(1, {
      kind: 'main', canonical,
      runOptions: {
        entry: options.entry ?? null,
        callArgs: options.callArgs ?? [],
      },
    }, { recordAddr: this._real.mainRecordAddr, regions });
    const settled = new Promise((resolve) => {
      this._real.mainRun = { resolve };
    });
    this._spawnWorker(1, boot, { isMain: true, recordAddr: this._real.mainRecordAddr });
    const end = await settled;
    this._realTeardown();
    if (this._io) flushIO(this._io);
    switch (end.type) {
      case 'exit': return { exitCode: end.code };
      case 'entry': return end.result;
      case 'all-exited': return { exitCode: 0 };
      case 'fault': throw end.error;
      default: throw new TinycDylinkError('tinyc: the run ended abnormally');
    }
  }

  /** Process end in the real tier is a coordinator act (§5.5):
   *  finalize every module on the coordinator's replica — with
   *  the module's stack top installed through the swap export
   *  first (§5.1) — in reverse load order, then terminate the
   *  remaining workers. */
  _realTeardown() {
    if (this._ended) return;
    this._ended = true;
    for (let i = this._loadOrder.length - 1; i >= 0; i--) {
      const rec = this._loadOrder[i];
      if (rec.state !== 'loaded' && rec.state !== 'instantiated') continue;
      rec.state = 'closed';
      try {
        const swap = rec.instance.exports.__tinyc_dylink_swap_sp;
        if (typeof swap === 'function') {
          swap(rec.memoryBase + rec.meta.memSize);
        }
        rec.instance.exports.__tinyc_dylink_fini(rec.handle, 1);
      } catch { /* a trapping finalizer cannot stop teardown */ }
    }
    for (const { worker } of this._real.workers.values()) {
      try { worker.terminate(); } catch { /* already gone */ }
    }
    this._real.workers.clear();
  }

  /** §5.4: broadcast a load delta to every live worker and
   *  collect acknowledgements. initBy 'coordinator' runs the
   *  inits here once all replicas exist; 'requester' leaves
   *  init to the requesting worker, which signals through the
   *  load-done word. Resolves when the load is complete
   *  process-wide. */
  async _realBroadcastLoad(delta, initBy, requesterTid) {
    const R = this._real;
    const seq = R.nextLoadSeq++;
    const payloadAddr = this._writeLoadTask(seq, delta, initBy, requesterTid);
    const view = new Int32Array(this.memory.buffer);
    Atomics.store(view, (R.control + CB_TASK_ADDR) / 4, payloadAddr);
    Atomics.store(view, (R.control + CB_LOAD_SEQ) / 4, seq);
    Atomics.notify(view, (R.control + CB_LOAD_SEQ) / 4);
    // Collect acks: every live worker's record word reaches
    // seq. Workers spawned later boot with the post-load
    // layout and count as acked from their spawn (§5.4).
    for (;;) {
      let done = true;
      for (const [tid, w] of R.workers) {
        if (!w.alive) continue;
        if (!w.recordAddr) continue;
        if (this._readI32(w.recordAddr + TR_ACK) < seq) { done = false; break; }
      }
      if (done) break;
      await new Promise((r) => setTimeout(r, 2));
    }
    Atomics.store(view, (R.control + CB_ACKS_DONE) / 4, seq);
    Atomics.notify(view, (R.control + CB_ACKS_DONE) / 4);
    if (initBy === 'coordinator') {
      // Publish each module's baked stack top (the value its
      // init left in the stack pointer) for the main thread's
      // worker, whose replica never runs init (§3.1, §5.4).
      const topsAddr = this._reserveStatic(delta.length * 4, 4);
      delta.forEach((rec, i) => {
        this._initRecord(rec);
        const swap = rec.instance.exports.__tinyc_dylink_swap_sp;
        let baked = 0;
        if (typeof swap === 'function') { baked = swap(0); swap(baked); }
        this._writeI32(topsAddr + i * 4, baked);
      });
      this._writeI32(payloadAddr + 36, topsAddr);
      Atomics.store(view, (R.control + CB_LOAD_DONE) / 4, seq);
      Atomics.notify(view, (R.control + CB_LOAD_DONE) / 4);
    } else {
      // The requesting worker runs init and sets the word.
      while (this._readI32(R.control + CB_LOAD_DONE) < seq) {
        if (this._readI32(R.control + CB_LOAD_FAILED) === seq) {
          throw new TinycDylinkError(this._readTaskError(payloadAddr));
        }
        await new Promise((r) => setTimeout(r, 2));
      }
      for (const rec of delta) rec.state = 'loaded';
    }
    R.lastTaskIndex = this._loadOrder.length;
    // Spawns gated on this load may proceed with the new layout.
    for (const spawn of R.spawnQueue.splice(0)) {
      this._spawnWorker(spawn.tid, this._bootData(spawn.tid,
        { kind: 'start', slot: spawn.slot, arg: spawn.arg },
        { recordAddr: spawn.recordAddr, regions: spawn.regions,
          keyArrayAddr: spawn.keyArrayAddr }),
        { recordAddr: spawn.recordAddr });
    }
  }

  _readTaskError(payloadAddr) {
    const dv = new DataView(this.memory.buffer);
    const len = dv.getInt32(payloadAddr + 40, true);
    const bytes = new Uint8Array(this.memory.buffer)
      .subarray(payloadAddr + 44, payloadAddr + 44 + len);
    return `tinyc: load failed in the requesting worker: ` +
      textDecoder.decode(bytes);
  }

  /** Serialize a load task into shared memory (see threads.mjs
   *  for the reader). Returns the payload address. */
  _writeLoadTask(seq, delta, initBy, requesterTid) {
    const enc = textEncoder;
    const chunks = [];
    let size = 44;
    const push = (bytes) => { chunks.push(bytes); size += bytes.length; };
    const gotEntries = [];
    for (const [key, e] of this._got) {
      if (!e.filled) continue;
      const idx = key.indexOf(':');
      gotEntries.push({
        kind: key.slice(0, idx), name: key.slice(idx + 1),
        value: e.global.value,
        definer: e.definer?.canonical ?? '', offset: e.offset ?? 0,
      });
    }
    const head = new DataView(new ArrayBuffer(44));
    const requested = delta[delta.length - 1];
    head.setInt32(0, seq, true);
    head.setInt32(4, initBy === 'requester' ? 1 : 0, true);
    head.setInt32(8, requesterTid, true);
    head.setInt32(12, requested ? requested.handle : 0, true);
    head.setInt32(16, delta.length, true);
    head.setInt32(20, this._tableFree, true);
    head.setInt32(24, gotEntries.length, true);
    head.setInt32(28, this._slotDesc.size, true);
    push(new Uint8Array(head.buffer));
    const w32 = (v) => {
      const b = new Uint8Array(4);
      new DataView(b.buffer).setInt32(0, v | 0, true);
      push(b);
    };
    const wstr = (s) => {
      const bytes = enc.encode(s);
      w32(bytes.length);
      push(bytes);
      const pad = (4 - (bytes.length % 4)) % 4;
      if (pad) push(new Uint8Array(pad));
    };
    for (const g of gotEntries) {
      w32(g.kind === 'func' ? 0 : g.kind === 'mem' ? 1 : 2);
      wstr(g.name);
      w32(g.value);
      wstr(g.definer);
      w32(g.offset);
    }
    for (const [slot, d] of this._slotDesc) {
      w32(slot);
      w32(d.kind === 'stub' ? 0 : d.kind === 'shim' ? 1 : 2);
      wstr(d.name);
      wstr(d.params.join(','));
      wstr(d.results.join(','));
    }
    for (const rec of delta) {
      wstr(rec.canonical);
      w32(rec.memoryBase);
      w32(rec.tableBase);
      w32(rec.handle);
      w32(rec.global ? 1 : 0);
      const manifestJson = enc.encode(JSON.stringify(rec.lib.manifest));
      w32(manifestJson.length);
      push(manifestJson);
      const pad = (4 - (manifestJson.length % 4)) % 4;
      if (pad) push(new Uint8Array(pad));
      const body = rec.bodyBytes ?? rec.lib.bodyBytes;
      w32(body.length);
      push(body);
      const pad2 = (4 - (body.length % 4)) % 4;
      if (pad2) push(new Uint8Array(pad2));
    }
    const hostImports = [...this._hostProvidedFns.keys()];
    w32(hostImports.length);
    for (const key of hostImports) wstr(key);
    const addr = this._reserveStatic(size, 4);
    const m = new Uint8Array(this.memory.buffer);
    let at = addr;
    for (const c of chunks) { m.set(c, at); at += c.length; }
    return addr;
  }

  /** A guest dlopen from a worker (§5.4): resolve and lay out
   *  here, broadcast, and let the requesting worker run init
   *  and complete its own call. Serialized with other loads. */
  async _realGuestLoad(msg) {
    const R = this._real;
    R.loadQueue.push(msg);
    if (R.loadsInFlight > 0) return;
    while (R.loadQueue.length > 0) {
      const req = R.loadQueue[0];
      R.loadsInFlight++;
      try {
        const canonical = canonicalModulePath(req.path, '/sysroot');
        await this._prefetchClosure(canonical);
        const before = this._loadOrder.length;
        this._loadModule(canonical, {
          flags: req.flags, requester: null, runtimeEdge: true,
        });
        const delta = this._loadOrder.slice(before);
        await this._realBroadcastLoad(delta, 'requester', req.fromTid);
      } catch (err) {
        // Name the failure in the task area the requester polls.
        const seq = R.nextLoadSeq;
        const bytes = textEncoder.encode(err.message);
        const addr = this._reserveStatic(44 + bytes.length + 4, 4);
        const dv = new DataView(this.memory.buffer);
        dv.setInt32(addr + 8, req.fromTid, true);
        dv.setInt32(addr + 40, bytes.length, true);
        new Uint8Array(this.memory.buffer).set(bytes, addr + 44);
        const view = new Int32Array(this.memory.buffer);
        Atomics.store(view, (R.control + CB_TASK_ADDR) / 4, addr);
        Atomics.store(view, (R.control + CB_LOAD_SEQ) / 4, seq);
        Atomics.store(view, (R.control + CB_LOAD_FAILED) / 4, seq);
        Atomics.notify(view, (R.control + CB_LOAD_SEQ) / 4);
        R.nextLoadSeq++;
      } finally {
        R.loadsInFlight--;
        R.loadQueue.shift();
      }
    }
  }

  /** Delegate a host callExport to the main thread's worker
   *  (§5.5): the call runs on the main thread's context at its
   *  next wait point, and the result returns by message. */
  _realCallExport(record, name, args) {
    const R = this._real;
    const main = R.workers.get(1);
    if (!main || !main.alive) {
      throw new TinycDylinkError(
        'tinyc: callExport: a real-tier process delegates calls ' +
        'to its main thread worker, and none is live');
    }
    for (const a of args) {
      if (typeof a !== 'number') {
        throw new TinycDylinkError(
          'tinyc: callExport: real-tier delegation carries i32 arguments only');
      }
    }
    const seq = R.nextCallSeq++;
    const enc = textEncoder;
    const cBytes = enc.encode(record.canonical);
    const nBytes = enc.encode(name);
    const size = 16 + cBytes.length + 4 + nBytes.length + 4 + args.length * 4;
    const addr = this._reserveStatic(size + 8, 4);
    const dv = new DataView(this.memory.buffer);
    const m = new Uint8Array(this.memory.buffer);
    let at = addr;
    dv.setInt32(at, seq, true); at += 4;
    dv.setInt32(at, 0, true); at += 4;   // result value (worker's)
    dv.setInt32(at, cBytes.length, true); at += 4;
    m.set(cBytes, at); at += cBytes.length;
    at += (4 - (cBytes.length % 4)) % 4;
    dv.setInt32(at, nBytes.length, true); at += 4;
    m.set(nBytes, at); at += nBytes.length;
    at += (4 - (nBytes.length % 4)) % 4;
    dv.setInt32(at, args.length, true); at += 4;
    for (const a of args) { dv.setInt32(at, a | 0, true); at += 4; }
    const view = new Int32Array(this.memory.buffer);
    Atomics.store(view, (R.control + CB_CALL_TASK_ADDR) / 4, addr);
    Atomics.store(view, (R.control + CB_CALL_SEQ) / 4, seq);
    Atomics.notify(view, (R.control + CB_CALL_SEQ) / 4);
    return new Promise((resolve, reject) => {
      R.callWaiters.set(seq, { resolve, reject });
    });
  }

  /** Reap a created thread in the real tier (§6.6). The
   *  exiting worker has already returned the thread's big
   *  regions — its per-module stacks and TLS areas and its
   *  key array — before its exit became observable; what
   *  remains here is the record itself, freed last, and its
   *  directory entry. The main thread's record is never
   *  reaped; it lives to teardown. */
  _reapRealThread(tid) {
    const w = this._real?.workers.get(tid);
    if (!w || w.reaped || !w.recordAddr) return;
    w.reaped = true;
    this._regionFree(w.recordAddr, 32, 4);
    this._dirClear(tid);
  }

  /** Execute one proxied host call (§5.7). The worker that
   *  owns the block is blocked on its futex word; the
   *  coordinator never blocks — a synchronous result is
   *  written back at once, a thenable's when it settles, and
   *  in both cases the answer lands in the block before the
   *  word is set and notified. Blocks are per thread, so
   *  concurrent calls from different workers proceed and
   *  complete in any order. A throw or a rejection crosses
   *  back as the error text the worker's stub rethrows. */
  _realHostCall(msg) {
    const base = msg.slot;
    if (!base) return;
    const dv = new DataView(this.memory.buffer);
    const m = new Uint8Array(this.memory.buffer);
    const view = new Int32Array(this.memory.buffer);
    const finish = (status, apply) => {
      try { if (apply) apply(); } finally {
        dv.setInt32(base + PX_STATUS, status, true);
        Atomics.store(view, base / 4, PX_DONE);
        Atomics.notify(view, base / 4);
      }
    };
    const fail = (text) => finish(PX_ERROR, () => {
      const bytes = textEncoder.encode(String(text))
        .subarray(0, PX_ERR_MAX);
      m.set(bytes, base + PX_ERR);
      dv.setInt32(base + PX_ERRLEN, bytes.length, true);
    });
    try {
      const nameLen = dv.getInt32(base + PX_NAMELEN, true);
      const key = textDecoder.decode(
        m.slice(base + PX_NAME, base + PX_NAME + nameLen));
      const fn = this._hostProvidedFns.get(key);
      if (typeof fn !== 'function') {
        fail(`tinyc: host function "${key}" is not registered ` +
          `with this process`);
        return;
      }
      const nargs = dv.getInt32(base + PX_NARGS, true);
      const args = [];
      for (let i = 0; i < nargs; i++) {
        const code = m[base + PX_ARGTYPES + i];
        const at = base + PX_ARGS + i * 8;
        if (code === 1) args.push(dv.getInt32(at, true));
        else if (code === 2) args.push(dv.getBigInt64(at, true));
        else if (code === 3) args.push(dv.getFloat32(at, true));
        else if (code === 4) args.push(dv.getFloat64(at, true));
        else {
          fail(`tinyc: host function "${key}": argument ${i} ` +
            `carries an unknown type code ${code}`);
          return;
        }
      }
      const resCode = dv.getInt32(base + PX_RESTYPE, true);
      const writeResult = (value) => finish(PX_OK, () => {
        const at = base + PX_RESULT;
        if (resCode === 1) dv.setInt32(at, value | 0, true);
        else if (resCode === 2) {
          let b;
          try {
            b = typeof value === 'bigint' ? value : BigInt(value);
          } catch {
            b = BigInt(Math.trunc(Number(value) || 0));
          }
          dv.setBigInt64(at, b, true);
        }
        else if (resCode === 3) dv.setFloat32(at, Number(value), true);
        else if (resCode === 4) dv.setFloat64(at, Number(value), true);
      });
      const out = fn(...args);
      if (out && typeof out.then === 'function') {
        out.then(writeResult,
          (err) => fail(err && err.message ? err.message : String(err)));
      } else {
        writeResult(out);
      }
    } catch (err) {
      fail(err && err.message ? err.message : String(err));
    }
  }

  _onWorkerMessage(tid, msg) {
    const R = this._real;
    if (!R || !msg || typeof msg !== 'object') return;
    switch (msg.type) {
      case 'io': {
        const bytes = msg.bytes instanceof Uint8Array
          ? msg.bytes : new Uint8Array(msg.bytes);
        if (msg.stream === 'err') this._byteIO.err(bytes);
        else this._byteIO.out(bytes);
        break;
      }
      case 'thread-spawn': {
        const spawn = {
          tid: msg.threadId, slot: msg.slot, arg: msg.arg,
          recordAddr: msg.recordAddr, regions: msg.regions,
          keyArrayAddr: msg.keyArrayAddr,
        };
        if (R.loadsInFlight > 0) { R.spawnQueue.push(spawn); break; }
        this._spawnWorker(spawn.tid, this._bootData(spawn.tid,
          { kind: 'start', slot: spawn.slot, arg: spawn.arg },
          { recordAddr: spawn.recordAddr, regions: spawn.regions,
            keyArrayAddr: spawn.keyArrayAddr }),
          { recordAddr: spawn.recordAddr });
        break;
      }
      case 'thread-exit': {
        const w = R.workers.get(tid);
        if (w) w.alive = false;
        if (tid === 1) R.mainExited = true;
        if (w && tid !== 1) {
          w.exited = true;
          // Reap (§6.6): a detached thread is reaped at its
          // exit; a joinable one when the joiner's reap note
          // arrives. The two messages come from different
          // workers and race, so either may land first.
          const detached = w.recordAddr &&
            this._readI32(w.recordAddr + TR_DETACHED) !== 0;
          if (detached || w.reapAsked) this._reapRealThread(tid);
        }
        // Rule 3: the main thread pthread_exited and every
        // other thread has since exited.
        if (R.mainRun && R.mainExited) {
          let anyAlive = false;
          for (const o of R.workers.values()) if (o.alive) anyAlive = true;
          if (!anyAlive) this._settleMainRun({ type: 'all-exited' });
        }
        break;
      }
      case 'thread-reap': {
        // A join has taken the thread's exit value (or a
        // detach found it already exited): the record may go
        // once the exit itself has been seen.
        const w = R.workers.get(msg.threadId);
        if (w && msg.threadId !== 1) {
          w.reapAsked = true;
          if (w.exited) this._reapRealThread(msg.threadId);
        }
        break;
      }
      case 'host-call': {
        this._realHostCall(msg);
        break;
      }
      case 'main-returned':
        this._settleMainRun({ type: 'entry', result: msg.result });
        break;
      case 'process-exit':
        this._settleMainRun({ type: 'exit', code: msg.code | 0 });
        break;
      case 'thread-fault':
        this._settleMainRun({
          type: 'fault',
          error: new Error(`tinyc: thread ${tid}: ${msg.message}`),
        });
        break;
      case 'dlopen-request':
        void this._realGuestLoad(msg);
        break;
      case 'dlclose-note': {
        const record = this._handles.get(msg.handle);
        if (record) {
          try { this._dlclose(msg.handle, null); } catch { /* accounting only */ }
        }
        break;
      }
      case 'call-done': {
        const waiter = R.callWaiters.get(msg.seq);
        if (waiter) {
          R.callWaiters.delete(msg.seq);
          if (msg.ok) waiter.resolve(msg.value);
          else waiter.reject(new TinycDylinkError(msg.message));
        }
        break;
      }
      default:
        break;
    }
  }

  _runMainModuleSync(bytes, options = {}) {
    if (this._main) {
      throw new TinycDylinkError('tinyc: this process already has a main module');
    }
    if (this._modules.size > 0) {
      throw new TinycDylinkError(
        'tinyc: a main module must be the first module loaded into a process');
    }
    const canonical = canonicalModulePath(options.path ?? '/a.out', '/');
    const lib = libraryFromBytes(canonical, bytes);
    this._libraries.set(canonical, lib);
    const record = this._loadModule(canonical, {
      flags: RTLD_GLOBAL | RTLD_NOW, isMain: true, lib,
    });
    this._main = record;
    this._holder.instance = record.instance;
    const exports = record.instance.exports;
    let result;
    try {
      if (options.entry !== undefined) {
        const fn = exports[options.entry];
        if (typeof fn !== 'function') {
          throw new TinycDylinkError(
            `tinyc: main module does not export "${options.entry}"`);
        }
        result = fn(...(options.callArgs ?? []));
      } else if (typeof exports._start === 'function') {
        exports._start();
        result = { exitCode: 0 };
      } else if (typeof exports.main === 'function') {
        result = { exitCode: this._callMain(record, options) };
      } else {
        throw new TinycDylinkError(
          'tinyc: main module exports neither _start nor main');
      }
    } catch (err) {
      if (err instanceof WasiExit || err instanceof ShimExit) {
        result = { exitCode: err.exitCode };
      } else {
        this._teardown();
        throw err;
      }
    }
    this._teardown();
    if (this._io) flushIO(this._io);
    return result;
  }

  /** Build the (argc, argv) pair for a main entry in the
   *  process's memory. */
  _mainArgv(options) {
    const args = (options.args ?? this._args).map(String);
    const strBytes = args.map((a) => textEncoder.encode(a + '\0'));
    const total = strBytes.reduce((n, b) => n + b.length, 0) + (args.length + 1) * 4;
    const block = this._reserveStatic(total, 4);
    const m = new Uint8Array(this.memory.buffer);
    const dv = new DataView(this.memory.buffer);
    let sp = block + (args.length + 1) * 4;
    args.forEach((a, i) => {
      dv.setUint32(block + i * 4, sp, true);
      m.set(strBytes[i], sp);
      sp += strBytes[i].length;
    });
    dv.setUint32(block + args.length * 4, 0, true);
    return { argc: args.length, argv: block };
  }

  _callMain(record, options) {
    const { argc, argv } = this._mainArgv(options);
    return record.instance.exports.main(argc, argv);
  }

  /** End the process: finalize every loaded module in reverse
   *  load order with the reserved flag set (process ending). */
  _teardown() {
    if (this._ended) return;
    this._ended = true;
    for (let i = this._loadOrder.length - 1; i >= 0; i--) {
      const rec = this._loadOrder[i];
      if (rec.state !== 'loaded') continue;
      rec.state = 'closed';
      try {
        rec.instance.exports.__tinyc_dylink_fini(rec.handle, 1);
      } catch { /* a trapping finalizer cannot stop teardown */ }
    }
  }

  dispose() {
    if (this.threadTier === 'real') {
      for (const { worker } of this._real.workers.values()) {
        try { worker.terminate(); } catch { /* already gone */ }
      }
      this._real.workers.clear();
    }
    this._teardown();
  }
}

/** Create a standalone dylink process (no Tinyc instance):
 *  libraries resolve through options.readModule and retrieval
 *  from options.baseUrl. See TinycProcess for the options. */
export function createProcess(options = {}) {
  return new TinycProcess(options);
}

// ---------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------

// Per-target native module cache: a target's module is loaded at
// most once, and only when an instance of that target is actually
// requested.
const targetModuleCache = new Map();

/** Targets whose native module has been loaded through the
 *  per-target loader (loadModule or the shipped envelope files). */
export function getLoadedTargets() {
  return [...targetModuleCache.keys()];
}

export function clearTargetModuleCache() {
  targetModuleCache.clear();
}

/* The compiler module from its asset envelope, retrieved through
 * the transport. */
async function createModuleFromAssetEnvelope(transport, envelopeUrl, target, io) {
  const asset = await retrieveAsset(
    transport, envelopeUrl, `tcc-${target}`);
  return buildModuleFromAsset(asset, envelopeUrl, target, io);
}

/** Build the compiler module from an asset envelope's payload —
 *  the one build path, shared by the transport retrieval and by
 *  the loadModule hook: the envelope registers the glue source
 *  text and the module bytes; the factory is built by importing
 *  the glue source from a blob URL (a module script cannot be
 *  imported from a file: page, and the envelope itself is a
 *  classic script, not a module), and the module bytes are handed
 *  to the factory directly, so the glue never fetches. */
async function buildModuleFromAsset(asset, envelopeUrl, target, io) {
  if (!asset || typeof asset.glue !== 'string' || typeof asset.wasm !== 'string') {
    throw new TinycError(
      `tinyc: asset envelope at ${envelopeUrl} carries no compiler module for target "${target}"`);
  }
  let factory;
  const blobUrl = URL.createObjectURL(
    new Blob([asset.glue], { type: 'text/javascript' }));
  try {
    factory = (await import(/* @vite-ignore */ blobUrl)).default;
  } catch {
    /* Some module loaders (Node's) refuse blob: URLs; a data: URL
     * carries the same source there. */
    try {
      const dataUrl = 'data:text/javascript;base64,' +
        bytesToBase64(textEncoder.encode(asset.glue));
      factory = (await import(/* @vite-ignore */ dataUrl)).default;
    } catch (err) {
      throw new TinycError(
        `tinyc: cannot build the compiler factory from ${envelopeUrl}: ${err.message}`);
    }
  } finally {
    URL.revokeObjectURL(blobUrl);
  }
  if (typeof factory !== 'function') {
    throw new TinycError(
      `tinyc: compiler glue in ${envelopeUrl} does not default-export a module factory`);
  }
  const wasmBytes = base64ToBytes(asset.wasm);
  return factory({
    print: (text) => io.out(text + '\n'),
    printErr: (text) => io.err(text + '\n'),
    /* The glue's own instantiation override: it hands us the
     * import object and takes the instance back, so the module
     * bytes come from the envelope and the glue never fetches. */
    instantiateWasm: (imports, success) => {
      WebAssembly.instantiate(wasmBytes, imports).then(
        ({ instance }) => success(instance));
      return {};
    },
    locateFile: (path, prefix) => prefix + path,
  });
}

/** The loadModule hook's half of module loading: the hook returns
 *  the envelope's content — its text, or its bytes as a Uint8Array
 *  or ArrayBuffer — and the package parses the envelope and builds
 *  the module from it through the same parse and build code as a
 *  directly retrieved envelope. Anything else the hook returns (a
 *  built module object above all) is rejected by name: a module
 *  can only be supplied as envelope content. */
async function buildModuleFromHookContent(content, envelopeUrl, target, io) {
  let text;
  if (typeof content === 'string') {
    text = content;
  } else if (content instanceof ArrayBuffer || ArrayBuffer.isView(content)) {
    text = textDecoder.decode(content);
  } else {
    throw new TinycError(
      `tinyc: the loadModule hook for target "${target}" returned ` +
      `${content === null ? 'null' : typeof content}; it must return the ` +
      `asset envelope's content — the envelope file's text or its bytes — ` +
      `and the package builds the module from it`);
  }
  const asset = parseAssetEnvelopeText(text, envelopeUrl);
  return buildModuleFromAsset(asset, envelopeUrl, target, io);
}

async function loadTargetModule(target, options, io) {
  const cached = targetModuleCache.get(target);
  if (cached) return cached;
  const files = targetModuleFiles(target);
  const promise = (async () => {
    if (typeof options.loadModule === 'function') {
      const envelopeUrl = assetUrl(files.envelopeFile, options.baseUrl);
      const content = await options.loadModule(target, {
        ...files,
        envelopeUrl,
      });
      return buildModuleFromHookContent(content, envelopeUrl, target, io);
    }
    /* The compiler's own bytes go through the §14 transport like
     * every other byte the package loads from its origin: the
     * compiler asset envelope is the only shipped form — under
     * fetch it is read as text and parsed, under the script
     * transport it is injected and read back from the registry,
     * and either way the module is built from the envelope's glue
     * text and bytes. (The loadModule hook above replaces this
     * retrieval: the caller returns the same envelope's content
     * and the package builds the module from it.) */
    const transport = createTransport(options.transport);
    const envelopeUrl = assetUrl(files.envelopeFile, options.baseUrl);
    return createModuleFromAssetEnvelope(transport, envelopeUrl, target, io);
  })();
  targetModuleCache.set(target, promise);
  promise.catch(() => {
    if (targetModuleCache.get(target) === promise) targetModuleCache.delete(target);
  });
  return promise;
}



/** The sysroot manifest for one target (sysroot-manifests/<target>.json).
 *  All manifests are statically imported with this module (see
 *  SYSROOT_MANIFESTS above), so this never fetches at runtime; the
 *  default target's manifest is also exported as `sysrootManifest`. */
export async function loadSysrootManifest(target = DEFAULT_TARGET) {
  assertKnownTarget(target);
  return SYSROOT_MANIFESTS[target];
}

async function loadSysroot(instance, sysrootOpt, transportOption) {
  if (!sysrootOpt) return;
  let manifest;
  let baseUrl = './sysroot/';
  let fetchImpl;
  let includeBinaries = false;
  if (sysrootOpt === true) {
    manifest = await loadSysrootManifest(instance.target);
  } else if (Array.isArray(sysrootOpt.includeFiles)) {
    manifest = sysrootOpt; // a bare manifest object
  } else {
    manifest = sysrootOpt.manifest ?? (await loadSysrootManifest(instance.target));
    baseUrl = sysrootOpt.baseUrl ?? baseUrl;
    fetchImpl = sysrootOpt.fetchImpl;
    includeBinaries = Boolean(sysrootOpt.includeBinaries);
  }
  const baseHref = /^[a-z][a-z0-9+.-]*:/i.test(baseUrl)
    ? baseUrl
    : assetUrl(baseUrl);
  const entries = [...(manifest.includeFiles ?? []), ...(manifest.libFiles ?? [])];
  /* The sysroot files are compiler assets: they travel through the
   * §14 transport as one envelope file sitting in the sysroot base
   * directory, next to the files it replaces — under fetch it is
   * read as text and parsed, under the script transport it is
   * injected and read back from the registry, and either way each
   * entry's bytes come out of the envelope, keyed by the entry's
   * manifest source. The envelope is retrieved at most once per
   * load. A fetchImpl handed in by the caller stays authoritative
   * for the fetch side. */
  const transport = createTransport(
    sysrootOpt?.transport ?? transportOption ?? (fetchImpl ? 'fetch' : undefined));
  let envelope = null;
  let envelopeUrl = null;
  for (const entry of entries) {
    if (entry.binary && !includeBinaries) continue;
    // Already in the virtual FS — persisted by an earlier load of
    // this target and restored from the store — is not fetched again.
    if (instance.fs.hasFile(entry.path)) continue;
    if (!envelope) {
      envelopeUrl = new URL(
        `sysroot-${instance.target}.tinyc.js`, baseHref).href;
      envelope = await retrieveAsset(
        transport, envelopeUrl, `sysroot-${instance.target}`, fetchImpl);
      if (!envelope || typeof envelope.files !== 'object' || !envelope.files) {
        throw new TinycError(
          `tinyc: asset envelope at ${envelopeUrl} carries no sysroot for target "${instance.target}"`);
      }
    }
    const b64 = envelope.files[entry.source];
    if (typeof b64 !== 'string') {
      throw new TinycError(
        `tinyc: sysroot file "${entry.source}" is missing from the asset envelope at ${envelopeUrl}`);
    }
    await instance.fs.addFile(entry.path, base64ToBytes(b64));
  }
  if (manifest.includeDir) instance.fs.registerIncludeDir(manifest.includeDir);
}

// ---------------------------------------------------------------------
// WasmFS OPFS mount
//
// The native module is built with -sWASMFS=1 (see build.sh): its
// filesystem root is the WasmFS memory backend. Before the virtual
// FS is mirrored in, loadTinyc tries to mount the WasmFS OPFS
// backend — the browser's on-disk origin private file system — at
// OPFS_MOUNT_POINT, through the C API the build exports for this
// (wasmfs_create_opfs_backend / wasmfs_create_directory from
// <emscripten/wasmfs.h>, called with ccall's async form: in builds
// without pthreads the backend's operations are asynchronous JS
// that suspends the call, in pthread builds they complete through
// the backend's proxy worker, and the async form covers both).
//
// The attempt runs only inside a Web Worker: creating the OPFS
// backend on the browser main thread can deadlock waiting for its
// dedicated worker thread to be spawned, and OPFS synchronous
// access handles exist only in workers. Every failure — not a
// worker, no OPFS in the browser, a module built without the
// exports, a rejected mount — is silent and leaves the memory
// backend in place; loadTinyc succeeds either way. The outcome is
// memoized per native module (repeat loads of a cached module do
// not mount twice) and reported as instance.fsBackend.
//
// A successful mount alone does not prove the backend works: in a
// build without pthreads the backend's file operations are async
// JS, and without Asyncify/JSPI to suspend into, the first real
// operation traps — after wasmfs_create_directory has already
// reported success. The mount is therefore followed by a
// write/read probe through the same JS FS operations the
// virtual-FS mirror uses (probeOpfsBackend below); only a probe
// whose bytes read back intact lets the instance report 'opfs'.
// A failed probe leaves the mount point in place but unused: the
// instance reports 'memory' and no path is ever rooted under it.
// ---------------------------------------------------------------------

const OPFS_MOUNT_POINT = '/opfs';
const opfsBackendByModule = new WeakMap();

const OPFS_PROBE_DIR = OPFS_MOUNT_POINT + '/.tinyc-probe';
const OPFS_PROBE_FILE = OPFS_PROBE_DIR + '/probe';
const OPFS_PROBE_BYTES = new Uint8Array([0x74, 0x69, 0x6e, 0x79, 0x63]); // "tinyc"

/** First-use probe of a freshly mounted OPFS backend: create a
 *  directory, write a small file into it and read the bytes back,
 *  through the JS FS API the virtual-FS mirror drives. Returns
 *  true only when every step succeeds and the bytes match. Any
 *  throw — the trap a non-suspending async backend produces on
 *  first use is a catchable RuntimeError at this boundary — and
 *  any mismatch means the backend cannot serve the working set.
 *  The probe directory is removed again on every path, so a
 *  working backend keeps no trace of it. */
function probeOpfsBackend(FS) {
  if (!FS || typeof FS.writeFile !== 'function' ||
      typeof FS.readFile !== 'function') {
    return false;
  }
  try {
    if (typeof FS.mkdirTree === 'function') FS.mkdirTree(OPFS_PROBE_DIR);
    FS.writeFile(OPFS_PROBE_FILE, OPFS_PROBE_BYTES);
    const back = FS.readFile(OPFS_PROBE_FILE);
    if (!back || back.length !== OPFS_PROBE_BYTES.length) return false;
    for (let i = 0; i < OPFS_PROBE_BYTES.length; i++) {
      if (back[i] !== OPFS_PROBE_BYTES[i]) return false;
    }
    return true;
  } catch {
    return false;
  } finally {
    try { FS.unlink(OPFS_PROBE_FILE); } catch { /* not there */ }
    if (typeof FS.rmdir === 'function') {
      try { FS.rmdir(OPFS_PROBE_DIR); } catch { /* not there */ }
    }
  }
}

async function tryMountOpfsBackend(native, bridge) {
  const canMemo =
    native !== null &&
    (typeof native === 'object' || typeof native === 'function');
  if (canMemo) {
    const memo = opfsBackendByModule.get(native);
    if (memo) return memo;
  }
  let result = 'memory';
  try {
    if (
      bridge.mode === 'emscripten' &&
      typeof native.ccall === 'function' &&
      typeof native._wasmfs_create_opfs_backend === 'function' &&
      typeof native._wasmfs_create_directory === 'function' &&
      typeof WorkerGlobalScope !== 'undefined' &&
      typeof self !== 'undefined' &&
      self instanceof WorkerGlobalScope
    ) {
      const backend = await native.ccall(
        'wasmfs_create_opfs_backend', 'number', [], [], { async: true }
      );
      if (backend) {
        const rc = await native.ccall(
          'wasmfs_create_directory', 'number',
          ['string', 'number', 'number'],
          [OPFS_MOUNT_POINT, 0o777, backend],
          { async: true }
        );
        // wasmfs_create_directory returns 0 on success and a
        // negative errno on failure, like mkdir. A mounted backend
        // still has to survive the probe before it is reported.
        if (rc === 0 && probeOpfsBackend(native.FS)) result = 'opfs';
      }
    }
  } catch {
    result = 'memory';
  }
  if (canMemo) opfsBackendByModule.set(native, result);
  return result;
}

/** Load the TinyC WASM module for one target and return a ready
 *  instance. Instances of different targets coexist; each has its
 *  own virtual FS and per-target sysroot.
 *
 *  options:
 *    target       one of SUPPORTED_TARGETS; default "wasm32-wasi".
 *                 Only the wasm32 target can run compiled output in
 *                 the browser; every other target compiles to files.
 *    baseUrl      directory URL the per-target asset envelopes
 *                 resolve against; default is the directory of
 *                 index.mjs (module build) or of tinyc-web.js
 *                 (script bundle)
 *    transport    "auto" | "fetch" | "script": how the package's
 *                 own asset envelopes (the compiler module and the
 *                 sysroot) are retrieved; default "auto" — the
 *                 script transport in a file: context, fetch
 *                 otherwise
 *    loadModule   async loader called with (target, files) that
 *                 returns the target's asset envelope content —
 *                 the envelope file's text, or its bytes as a
 *                 Uint8Array or ArrayBuffer; files carries the
 *                 envelope file name and its resolved URL. The
 *                 package parses the envelope and builds the
 *                 module from it itself, through the same code as
 *                 a directly retrieved envelope; a return value
 *                 that is not envelope content (a built module
 *                 object above all) is rejected. The compiler
 *                 module is only ever obtained from the shipped
 *                 envelope — there is no option to supply a module
 *                 object directly
 *    sysroot      true for this target's bundled manifest, or
 *                 { manifest, baseUrl, fetchImpl, includeBinaries }
 *    fsStore      a custom FsStore instance for the virtual FS
 *                 (see vfs-store.mjs); storeFactory is the function
 *                 form, called with the target. Default: one
 *                 IndexedDB database per target
 *                 ("tinyc-web-fs-<target>"), or the in-memory
 *                 backend where IndexedDB does not exist. The store
 *                 is opened and its persisted index loaded before
 *                 this resolves, so files from an earlier session
 *                 of the same target are already in the index (and
 *                 sysroot files among them are not fetched again).
 *    io           { out, err, in } JS functions; default console
 */
/** Internal test seam — not part of the public API. Builds an
 *  instance around a native module the caller already holds.
 *  loadTinyc itself only ever obtains the module from the target's
 *  asset envelope (directly, or through the loadModule hook); the
 *  package's own test suite uses this seam to run against a mock
 *  native module. */
export async function loadTinycWithNative(native, options = {}) {
  const target = options.target ?? DEFAULT_TARGET;
  assertKnownTarget(target);
  const io = normalizeIO(options.io);
  const store = await openFsStore(target, options);
  const fs = new VirtualFS(store);
  const bridge = new NativeBridge(native, io);
  const instance = new Tinyc(bridge, { fs, io, target });

  // === Emscripten FS adaptation point ===
  // First try to mount the WasmFS OPFS backend at /opfs (fail-soft;
  // the outcome lands on instance.fsBackend). The bridge's
  // moduleFsPrefix records where native-side paths are rooted —
  // "/opfs" on success, "" (the memory-backed root) otherwise —
  // and is handed to the mirror so files land where the paths
  // handed to the native module point. Then, if the module carries
  // an FS, mirror the virtual FS into it and keep it in sync on
  // later writes (see VirtualFS.mountTo).
  instance._fsBackend = await tryMountOpfsBackend(native, bridge);
  bridge.moduleFsPrefix =
    instance._fsBackend === 'opfs' ? OPFS_MOUNT_POINT : '';
  if (native && native.FS) await fs.mountTo(native.FS, bridge.moduleFsPrefix);

  await loadSysroot(instance, options.sysroot, options.transport);
  return instance;
}

export async function loadTinyc(options = {}) {
  const target = options.target ?? DEFAULT_TARGET;
  assertKnownTarget(target);
  const io = normalizeIO(options.io);
  const native = await loadTargetModule(target, options, io);
  return loadTinycWithNative(native, options);
}

export default loadTinyc;
