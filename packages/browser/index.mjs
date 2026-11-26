// tinyc-web: TinyC (TCC) WebAssembly glue layer.
//
// A TCC build has exactly one code generation target, so the native
// side is one WASM module per target (see build.sh), named
// tcc-<target>.wasm with glue tcc-<target>.mjs. loadTinyc() picks the
// module for the requested target from a registry and loads it
// lazily; instances of different targets coexist. This file provides,
// in plain JS:
//   - loadTinyc()          instantiate the WASM module (or an injected one)
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
// Native module resolution order in loadTinyc(), per target:
//   options.module        an already-created Emscripten module object
//   options.createModule  an Emscripten MODULARIZE factory
//   options.wasmExports   raw WebAssembly exports, or a plain mock object
//                         implementing the same tcc_* names (test double)
//   options.loadModule    async loader called with (target, files);
//                         its result is cached per target
//   (default)             the per-target glue tcc-<target>.mjs built
//                         by build.sh, with the wasm at options.wasmUrl
//                         or tcc-<target>.wasm alongside the glue

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
import { MemoryFsStore, openFsStore } from './vfs-store.mjs';

export { MemoryFsStore, IndexedDbFsStore, createFsStore } from './vfs-store.mjs';

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
// The per-target glue/wasm files and the sysroot files resolve
// against a base captured for the environment this code runs in:
//   - ES module: this module's own URL (import.meta.url), so assets
//     sitting next to index.mjs are found;
//   - classic-script bundle (tinyc-web.js): the directory of the
//     <script> element that loaded the bundle, captured below from
//     document.currentScript while the bundle executes, so assets
//     sitting next to tinyc-web.js are found; when no script URL is
//     available, the page's own directory (location.href) is used.
// loadTinyc's baseUrl option overrides the base per call; its
// wasmUrl / glueUrl options override individual files.
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
// module per target, named tcc-<target>.wasm (glue tcc-<target>.mjs),
// chosen through the registry below and loaded lazily — only the
// requested target's module is ever fetched. Instances of different
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

export function isRunnableTarget(target) {
  return Boolean(TARGETS[target]?.runnable);
}

export function targetModuleFiles(target) {
  return { wasmFile: `tcc-${target}.wasm`, glueFile: `tcc-${target}.mjs` };
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
//                 This is the injection point used by the tests and
//                 by anyone prototyping without a WASM build.
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
      return textDecoder.decode(m.subarray(ptr, end));
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
    return this;
  }

  setOptions(str) {
    return this._checkRc('tcc_set_options',
      this._call('tcc_set_options', 'number', ['string'], [str]));
  }

  addIncludePath(path) {
    return this._checkRc('tcc_add_include_path',
      this._call('tcc_add_include_path', 'number', ['string'], [this._bridge.modulePath(path)]));
  }

  addSysincludePath(path) {
    return this._checkRc('tcc_add_sysinclude_path',
      this._call('tcc_add_sysinclude_path', 'number', ['string'], [this._bridge.modulePath(path)]));
  }

  addLibraryPath(path) {
    return this._checkRc('tcc_add_library_path',
      this._call('tcc_add_library_path', 'number', ['string'], [this._bridge.modulePath(path)]));
  }

  addLibrary(name) {
    return this._checkRc('tcc_add_library',
      this._call('tcc_add_library', 'number', ['string'], [name]));
  }

  defineSymbol(sym, value = '1') {
    this._call('tcc_define_symbol', null, ['string', 'string'], [sym, value]);
    return this;
  }

  undefineSymbol(sym) {
    this._call('tcc_undefine_symbol', null, ['string'], [sym]);
    return this;
  }

  setOutputType(type) {
    const rc = this._checkRc('tcc_set_output_type',
      this._call('tcc_set_output_type', 'number', ['number'], [type]));
    this._outputTypeSet = true;
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
    return this._checkRc('tcc_add_file',
      this._call('tcc_add_file', 'number', ['string'], [this._bridge.modulePath(p)]));
  }

  compileString(source) {
    this._ensureOutputType();
    return this._checkRc('tcc_compile_string',
      this._call('tcc_compile_string', 'number', ['string'], [source]));
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
    return this._checkRc('tcc_compile_string',
      this._call('tcc_compile_string', 'number', ['string'], [marked]));
  }

  /** Write the compiled product to a virtual path. TCC writes it
   *  inside the module FS (at bridge.modulePath(path)); exportFile
   *  reads it back from there into the virtual FS. */
  outputFile(path) {
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
   *  options.entry is given. This switches the state to EXE output
   *  type. On every other target the glue layer throws
   *  TinycRunNotSupportedError before anything is passed to the
   *  native module — those targets compile to files only.
   *  options: { outputPath, entry, callArgs, env, imports, shims,
   *  stdin, heapBase, io } — entry/callArgs/env/imports/shims/stdin/
   *  heapBase forward to runWasmModuleSync, and the instance's
   *  registered shims apply automatically; io is a text IO partial
   *  (default: this state's IO) whose out/err receive the program's
   *  fd_write output, decoded. */
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
    if (this._instance._dylinkOptionOk()) this.setOptions('-wasm-dylink=main');
    this.setOutputType(TCC_OUTPUT.EXE);
    this.outputFile(outputPath);
    const bytes = this._instance.exportFile(outputPath);
    const io = normalizeIO(options.io, this.io);
    if (isDylinkUnit(bytes)) {
      const proc = this._instance.createProcess({
        io,
        args: [outputPath, ...args.map(String)],
        env: options.env,
        stdin: options.stdin,
        shims: options.shims,
        imports: options.imports,
      });
      const result = proc.runMainModule(bytes, { path: outputPath });
      flushIO(io);
      if (result && typeof result === 'object' && 'exitCode' in result) {
        return result.exitCode;
      }
      return result;
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
      const proc = this.createProcess({
        io: this._io,
        args: [src, ...progArgs],
        shims: options.shims,
        imports: options.imports,
        stdin: options.stdin,
      });
      const result = proc.runMainModule(bytes, { path: DEFAULT_RUN_OUTPUT });
      flushIO(this._io);
      if (typeof result === 'number') return result;
      return result?.exitCode ?? 0;
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
    needed: Array.isArray(manifest.needed) ? manifest.needed.map(String) : [],
    exports: (Array.isArray(manifest.exports) ? manifest.exports : [])
      .map((e) => ({ name: String(e.name), kind: e.kind | 0, value: e.value | 0 })),
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

/** A dylink process. Created attached to a Tinyc instance
 *  (instance.createProcess: libraries resolve in the instance's
 *  virtual FS, retrievals write back into it, the instance's
 *  registered shims apply) or standalone (createProcess: libraries
 *  resolve through options.readModule(canonicalPath) -> bytes and
 *  through retrieval from options.baseUrl).
 *
 *  options: { io, args, env, stdin, shims, imports, baseUrl,
 *  transport, readModule } — io is a text IO partial like the
 *  instance IO; args/env/stdin seed the process's WASI and libc
 *  layers; shims/imports are extra import layers with the
 *  runWasmModule priority (after the built-in layers, before
 *  symbol resolution); transport is "auto" | "fetch" | "script". */
export class TinycProcess {
  constructor(options = {}) {
    this._tinyc = options.tinyc ?? null;
    this._readModule = typeof options.readModule === 'function'
      ? options.readModule : null;
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
    this.memory = new WebAssembly.Memory({ initial: 1 });
    this.table = new WebAssembly.Table({ element: 'anyfunc', initial: 2 });
    // Slot 0 stays null (the engine traps calls through it); slot 1
    // is the linker's stub for a null function pointer.
    this.table.set(1, NULL_STUB_FN);
    this._tableFree = 2;
    this._holder = { memory: this.memory, instance: null };
    // The shim allocator's frontier starts past the reserved
    // [0, 1024) span; module static regions are reserved from it.
    this._allocatorState = { heapTop: 1024, freeHead: 0 };
    this._io = options.io ? normalizeIO(options.io) : null;
    this._byteIO = this._io ? textIOToByteIO(this._io) : createDefaultByteIO();
    this._wasi = createWasiShim(
      { args: this._args, env: options.env ?? {}, stdin: options.stdin },
      this._holder, this._byteIO);
    const def = createDefaultShims({
      io: this._byteIO, holder: this._holder, stdin: options.stdin,
      heapBase: 1024, allocatorState: this._allocatorState,
      processTable: this.table,
    });
    this._defaultEnv = def.env;
    this._reserveStatic = def.reserveStatic;
    this._shimsOption = options.shims ?? null;
    this._importsOption = options.imports ?? null;
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
    return textDecoder.decode(m.subarray(ptr, end));
  }

  // -- library resolution (design §13) --------------------------------

  /** A library from local sources only — the resolved cache, the
   *  script registry, the attached instance's virtual FS, the
   *  readModule callback. A local file that fails its integrity
   *  check counts as absent; the reason is remembered for error
   *  messages. Returns null when nothing usable is local. */
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
    if (lib) {
      this._libraries.set(canonical, lib);
      this._libraryErrors.delete(canonical);
    }
    return lib;
  }

  /** A library, retrieving its manifest through the transport when
   *  no usable copy is local; a retrieval is written back into the
   *  virtual FS, best-effort. */
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
      try {
        this._tinyc.fs.addFileSync(
          canonical, manifestEnvelopeText(canonical, manifest));
      } catch { /* write-back is best-effort; never a load failure */ }
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
    }
    return slot;
  }

  _shimSlot(name, fn, requester = null) {
    let slot = this._shimSlots.get(name);
    if (slot === undefined) {
      let type = requester?.importTypes?.get(`env.${name}`) ?? null;
      if (!type) {
        for (const rec of this._modules.values()) {
          type = rec.importTypes?.get(`env.${name}`) ?? null;
          if (type) break;
        }
      }
      slot = this._jsFunctionSlot(fn, type ?? DEFAULT_CALL_TYPE);
      this._shimSlots.set(name, slot);
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
        this._globalSymbols.set(name, { kind: e.kind, value: e.value, record });
      }
    }
    record.global = true;
  }

  _withdrawGlobals(record) {
    for (const [name, e] of this._globalSymbols) {
      if (e.record === record) this._globalSymbols.delete(name);
    }
    for (const [name, e] of record.exports) {
      const entry = this._got.get(`${e.kind === 0 ? 'func' : 'mem'}:${name}`);
      if (entry && entry.definer === record) {
        entry.global.value = e.kind === 0 ? this._stubSlot(name) : 0;
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
    for (const [name, fn] of Object.entries(this._defaultEnv)) {
      put('env', name, fn);
    }
    const dl = this._dlfcnFor(record);
    for (const [name, fn] of Object.entries(dl)) put('env', name, fn);
    const ctx = createShimContext(this._holder, this._byteIO);
    for (const layer of [globalShimRegistry, this._tinyc?._shims, this._shimsOption]) {
      if (!layer) continue;
      for (const [key, fn] of shimEntriesOf(layer)) {
        const { module: mn, name } = splitShimKey(key);
        put(mn, name, (...a) => fn(ctx, ...a));
      }
    }
    for (const [mn, fns] of Object.entries(this._importsOption ?? {})) {
      for (const [name, v] of Object.entries(fns)) put(mn, name, v);
    }
    // Structural imports: process-owned, not overridable.
    put('env', 'memory', this.memory);
    put('env', '__indirect_function_table', this.table);
    put('env', '__memory_base',
      new WebAssembly.Global({ value: 'i32', mutable: false }, record.memoryBase));
    put('env', '__table_base',
      new WebAssembly.Global({ value: 'i32', mutable: false }, record.tableBase));
    const unresolvedFunctions = [];
    const unresolvedGotMem = [];
    const unresolvedOther = [];
    for (const imp of WebAssembly.Module.imports(wasmModule)) {
      const key = `${imp.module}.${imp.name}`;
      if (imp.kind === 'global' &&
          (imp.module === 'GOT.func' || imp.module === 'GOT.mem')) {
        const kind = imp.module === 'GOT.func' ? 'func' : 'mem';
        const entry = this._gotEntry(kind, imp.name);
        if (!entry.filled) {
          const hit = this._resolveGot(kind, imp.name, record, provided);
          if (hit) this._fillGot(kind, imp.name, hit.value, hit.definer);
          else if (kind === 'mem') unresolvedGotMem.push(imp.name);
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
    return { imports, unresolvedFunctions, unresolvedGotMem, unresolvedOther };
  }

  /** A GOT value for a not-yet-filled entry, in the §5 order:
   *  shim layer (functions only), global table, requester closure. */
  _resolveGot(kind, name, record, provided) {
    const wantKind = kind === 'func' ? 0 : 1;
    if (kind === 'func') {
      const shim = provided.get(`env.${name}`);
      if (typeof shim === 'function') {
        return { value: this._shimSlot(name, shim, record), definer: 'shim' };
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
        const value = e.kind === 0
          ? record.tableBase + e.value
          : record.memoryBase + e.value;
        record.exports.set(e.name, { kind: e.kind, value });
      }
      for (const [name, e] of record.exports) {
        this._fillGot(e.kind === 0 ? 'func' : 'mem', name, e.value, record);
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
      // Instantiate.
      const wasmModule = lib.wasmModule ?? new WebAssembly.Module(lib.bodyBytes);
      record.wasmModule = wasmModule;
      record.importTypes = parseImportTypes(lib.bodyBytes);
      const { imports, unresolvedFunctions, unresolvedGotMem, unresolvedOther } =
        this._buildImports(record, wasmModule);
      if (unresolvedGotMem.length > 0) {
        throw new TinycDylinkError(
          `tinyc: cannot load "${canonical}": unresolved variable(s) ` +
          `${unresolvedGotMem.map((n) => `"${n}"`).join(', ')} ` +
          `(GOT.mem references bind eagerly)`);
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
      // Initialize (post-order: dependencies are loaded by now).
      record.state = 'initializing';
      let rc;
      try {
        rc = instance.exports.__tinyc_dylink_init(record.handle);
      } catch (err) {
        throw new TinycDylinkError(
          `tinyc: initialization of "${canonical}" trapped: ${err.message}`);
      }
      if (rc !== 0) {
        throw new TinycDylinkError(
          `tinyc: initialization of "${canonical}" failed: its DllMain returned FALSE`);
      }
      record.state = 'loaded';
      this._loadOrder.push(record);
      return record;
    } catch (err) {
      this._rollbackLoad(record);
      throw err;
    }
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
    if (!handle) {
      const g = this._globalSymbols.get(name);
      if (g) return g.value;
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
    if (own) return own.value;
    const hit = this._lookupClosure(name, record, false);
    if (hit) return hit.entry.value;
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
        try {
          const rec = proc._loadModule(canonical, {
            flags, requester, runtimeEdge: true,
          });
          return rec.handle;
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
    const rec = this._loadModule(canonical, { flags, hostEdge: true });
    return rec.handle;
  }

  /** Synchronous dlopen: the library and its closure must already
   *  be local (see loadModule for the retrieving form). Returns
   *  the handle; throws TinycDylinkError on failure. */
  dlopen(path, flags = RTLD_LAZY) {
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
   *  name — the host counterpart of a guest function pointer. */
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
    return fn(...args);
  }

  /** Read a variable exported by a loaded module, as an i32. */
  readExportI32(handle, name) {
    const record = this._handles.get(handle);
    if (!record) throw new TinycDylinkError(`tinyc: readExportI32: unknown handle ${handle}`);
    const e = record.exports.get(name);
    if (!e || e.kind !== 1) {
      throw new TinycDylinkError(
        `tinyc: module "${record.canonical}" does not export variable "${name}"`);
    }
    return new DataView(this.memory.buffer).getInt32(e.value, true);
  }

  writeExportI32(handle, name, value) {
    const record = this._handles.get(handle);
    if (!record) throw new TinycDylinkError(`tinyc: writeExportI32: unknown handle ${handle}`);
    const e = record.exports.get(name);
    if (!e || e.kind !== 1) {
      throw new TinycDylinkError(
        `tinyc: module "${record.canonical}" does not export variable "${name}"`);
    }
    new DataView(this.memory.buffer).setInt32(e.value, value | 0, true);
  }

  /** Load raw module bytes as the process's main module and run
   *  it: the product of a dylink-main compile, whose exports enter
   *  the global symbol table. The entry is options.entry when
   *  given, else _start, else main(argc, argv) with the process's
   *  args. Running the main module ends the process: every loaded
   *  module is finalized in reverse load order. Returns the
   *  entry's return value, or { exitCode } for a _start main. */
  runMainModule(bytes, options = {}) {
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

  _callMain(record, options) {
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
    return record.instance.exports.main(args.length, block);
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

// Per-target native module cache. Only modules obtained through
// loadModule or the shipped glue files are cached: a target's module
// is loaded at most once, and only when an instance of that target
// is actually requested. Injected modules (module / createModule /
// wasmExports) are used as-is per call and never enter the cache, so
// callers can inject a different module for the same target.
const targetModuleCache = new Map();

/** Targets whose native module has been loaded through the
 *  per-target loader (loadModule or the shipped glue files). */
export function getLoadedTargets() {
  return [...targetModuleCache.keys()];
}

export function clearTargetModuleCache() {
  targetModuleCache.clear();
}

async function createModuleFromGlue(glueUrl, wasmUrl, io) {
  let factory;
  try {
    factory = (await import(/* @vite-ignore */ glueUrl)).default;
  } catch {
    throw new TinycError(
      `tinyc: the Emscripten glue ${glueUrl} could not be loaded. ` +
      `Build it with build.sh, or pass module / createModule / wasmExports / loadModule.`
    );
  }
  if (typeof factory !== 'function') {
    throw new TinycError(`tinyc: Emscripten glue ${glueUrl} does not default-export a module factory`);
  }
  return factory({
    print: (text) => io.out(text + '\n'),
    printErr: (text) => io.err(text + '\n'),
    locateFile: (path, prefix) =>
      wasmUrl && path.endsWith('.wasm') ? wasmUrl : prefix + path,
  });
}

async function loadTargetModule(target, options, io) {
  const cached = targetModuleCache.get(target);
  if (cached) return cached;
  const files = targetModuleFiles(target);
  const promise = (async () => {
    if (typeof options.loadModule === 'function') {
      return options.loadModule(target, {
        ...files,
        wasmUrl: options.wasmUrl ?? assetUrl(files.wasmFile, options.baseUrl),
        glueUrl: options.glueUrl ?? assetUrl(files.glueFile, options.baseUrl),
      });
    }
    const glueUrl = options.glueUrl ?? assetUrl(files.glueFile, options.baseUrl);
    const wasmUrl = options.wasmUrl ?? assetUrl(files.wasmFile, options.baseUrl);
    return createModuleFromGlue(glueUrl, wasmUrl, io);
  })();
  targetModuleCache.set(target, promise);
  promise.catch(() => {
    if (targetModuleCache.get(target) === promise) targetModuleCache.delete(target);
  });
  return promise;
}

async function resolveNative(options, io, target) {
  if (options.module) return options.module;
  if (options.wasmExports) return options.wasmExports;
  if (typeof options.createModule === 'function') {
    return await options.createModule({
      print: (text) => io.out(text + '\n'),
      printErr: (text) => io.err(text + '\n'),
      locateFile: (path, prefix) =>
        options.wasmUrl && path.endsWith('.wasm') ? options.wasmUrl : prefix + path,
    });
  }
  // Default: the per-target module file (options.loadModule when
  // given, otherwise the shipped glue tcc-<target>.mjs), cached and
  // loaded lazily by loadTargetModule.
  return loadTargetModule(target, options, io);
}

/** The sysroot manifest for one target (sysroot-manifests/<target>.json).
 *  All manifests are statically imported with this module (see
 *  SYSROOT_MANIFESTS above), so this never fetches at runtime; the
 *  default target's manifest is also exported as `sysrootManifest`. */
export async function loadSysrootManifest(target = DEFAULT_TARGET) {
  assertKnownTarget(target);
  return SYSROOT_MANIFESTS[target];
}

async function loadSysroot(instance, sysrootOpt) {
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
  for (const entry of entries) {
    if (entry.binary && !includeBinaries) continue;
    // Already in the virtual FS — persisted by an earlier load of
    // this target and restored from the store — is not fetched again.
    if (instance.fs.hasFile(entry.path)) continue;
    const url = entry.url ?? new URL(entry.source, baseHref).href;
    await instance.fs.addFileFromUrl(entry.path, url, { fetchImpl });
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
 *    wasmUrl      URL of this target's tcc-<target>.wasm (the glue
 *                 tcc-<target>.mjs is loaded alongside), or inject a
 *                 module with module / createModule / wasmExports /
 *                 loadModule instead
 *    baseUrl      directory URL the per-target glue/wasm files
 *                 resolve against; default is the directory of
 *                 index.mjs (module build) or of tinyc-web.js
 *                 (script bundle). wasmUrl / glueUrl override
 *                 individual files
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
export async function loadTinyc(options = {}) {
  const target = options.target ?? DEFAULT_TARGET;
  assertKnownTarget(target);
  const io = normalizeIO(options.io);
  const store = await openFsStore(target, options);
  const fs = new VirtualFS(store);
  const native = await resolveNative(options, io, target);
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

  await loadSysroot(instance, options.sysroot);
  return instance;
}

export default loadTinyc;
