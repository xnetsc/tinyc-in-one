// tinyc-web worker interface.
//
// Two halves in one file:
//   - Main thread: the TinycWorker class spawns this file as a module
//     Worker and exposes promise-based compile / runCli methods.
//     Compilation blocks the WASM module for its whole duration, so
//     anything longer than a snippet belongs here, not on the UI
//     thread.
//   - Worker thread: when this file runs inside a
//     DedicatedWorkerGlobalScope it installs the message handler and
//     never touches the class.
//
// Protocol:  request  { id, type, payload }
//            response { id, ok: true, result } | { id, ok: false, error }
//            io push  { type: 'io', stream: 'out'|'err', text }
// IO callbacks are functions and cannot cross postMessage, so native
// output produced inside the worker is pushed to the main thread as
// 'io' messages and re-dispatched to the io option given to
// TinycWorker (default: console, line-buffered by index.mjs wiring
// on the worker side is raw chunk forwarding).

import {
  loadTinyc, normalizeIO, flushIO,
  manifestToLibrary, libraryFromBytes, parseImportTypes,
  makeWrapperFunction, createWasiShim, createShimContext,
  canonicalModulePath, libraryNameForms, evaluateJsModule,
} from './index.mjs';
import { startThreadWorker } from './threads.mjs';

function shimBoundaryError(method) {
  return new Error(
    `tinyc: TinycWorker.${method} cannot work: JS functions cannot cross ` +
    `postMessage, so a shim function cannot be sent to the worker. Pass ` +
    `shimModules: [url, ...] to the TinycWorker constructor instead — each ` +
    `module's default export must be a shim map object ({ name: fn }), which ` +
    `the worker imports and registers on its instance.`
  );
}

export class TinycWorker {
  constructor({ workerUrl, sysroot, target, io, shimModules } = {}) {
    if (typeof Worker === 'undefined') {
      throw new Error('tinyc: TinycWorker needs the Worker API (browser main thread)');
    }
    this._worker = new Worker(
      workerUrl ?? new URL('./worker.mjs', import.meta.url),
      { type: 'module' }
    );
    // target selects the per-target WASM module inside the worker
    // (default: wasm32-wasi). runCli with "-run" is rejected inside
    // the worker for every non-wasm32 target, by the same glue-layer
    // guard as on the main thread.
    // shimModules: URLs of JS modules whose default export is a
    // shim map; the worker imports them and registers the maps on
    // its instance (functions themselves cannot cross postMessage —
    // see registerShim below).
    this._initPayload = { sysroot, target, shimModules };
    this._io = normalizeIO(io);
    this._pending = new Map();
    this._nextId = 1;
    this._initPromise = null;
    this._worker.addEventListener('message', (ev) => this._onMessage(ev.data));
    this._worker.addEventListener('error', (ev) => this._onError(ev));
  }

  _onMessage(msg) {
    if (!msg || typeof msg !== 'object') return;
    if (msg.type === 'io') {
      const fn = msg.stream === 'err' ? this._io.err : this._io.out;
      fn(msg.text);
      return;
    }
    const entry = this._pending.get(msg.id);
    if (!entry) return;
    this._pending.delete(msg.id);
    if (msg.ok) entry.resolve(msg.result);
    else {
      const err = new Error(msg.error?.message ?? 'tinyc worker error');
      err.name = msg.error?.name ?? 'TinycWorkerError';
      entry.reject(err);
    }
  }

  _onError(ev) {
    const err = new Error(ev.message ?? 'tinyc worker failed');
    for (const entry of this._pending.values()) entry.reject(err);
    this._pending.clear();
  }

  _request(type, payload) {
    const id = this._nextId++;
    return new Promise((resolve, reject) => {
      this._pending.set(id, { resolve, reject });
      this._worker.postMessage({ id, type, payload });
    });
  }

  /** Load the WASM module inside the worker. Idempotent. */
  init() {
    if (!this._initPromise) {
      this._initPromise = this._request('init', this._initPayload);
    }
    return this._initPromise;
  }

  async compileString(source, opts) {
    await this.init();
    return this._request('compileString', { source: String(source), opts });
  }

  /** content: string (sent as-is) or Uint8Array (sent as ArrayBuffer). */
  async compileFile(path, content) {
    await this.init();
    return this._request('compileFile', { path, content });
  }

  async addFile(path, content) {
    await this.init();
    return this._request('addFile', { path, content });
  }

  async registerIncludeDir(path) {
    await this.init();
    return this._request('registerIncludeDir', { path });
  }

  /** Command-line mode inside the worker; resolves to the exit code. */
  async runCli(args = []) {
    await this.init();
    return this._request('callMain', { args: args.map(String) });
  }

  /** Shims cannot be registered from the main thread on a
   *  TinycWorker: the function would have to cross postMessage.
   *  Use the constructor's shimModules option instead. */
  registerShim() {
    throw shimBoundaryError('registerShim');
  }

  registerShims() {
    throw shimBoundaryError('registerShims');
  }

  terminate() {
    this._worker.terminate();
    for (const entry of this._pending.values()) {
      entry.reject(new Error('tinyc: worker terminated'));
    }
    this._pending.clear();
  }
}

// ---------------------------------------------------------------------
// Worker side
// ---------------------------------------------------------------------

const isDedicatedWorker =
  typeof DedicatedWorkerGlobalScope !== 'undefined' &&
  typeof self !== 'undefined' &&
  self instanceof DedicatedWorkerGlobalScope;

if (isDedicatedWorker) {
  let instancePromise = null;

  const workerIO = {
    out: (text) => self.postMessage({ type: 'io', stream: 'out', text }),
    err: (text) => self.postMessage({ type: 'io', stream: 'err', text }),
    in: () => null,
  };

  async function getInstance(payload) {
    if (!instancePromise) {
      instancePromise = (async () => {
        const opts = { io: workerIO, sysroot: payload?.sysroot, target: payload?.target };
        const instance = await loadTinyc(opts);
        // Shim modules arrive as URLs (functions cannot cross
        // postMessage): import each one and register its default
        // export — a shim map object — on this instance.
        if (Array.isArray(payload?.shimModules)) {
          for (const url of payload.shimModules) {
            const mod = await import(/* @vite-ignore */ url);
            if (mod?.default && typeof mod.default === 'object') {
              instance.registerShims(mod.default);
            }
          }
        }
        return instance;
      })();
    }
    return instancePromise;
  }

  const handlers = {
    async init(payload) {
      await getInstance(payload);
      return { ready: true };
    },
    async addFile(payload, payloadInit) {
      const instance = await getInstance(payloadInit);
      if (typeof payload.content === 'string') {
        return instance.addFileSync(payload.path, payload.content);
      }
      return instance.addFileSync(payload.path, new Uint8Array(payload.content));
    },
    async registerIncludeDir(payload, payloadInit) {
      const instance = await getInstance(payloadInit);
      return instance.registerIncludeDir(payload.path);
    },
    async compileString(payload, payloadInit) {
      const instance = await getInstance(payloadInit);
      return instance.compileString(payload.source, payload.opts ?? {});
    },
    async compileFile(payload, payloadInit) {
      const instance = await getInstance(payloadInit);
      if (payload.content !== undefined) {
        if (typeof payload.content === 'string') {
          instance.addFileSync(payload.path, payload.content);
        } else {
          instance.addFileSync(payload.path, new Uint8Array(payload.content));
        }
      }
      const state = instance.createState();
      try {
        return state.compileFile(payload.path);
      } finally {
        state.delete();
      }
    },
    async callMain(payload, payloadInit) {
      const instance = await getInstance(payloadInit);
      return instance.callMain(payload.args ?? []);
    },
  };

  let initPayload = null;
  let threadMode = false;

  self.addEventListener('message', async (ev) => {
    // Thread mode (threads design §5.1): a dylink process's
    // thread worker is booted through this same entry point.
    // Once booted it never returns to compiler duty.
    if (ev.data && ev.data.type === 'tinyc-thread-boot') {
      threadMode = true;
      startThreadWorker(ev.data, {
        manifestToLibrary, libraryFromBytes, parseImportTypes,
        makeWrapperFunction, createWasiShim, createShimContext,
        canonicalModulePath, libraryNameForms, evaluateJsModule,
      }, self);
      return;
    }
    if (threadMode) return;
    const { id, type, payload } = ev.data ?? {};
    if (type === 'init') initPayload = payload;
    const handler = handlers[type];
    if (!handler) {
      self.postMessage({
        id, ok: false,
        error: { name: 'TinycWorkerError', message: `unknown message type: ${type}` },
      });
      return;
    }
    try {
      const result = await handler(payload, initPayload);
      flushIO(workerIO);
      self.postMessage({ id, ok: true, result });
    } catch (err) {
      self.postMessage({
        id, ok: false,
        error: { name: err?.name ?? 'Error', message: err?.message ?? String(err) },
      });
    }
  });
}

export default TinycWorker;
