'use strict';
/* tinyc - TinyC (TCC) embedded compiler for Node.js.
 *
 * Two entry styles over one dual-form native file (executable +
 * library), loaded with koffi:
 *   - runCli([...]) drives the exported tinyc_main with command-line
 *     arguments, exactly like running the tcc command.
 *   - State wraps the libtcc tcc_* API for step-by-step control.
 *
 * Compiler output goes through the TCCIO interface; with no IO
 * installed it writes directly to the process's real stdout/stderr.
 */
const fs = require('fs');
const path = require('path');
const koffi = require('koffi');

const TCC_OUTPUT_MEMORY = 1;
const TCC_OUTPUT_EXE = 2;
const TCC_OUTPUT_OBJ = 3;
const TCC_OUTPUT_DLL = 4;
const TCC_OUTPUT_PREPROCESS = 5;

class TinyCError extends Error {}

/* ---------------------------------------------------------- resolution
 * Distribution model: package type x host x target. The host (OS x
 * 64-bit CPU, where TCC itself runs) selects the dual-form binary;
 * the target selects the TCC variant (one binary per target) and its
 * sysroot. Package layout, identical across ecosystems:
 *
 *   <root>/packaging-manifest.json
 *   <root>/natives/<host>/toolchain/<target>/tinyc[.exe]
 *   <root>/sysroot/<target>/lib/tcc/...   (one copy per target)
 *
 * Resolution: detect the host triple ($TINYC_HOST_TRIPLE overrides,
 * $TINYC_TRIPLE accepted as an alias; $TINYC_TARGET_TRIPLE selects a
 * cross target, defaulting to the host), look the (host, target)
 * combination up in packaging-manifest.json, and locate the variant
 * under natives/. Roots are searched in order: $TINYC_DIST, the
 * platform subpackage @xnetx/tinyc-native-<host> (optional dependency),
 * then this package itself (single-package mode). Hosts and targets
 * are 64-bit only: a 32-bit host throws instead of falling back to
 * some other platform's files.
 */
const TRIPLE_32BIT = /^(i[3-6]86|armv[0-7]|armhf|ia32)(-|$)/;

function assert64BitTriple(triple) {
  if (TRIPLE_32BIT.test(triple)) {
    throw new TinyCError(
      `tinyc: 32-bit hosts are not supported (host triple: ${triple}); ` +
      'tinyc ships 64-bit host binaries only');
  }
}

function hostTriple() {
  const override = process.env.TINYC_HOST_TRIPLE || process.env.TINYC_TRIPLE;
  if (override) {
    assert64BitTriple(override);
    return override;
  }
  const { platform, arch } = process;
  if (arch === 'ia32' || arch === 'arm') {
    throw new TinyCError(`tinyc: 32-bit hosts are not supported (arch: ${arch})`);
  }
  if (platform === 'linux') {
    if (arch === 'x64') return 'x86_64-linux-gnu';
    if (arch === 'arm64') return 'aarch64-linux-gnu';
    if (arch === 'riscv64') return 'riscv64-linux-gnu';
  } else if (platform === 'win32' && arch === 'x64') {
    return 'x86_64-w64-mingw32';
  } else if (platform === 'darwin' && arch === 'arm64') {
    return 'arm64-apple-darwin';
  } else if (platform === 'darwin' && arch === 'x64') {
    return 'x86_64-apple-darwin';
  }
  throw new TinyCError(`tinyc: unsupported host platform ${platform}/${arch}`);
}

function targetTriple() {
  const target = process.env.TINYC_TARGET_TRIPLE || hostTriple();
  assert64BitTriple(target);
  return target;
}

function packageRoots() {
  const roots = [];
  if (process.env.TINYC_DIST) roots.push(process.env.TINYC_DIST);
  try {
    const pkgJson = require.resolve(`@xnetx/tinyc-native-${hostTriple()}/package.json`);
    roots.push(path.dirname(pkgJson));
  } catch { /* subpackage not installed: single-package mode */ }
  roots.push(__dirname);
  return roots;
}

let _resolved = null;

function resolveNative() {
  if (_resolved) return _resolved;
  const host = hostTriple();
  const target = targetTriple();
  const available = [];
  let sawManifest = false;
  for (const root of packageRoots()) {
    const mfPath = path.join(root, 'packaging-manifest.json');
    if (!fs.existsSync(mfPath)) continue;
    sawManifest = true;
    const manifest = JSON.parse(fs.readFileSync(mfPath, 'utf8'));
    for (const c of manifest.combinations || []) {
      available.push(`${c.host} x ${c.target}`);
      if (c.host === host && c.target === target) {
        _resolved = {
          host, target, root,
          binaryPath: path.join(root, c.binary),
          sysrootDir: path.join(root, c.sysroot),
          // The runtime directory ({B} base for tcc_set_lib_path)
          // comes from the manifest, staged from the target's dist
          // manifest: lib/tcc on ELF/Mach-O targets, one level up
          // (lib/tcc/win32) on PE targets, whose search paths are
          // {B}/lib and {B}/include.
          libPath: c.libPath
            ? path.join(root, c.libPath)
            : path.join(root, c.sysroot, 'lib', 'tcc'),
          // Output formats come from the manifest (copied verbatim
          // from the target's dist manifest); never hardcoded here.
          outputs: c.outputs ?? null,
        };
        return _resolved;
      }
    }
  }
  if (!sawManifest) {
    throw new TinyCError(
      'tinyc: packaging-manifest.json not found in this package; ' +
      'the native distribution is missing (run bundle_native.js)');
  }
  throw new TinyCError(
    `tinyc: no native build for host ${host} / target ${target} in this package; ` +
    `available combinations: ${available.join(', ') || 'none'}`);
}

function nativePath() {
  return process.env.TINYC_NATIVE || resolveNative().binaryPath;
}

function libPath() {
  return resolveNative().libPath;
}

/** Package root that provided the resolved native build. */
function distDir() {
  return resolveNative().root;
}

let _lib = null;
let _fn = null;
let _protos = null;

function load() {
  if (_lib) return _fn;
  _lib = koffi.load(nativePath());
  // On Windows the dual-form file is an EXE loaded as a library:
  // the loader never runs its process entry, so the image's import
  // table is bound by tinyc_init, which must run before any other
  // symbol is bound or called. Other hosts' images are fully
  // initialized by the loader and export no tinyc_init, so nothing
  // changes there.
  if (process.platform === 'win32') {
    let tinycInit = null;
    try { tinycInit = _lib.func('int tinyc_init()'); } catch { /* not exported */ }
    if (tinycInit) {
      const rc = tinycInit();
      if (rc !== 0) {
        throw new TinyCError(`tinyc: tinyc_init failed with code ${rc} in ${nativePath()}`);
      }
    }
  }
  _protos = {
    errorFunc: koffi.proto('void TccErrorFunc(void *opaque, const char *msg)'),
    ioWrite: koffi.proto('int TccIoWrite(void *ctx, const uint8_t *buf, size_t len)'),
    ioRead: koffi.proto('int TccIoRead(void *ctx, uint8_t *buf, size_t cap)'),
  };
  _fn = {
    tcc_new: _lib.func('void *tcc_new()'),
    tcc_delete: _lib.func('void tcc_delete(void *s)'),
    tcc_set_lib_path: _lib.func('void tcc_set_lib_path(void *s, const char *p)'),
    tcc_set_output_type: _lib.func('void tcc_set_output_type(void *s, int t)'),
    tcc_set_error_func: _lib.func('void tcc_set_error_func(void *s, void *opaque, TccErrorFunc *cb)'),
    tcc_compile_string: _lib.func('int tcc_compile_string(void *s, const char *src)'),
    tcc_add_include_path: _lib.func('void tcc_add_include_path(void *s, const char *p)'),
    tcc_add_sysinclude_path: _lib.func('void tcc_add_sysinclude_path(void *s, const char *p)'),
    tcc_add_library_path: _lib.func('void tcc_add_library_path(void *s, const char *p)'),
    tcc_add_library: _lib.func('int tcc_add_library(void *s, const char *name)'),
    tcc_add_file: _lib.func('int tcc_add_file(void *s, const char *path)'),
    tcc_define_symbol: _lib.func('void tcc_define_symbol(void *s, const char *name, const char *value)'),
    tcc_undefine_symbol: _lib.func('void tcc_undefine_symbol(void *s, const char *name)'),
    // Cross builds (host != target) have no tcc_relocate/tcc_run:
    // relocating target code into this process is native-only.
    tcc_relocate: (() => {
      try { return _lib.func('int tcc_relocate(void *s, void *ptr)'); }
      catch { return null; }
    })(),
    tcc_get_symbol: _lib.func('void *tcc_get_symbol(void *s, const char *name)'),
    tcc_output_file: _lib.func('int tcc_output_file(void *s, const char *path)'),
    tinyc_main: _lib.func('int tinyc_main(int argc, const char **argv)'),
    tinyc_main_io: _lib.func('int tinyc_main_io(int argc, const char **argv, void *io)'),
    tcc_io_buffer: _lib.func('void *tcc_io_buffer()'),
    tcc_io_buffer_merged: _lib.func('void *tcc_io_buffer_merged()'),
    tcc_io_null: _lib.func('void *tcc_io_null()'),
    tcc_io_stdio: _lib.func('void *tcc_io_stdio()'),
    tcc_io_file: _lib.func('void *tcc_io_file(const char *out, const char *err)'),
    tcc_io_fd: _lib.func('void *tcc_io_fd(int out_fd, int err_fd, int in_fd)'),
    tcc_io_callback: _lib.func('void *tcc_io_callback(TccIoWrite *wo, TccIoWrite *we, TccIoRead *ri, void *ctx)'),
    tcc_io_buffer_data: _lib.func('const char *tcc_io_buffer_data(void *io, int channel, void *len)'),
    tcc_io_buffer_clear: _lib.func('void tcc_io_buffer_clear(void *io)'),
    tcc_io_free: _lib.func('void tcc_io_free(void *io)'),
  };
  return _fn;
}

/* ------------------------------------------------------------------ IO */

class IO {
  constructor(ptr, owned = true) {
    this.ptr = ptr;
    this._owned = owned;
    this._registered = [];
  }
  close() {
    if (this.ptr && this._owned) load().tcc_io_free(this.ptr);
    this.ptr = null;
    for (const r of this._registered) koffi.unregister(r);
    this._registered = [];
  }
  static stdio() { return new IO(load().tcc_io_stdio(), false); }
  static null() { return new IO(load().tcc_io_null(), false); }
  static buffer(merged = false) {
    const f = load();
    return new IO(merged ? f.tcc_io_buffer_merged() : f.tcc_io_buffer());
  }
  static file(outPath, errPath = null) {
    const ptr = load().tcc_io_file(outPath, errPath);
    if (!ptr) throw new TinyCError('cannot open IO file: ' + outPath);
    return new IO(ptr);
  }
  static fd(outFd = -1, errFd = -1, inFd = -1) {
    return new IO(load().tcc_io_fd(outFd, errFd, inFd));
  }
  /* out/err: (chunk: Buffer) => number|void; readIn: (cap) => Buffer|null */
  static callback({ out = null, err = null, readIn = null } = {}) {
    const f = load();
    const registered = [];
    const wrapWrite = (fn) => {
      if (!fn) return null;
      const cb = (ctx, buf, len) => {
        try {
          const chunk = Buffer.from(koffi.decode(buf, koffi.array('uint8_t', Number(len))));
          const r = fn(chunk);
          return r === undefined ? Number(len) : r;
        } catch (e) { return -1; }
      };
      const reg = koffi.register(cb, koffi.pointer(_protos.ioWrite));
      registered.push(reg);
      return reg;
    };
    const wrapRead = (fn) => {
      if (!fn) return null;
      const cb = (ctx, buf, cap) => {
        try {
          const data = fn(Number(cap));
          if (!data || data.length === 0) return 0;
          const n = Math.min(data.length, Number(cap));
          koffi.encode(buf, koffi.array('uint8_t', n), data);
          return n;
        } catch (e) { return -1; }
      };
      const reg = koffi.register(cb, koffi.pointer(_protos.ioRead));
      registered.push(reg);
      return reg;
    };
    const ptr = f.tcc_io_callback(wrapWrite(out), wrapWrite(err), wrapRead(readIn), null);
    const io = new IO(ptr);
    io._registered = registered;
    return io;
  }
  _bufferBytes(channel) {
    const s = load().tcc_io_buffer_data(this.ptr, channel, null);
    return Buffer.from(s || '', 'utf8');
  }
  get out() { return this._bufferBytes(0); }
  get err() { return this._bufferBytes(1); }
  clear() { load().tcc_io_buffer_clear(this.ptr); }
}

function coerceIO(io) {
  if (io === null || io === undefined || io instanceof IO) return io || null;
  if (io === 'capture') return IO.buffer();
  if (typeof io === 'function') return IO.callback({ out: io, err: io });
  if (io && typeof io.write === 'function') {
    return IO.callback({ out: (b) => io.write(b), err: (b) => io.write(b) });
  }
  throw new TinyCError('unsupported io argument: ' + String(io));
}

/* ------------------------------------------------------------------ CLI */

function runCli(args, { io = null } = {}) {
  const f = load();
  // The binary does not sit next to its runtime in the unified layout,
  // so point TCC at the target sysroot with -B unless the caller did.
  const hasB = args.some((a) => a === '-B' || a.startsWith('-B'));
  const full = [nativePath(), ...(hasB ? [] : ['-B' + libPath()]), ...args];
  const coerced = coerceIO(io);
  if (!coerced) return f.tinyc_main(full.length, full);
  return f.tinyc_main_io(full.length, full, coerced.ptr);
}

function runCliCapture(args) {
  const io = IO.buffer(true);
  try {
    const code = runCli(args, { io });
    return [code, io.out];
  } finally { io.close(); }
}

/* ------------------------------------------------------------------ API */

class State {
  static _fnSeq = 0;
  constructor(outputType = TCC_OUTPUT_MEMORY) {
    const f = load();
    this._f = f;
    this._s = f.tcc_new();
    if (!this._s) throw new TinyCError('tcc_new failed');
    this.errors = [];
    this._errCb = koffi.register(
      (opaque, msg) => { this.errors.push(String(msg)); },
      koffi.pointer(_protos.errorFunc));
    f.tcc_set_error_func(this._s, null, this._errCb);
    f.tcc_set_lib_path(this._s, libPath());
    if (outputType !== null) f.tcc_set_output_type(this._s, outputType);
    this._relocated = false;
  }
  close() {
    if (this._s) { this._f.tcc_delete(this._s); this._s = null; }
    if (this._errCb) { koffi.unregister(this._errCb); this._errCb = null; }
  }
  _check(ret, what) {
    if (ret !== 0) {
      throw new TinyCError(what + ' failed: ' + (this.errors.join('; ') || 'no diagnostics'));
    }
    return ret;
  }
  addIncludePath(p) { this._f.tcc_add_include_path(this._s, String(p)); }
  addSysincludePath(p) { this._f.tcc_add_sysinclude_path(this._s, String(p)); }
  addLibraryPath(p) { this._f.tcc_add_library_path(this._s, String(p)); }
  addLibrary(name) { return this._check(this._f.tcc_add_library(this._s, String(name)), 'addLibrary'); }
  addFile(p) { return this._check(this._f.tcc_add_file(this._s, String(p)), 'addFile'); }
  compileFile(p) { return this.addFile(p); }
  defineSymbol(name, value = '1') { this._f.tcc_define_symbol(this._s, String(name), String(value)); }
  undefineSymbol(name) { this._f.tcc_undefine_symbol(this._s, String(name)); }
  compileString(src) { return this._check(this._f.tcc_compile_string(this._s, String(src)), 'compileString'); }
  setOutputType(t) { this._f.tcc_set_output_type(this._s, t); }
  outputFile(p) { return this._check(this._f.tcc_output_file(this._s, String(p)), 'outputFile'); }
  relocate() {
    if (!this._f.tcc_relocate) {
      const r = resolveNative();
      throw new TinyCError(
        `tcc_relocate is not available for target ${r.target} on host ${r.host}: ` +
        'in-process relocation/run requires a native (host == target) combination; ' +
        'cross combinations compile to files (outputFile / runCli)');
    }
    this._check(this._f.tcc_relocate(this._s, null), 'relocate');
    this._relocated = true;
  }
  getSymbol(name) {
    if (!this._relocated) this.relocate();
    const addr = this._f.tcc_get_symbol(this._s, String(name));
    if (!addr) throw new TinyCError('symbol not found: ' + name);
    return addr;
  }
  /* Return a JS callable for a compiled symbol, e.g.
   * state.function('add', 'int', ['int', 'int']) */
  function(name, restype, argtypes) {
    const addr = this.getSymbol(name);
    const proto = koffi.proto(
      `${restype} TinycFn_${name}_${State._fnSeq++}(${argtypes.join(', ')})`);
    return (...args) => koffi.call(addr, proto, ...args);
  }
}

module.exports = {
  runCli, runCliCapture, State, IO, TinyCError,
  nativePath, distDir, hostTriple, targetTriple, resolveNative, libPath,
  TCC_OUTPUT_MEMORY, TCC_OUTPUT_EXE, TCC_OUTPUT_OBJ, TCC_OUTPUT_DLL,
  TCC_OUTPUT_PREPROCESS,
};
