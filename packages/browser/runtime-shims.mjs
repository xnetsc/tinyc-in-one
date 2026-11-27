// runtime-shims.mjs: default "env" imports for running wasm32
// products compiled from C in the browser.
//
// A C program compiled to wasm32 imports the libc functions it uses.
// There is no libc .a to link in this package, so runWasmModule
// supplies this JS implementation of a common libc subset by
// default (module name "env"). Every function operates on the
// running instance's linear memory.
//
// Timing: the import functions are created before the instance
// exists. They never capture the memory directly — they read it
// through `holder` on every call. The glue (index.mjs) fills
// holder.memory and holder.instance right after instantiation,
// before any entry point (_start) is invoked, so by the time a shim
// can be called the memory is there.
//
// Priority: these are the lowest-priority layer. Per-run shims,
// instance-registered shims, package-global shims and raw
// options.imports entries all override individual entries (see
// index.mjs, buildWasmImports).

const textEncoder = new TextEncoder();
const textDecoder = new TextDecoder();

/** Thrown by the libc exit/abort shims. The runner in index.mjs
 *  catches it and reports { exitCode }, exactly like WASI
 *  proc_exit — exiting a guest program never terminates the host. */
export class ShimExit extends Error {
  constructor(code) {
    super(`tinyc: libc exit(${code})`);
    this.name = 'ShimExit';
    this.exitCode = code;
  }
}

const ALLOC_USED = 0xa110ce11;

/**
 * Build the default shim imports.
 *
 * options:
 *   io        byte IO: { out(Uint8Array), err(Uint8Array),
 *             in() -> Uint8Array|string|null } — stdio output goes
 *             to out/err as bytes; stdin is pulled from `stdin`
 *             first, then from io.in() chunks.
 *   holder    { memory, instance } — filled by the glue after
 *             instantiation (see the timing note above).
 *   stdin     Uint8Array|string served to fread/fgetc before io.in.
 *   heapBase  start address of the shim allocator's heap region in
 *             linear memory (default 65536). The allocator grows
 *             the memory when it runs out.
 *   allocatorState  shared { heapTop, freeHead } for the internal
 *             allocator, so several shim sets (or a process loader)
 *             bump-allocate from one frontier. Default: private.
 *   memoryAllocator  process mode: the allocator state lives in
 *             the memory's reserved header words ([512,516) and
 *             [516,520)) under the spin lock at [520,524), and the
 *             returned allocatorState is the accessor view over
 *             those words. Takes precedence over allocatorState.
 *   processTable  a WebAssembly.Table that atexit handlers are
 *             resolved through on exit (a process's shared table);
 *             default: the running instance's exported table.
 *   fileSystem  backing store for the stdio file functions
 *             (fopen & family): { read(path) -> Uint8Array|null,
 *             write(path, bytes), remove(path) }, synchronous,
 *             paths absolute and normalized. Default: a private
 *             in-memory map owned by this shim set, so a run's
 *             files live exactly as long as the run.
 *   args/env  accepted for symmetry with the WASI shim options;
 *             libc shims do not consume argv, and getenv returns
 *             null by design, so neither is read.
 *
 * Returns { env: { ... } }, ready to merge into an importObject.
 *
 * Allocator rule: malloc/calloc/realloc/free are served by a simple
 * free-list allocator inside the module's linear memory — unless
 * the module exports its own malloc, in which case the shim
 * forwards allocation calls to the module's own allocator and the
 * internal one never touches the heap. (If the module exports no
 * malloc, nothing else can allocate, so there is no conflict; if it
 * does export one, forwarding keeps exactly one allocator in play.)
 */
export function createDefaultShims(options = {}) {
  const { holder } = options;
  const io = options.io ?? {};
  const emitOut = typeof io.out === 'function' ? io.out : () => {};
  const emitErr = typeof io.err === 'function' ? io.err : () => {};
  const heapBase = options.heapBase ?? 65536;

  // -- linear memory access (always through the holder) --
  const memory = () => holder?.memory ?? null;
  const u8 = () => new Uint8Array(memory().buffer);
  const dv = () => new DataView(memory().buffer);
  const readU32 = (ptr) => dv().getUint32(ptr, true);
  const writeU32 = (ptr, v) => dv().setUint32(ptr, v >>> 0, true);
  const readBytes = (ptr, len) => u8().slice(ptr, ptr + len);
  const writeBytes = (ptr, data) => { u8().set(data, ptr); };
  const readCString = (ptr, max = 1 << 20) => {
    if (!ptr) return '';
    const m = u8();
    let end = ptr;
    while (end < m.length && end - ptr < max && m[end] !== 0) end++;
    // Copy before decoding: TextDecoder refuses views over a
    // shared memory (the real thread tier's workers).
    return textDecoder.decode(m.slice(ptr, end));
  };
  const writeCStringBytes = (ptr, bytes) => {
    writeBytes(ptr, bytes);
    u8()[ptr + bytes.length] = 0;
    return bytes.length;
  };

  // ---------------------------------------------------------------
  // Error number state for this shim set (one per run / process):
  // the file and allocation functions below record the failure
  // code here, and perror reports it. The sysroot has no errno
  // variable surface — in this process model a data symbol
  // cannot be supplied by the shim layer (the same reason
  // stdin/stdout/stderr are macros over handle values) — so the
  // number lives only here, and perror is its only reader.
  // Successful calls never clear it.
  // ---------------------------------------------------------------
  const ERRNO = {
    ENOENT: 2, EBADF: 9, ENOMEM: 12, EEXIST: 17, EINVAL: 22,
  };
  let errnoValue = 0;
  const setErrno = (code) => { errnoValue = code; };

  // ---------------------------------------------------------------
  // Allocator: first-fit free list over [heapBase, memory end).
  // Block layout: [payloadSize u32][nextOrUsed u32][payload...];
  // the payload pointer handed out is header + 8.
  //
  // In a process (options.memoryAllocator) the allocator state
  // does not live in JS: heapTop and freeHead are words of the
  // process's reserved header span — [512,516) and [516,520) —
  // because in the real thread tier several workers allocate from
  // the same state.  Every mutation (rawMalloc, rawFree,
  // reserveStatic) runs under the spin lock at [520,524): taken
  // with a compare-exchange, released with a store and a notify,
  // and never held across a wait.  In the simulated tier the lock
  // is simply never contended.  Standalone runs keep the state in
  // a plain JS object, exactly as before.
  // ---------------------------------------------------------------
  const memoryAllocator = options.memoryAllocator === true;
  const HDR_HEAP_TOP = 512, HDR_FREE_HEAD = 516, HDR_ALLOC_LOCK = 520;
  const allocState = memoryAllocator
    ? {
        get heapTop() { return dv().getInt32(HDR_HEAP_TOP, true); },
        set heapTop(v) { dv().setInt32(HDR_HEAP_TOP, v | 0, true); },
        get freeHead() { return dv().getInt32(HDR_FREE_HEAD, true); },
        set freeHead(v) { dv().setInt32(HDR_FREE_HEAD, v | 0, true); },
      }
    : options.allocatorState ?? { heapTop: 0, freeHead: 0 };

  function allocLock() {
    if (!memoryAllocator) return;
    for (;;) {
      // the view is rebuilt per attempt: growing a non-shared
      // memory replaces its buffer
      const words = new Int32Array(memory().buffer);
      if (Atomics.compareExchange(words, HDR_ALLOC_LOCK / 4, 0, 1) === 0)
        return;
    }
  }
  function allocUnlock() {
    if (!memoryAllocator) return;
    const words = new Int32Array(memory().buffer);
    Atomics.store(words, HDR_ALLOC_LOCK / 4, 0);
    Atomics.notify(words, HDR_ALLOC_LOCK / 4, 1);
  }
  function withAllocLock(fn) {
    if (!memoryAllocator) return fn();
    allocLock();
    try { return fn(); } finally { allocUnlock(); }
  }

  const moduleAllocator = () => {
    const ex = holder?.instance?.exports;
    return ex && typeof ex.malloc === 'function' ? ex : null;
  };

  function ensureCapacity(end) {
    const mem = memory();
    if (!mem) return false;
    if (mem.buffer.byteLength >= end) return true;
    const pages = Math.ceil((end - mem.buffer.byteLength) / 65536);
    try {
      mem.grow(pages);
      return true;
    } catch {
      return false;
    }
  }

  function rawMallocImpl(size) {
    size = Math.max(8, (size + 7) & ~7);
    let prev = 0;
    let cur = allocState.freeHead;
    while (cur) {
      const csize = readU32(cur);
      const cnext = readU32(cur + 4);
      if (csize >= size) {
        if (csize - size >= 16) {
          // split: tail stays on the free list
          const rem = cur + 8 + size;
          writeU32(rem, csize - size - 8);
          writeU32(rem + 4, cnext);
          writeU32(cur, size);
          if (prev) writeU32(prev + 4, rem); else allocState.freeHead = rem;
        } else {
          if (prev) writeU32(prev + 4, cnext); else allocState.freeHead = cnext;
        }
        writeU32(cur + 4, ALLOC_USED);
        return cur + 8;
      }
      prev = cur;
      cur = cnext;
    }
    if (!allocState.heapTop) allocState.heapTop = heapBase;
    const header = allocState.heapTop;
    const end = header + 8 + size;
    if (!ensureCapacity(end)) return 0;
    writeU32(header, size);
    writeU32(header + 4, ALLOC_USED);
    allocState.heapTop = end;
    return header + 8;
  }

  function rawFreeImpl(ptr) {
    if (!ptr) return;
    const header = ptr - 8;
    if (readU32(header + 4) !== ALLOC_USED) return; // foreign/double free
    writeU32(header + 4, allocState.freeHead);
    allocState.freeHead = header;
  }

  // Reserve a static region of `size` bytes at the allocator
  // frontier and return its base address. Used by the process
  // loader to place a module's static span; the region is never
  // freed (module memory is retained for the process's life).
  function reserveStaticImpl(size, align = 16) {
    if (!allocState.heapTop) allocState.heapTop = heapBase;
    const base = (allocState.heapTop + (align - 1)) & ~(align - 1);
    const end = base + size;
    if (!ensureCapacity(end))
      throw new Error(`tinyc: cannot grow linear memory to reserve ${size} bytes at ${base}`);
    allocState.heapTop = end;
    return base;
  }

  function rawMalloc(size) { return withAllocLock(() => rawMallocImpl(size)); }
  function rawFree(ptr) { return withAllocLock(() => rawFreeImpl(ptr)); }
  function reserveStatic(size, align) {
    return withAllocLock(() => reserveStaticImpl(size, align));
  }

  function malloc(size) {
    const own = moduleAllocator();
    const ptr = own ? own.malloc(size) : rawMalloc(size);
    if (!ptr && size) setErrno(ERRNO.ENOMEM);
    return ptr;
  }

  function free(ptr) {
    const own = moduleAllocator();
    if (own && typeof own.free === 'function') {
      own.free(ptr);
      return;
    }
    rawFree(ptr);
  }

  function calloc(n, size) {
    const own = moduleAllocator();
    if (own && typeof own.calloc === 'function') return own.calloc(n, size);
    const total = n * size;
    const ptr = malloc(total);
    if (ptr) writeBytes(ptr, new Uint8Array(total));
    return ptr;
  }

  function realloc(ptr, size) {
    if (!ptr) return malloc(size);
    if (size === 0) {
      free(ptr);
      return 0;
    }
    const own = moduleAllocator();
    if (own) {
      if (typeof own.realloc === 'function') return own.realloc(ptr, size);
      const np = own.malloc(size);
      if (!np) {
        setErrno(ERRNO.ENOMEM);
        return 0;
      }
      // The preserved prefix is what matters; bytes past the old
      // block's end land in the new block's uninitialized tail.
      writeBytes(np, readBytes(ptr, size));
      if (typeof own.free === 'function') own.free(ptr);
      return np;
    }
    const oldSize = readU32(ptr - 8);
    if (size <= oldSize) return ptr;
    const np = rawMalloc(size);
    if (!np) {
      setErrno(ERRNO.ENOMEM);
      return 0;
    }
    writeBytes(np, readBytes(ptr, oldSize));
    rawFree(ptr);
    return np;
  }

  // ---------------------------------------------------------------
  // Memory and string functions
  // ---------------------------------------------------------------
  function memcpy(dest, src, n) {
    writeBytes(dest, readBytes(src, n)); // readBytes copies: overlap-safe
    return dest;
  }
  function memmove(dest, src, n) {
    writeBytes(dest, readBytes(src, n));
    return dest;
  }
  function memset(ptr, value, n) {
    u8().fill(value & 0xff, ptr, ptr + n);
    return ptr;
  }
  function memcmp(a, b, n) {
    const ma = u8();
    for (let i = 0; i < n; i++) {
      const d = ma[a + i] - ma[b + i];
      if (d !== 0) return d;
    }
    return 0;
  }
  function strlen(ptr) {
    const m = u8();
    let n = 0;
    while (m[ptr + n] !== 0) n++;
    return n;
  }
  function strcpy(dest, src) {
    writeBytes(dest, readBytes(src, strlen(src) + 1));
    return dest;
  }
  function strncpy(dest, src, n) {
    const m = u8();
    let i = 0;
    while (i < n && m[src + i] !== 0) {
      m[dest + i] = m[src + i];
      i++;
    }
    while (i < n) m[dest + i++] = 0;
    return dest;
  }
  function strcat(dest, src) {
    writeBytes(dest + strlen(dest), readBytes(src, strlen(src) + 1));
    return dest;
  }
  function strncat(dest, src, n) {
    const m = u8();
    const at = dest + strlen(dest);
    let i = 0;
    while (i < n && m[src + i] !== 0) {
      m[at + i] = m[src + i];
      i++;
    }
    m[at + i] = 0;
    return dest;
  }
  function strcmp(a, b) {
    const m = u8();
    let i = 0;
    for (;;) {
      const d = m[a + i] - m[b + i];
      if (d !== 0 || m[a + i] === 0) return d;
      i++;
    }
  }
  function strncmp(a, b, n) {
    const m = u8();
    for (let i = 0; i < n; i++) {
      const d = m[a + i] - m[b + i];
      if (d !== 0 || m[a + i] === 0) return d;
    }
    return 0;
  }
  function strchr(ptr, c) {
    const m = u8();
    const want = c & 0xff;
    let i = 0;
    for (;;) {
      if (m[ptr + i] === want) return ptr + i;
      if (m[ptr + i] === 0) return 0;
      i++;
    }
  }
  function strrchr(ptr, c) {
    const m = u8();
    const want = c & 0xff;
    let found = 0;
    let i = 0;
    for (;;) {
      if (m[ptr + i] === want) found = ptr + i;
      if (m[ptr + i] === 0) return found;
      i++;
    }
  }
  function strstr(haystack, needle) {
    const h = readCString(haystack);
    const ndl = readCString(needle);
    if (ndl === '') return haystack;
    const idx = h.indexOf(ndl);
    return idx < 0 ? 0 : haystack + textEncoder.encode(h.slice(0, idx)).length;
  }
  function strdup(ptr) {
    const copy = readBytes(ptr, strlen(ptr) + 1);
    const dest = malloc(copy.length);
    if (dest) writeBytes(dest, copy);
    return dest;
  }
  function memchr(ptr, c, n) {
    const m = u8();
    const want = c & 0xff;
    for (let i = 0; i < n; i++) {
      if (m[ptr + i] === want) return ptr + i;
    }
    return 0;
  }
  function strspn(ptr, acceptPtr) {
    const s = readCString(ptr);
    const accept = readCString(acceptPtr);
    let n = 0;
    while (n < s.length && accept.includes(s[n])) n++;
    return n;
  }
  function strcspn(ptr, rejectPtr) {
    const s = readCString(ptr);
    const reject = readCString(rejectPtr);
    let n = 0;
    while (n < s.length && !reject.includes(s[n])) n++;
    return n;
  }
  function strpbrk(ptr, acceptPtr) {
    const n = strcspn(ptr, acceptPtr);
    return n < strlen(ptr) ? ptr + n : 0;
  }
  let strtokSave = 0;
  function strtok(ptr, delimPtr) {
    const delim = readCString(delimPtr);
    let start;
    if (ptr) {
      start = ptr;
    } else {
      if (!strtokSave) return 0;
      start = strtokSave;
    }
    const m = u8();
    let i = start;
    while (m[i] !== 0 && delim.includes(String.fromCharCode(m[i]))) i++;
    if (m[i] === 0) {
      strtokSave = 0;
      return 0;
    }
    const token = i;
    while (m[i] !== 0 && !delim.includes(String.fromCharCode(m[i]))) i++;
    if (m[i] !== 0) {
      m[i] = 0;
      strtokSave = i + 1;
    } else {
      strtokSave = 0;
    }
    return token;
  }
  function strerrorText(errnum) {
    // The C-locale texts for the error codes this layer records
    // (and the common codes a guest can name itself).
    const table = {
      0: 'Success',
      1: 'Operation not permitted',
      2: 'No such file or directory',
      9: 'Bad file descriptor',
      12: 'Cannot allocate memory',
      17: 'File exists',
      22: 'Invalid argument',
    };
    return table[errnum] ?? `Unknown error ${errnum | 0}`;
  }
  function strerror(errnum) {
    const bytes = textEncoder.encode(strerrorText(errnum));
    const buf = malloc(bytes.length + 1);
    if (buf) writeCStringBytes(buf, bytes);
    return buf;
  }
  function strcoll(a, b) {
    return strcmp(a, b);
  }
  function strxfrm(dest, src, n) {
    const len = strlen(src);
    if (dest && n > 0) {
      const take = Math.min(len, n - 1);
      writeBytes(dest, readBytes(src, take));
      u8()[dest + take] = 0;
    }
    return len;
  }

  // ---------------------------------------------------------------
  // ctype: table-driven. Flags per byte: upper/lower/digit/space/
  // punct/cntrl/xdigit/blank.
  // ---------------------------------------------------------------
  const CT = (() => {
    const t = new Uint8Array(256);
    const U = 1, L = 2, D = 4, S = 8, P = 16, C = 32, X = 64, B = 128;
    for (let c = 0x00; c < 0x20; c++) t[c] |= C;
    t[0x7f] |= C;
    for (let c = 0x30; c <= 0x39; c++) t[c] |= D | X;
    for (let c = 0x41; c <= 0x46; c++) t[c] |= X;
    for (let c = 0x61; c <= 0x66; c++) t[c] |= X;
    for (let c = 0x41; c <= 0x5a; c++) t[c] |= U;
    for (let c = 0x61; c <= 0x7a; c++) t[c] |= L;
    for (const c of [0x20, 0x09, 0x0a, 0x0b, 0x0c, 0x0d]) t[c] |= S;
    t[0x20] |= B;
    t[0x09] |= B;
    for (let c = 0x21; c <= 0x7e; c++) {
      if (!(t[c] & (U | L | D))) t[c] |= P;
    }
    return { t, U, L, D, S, P, C, X, B };
  })();
  const ctFlag = (bit) => (c) =>
    c >= 0 && c <= 255 ? (CT.t[c] & bit ? 1 : 0) : 0;
  const isupper = ctFlag(CT.U);
  const islower = ctFlag(CT.L);
  const isdigit = ctFlag(CT.D);
  const isspace = ctFlag(CT.S);
  const ispunct = ctFlag(CT.P);
  const iscntrl = ctFlag(CT.C);
  const isxdigit = ctFlag(CT.X);
  const isblank = ctFlag(CT.B);
  const isalpha = (c) => (isupper(c) || islower(c) ? 1 : 0);
  const isalnum = (c) => (isalpha(c) || isdigit(c) ? 1 : 0);
  const isprint = (c) => (c >= 0x20 && c <= 0x7e ? 1 : 0);
  const isgraph = (c) => (c >= 0x21 && c <= 0x7e ? 1 : 0);
  const tolower = (c) => (isupper(c) ? c + 32 : c);
  const toupper = (c) => (islower(c) ? c - 32 : c);

  // ---------------------------------------------------------------
  // stdlib conversions
  // ---------------------------------------------------------------
  function scanInt(ptr, base) {
    const s = readCString(ptr);
    let i = 0;
    while (i < s.length && ' \t\n\v\f\r'.includes(s[i])) i++;
    let sign = 1n;
    if (s[i] === '+' || s[i] === '-') {
      if (s[i] === '-') sign = -1n;
      i++;
    }
    let b = base;
    if (b === 0) {
      if (s[i] === '0' && (s[i + 1] === 'x' || s[i + 1] === 'X')) b = 16;
      else if (s[i] === '0') b = 8;
      else b = 10;
    }
    if (b === 16 && s[i] === '0' && (s[i + 1] === 'x' || s[i + 1] === 'X')) i += 2;
    const digits = '0123456789abcdefghijklmnopqrstuvwxyz';
    let value = 0n;
    let any = false;
    while (i < s.length) {
      const d = digits.indexOf(s[i].toLowerCase());
      if (d < 0 || d >= b) break;
      value = value * BigInt(b) + BigInt(d);
      i++;
      any = true;
    }
    return { value: sign * value, consumed: any ? i : 0, any };
  }
  function finishEndptr(endptr, ptr, consumed) {
    if (endptr) writeU32(endptr, ptr + consumed);
  }
  const I32_MIN = -2147483648n;
  const I32_MAX = 2147483647n;
  const U32_MAX = 4294967295n;
  function clampBig(v, lo, hi) {
    return v < lo ? lo : v > hi ? hi : v;
  }
  function strtol(ptr, endptr, base) {
    const r = scanInt(ptr, base | 0);
    finishEndptr(endptr, ptr, r.consumed);
    return Number(clampBig(r.value, I32_MIN, I32_MAX));
  }
  function strtoul(ptr, endptr, base) {
    const r = scanInt(ptr, base | 0);
    finishEndptr(endptr, ptr, r.consumed);
    return Number(clampBig(r.value, 0n, U32_MAX)) >>> 0;
  }
  function strtoll(ptr, endptr, base) {
    const r = scanInt(ptr, base | 0);
    finishEndptr(endptr, ptr, r.consumed);
    return r.value;
  }
  function strtoull(ptr, endptr, base) {
    const r = scanInt(ptr, base | 0);
    finishEndptr(endptr, ptr, r.consumed);
    return r.value < 0n ? 0n : r.value;
  }
  function scanFloat(ptr) {
    const s = readCString(ptr);
    const m = /^[\s]*[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?/.exec(s) ??
      /^[\s]*[+-]?(inf(inity)?|nan)/i.exec(s);
    if (!m) return { value: 0, consumed: 0 };
    return { value: parseFloat(m[0]), consumed: m[0].length };
  }
  function strtod(ptr, endptr) {
    const r = scanFloat(ptr);
    finishEndptr(endptr, ptr, r.consumed);
    return r.value;
  }
  function strtof(ptr, endptr) {
    return Math.fround(strtod(ptr, endptr));
  }
  function atoi(ptr) {
    return Number(clampBig(scanInt(ptr, 10).value, I32_MIN, I32_MAX));
  }
  function atol(ptr) {
    return atoi(ptr);
  }
  function atoll(ptr) {
    return scanInt(ptr, 10).value;
  }
  function atof(ptr) {
    return scanFloat(ptr).value;
  }
  const abs = (v) => Math.abs(v | 0);
  const labs = (v) => Math.abs(v | 0);
  const llabs = (v) => (v < 0n ? -v : v);

  // qsort/bsearch call the guest comparator through the module's
  // indirect function table — the same route atexit handlers
  // take. Without a table there is nothing to call through, and
  // the functions leave the array as it is.
  function comparatorFn(cmpPtr) {
    const table = options.processTable ??
      holder?.instance?.exports?.__indirect_function_table;
    if (!table || typeof table.get !== 'function') return null;
    const fn = table.get(cmpPtr);
    return typeof fn === 'function' ? fn : null;
  }
  function qsort(base, nmemb, size, cmpPtr) {
    const cmp = comparatorFn(cmpPtr);
    if (!cmp || nmemb < 2 || size <= 0) return;
    // Sort an index permutation, comparing through the elements'
    // real addresses, then apply the permutation in one pass.
    const order = Array.from({ length: nmemb }, (_, i) => i);
    order.sort((ia, ib) => cmp(base + ia * size, base + ib * size) | 0);
    const items = order.map((i) => readBytes(base + i * size, size));
    for (let i = 0; i < nmemb; i++) writeBytes(base + i * size, items[i]);
  }
  function bsearch(key, base, nmemb, size, cmpPtr) {
    const cmp = comparatorFn(cmpPtr);
    if (!cmp) return 0;
    let lo = 0;
    let hi = nmemb - 1;
    while (lo <= hi) {
      const mid = (lo + hi) >> 1;
      const r = cmp(key, base + mid * size) | 0;
      if (r === 0) return base + mid * size;
      if (r < 0) hi = mid - 1;
      else lo = mid + 1;
    }
    return 0;
  }

  let randState = 1;
  function srand(seed) {
    randState = seed >>> 0 || 1;
  }
  function rand() {
    randState = (Math.imul(randState, 1103515245) + 12345) & 0x7fffffff;
    return (randState >> 16) & 0x7fff;
  }

  // ---------------------------------------------------------------
  // exit / atexit (minimal semantics): handlers are C function
  // pointers — wasm table indices. They can only be invoked if the
  // module exports its indirect function table; when it does not,
  // handlers are skipped and exit still unwinds via ShimExit.
  // ---------------------------------------------------------------
  const atexitHandlers = [];
  function atexit(fnPtr) {
    atexitHandlers.push(fnPtr);
    return 0;
  }
  function exit(code) {
    const table = options.processTable ??
      holder?.instance?.exports?.__indirect_function_table;
    if (table && typeof table.get === 'function') {
      for (let i = atexitHandlers.length - 1; i >= 0; i--) {
        try {
          const fn = table.get(atexitHandlers[i]);
          if (typeof fn === 'function') fn();
        } catch {
          // A failing handler must not mask the exit itself.
        }
      }
    }
    throw new ShimExit(code | 0);
  }
  function abort() {
    throw new ShimExit(134);
  }

  // ---------------------------------------------------------------
  // stdin: `options.stdin` bytes first, then io.in() chunks.
  // ---------------------------------------------------------------
  let stdinBuf = null;
  let stdinPos = 0;
  const pushback = [];
  if (options.stdin != null) {
    stdinBuf =
      typeof options.stdin === 'string'
        ? textEncoder.encode(options.stdin)
        : options.stdin;
  }
  function stdinPull() {
    if (stdinBuf && stdinPos < stdinBuf.length) return true;
    if (typeof io.in === 'function') {
      const chunk = io.in();
      if (chunk == null) return false;
      const b = typeof chunk === 'string' ? textEncoder.encode(chunk) : chunk;
      if (!b || b.length === 0) return false;
      stdinBuf = b;
      stdinPos = 0;
      return true;
    }
    return false;
  }
  function stdinRead(n) {
    const parts = [];
    let got = 0;
    while (got < n) {
      if (pushback.length) {
        parts.push(Uint8Array.of(pushback.pop()));
        got++;
        continue;
      }
      if (!stdinPull()) break;
      const take = Math.min(n - got, stdinBuf.length - stdinPos);
      parts.push(stdinBuf.slice(stdinPos, stdinPos + take));
      stdinPos += take;
      got += take;
    }
    const result = new Uint8Array(got);
    let at = 0;
    for (const p of parts) {
      result.set(p, at);
      at += p.length;
    }
    return result;
  }

  // ---------------------------------------------------------------
  // stdio. Variadic functions receive a va_list pointer: TCC's
  // wasm32 va_list is a pointer into a memory region where the
  // variadic arguments are laid out in order, each at its natural
  // alignment — i32/pointers in 4-byte slots, i64 and f64 in
  // 8-byte slots aligned to 8. printf's format parsing happens
  // here in JS, reading the format string from linear memory.
  //
  // FILE* routing: stream values are opaque in this shim. Output
  // goes to io.out, except stream value 2 (stderr's conventional
  // value), which goes to io.err. fread/fgetc read stdin.
  // ---------------------------------------------------------------
  function makeVa(vaPtr) {
    let cur = vaPtr;
    const alignTo = (n) => {
      cur = (cur + n - 1) & ~(n - 1);
    };
    return {
      i32() {
        alignTo(4);
        const v = dv().getInt32(cur, true);
        cur += 4;
        return v;
      },
      u32() {
        alignTo(4);
        const v = dv().getUint32(cur, true);
        cur += 4;
        return v;
      },
      i64() {
        alignTo(8);
        const v = dv().getBigInt64(cur, true);
        cur += 8;
        return v;
      },
      u64() {
        alignTo(8);
        const v = dv().getBigUint64(cur, true);
        cur += 8;
        return v;
      },
      f64() {
        alignTo(8);
        const v = dv().getFloat64(cur, true);
        cur += 8;
        return v;
      },
    };
  }

  function padSpaces(s, width, left) {
    if (s.length >= width) return s;
    const p = ' '.repeat(width - s.length);
    return left ? s + p : p + s;
  }

  function fmtInteger(digits, sign, prefix, flags, width, precision) {
    let body = digits;
    if (precision >= 0) body = body.padStart(precision, '0');
    const head = sign + prefix;
    if (flags.includes('0') && !flags.includes('-') && precision < 0) {
      const total = head.length + body.length;
      if (total < width) body = '0'.repeat(width - total) + body;
      return head + body;
    }
    return padSpaces(head + body, width, flags.includes('-'));
  }

  function fmtFloatNumber(body, flags, width) {
    let sign = '';
    let rest = body;
    if (rest.startsWith('-')) {
      sign = '-';
      rest = rest.slice(1);
    } else if (flags.includes('+')) {
      sign = '+';
    } else if (flags.includes(' ')) {
      sign = ' ';
    }
    const special = /^(inf|nan)/i.test(rest);
    if (flags.includes('0') && !flags.includes('-') && !special) {
      const total = sign.length + rest.length;
      if (total < width) rest = '0'.repeat(width - total) + rest;
      return sign + rest;
    }
    return padSpaces(sign + rest, width, flags.includes('-'));
  }

  function expParts(s) {
    const [mant, exp] = s.split('e');
    const e = parseInt(exp, 10);
    return [mant, (e < 0 ? '-' : '+') + String(Math.abs(e)).padStart(2, '0')];
  }
  function fmtE(v, precision, upper) {
    if (Number.isNaN(v)) return upper ? 'NAN' : 'nan';
    if (!Number.isFinite(v)) return (v > 0 ? '' : '-') + (upper ? 'INF' : 'inf');
    const [mant, e] = expParts(v.toExponential(precision));
    return mant + (upper ? 'E' : 'e') + e;
  }
  function fmtF(v, precision, upper) {
    if (Number.isNaN(v)) return upper ? 'NAN' : 'nan';
    if (!Number.isFinite(v)) return (v > 0 ? '' : '-') + (upper ? 'INF' : 'inf');
    return v.toFixed(precision);
  }
  function fmtG(v, precision, upper, alt) {
    if (Number.isNaN(v) || !Number.isFinite(v)) return fmtF(v, 0, upper);
    const p = precision <= 0 ? 1 : precision;
    let s = v.toPrecision(p);
    if (!alt) {
      const strip = (m) =>
        m.includes('.') ? m.replace(/0+$/, '').replace(/\.$/, '') : m;
      if (s.includes('e')) {
        const [m, e] = s.split('e');
        s = strip(m) + 'e' + e;
      } else {
        s = strip(s);
      }
      s = s.replace(/e([+-])(\d)$/, 'e$10$2');
    }
    return upper ? s.toUpperCase() : s;
  }

  function convert(conv, { flags, width, precision, length, va }, outLen) {
    switch (conv) {
      case 'd':
      case 'i': {
        let sign = '';
        let digits;
        if (length === 'll') {
          const v = va.i64();
          if (v < 0n) {
            sign = '-';
            digits = (-v).toString(10);
          } else digits = v.toString(10);
          if (v === 0n && precision === 0) digits = '';
        } else {
          let v = va.i32();
          if (length === 'hh') v = (v << 24) >> 24;
          else if (length === 'h') v = (v << 16) >> 16;
          if (v < 0) {
            sign = '-';
            digits = String(-v);
          } else digits = String(v);
          if (v === 0 && precision === 0) digits = '';
        }
        if (!sign && flags.includes('+')) sign = '+';
        else if (!sign && flags.includes(' ')) sign = ' ';
        return fmtInteger(digits, sign, '', flags, width, precision);
      }
      case 'u':
      case 'x':
      case 'X':
      case 'o': {
        const radix = conv === 'u' ? 10 : conv === 'o' ? 8 : 16;
        let digits;
        let isZero;
        if (length === 'll') {
          const v = va.u64();
          isZero = v === 0n;
          digits = v.toString(radix);
          if (isZero && precision === 0) digits = '';
        } else {
          let v = va.u32();
          if (length === 'hh') v &= 0xff;
          else if (length === 'h') v &= 0xffff;
          isZero = v === 0;
          digits = v.toString(radix);
          if (isZero && precision === 0) digits = '';
        }
        if (conv === 'X') digits = digits.toUpperCase();
        let prefix = '';
        if (flags.includes('#') && conv !== 'u') {
          if (conv === 'o') {
            if (!digits.startsWith('0')) prefix = '0';
          } else if (!isZero) {
            prefix = conv === 'X' ? '0X' : '0x';
          }
        }
        return fmtInteger(digits, '', prefix, flags, width, precision);
      }
      case 'c': {
        const ch = String.fromCharCode(va.i32() & 0xff);
        return padSpaces(ch, width, flags.includes('-'));
      }
      case 's': {
        const ptr = va.u32();
        let s = ptr ? readCString(ptr) : '(null)';
        if (precision >= 0) s = s.slice(0, precision);
        return padSpaces(s, width, flags.includes('-'));
      }
      case 'p': {
        const ptr = va.u32();
        return padSpaces('0x' + ptr.toString(16), width, flags.includes('-'));
      }
      case 'f':
      case 'F':
        return fmtFloatNumber(
          fmtF(va.f64(), precision < 0 ? 6 : precision, conv === 'F'),
          flags,
          width,
        );
      case 'e':
      case 'E':
        return fmtFloatNumber(
          fmtE(va.f64(), precision < 0 ? 6 : precision, conv === 'E'),
          flags,
          width,
        );
      case 'g':
      case 'G':
        return fmtFloatNumber(
          fmtG(va.f64(), precision < 0 ? 6 : precision, conv === 'G', flags.includes('#')),
          flags,
          width,
        );
      case 'a':
      case 'A': {
        const v = va.f64();
        return fmtFloatNumber(String(v), flags, width);
      }
      case 'n': {
        const ptr = va.u32();
        if (ptr) {
          if (length === 'll') dv().setBigInt64(ptr, BigInt(outLen), true);
          else dv().setInt32(ptr, outLen, true);
        }
        return '';
      }
      default:
        return '%' + conv;
    }
  }

  function formatString(fmtPtr, vaPtr) {
    const fmt = readCString(fmtPtr);
    const va = makeVa(vaPtr);
    let out = '';
    let i = 0;
    while (i < fmt.length) {
      if (fmt[i] !== '%') {
        out += fmt[i++];
        continue;
      }
      i++;
      if (fmt[i] === '%') {
        out += '%';
        i++;
        continue;
      }
      let flags = '';
      while (i < fmt.length && '-+ #0'.includes(fmt[i])) flags += fmt[i++];
      let width = 0;
      if (fmt[i] === '*') {
        width = va.i32();
        i++;
        if (width < 0) {
          flags += '-';
          width = -width;
        }
      } else {
        while (i < fmt.length && fmt[i] >= '0' && fmt[i] <= '9') {
          width = width * 10 + (fmt.charCodeAt(i++) - 48);
        }
      }
      let precision = -1;
      if (fmt[i] === '.') {
        i++;
        if (fmt[i] === '*') {
          precision = va.i32();
          i++;
        } else {
          precision = 0;
          while (i < fmt.length && fmt[i] >= '0' && fmt[i] <= '9') {
            precision = precision * 10 + (fmt.charCodeAt(i++) - 48);
          }
        }
        if (precision < 0) precision = -1;
      }
      let length = '';
      if (fmt.startsWith('hh', i)) {
        length = 'hh';
        i += 2;
      } else if (fmt.startsWith('ll', i)) {
        length = 'll';
        i += 2;
      } else if (i < fmt.length && 'hlzjtL'.includes(fmt[i])) {
        length = fmt[i++];
      }
      const conv = fmt[i++] ?? '';
      out += convert(conv, { flags, width, precision, length, va }, out.length);
    }
    return out;
  }

  function emitText(text, toErr) {
    const bytes = textEncoder.encode(text);
    (toErr ? emitErr : emitOut)(bytes);
    return bytes.length;
  }

  function printf(fmt, va) {
    return emitText(formatString(fmt, va), false);
  }
  function fprintf(stream, fmt, va) {
    return streamWriteText(stream, formatString(fmt, va));
  }
  function sprintf(dest, fmt, va) {
    const bytes = textEncoder.encode(formatString(fmt, va));
    return writeCStringBytes(dest, bytes);
  }
  function snprintf(dest, size, fmt, va) {
    const bytes = textEncoder.encode(formatString(fmt, va));
    if (size > 0) {
      const n = Math.min(bytes.length, size - 1);
      writeBytes(dest, bytes.subarray(0, n));
      u8()[dest + n] = 0;
    }
    return bytes.length;
  }
  // ---------------------------------------------------------------
  // Streams and files. A FILE* in this layer is an opaque handle
  // value, never a pointer into guest memory: 0, 1 and 2 are the
  // standard streams (stdin reads the run's stdin bytes, stdout
  // writes to io.out, stderr to io.err — the values stdio.h
  // exposes as the stdin/stdout/stderr macros), and fopen hands
  // out fresh handle values above them. An open file's bytes are
  // held in JS and written through to the backing store (the
  // options.fileSystem store, or a private in-memory map) on
  // every write, so a reader that opens the file afterwards —
  // in this run or through the store — sees the content without
  // waiting for fclose.
  // ---------------------------------------------------------------
  const backing = options.fileSystem ?? null;
  const memFiles = new Map();
  const backingRead = backing
    ? (p) => backing.read(p)
    : (p) => (memFiles.has(p) ? memFiles.get(p) : null);
  const backingWrite = backing
    ? (p, bytes) => backing.write(p, bytes)
    : (p, bytes) => { memFiles.set(p, bytes.slice()); };
  const backingRemove = backing
    ? (p) => (typeof backing.remove === 'function' ? backing.remove(p) : false)
    : (p) => memFiles.delete(p);

  function guestPath(ptr) {
    const raw = readCString(ptr);
    const parts = [];
    for (const seg of raw.split('/')) {
      if (seg === '' || seg === '.') continue;
      if (seg === '..') parts.pop();
      else parts.push(seg);
    }
    return '/' + parts.join('/');
  }

  // handle -> { path, data, pos, readable, writable, append,
  //             eof, err, unget[] } for open files. The standard
  // streams are not in this map; handles 0/1/2 are recognized by
  // value in the stream helpers below.
  const openFiles = new Map();
  let nextHandle = 3;
  let stdinEof = false;

  function flushStream(st) {
    backingWrite(st.path, st.data);
  }

  // One byte from a readable stream, or -1 at end of input.
  function streamGetByte(handle) {
    if (handle === 0) {
      const data = stdinRead(1);
      if (!data.length) stdinEof = true;
      return data.length ? data[0] : -1;
    }
    const st = openFiles.get(handle);
    if (!st || !st.readable) return -1;
    if (st.unget.length) return st.unget.pop();
    if (st.pos >= st.data.length) {
      st.eof = true;
      return -1;
    }
    return st.data[st.pos++];
  }

  function streamUnget(handle, c) {
    if (handle === 0) {
      pushback.push(c & 0xff);
      stdinEof = false;
      return;
    }
    const st = openFiles.get(handle);
    if (st) {
      st.unget.push(c & 0xff);
      st.eof = false;
    }
  }

  // Up to n bytes from a readable stream.
  function streamRead(handle, n) {
    if (handle === 0) {
      const data = stdinRead(n);
      if (data.length < n) stdinEof = true;
      return data;
    }
    const st = openFiles.get(handle);
    if (!st || !st.readable) return new Uint8Array(0);
    const parts = [];
    let got = 0;
    while (got < n && st.unget.length) {
      parts.push(Uint8Array.of(st.unget.pop()));
      got++;
    }
    if (got < n) {
      const take = Math.min(n - got, st.data.length - st.pos);
      if (take > 0) {
        parts.push(st.data.slice(st.pos, st.pos + take));
        st.pos += take;
        got += take;
      }
    }
    if (got < n) st.eof = true;
    const out = new Uint8Array(got);
    let at = 0;
    for (const p of parts) {
      out.set(p, at);
      at += p.length;
    }
    return out;
  }

  // Write bytes to a writable stream; returns the count written.
  function streamWrite(handle, bytes) {
    if (handle === 1) {
      emitOut(bytes);
      return bytes.length;
    }
    if (handle === 2) {
      emitErr(bytes);
      return bytes.length;
    }
    const st = openFiles.get(handle);
    if (!st || !st.writable) return 0;
    if (st.append) st.pos = st.data.length;
    const end = st.pos + bytes.length;
    if (end > st.data.length) {
      const grown = new Uint8Array(end);
      grown.set(st.data);
      st.data = grown;
    }
    st.data.set(bytes, st.pos);
    st.pos = end;
    flushStream(st);
    return bytes.length;
  }

  function streamWriteText(handle, text) {
    return streamWrite(handle, textEncoder.encode(text));
  }

  function fopen(pathPtr, modePtr) {
    const path = guestPath(pathPtr);
    const mode = readCString(modePtr);
    const base = mode[0];
    if (base !== 'r' && base !== 'w' && base !== 'a') {
      setErrno(ERRNO.EINVAL);
      return 0;
    }
    const plus = mode.includes('+');
    if (base === 'r') {
      const data = backingRead(path);
      if (data == null) {
        setErrno(ERRNO.ENOENT);
        return 0;
      }
      const handle = nextHandle++;
      openFiles.set(handle, {
        path, data: data.slice(), pos: 0,
        readable: true, writable: plus, append: false,
        eof: false, err: false, unget: [],
      });
      return handle;
    }
    if (base === 'w') {
      if (mode.includes('x') && backingRead(path) != null) {
        setErrno(ERRNO.EEXIST);
        return 0;
      }
      const handle = nextHandle++;
      const st = {
        path, data: new Uint8Array(0), pos: 0,
        readable: plus, writable: true, append: false,
        eof: false, err: false, unget: [],
      };
      openFiles.set(handle, st);
      flushStream(st);
      return handle;
    }
    // 'a'
    const existing = backingRead(path);
    const handle = nextHandle++;
    openFiles.set(handle, {
      path, data: existing ? existing.slice() : new Uint8Array(0),
      pos: existing ? existing.length : 0,
      readable: plus, writable: true, append: true,
      eof: false, err: false, unget: [],
    });
    return handle;
  }

  function fclose(handle) {
    const st = openFiles.get(handle);
    if (!st) return handle >= 0 && handle <= 2 ? 0 : -1;
    // A tmpfile stream's backing entry is unlinked at close.
    if (st.temp) backingRemove(st.path);
    else flushStream(st);
    openFiles.delete(handle);
    return 0;
  }

  function fflush(handle) {
    // fflush(NULL) is stream value 0 in C — which is also this
    // layer's stdin handle. Flushing every open file covers both
    // readings and is a no-op for the standard streams.
    if (handle === 0) {
      for (const st of openFiles.values()) flushStream(st);
      return 0;
    }
    const st = openFiles.get(handle);
    if (st) flushStream(st);
    return 0;
  }

  function fseek(handle, offset, whence) {
    const st = openFiles.get(handle);
    if (!st) return -1;
    const base = whence === 0 ? 0 : whence === 1 ? st.pos : st.data.length;
    const np = base + offset;
    if (np < 0) {
      st.err = true;
      setErrno(ERRNO.EINVAL);
      return -1;
    }
    st.pos = np;
    st.eof = false;
    st.unget.length = 0;
    return 0;
  }

  function ftell(handle) {
    const st = openFiles.get(handle);
    return st ? st.pos : -1;
  }

  function rewind(handle) {
    const st = openFiles.get(handle);
    if (!st) return;
    st.pos = 0;
    st.eof = false;
    st.err = false;
    st.unget.length = 0;
  }

  function fgetpos(handle, posPtr) {
    const st = openFiles.get(handle);
    if (!st) return -1;
    dv().setBigInt64(posPtr, BigInt(st.pos), true);
    return 0;
  }

  function fsetpos(handle, posPtr) {
    return fseek(handle, Number(dv().getBigInt64(posPtr, true)), 0);
  }

  function feof(handle) {
    if (handle === 0) return stdinEof ? 1 : 0;
    const st = openFiles.get(handle);
    return st && st.eof ? 1 : 0;
  }

  function ferror(handle) {
    const st = openFiles.get(handle);
    return st && st.err ? 1 : 0;
  }

  function clearerr(handle) {
    if (handle === 0) {
      stdinEof = false;
      return;
    }
    const st = openFiles.get(handle);
    if (st) {
      st.eof = false;
      st.err = false;
    }
  }

  function freopen(pathPtr, modePtr, handle) {
    if (!pathPtr) return handle;
    const st = openFiles.get(handle);
    if (st) {
      flushStream(st);
      openFiles.delete(handle);
    }
    const fresh = fopen(pathPtr, modePtr);
    if (!fresh) return 0;
    const nst = openFiles.get(fresh);
    openFiles.delete(fresh);
    openFiles.set(handle, nst);
    return handle;
  }

  function removeFile(pathPtr) {
    if (backingRemove(guestPath(pathPtr))) return 0;
    setErrno(ERRNO.ENOENT);
    return -1;
  }

  function renameFile(oldPtr, newPtr) {
    const data = backingRead(guestPath(oldPtr));
    if (data == null) {
      setErrno(ERRNO.ENOENT);
      return -1;
    }
    backingWrite(guestPath(newPtr), data);
    backingRemove(guestPath(oldPtr));
    return 0;
  }

  function puts(ptr) {
    return streamWriteText(1, readCString(ptr) + '\n') >= 0 ? 0 : -1;
  }
  function putchar(c) {
    streamWrite(1, Uint8Array.of(c & 0xff));
    return c;
  }
  function fputs(ptr, stream) {
    const bytes = textEncoder.encode(readCString(ptr));
    return streamWrite(stream, bytes) === bytes.length ? 0 : -1;
  }
  function putc(c, stream) {
    return streamWrite(stream, Uint8Array.of(c & 0xff)) === 1 ? c : -1;
  }
  function fwrite(ptr, size, nmemb, stream) {
    const total = size * nmemb;
    if (total === 0) return 0;
    const written = streamWrite(stream, readBytes(ptr, total));
    return Math.floor(written / size);
  }
  function fread(ptr, size, nmemb, stream) {
    if (size === 0 || nmemb === 0) return 0;
    const data = streamRead(stream, size * nmemb);
    if (data.length) writeBytes(ptr, data);
    return Math.floor(data.length / size);
  }
  function fgetc(stream) {
    return streamGetByte(stream);
  }
  function ungetc(c, stream) {
    if (c < 0) return -1;
    streamUnget(stream, c);
    return c;
  }
  function fgets(s, size, stream) {
    if (size <= 0) return 0;
    if (size === 1) {
      u8()[s] = 0;
      return s;
    }
    const bytes = [];
    while (bytes.length < size - 1) {
      const c = streamGetByte(stream);
      if (c < 0) break;
      bytes.push(c);
      if (c === 0x0a) break;
    }
    if (bytes.length === 0) return 0;
    writeBytes(s, Uint8Array.from(bytes));
    u8()[s + bytes.length] = 0;
    return s;
  }

  // fdopen: in this layer a FILE handle value is the descriptor
  // — the standard streams are 0/1/2 and fopen hands out the
  // values above them — so associating a stream with a
  // descriptor is validating the value and handing it back. The
  // returned stream is the same stream: closing either closes
  // both. The mode's base letter must be a valid one; the
  // descriptor's own direction is what the stream already has.
  function fdopen(fd, modePtr) {
    const base = readCString(modePtr)[0];
    if (base !== 'r' && base !== 'w' && base !== 'a') {
      setErrno(ERRNO.EINVAL);
      return 0;
    }
    if (fd >= 0 && fd <= 2) return fd;
    if (!openFiles.has(fd)) {
      setErrno(ERRNO.EBADF);
      return 0;
    }
    return fd;
  }

  // tmpfile: a binary read/write stream over a backing entry
  // with a generated name no other open can collide with; the
  // entry is unlinked when the stream is closed (see fclose).
  let tmpfileCounter = 0;
  function tmpfile() {
    let path;
    do {
      tmpfileCounter++;
      const rand = Math.floor(Math.random() * 0x100000000).toString(36);
      path = `/tmpfile-${tmpfileCounter.toString(36)}-${rand}`;
    } while (backingRead(path) != null);
    const handle = nextHandle++;
    const st = {
      path, data: new Uint8Array(0), pos: 0,
      readable: true, writable: true, append: false,
      eof: false, err: false, unget: [], temp: true,
    };
    openFiles.set(handle, st);
    flushStream(st);
    return handle;
  }

  // Buffering control. Writes in this layer go through to the
  // backing store as they happen, so every stream is effectively
  // unbuffered whatever mode is requested: setvbuf records the
  // requested mode and buffer on the stream and reports success,
  // and output is visible to other readers immediately in all
  // modes. No mode makes output wait for a flush.
  function setvbuf(stream, buf, mode, size) {
    if (stream >= 0 && stream <= 2) return 0;
    const st = openFiles.get(stream);
    if (!st) {
      setErrno(ERRNO.EBADF);
      return -1;
    }
    st.bufMode = mode;
    st.bufPtr = buf;
    st.bufSize = size;
    return 0;
  }
  function setbuf(stream, buf) {
    // _IOFBF is 0 and _IONBF is 2, the values stdio.h publishes;
    // a null buffer means no buffering, a non-null one full
    // buffering with a BUFSIZ (1024) caller buffer.
    return setvbuf(stream, buf, buf ? 0 : 2, buf ? 1024 : 0);
  }

  // perror: report the layer's recorded error number for the
  // most recent failed call on the error stream, in the standard
  // form "<s>: <message>" (or just the message for a null or
  // empty prefix).
  function perror(sPtr) {
    const prefix = sPtr ? readCString(sPtr) : '';
    const text = (prefix ? `${prefix}: ` : '') + strerrorText(errnoValue) + '\n';
    streamWriteText(2, text);
  }

  // ---------------------------------------------------------------
  // scanf family: the mirror of the printf formatter. The input
  // source is stdin, an open file, or a guest string (sscanf);
  // every conversion consumes one pointer argument from the
  // va_list area and stores through it. Supported conversions:
  // d i u x X o c s [ f e E g G a p n and %%, with field width,
  // assignment suppression (*) and the hh/h/l/ll/z/t/j/L length
  // modifiers (long is 32-bit and long double is double in this
  // target's ABI, as in the printf formatter).
  // ---------------------------------------------------------------
  function scanFrom(getByte, putBack, fmtPtr, vaPtr) {
    const fmt = readCString(fmtPtr);
    const va = makeVa(vaPtr);
    let assigned = 0;
    let consumed = 0;
    let inputEof = false;
    const held = [];
    const rd = () => {
      if (held.length) {
        consumed++;
        return held.pop();
      }
      const c = getByte();
      if (c < 0) inputEof = true;
      else consumed++;
      return c;
    };
    const un = (c) => {
      if (c >= 0) {
        held.push(c);
        consumed--;
      }
    };
    const pk = () => {
      const c = rd();
      if (c >= 0) un(c);
      return c;
    };
    const isWs = (c) => c === 0x20 || (c >= 0x09 && c <= 0x0d);
    // Returns false when input ends while skipping.
    const skipWs = () => {
      for (;;) {
        const c = rd();
        if (c < 0) return false;
        if (!isWs(c)) {
          un(c);
          return true;
        }
      }
    };
    const DIGITS = '0123456789abcdefghijklmnopqrstuvwxyz';
    function scanIntToken(conv, width) {
      // -> { value: BigInt } | { fail: 'input' | 'match' }
      let text = '';
      const room = () => width < 0 || text.length < width;
      if (room()) {
        const c = pk();
        if (c === 0x2b || c === 0x2d) text += String.fromCharCode(rd());
      }
      let radix = conv === 'x' || conv === 'X' ? 16
        : conv === 'o' ? 8 : conv === 'i' ? 0 : 10;
      if ((conv === 'x' || conv === 'X' || conv === 'i') && room()) {
        const a = pk();
        if (a === 0x30) {
          rd();
          text += '0';
          const b = room() ? pk() : -1;
          if (b === 0x78 || b === 0x58) {
            rd();
            const d = pk();
            if (d >= 0 && DIGITS.indexOf(String.fromCharCode(d).toLowerCase()) < 16 &&
                DIGITS.indexOf(String.fromCharCode(d).toLowerCase()) >= 0) {
              text += 'x';
              radix = 16;
            } else {
              un(b);
            }
          }
        }
      }
      if (radix === 0) {
        radix = text.includes('x') ? 16
          : text.replace(/^[+-]/, '').startsWith('0') ? 8 : 10;
      }
      let digits = 0;
      while (room()) {
        const c = pk();
        if (c < 0) break;
        const d = DIGITS.indexOf(String.fromCharCode(c).toLowerCase());
        if (d < 0 || d >= radix) break;
        // A leading '0' already taken as a prefix is itself a
        // digit for octal and decimal readings.
        text += String.fromCharCode(rd());
        digits++;
      }
      const digitText = text.replace(/^[+-]/, '').replace(/^0[xX]/, '');
      if (digitText.length === 0) {
        // Push back what was speculatively taken (sign, prefix),
        // in reverse read order.
        for (let i = text.length - 1; i >= 0; i--) un(text.charCodeAt(i));
        return { fail: inputEof && text.length === 0 ? 'input' : 'match' };
      }
      let v = 0n;
      const br = BigInt(radix);
      for (const ch of digitText) {
        v = v * br + BigInt(DIGITS.indexOf(ch.toLowerCase()));
      }
      if (text.startsWith('-')) v = -v;
      return { value: v };
    }
    function scanFloatToken(width) {
      let text = '';
      const room = () => width < 0 || text.length < width;
      if (room()) {
        const c = pk();
        if (c === 0x2b || c === 0x2d) text += String.fromCharCode(rd());
      }
      const word = (w) => {
        // Try to extend the token by the inf/nan word w; on a
        // mismatch this attempt's reads are rolled back.
        const base = text;
        let t = text;
        for (const ch of w) {
          if (!room()) return false;
          const c = pk();
          if (c < 0 || String.fromCharCode(c).toLowerCase() !== ch) {
            for (let i = t.length - 1; i >= base.length; i--) un(t.charCodeAt(i));
            return false;
          }
          t += String.fromCharCode(rd());
        }
        text = t;
        return true;
      };
      if (word('inf')) {
        if (!word('inity')) { /* plain "inf" stands */ }
        return { value: text.startsWith('-') ? -Infinity : Infinity };
      }
      if (word('nan')) return { value: NaN };
      let sawDigit = false;
      let sawDot = false;
      while (room()) {
        const c = pk();
        if (c < 0) break;
        if (c >= 0x30 && c <= 0x39) {
          text += String.fromCharCode(rd());
          sawDigit = true;
        } else if (c === 0x2e && !sawDot) {
          text += String.fromCharCode(rd());
          sawDot = true;
        } else break;
      }
      if (!sawDigit) {
        for (let i = text.length - 1; i >= 0; i--) un(text.charCodeAt(i));
        return { fail: inputEof && text.length === 0 ? 'input' : 'match' };
      }
      if (room()) {
        const c = pk();
        if (c === 0x65 || c === 0x45) {
          rd();
          let exp = 'e';
          if (room()) {
            const s = pk();
            if (s === 0x2b || s === 0x2d) exp += String.fromCharCode(rd());
          }
          let expDigits = '';
          while (room()) {
            const d = pk();
            if (d < 0x30 || d > 0x39) break;
            expDigits += String.fromCharCode(rd());
          }
          if (expDigits) text += exp + expDigits;
          else {
            // No exponent digits: the 'e' and sign are not ours.
            const tail = exp.slice(1);
            for (let i = tail.length - 1; i >= 0; i--) un(tail.charCodeAt(i));
            un(c);
          }
        }
      }
      return { value: parseFloat(text) };
    }
    function storeInt(ptr, value, length) {
      if (length === 'll' || length === 'j') {
        dv().setBigInt64(ptr, value, true);
      } else if (length === 'hh') {
        u8()[ptr] = Number(value & 0xffn);
      } else if (length === 'h') {
        dv().setInt16(ptr, Number(BigInt.asIntN(16, value)), true);
      } else {
        dv().setInt32(ptr, Number(BigInt.asIntN(32, value)), true);
      }
    }

    let result = null; // null: still scanning
    let i = 0;
    while (i < fmt.length && result === null) {
      const fc = fmt[i];
      if (isWs(fc.charCodeAt(0))) {
        while (i < fmt.length && isWs(fmt.charCodeAt(i))) i++;
        if (!skipWs()) {
          result = assigned > 0 ? assigned : -1;
        }
        continue;
      }
      if (fc !== '%') {
        const c = rd();
        if (c < 0) result = assigned > 0 ? assigned : -1;
        else if (c !== fc.charCodeAt(0)) {
          un(c);
          result = assigned;
        }
        i++;
        continue;
      }
      i++;
      if (fmt[i] === '%') {
        const c = rd();
        if (c < 0) result = assigned > 0 ? assigned : -1;
        else if (c !== 0x25) {
          un(c);
          result = assigned;
        }
        i++;
        continue;
      }
      let suppress = false;
      if (fmt[i] === '*') {
        suppress = true;
        i++;
      }
      let width = -1;
      if (i < fmt.length && fmt[i] >= '0' && fmt[i] <= '9') {
        width = 0;
        while (i < fmt.length && fmt[i] >= '0' && fmt[i] <= '9') {
          width = width * 10 + (fmt.charCodeAt(i++) - 48);
        }
      }
      let length = '';
      if (fmt.startsWith('hh', i)) { length = 'hh'; i += 2; }
      else if (fmt.startsWith('ll', i)) { length = 'll'; i += 2; }
      else if (i < fmt.length && 'hlzjtL'.includes(fmt[i])) length = fmt[i++];
      const conv = fmt[i++] ?? '';
      const argPtr = suppress || conv === '%' ? 0 : va.i32();
      const fail = (kind) => {
        result = kind === 'input' && assigned === 0 ? -1 : assigned;
      };
      switch (conv) {
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o': {
          if (!skipWs()) { fail('input'); break; }
          const r = scanIntToken(conv === 'd' || conv === 'u' ? 'd' : conv, width);
          if (r.fail) { fail(r.fail); break; }
          if (!suppress) {
            storeInt(argPtr, r.value, length);
            assigned++;
          }
          break;
        }
        case 'f':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'a':
        case 'A': {
          if (!skipWs()) { fail('input'); break; }
          const r = scanFloatToken(width);
          if (r.fail) { fail(r.fail); break; }
          if (!suppress) {
            if (length === 'l' || length === 'L') {
              dv().setFloat64(argPtr, r.value, true);
            } else {
              dv().setFloat32(argPtr, Math.fround(r.value), true);
            }
            assigned++;
          }
          break;
        }
        case 'c': {
          const want = width < 0 ? 1 : width;
          const bytes = [];
          while (bytes.length < want) {
            const c = rd();
            if (c < 0) break;
            bytes.push(c);
          }
          if (bytes.length === 0) { fail('input'); break; }
          if (!suppress) {
            writeBytes(argPtr, Uint8Array.from(bytes));
            assigned++;
          }
          break;
        }
        case 's': {
          if (!skipWs()) { fail('input'); break; }
          const bytes = [];
          while (width < 0 || bytes.length < width) {
            const c = pk();
            if (c < 0 || isWs(c)) break;
            bytes.push(rd());
          }
          if (bytes.length === 0) { fail(inputEof ? 'input' : 'match'); break; }
          if (!suppress) {
            writeBytes(argPtr, Uint8Array.from(bytes));
            u8()[argPtr + bytes.length] = 0;
            assigned++;
          }
          break;
        }
        case '[': {
          let negate = false;
          if (fmt[i] === '^') {
            negate = true;
            i++;
          }
          const set = new Set();
          let first = true;
          while (i < fmt.length && (fmt[i] !== ']' || first)) {
            first = false;
            if (fmt[i + 1] === '-' && fmt[i + 2] !== undefined && fmt[i + 2] !== ']') {
              const lo = fmt.charCodeAt(i);
              const hi = fmt.charCodeAt(i + 2);
              for (let c = lo; c <= hi; c++) set.add(c);
              i += 3;
            } else {
              set.add(fmt.charCodeAt(i));
              i++;
            }
          }
          i++; // the ']'
          const bytes = [];
          while (width < 0 || bytes.length < width) {
            const c = pk();
            if (c < 0) break;
            if (set.has(c) === negate) break;
            bytes.push(rd());
          }
          if (bytes.length === 0) { fail(inputEof ? 'input' : 'match'); break; }
          if (!suppress) {
            writeBytes(argPtr, Uint8Array.from(bytes));
            u8()[argPtr + bytes.length] = 0;
            assigned++;
          }
          break;
        }
        case 'p': {
          if (!skipWs()) { fail('input'); break; }
          const r = scanIntToken('x', width);
          if (r.fail) { fail(r.fail); break; }
          if (!suppress) {
            dv().setInt32(argPtr, Number(BigInt.asIntN(32, r.value)), true);
            assigned++;
          }
          break;
        }
        case 'n': {
          if (!suppress) storeInt(argPtr, BigInt(consumed), length);
          break;
        }
        default:
          result = assigned;
          break;
      }
    }
    // Hand back any lookahead char this call did not consume.
    for (let k = 0; k < held.length; k++) putBack(held[k]);
    return result === null ? assigned : result;
  }

  function scanf(fmt, va) {
    return scanFrom(() => streamGetByte(0), (c) => streamUnget(0, c), fmt, va);
  }
  function fscanf(stream, fmt, va) {
    return scanFrom(
      () => streamGetByte(stream), (c) => streamUnget(stream, c), fmt, va);
  }
  function sscanf(str, fmt, va) {
    let idx = 0;
    const get = () => {
      const c = u8()[str + idx];
      if (c === 0) return -1;
      idx++;
      return c;
    };
    return scanFrom(get, () => {}, fmt, va);
  }

  // ---------------------------------------------------------------
  // math: the full <math.h> surface. The header declares the
  // C99 functions in double, float and long double forms (171
  // functions). In this target's ABI long double is double —
  // sizeof(long double) is 8 and the compiler emits f64
  // signatures for the l forms — so the l forms share the double
  // implementations under their own names. The f forms run the
  // double implementation and round the result with
  // Math.fround, the float rounding an f32 return carries.
  // Where JS Math has the function it is the base; the rest are
  // implemented here, each with its precision stated where it is
  // defined.
  // ---------------------------------------------------------------
  const f64view = new DataView(new ArrayBuffer(8));
  const f64Bits = (x) => {
    f64view.setFloat64(0, x, true);
    return f64view.getBigUint64(0, true);
  };
  const bitsToF64 = (b) => {
    f64view.setBigUint64(0, b, true);
    return f64view.getFloat64(0, true);
  };
  const f32Bits = (x) => {
    f64view.setFloat32(0, x, true);
    return f64view.getUint32(0, true);
  };
  const bitsToF32 = (b) => {
    f64view.setUint32(0, b >>> 0, true);
    return f64view.getFloat32(0, true);
  };
  const F64_SIGN = 1n << 63n;
  const F64_FRAC = (1n << 52n) - 1n;

  // Split a finite double: value = (neg ? -1 : 1) * sig * 2^exp
  // with sig a 53-bit integer (subnormals are normalized up).
  function decomposeF64(x) {
    const bits = f64Bits(x);
    const neg = (bits & F64_SIGN) !== 0n;
    const be = Number((bits >> 52n) & 0x7ffn);
    const frac = bits & F64_FRAC;
    if (be !== 0) return { neg, sig: frac | (1n << 52n), exp: be - 1075 };
    if (frac === 0n) return { neg, sig: 0n, exp: 0 };
    let sig = frac;
    let shift = 0;
    while ((sig & (1n << 52n)) === 0n) {
      sig <<= 1n;
      shift++;
    }
    return { neg, sig, exp: -1074 - shift };
  }

  // The double nearest to (neg ? -1 : 1) * sig * 2^exp, rounding
  // to nearest, ties to even, with gradual underflow and overflow
  // to infinity. Exact whenever the value is representable, so
  // scalbn/ldexp, frexp-based results and the integer-exact
  // remainder and fma built on it carry no rounding error of
  // their own.
  function composeF64(neg, sig, exp) {
    if (sig === 0n) return neg ? -0 : 0;
    let bits = sig.toString(2).length;
    if (bits > 53) {
      const drop = BigInt(bits - 53);
      const dropped = sig & ((1n << drop) - 1n);
      const half = 1n << (drop - 1n);
      sig >>= drop;
      exp += bits - 53;
      if (dropped > half || (dropped === half && (sig & 1n) === 1n)) {
        sig += 1n;
        if (sig === (1n << 53n)) {
          sig >>= 1n;
          exp += 1;
        }
      }
      bits = 53;
    }
    const topExp = exp + bits - 1;
    if (topExp > 1023) return neg ? -Infinity : Infinity;
    const sign = neg ? F64_SIGN : 0n;
    if (topExp >= -1022) {
      const s = sig << BigInt(53 - bits);
      return bitsToF64(sign | (BigInt(topExp + 1023) << 52n) | (s & F64_FRAC));
    }
    // Subnormal grid: units of 2^-1074. A rounded result of
    // exactly 2^52 units is the smallest normal, whose bit
    // pattern is the integer 2^52 in the low field, so the same
    // expression covers it.
    const sh = exp + 1074;
    let units;
    if (sh >= 0) {
      units = sig << BigInt(sh);
    } else if (-sh > bits) {
      // The value is below half the smallest subnormal unit
      // (the exactly-half case is a tie to the even unit 0):
      // it rounds to zero. This also keeps an extreme exponent
      // (scalbn with a huge count) from building a huge BigInt.
      units = 0n;
    } else {
      const drop = BigInt(-sh);
      const dropped = sig & ((1n << drop) - 1n);
      const half = 1n << (drop - 1n);
      units = sig >> drop;
      if (dropped > half || (dropped === half && (units & 1n) === 1n)) units += 1n;
    }
    return bitsToF64(sign | units);
  }

  // The float counterpart of composeF64 (24-bit significand).
  function composeF32(neg, sig, exp) {
    if (sig === 0n) return neg ? -0 : 0;
    let bits = sig.toString(2).length;
    if (bits > 24) {
      const drop = BigInt(bits - 24);
      const dropped = sig & ((1n << drop) - 1n);
      const half = 1n << (drop - 1n);
      sig >>= drop;
      exp += bits - 24;
      if (dropped > half || (dropped === half && (sig & 1n) === 1n)) {
        sig += 1n;
        if (sig === (1n << 24n)) {
          sig >>= 1n;
          exp += 1;
        }
      }
      bits = 24;
    }
    const topExp = exp + bits - 1;
    if (topExp > 127) return neg ? -Infinity : Infinity;
    const sign = neg ? 0x80000000 : 0;
    if (topExp >= -126) {
      const s = Number(sig << BigInt(24 - bits));
      return bitsToF32((sign | ((topExp + 127) << 23) | (s & 0x7fffff)) >>> 0);
    }
    const sh = exp + 149;
    let units;
    if (sh >= 0) {
      units = sig << BigInt(sh);
    } else if (-sh > bits) {
      units = 0n;
    } else {
      const drop = BigInt(-sh);
      const dropped = sig & ((1n << drop) - 1n);
      const half = 1n << (drop - 1n);
      units = sig >> drop;
      if (dropped > half || (dropped === half && (units & 1n) === 1n)) units += 1n;
    }
    return bitsToF32((sign | Number(units)) >>> 0);
  }

  // frexp: mantissa in [0.5, 1) and the exponent, both exact.
  function frexpCore(x) {
    if (x === 0 || !Number.isFinite(x)) return [x, 0];
    const d = decomposeF64(x);
    const m = (d.neg ? -1 : 1) * Number(d.sig) * 2 ** -53;
    return [m, d.exp + 53];
  }
  function frexp(x, eptr) {
    const [m, e] = frexpCore(x);
    dv().setInt32(eptr, e, true);
    return m;
  }
  function frexpf(x, eptr) {
    const [m, e] = frexpCore(x);
    dv().setInt32(eptr, e, true);
    return Math.fround(m);
  }

  // ilogb/logb: FP_ILOGB0 and FP_ILOGBNAN are the values the
  // header publishes (INT_MIN and INT_MAX).
  function ilogbImpl(x) {
    if (Number.isNaN(x)) return 2147483647;
    if (x === 0) return -2147483647 - 1;
    if (!Number.isFinite(x)) return 2147483647;
    return decomposeF64(x).exp + 52;
  }
  function logbImpl(x) {
    if (Number.isNaN(x)) return NaN;
    if (x === 0) return -Infinity;
    if (!Number.isFinite(x)) return Infinity;
    return ilogbImpl(x);
  }

  // scalbn/ldexp (and scalbln, whose long argument is 32-bit in
  // this ABI): exact, via decompose/compose.
  function scalbnImpl(x, n) {
    if (x === 0 || !Number.isFinite(x) || n === 0) return x;
    const d = decomposeF64(x);
    return composeF64(d.neg, d.sig, d.exp + n);
  }

  // exp2: exact on the bit grid for integer exponents in range
  // (the common case and the one a rounded pow could in
  // principle miss); other arguments go through Math.pow.
  function exp2Impl(x) {
    if (Number.isInteger(x) && x >= -1074 && x <= 1023) {
      if (x >= -1022) return bitsToF64(BigInt(x + 1023) << 52n);
      return bitsToF64(1n << BigInt(x + 1074));
    }
    return Math.pow(2, x);
  }

  // pow: the C special cases JS Math.pow does not reproduce —
  // pow(x, +-0) is 1 for every x including NaN, pow(1, y) is 1
  // for every y including NaN, pow(-1, +-inf) is 1.
  function powImpl(x, y) {
    if (y === 0) return 1;
    if (x === 1) return 1;
    if (x === -1 && !Number.isFinite(y) && !Number.isNaN(y)) return 1;
    return Math.pow(x, y);
  }

  const TWO_POW_52 = 4503599627370496;
  // Round to an integer value, ties to even — the only rounding
  // direction this layer has. Adding 2^52 puts the value on the
  // integer grid under the hardware's round-to-nearest-even;
  // magnitudes at or above 2^52 are already integral.
  function rintImpl(x) {
    if (!Number.isFinite(x) || x === 0) return x;
    const ax = Math.abs(x);
    if (ax >= TWO_POW_52) return x;
    const r = (ax + TWO_POW_52) - TWO_POW_52;
    return x < 0 ? -r : r;
  }
  // C round: halfway cases away from zero. Math.round alone
  // rounds halfway cases toward +infinity, which is wrong for
  // negative arguments, so the sign is applied outside.
  function roundImpl(x) {
    if (!Number.isFinite(x) || x === 0) return x;
    const r = Math.round(Math.abs(x));
    return x < 0 ? -r : r;
  }
  // lrint/lround outside long's range are unspecified in C;
  // this layer saturates the way the wasm saturating
  // float-to-int conversions do, with NaN converting to 0.
  function toLongSat(x) {
    if (Number.isNaN(x)) return 0;
    if (x >= 2147483648) return 2147483647;
    if (x < -2147483648) return -2147483647 - 1;
    return x | 0;
  }
  function toLLongSat(x) {
    if (Number.isNaN(x)) return 0n;
    if (x >= 9223372036854775808) return 9223372036854775807n;
    if (x < -9223372036854775808) return -9223372036854775808n;
    return BigInt(x);
  }

  // modf: the split is exact — the fractional part's bits are a
  // subset of the argument's.
  function modfCore(x) {
    if (Number.isNaN(x)) return [NaN, NaN];
    if (!Number.isFinite(x)) return [x < 0 ? -0 : 0, x];
    // Both parts of a zero argument are the signed zero itself;
    // x - trunc(x) would turn -0 into +0.
    if (x === 0) return [x, x];
    const ip = Math.trunc(x);
    return [x - ip, ip];
  }
  function modf(x, iptr) {
    const [frac, ip] = modfCore(x);
    dv().setFloat64(iptr, ip, true);
    return frac;
  }
  function modff(x, iptr) {
    const [frac, ip] = modfCore(x);
    dv().setFloat32(iptr, Math.fround(ip), true);
    return Math.fround(frac);
  }

  // IEEE 754 remainder, computed exactly: both operands are
  // decomposed to integer significands over a common exponent
  // and the quotient is rounded to nearest, ties to even, in
  // BigInt arithmetic. The mathematical remainder of two doubles
  // is always representable, so the result is exact. remquo
  // additionally stores the low 32 bits of the signed quotient,
  // the same truncation the platform libraries store.
  function remainderCore(x, y) {
    const dx = decomposeF64(x);
    const dy = decomposeF64(y);
    const e = Math.min(dx.exp, dy.exp);
    const X = dx.sig << BigInt(dx.exp - e);
    const Y = dy.sig << BigInt(dy.exp - e);
    let q = X / Y;
    const rem = X % Y;
    const twice = rem * 2n;
    if (twice > Y || (twice === Y && (q & 1n) === 1n)) q += 1n;
    const rInt = X - q * Y;
    // r = x - q*y with signed operands: on magnitudes that is
    // (-1)^dx.neg * rInt, so the result sign is dx.neg XOR the
    // sign of rInt (which is negative when the quotient rounded
    // away from zero past the half point).
    const r = composeF64(dx.neg !== (rInt < 0n), rInt < 0n ? -rInt : rInt, e);
    return [r, dx.neg !== dy.neg ? -q : q];
  }
  function remainderImpl(x, y) {
    if (Number.isNaN(x) || Number.isNaN(y)) return NaN;
    if (!Number.isFinite(x) || y === 0) return NaN;
    if (!Number.isFinite(y) || x === 0) return x;
    return remainderCore(x, y)[0];
  }
  function remquoImpl(x, y, qptr) {
    if (Number.isNaN(x) || Number.isNaN(y) || !Number.isFinite(x) || y === 0) return NaN;
    if (!Number.isFinite(y) || x === 0) {
      dv().setInt32(qptr, 0, true);
      return x;
    }
    const [r, n] = remainderCore(x, y);
    dv().setInt32(qptr, Number(BigInt.asIntN(32, n)), true);
    return r;
  }
  function remquofImpl(x, y, qptr) {
    return Math.fround(remquoImpl(x, y, qptr));
  }

  // nextafter: one step on the sign-magnitude bit grid.
  function nextafterImpl(x, y) {
    if (Number.isNaN(x) || Number.isNaN(y)) return NaN;
    if (x === y) return y;
    if (x === 0) return bitsToF64((f64Bits(y) & F64_SIGN) | 1n);
    let b = f64Bits(x);
    const up = y > x;
    if ((b & F64_SIGN) === 0n) b += up ? 1n : -1n;
    else b += up ? -1n : 1n;
    return bitsToF64(b);
  }
  function nextafterfImpl(x, y) {
    if (Number.isNaN(x) || Number.isNaN(y)) return NaN;
    if (x === y) return Math.fround(y);
    if (x === 0) return bitsToF32(((f32Bits(y) & 0x80000000) | 1) >>> 0);
    let b = f32Bits(x);
    const up = y > x;
    if ((b & 0x80000000) === 0) b += up ? 1 : -1;
    else b += up ? -1 : 1;
    return bitsToF32(b >>> 0);
  }

  function copysignImpl(x, y) {
    return bitsToF64((f64Bits(x) & ~F64_SIGN) | (f64Bits(y) & F64_SIGN));
  }
  function copysignfImpl(x, y) {
    return bitsToF32(((f32Bits(x) & 0x7fffffff) | (f32Bits(y) & 0x80000000)) >>> 0);
  }

  // fmax/fmin: a NaN operand yields the other operand (the C
  // semantics; Math.max/min would propagate the NaN). Signed
  // zeros keep Math.max/min's ordering, +0 above -0.
  function fmaxImpl(a, b) {
    if (Number.isNaN(a)) return b;
    if (Number.isNaN(b)) return a;
    return Math.max(a, b);
  }
  function fminImpl(a, b) {
    if (Number.isNaN(a)) return b;
    if (Number.isNaN(b)) return a;
    return Math.min(a, b);
  }
  function fdimImpl(a, b) {
    if (Number.isNaN(a) || Number.isNaN(b)) return NaN;
    return a > b ? a - b : 0;
  }

  // fma, correctly rounded: the exact product and the exact sum
  // are formed in BigInt arithmetic over the decomposed operands
  // and rounded once by composeF64/composeF32. The zero-product
  // and infinite cases follow the C rules directly.
  function fmaExact(x, y, z, decompose, compose) {
    const dx = decompose(x);
    const dy = decompose(y);
    const dz = decompose(z);
    const pSig = dx.sig * dy.sig;
    const pExp = dx.exp + dy.exp;
    const e = Math.min(pExp, dz.exp);
    let total = pSig << BigInt(pExp - e);
    if (dx.neg !== dy.neg) total = -total;
    let zTerm = dz.sig << BigInt(dz.exp - e);
    if (dz.neg) zTerm = -zTerm;
    total += zTerm;
    const neg = total < 0n;
    return compose(neg, neg ? -total : total, e);
  }
  function fmaImpl(x, y, z) {
    if (Number.isNaN(x) || Number.isNaN(y) || Number.isNaN(z)) return NaN;
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      const p = x * y;
      return Number.isNaN(p) ? NaN : p + z;
    }
    if (!Number.isFinite(z)) return z;
    if (x === 0 || y === 0) return x * y + z;
    if (z === 0) return x * y;
    return fmaExact(x, y, z, decomposeF64, composeF64);
  }
  // Split a float (held exactly in a double) on the float grid.
  function decomposeF32(x) {
    const bits = f32Bits(x);
    const neg = (bits & 0x80000000) !== 0;
    const be = (bits >> 23) & 0xff;
    const frac = BigInt(bits & 0x7fffff);
    if (be !== 0) return { neg, sig: frac | (1n << 23n), exp: be - 150 };
    if (frac === 0n) return { neg, sig: 0n, exp: 0 };
    let sig = frac;
    let shift = 0;
    while ((sig & (1n << 23n)) === 0n) {
      sig <<= 1n;
      shift++;
    }
    return { neg, sig, exp: -149 - shift };
  }
  function fmafImpl(x, y, z) {
    if (Number.isNaN(x) || Number.isNaN(y) || Number.isNaN(z)) return NaN;
    if (!Number.isFinite(x) || !Number.isFinite(y)) {
      const p = x * y;
      return Number.isNaN(p) ? NaN : Math.fround(p + z);
    }
    if (!Number.isFinite(z)) return Math.fround(z);
    if (x === 0 || y === 0) return Math.fround(x * y + z);
    if (z === 0) return Math.fround(x * y);
    return fmaExact(x, y, z, decomposeF32, composeF32);
  }

  // erf and erfc. For |x| <= 2.5 the error function is computed
  // from the confluent hypergeometric series
  //   erf(x) = (2x / sqrt(pi)) * e^(-x^2) * 1F1(1; 3/2; x^2),
  // whose terms are all of one sign, so no cancellation occurs.
  // For larger arguments erfc comes from the continued fraction
  // 7.1.14 of Abramowitz & Stegun,
  //   2 e^(x^2) int_x^inf e^(-t^2) dt
  //     = 1 / (x + (1/2) / (x + 1 / (x + (3/2) / (x + ...)))),
  // evaluated by Lentz's method, and erf is 1 - erfc. Measured
  // against the reference values in the math battery: erf
  // agrees to within 3 ulp; erfc to within a few ulp on the
  // series side of its crossover and about 13 ulp in the
  // fraction's range.
  const SQRT_PI = 1.772453850905516;
  function erfSeries(ax) {
    const z = ax * ax;
    let term = 1;
    let sum = 1;
    for (let k = 0; k < 300; k++) {
      term *= (2 * z) / (2 * k + 3);
      sum += term;
      if (term < sum * 1e-18) break;
    }
    return ((2 * ax) / SQRT_PI) * Math.exp(-z) * sum;
  }
  function erfcFraction(x) {
    // Lentz's method on b0 = 0, a1 = 1, b1 = x, then
    // a_n = (n - 1) / 2 and b_n = x for n >= 2.
    const TINY = 1e-300;
    let f = TINY;
    let c = TINY;
    let d = 0;
    for (let n = 1; n <= 1000; n++) {
      const a = n === 1 ? 1 : (n - 1) / 2;
      d = x + a * d;
      if (d === 0) d = TINY;
      d = 1 / d;
      c = x + a / c;
      if (c === 0) c = TINY;
      const delta = c * d;
      f *= delta;
      if (Math.abs(delta - 1) < 1e-17) break;
    }
    return (Math.exp(-x * x) / SQRT_PI) * f;
  }
  function erfImpl(x) {
    if (Number.isNaN(x)) return NaN;
    if (!Number.isFinite(x)) return x > 0 ? 1 : -1;
    const ax = Math.abs(x);
    // erf(6) already rounds to exactly 1 in double precision;
    // the cutoff also keeps the fraction's Lentz recurrence
    // away from arguments large enough to overflow it.
    if (ax >= 6) return x < 0 ? -1 : 1;
    const r = ax <= 2.5 ? erfSeries(ax) : 1 - erfcFraction(ax);
    return x < 0 ? -r : r;
  }
  function erfcImpl(x) {
    if (Number.isNaN(x)) return NaN;
    if (!Number.isFinite(x)) return x > 0 ? 0 : 2;
    if (x < 0) return 2 - erfcImpl(-x);
    // Past 27.5 the true value is below half the smallest
    // subnormal and rounds to zero.
    if (x >= 27.5) return 0;
    // The complement of the erf series loses precision to
    // cancellation as x grows (about 5000 ulp at 2.5), so the
    // continued fraction takes over at 0.75; it stays within
    // about 13 ulp of the exact value over its whole range,
    // while the series path below 0.75 is within a few ulp.
    return x < 0.75 ? 1 - erfSeries(x) : erfcFraction(x);
  }

  // The gamma functions, after Lanczos (g = 7, the nine
  // coefficients tabulated for that parameter in Numerical
  // Recipes and in the Lanczos approximation literature):
  // lgamma's absolute error is a few 1e-15 over the battery
  // grid. Negative arguments go through the reflection
  // formula, and tgamma is exp of the log form, so its
  // relative error grows with the log value's magnitude
  // (about 3e-13 at the top of the range).
  const LANCZOS = [
    0.99999999999980993, 676.5203681218851, -1259.1392167224028,
    771.32342877765313, -176.61502916214059, 12.507343278686905,
    -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7,
  ];
  const HALF_LOG_2PI = 0.9189385332046727;
  function logGammaAbs(x) {
    // log |Gamma(x)| for x not a non-positive integer.
    if (x !== 0 && Math.abs(x) < 1e-8) {
      // Near the pole at 0, Gamma(x) = 1/x - gamma + O(x).
      // The reflection path below cannot be used down here:
      // sin(pi*x) for a subnormal-scale x keeps only a few
      // significand bits, which the logarithm turns into a
      // gross absolute error.
      return -Math.log(Math.abs(x)) - 0.5772156649015329 * x;
    }
    if (x < 0.5) {
      return Math.log(Math.PI) -
        Math.log(Math.abs(Math.sin(Math.PI * x))) - logGammaAbs(1 - x);
    }
    const z = x - 1;
    let a = LANCZOS[0];
    for (let k = 1; k < LANCZOS.length; k++) a += LANCZOS[k] / (z + k);
    const t = z + 7.5;
    return HALF_LOG_2PI + (z + 0.5) * Math.log(t) - t + Math.log(a);
  }
  function lgammaImpl(x) {
    if (Number.isNaN(x)) return NaN;
    if (x === 0 || (Number.isInteger(x) && x < 0)) return Infinity;
    if (!Number.isFinite(x)) return Infinity;
    return logGammaAbs(x);
  }
  function tgammaImpl(x) {
    if (Number.isNaN(x)) return NaN;
    if (x === 0) return 1 / x;
    if (Number.isInteger(x) && x < 0) return NaN;
    if (!Number.isFinite(x)) return x > 0 ? Infinity : NaN;
    if (x < 0) {
      return Math.PI / (Math.sin(Math.PI * x) * Math.exp(logGammaAbs(1 - x)));
    }
    return Math.exp(logGammaAbs(x));
  }

  // nan(tagp): the payload is parsed the way strtoull with base
  // 0 parses it — 0x hexadecimal, leading-0 octal, else decimal;
  // an unparseable tag gives a plain quiet NaN.
  function nanPayload(tagPtr) {
    if (!tagPtr) return 0n;
    const t = readCString(tagPtr).replace(/^\s+/, '');
    try {
      if (/^0[xX][0-9a-fA-F]+$/.test(t)) return BigInt(t);
      if (/^0[0-7]+$/.test(t)) return BigInt(`0o${t.slice(1)}`);
      if (/^[0-9]+$/.test(t)) return BigInt(t);
    } catch {
      return 0n;
    }
    return 0n;
  }
  function nanImpl(tagPtr) {
    return bitsToF64(0x7ff8000000000000n | (nanPayload(tagPtr) & ((1n << 51n) - 1n)));
  }
  function nanfImpl(tagPtr) {
    return bitsToF32(0x7fc00000 | Number(nanPayload(tagPtr) & 0x3fffffn));
  }

  // The float forms of the value-returning functions: the double
  // implementation with the result rounded to float precision.
  const asF32 = (fn) => (...args) => Math.fround(fn(...args));

  const env = {
    // memory / string
    memcpy,
    memmove,
    memset,
    memcmp,
    strlen,
    strcpy,
    strncpy,
    strcat,
    strncat,
    strcmp,
    strncmp,
    strchr,
    strrchr,
    strstr,
    strdup,
    memchr,
    strspn,
    strcspn,
    strpbrk,
    strtok,
    strerror,
    strcoll,
    strxfrm,
    // allocation
    malloc,
    calloc,
    realloc,
    free,
    // stdio
    printf,
    fprintf,
    vprintf: printf,
    vfprintf: fprintf,
    sprintf,
    vsprintf: sprintf,
    snprintf,
    vsnprintf: snprintf,
    puts,
    putchar,
    fputs,
    putc,
    fputc: putc,
    fwrite,
    fread,
    fgetc,
    getchar: () => fgetc(0),
    ungetc,
    fopen,
    fclose,
    fflush,
    fseek,
    ftell,
    rewind,
    fgetpos,
    fsetpos,
    feof,
    ferror,
    clearerr,
    freopen,
    remove: removeFile,
    rename: renameFile,
    fdopen,
    tmpfile,
    setbuf,
    setvbuf,
    perror,
    fgets,
    scanf,
    fscanf,
    sscanf,
    vscanf: scanf,
    vfscanf: fscanf,
    vsscanf: sscanf,
    // stdlib
    exit,
    abort,
    atexit,
    atoi,
    atol,
    atoll,
    atof,
    strtol,
    strtoul,
    strtoll,
    strtoull,
    strtod,
    strtof,
    abs,
    labs,
    llabs,
    bsearch,
    qsort,
    rand,
    srand,
    getenv: (_name) => 0,
    system: (cmd) => (cmd === 0 ? 0 : -1),
    // ctype
    isalpha,
    isdigit,
    isspace,
    isalnum,
    isupper,
    islower,
    ispunct,
    iscntrl,
    isxdigit,
    isblank,
    isprint,
    isgraph,
    tolower,
    toupper,
    // math — the full <math.h> surface (see the implementation
    // block above). The l forms are the double implementations:
    // long double is double in this target's ABI.
    sin: Math.sin, sinf: asF32(Math.sin), sinl: Math.sin,
    cos: Math.cos, cosf: asF32(Math.cos), cosl: Math.cos,
    tan: Math.tan, tanf: asF32(Math.tan), tanl: Math.tan,
    asin: Math.asin, asinf: asF32(Math.asin), asinl: Math.asin,
    acos: Math.acos, acosf: asF32(Math.acos), acosl: Math.acos,
    atan: Math.atan, atanf: asF32(Math.atan), atanl: Math.atan,
    atan2: Math.atan2, atan2f: asF32(Math.atan2), atan2l: Math.atan2,
    sinh: Math.sinh, sinhf: asF32(Math.sinh), sinhl: Math.sinh,
    cosh: Math.cosh, coshf: asF32(Math.cosh), coshl: Math.cosh,
    tanh: Math.tanh, tanhf: asF32(Math.tanh), tanhl: Math.tanh,
    asinh: Math.asinh, asinhf: asF32(Math.asinh), asinhl: Math.asinh,
    acosh: Math.acosh, acoshf: asF32(Math.acosh), acoshl: Math.acosh,
    atanh: Math.atanh, atanhf: asF32(Math.atanh), atanhl: Math.atanh,
    exp: Math.exp, expf: asF32(Math.exp), expl: Math.exp,
    exp2: exp2Impl, exp2f: asF32(exp2Impl), exp2l: exp2Impl,
    expm1: Math.expm1, expm1f: asF32(Math.expm1), expm1l: Math.expm1,
    frexp, frexpf, frexpl: frexp,
    ilogb: ilogbImpl, ilogbf: ilogbImpl, ilogbl: ilogbImpl,
    ldexp: scalbnImpl, ldexpf: asF32(scalbnImpl), ldexpl: scalbnImpl,
    log: Math.log, logf: asF32(Math.log), logl: Math.log,
    log10: Math.log10, log10f: asF32(Math.log10), log10l: Math.log10,
    log1p: Math.log1p, log1pf: asF32(Math.log1p), log1pl: Math.log1p,
    log2: Math.log2, log2f: asF32(Math.log2), log2l: Math.log2,
    logb: logbImpl, logbf: logbImpl, logbl: logbImpl,
    modf, modff, modfl: modf,
    scalbn: scalbnImpl, scalbnf: asF32(scalbnImpl), scalbnl: scalbnImpl,
    scalbln: scalbnImpl, scalblnf: asF32(scalbnImpl), scalblnl: scalbnImpl,
    cbrt: Math.cbrt, cbrtf: asF32(Math.cbrt), cbrtl: Math.cbrt,
    fabs: Math.abs, fabsf: asF32(Math.abs), fabsl: Math.abs,
    hypot: Math.hypot, hypotf: asF32(Math.hypot), hypotl: Math.hypot,
    pow: powImpl, powf: asF32(powImpl), powl: powImpl,
    sqrt: Math.sqrt, sqrtf: asF32(Math.sqrt), sqrtl: Math.sqrt,
    erf: erfImpl, erff: asF32(erfImpl), erfl: erfImpl,
    erfc: erfcImpl, erfcf: asF32(erfcImpl), erfcl: erfcImpl,
    lgamma: lgammaImpl, lgammaf: asF32(lgammaImpl), lgammal: lgammaImpl,
    tgamma: tgammaImpl, tgammaf: asF32(tgammaImpl), tgammal: tgammaImpl,
    ceil: Math.ceil, ceilf: asF32(Math.ceil), ceill: Math.ceil,
    floor: Math.floor, floorf: asF32(Math.floor), floorl: Math.floor,
    nearbyint: rintImpl, nearbyintf: asF32(rintImpl), nearbyintl: rintImpl,
    rint: rintImpl, rintf: asF32(rintImpl), rintl: rintImpl,
    lrint: (x) => toLongSat(rintImpl(x)),
    lrintf: (x) => toLongSat(rintImpl(x)),
    lrintl: (x) => toLongSat(rintImpl(x)),
    llrint: (x) => toLLongSat(rintImpl(x)),
    llrintf: (x) => toLLongSat(rintImpl(x)),
    llrintl: (x) => toLLongSat(rintImpl(x)),
    round: roundImpl, roundf: asF32(roundImpl), roundl: roundImpl,
    lround: (x) => toLongSat(roundImpl(x)),
    lroundf: (x) => toLongSat(roundImpl(x)),
    lroundl: (x) => toLongSat(roundImpl(x)),
    llround: (x) => toLLongSat(roundImpl(x)),
    llroundf: (x) => toLLongSat(roundImpl(x)),
    llroundl: (x) => toLLongSat(roundImpl(x)),
    trunc: Math.trunc, truncf: asF32(Math.trunc), truncl: Math.trunc,
    fmod: (a, b) => a % b, fmodf: asF32((a, b) => a % b), fmodl: (a, b) => a % b,
    remainder: remainderImpl, remainderf: asF32(remainderImpl), remainderl: remainderImpl,
    remquo: remquoImpl, remquof: remquofImpl, remquol: remquoImpl,
    copysign: copysignImpl, copysignf: copysignfImpl, copysignl: copysignImpl,
    nan: nanImpl, nanf: nanfImpl, nanl: nanImpl,
    nextafter: nextafterImpl, nextafterf: nextafterfImpl, nextafterl: nextafterImpl,
    nexttoward: nextafterImpl, nexttowardf: nextafterfImpl, nexttowardl: nextafterImpl,
    fdim: fdimImpl, fdimf: asF32(fdimImpl), fdiml: fdimImpl,
    fmax: fmaxImpl, fmaxf: asF32(fmaxImpl), fmaxl: fmaxImpl,
    fmin: fminImpl, fminf: asF32(fminImpl), fminl: fminImpl,
    fma: fmaImpl, fmaf: fmafImpl, fmal: fmaImpl,
    // time
    time: (tloc) => {
      const sec = Math.floor(Date.now() / 1000);
      if (tloc) writeU32(tloc, sec);
      return sec;
    },
    clock: () => Math.floor(performance.now() * 1000),
    difftime: (a, b) => a - b,
  };

  const result = { env };
  Object.defineProperty(result, 'reserveStatic', {
    value: reserveStatic,
    enumerable: false,
  });
  // The allocator state object in use — in a process, the
  // memory-resident view over the header words, so the loader
  // reads and writes the same state the shims allocate from.
  Object.defineProperty(result, 'allocatorState', {
    value: allocState,
    enumerable: false,
  });
  return result;
}
