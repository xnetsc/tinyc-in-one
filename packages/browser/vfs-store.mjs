// Persistent storage layer for the virtual filesystem (index.mjs).
//
// A store persists file records { bytes, size, mtime } keyed by path
// and, once opened, exposes the persisted index (path -> { size,
// mtime }) without file contents. VirtualFS keeps only that index
// (plus a small content cache) in memory; the bytes live here.
//
// Two backends with identical semantics:
//   IndexedDbFsStore  one IndexedDB database per target, named
//                     "tinyc-web-fs-<target>", with a single object
//                     store "files" (key = path, value = the record).
//                     Used whenever the environment has IndexedDB
//                     (browsers and Web Workers both do).
//   MemoryFsStore     a plain Map. Used where IndexedDB does not
//                     exist (Node, tests) and as the fallback when an
//                     IndexedDB open fails (e.g. blocked in private
//                     browsing). Volatile: contents last as long as
//                     the store object.
//
// A custom store — or a factory producing one — can be injected per
// loadTinyc call (options.fsStore / options.storeFactory), which is
// how the test suite simulates IndexedDB semantics, including its
// asynchronous write queue, without a browser.

/** In-memory store: the no-IndexedDB fallback backend. */
export class MemoryFsStore {
  constructor() {
    this.kind = 'memory';
    this._records = new Map();
    this.index = new Map();
  }

  async open() {
    this.index = new Map();
    for (const [path, rec] of this._records) {
      this.index.set(path, { size: rec.size, mtime: rec.mtime });
    }
    return this.index;
  }

  async get(path) {
    const rec = this._records.get(path);
    return rec ? { bytes: rec.bytes, size: rec.size, mtime: rec.mtime } : null;
  }

  async put(path, record) {
    this._records.set(path, {
      bytes: record.bytes,
      size: record.size,
      mtime: record.mtime,
    });
  }

  async delete(path) {
    this._records.delete(path);
  }

  async close() {}
}

/** IndexedDB store: one database per target, one "files" table. */
export class IndexedDbFsStore {
  constructor(dbName) {
    this.kind = 'indexeddb';
    this.dbName = dbName;
    this.index = new Map();
    this._db = null;
  }

  _openDatabase() {
    return new Promise((resolve, reject) => {
      const req = globalThis.indexedDB.open(this.dbName, 1);
      req.onupgradeneeded = () => {
        const db = req.result;
        if (!db.objectStoreNames.contains('files')) db.createObjectStore('files');
      };
      req.onsuccess = () => resolve(req.result);
      req.onerror = () => reject(req.error);
    });
  }

  async open() {
    this._db = await this._openDatabase();
    this.index = new Map();
    // Walk the table once with a cursor, keeping only each record's
    // metadata for the index; the bytes stay in the database.
    await new Promise((resolve, reject) => {
      const tx = this._db.transaction('files', 'readonly');
      const req = tx.objectStore('files').openCursor();
      req.onsuccess = () => {
        const cursor = req.result;
        if (!cursor) {
          resolve();
          return;
        }
        const value = cursor.value;
        this.index.set(cursor.key, { size: value.size, mtime: value.mtime });
        cursor.continue();
      };
      req.onerror = () => reject(req.error);
      tx.onerror = () => reject(tx.error);
    });
    return this.index;
  }

  _request(request) {
    return new Promise((resolve, reject) => {
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  _transactionDone(tx) {
    return new Promise((resolve, reject) => {
      tx.oncomplete = () => resolve();
      tx.onerror = () => reject(tx.error);
      tx.onabort = () => reject(tx.error);
    });
  }

  async get(path) {
    const tx = this._db.transaction('files', 'readonly');
    const rec = await this._request(tx.objectStore('files').get(path));
    return rec ?? null;
  }

  async put(path, record) {
    const tx = this._db.transaction('files', 'readwrite');
    tx.objectStore('files').put(
      { bytes: record.bytes, size: record.size, mtime: record.mtime },
      path
    );
    await this._transactionDone(tx);
  }

  async delete(path) {
    const tx = this._db.transaction('files', 'readwrite');
    tx.objectStore('files').delete(path);
    await this._transactionDone(tx);
  }

  async close() {
    if (this._db) {
      this._db.close();
      this._db = null;
    }
  }
}

/** Pick the store for one loadTinyc call: an injected store
 *  (options.fsStore) wins, then an injected factory
 *  (options.storeFactory, called with the target), then IndexedDB
 *  when the environment has it, then the in-memory backend. */
export function createFsStore(target, options = {}) {
  if (options.fsStore) return options.fsStore;
  if (typeof options.storeFactory === 'function') {
    return options.storeFactory(target);
  }
  if (typeof globalThis.indexedDB !== 'undefined' && globalThis.indexedDB) {
    return new IndexedDbFsStore(`tinyc-web-fs-${target}`);
  }
  return new MemoryFsStore();
}

/** Create the store for a target and open it (loading its index).
 *  When the default IndexedDB backend fails to open, this falls
 *  back to the in-memory backend rather than failing the load;
 *  errors from an injected store or factory propagate. */
export async function openFsStore(target, options = {}) {
  const injected = Boolean(options.fsStore) || typeof options.storeFactory === 'function';
  let store = await createFsStore(target, options);
  try {
    await store.open();
  } catch (err) {
    if (injected || store.kind !== 'indexeddb') throw err;
    store = new MemoryFsStore();
    await store.open();
  }
  return store;
}
