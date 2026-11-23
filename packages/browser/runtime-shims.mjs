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
    return textDecoder.decode(m.subarray(ptr, end));
  };
  const writeCStringBytes = (ptr, bytes) => {
    writeBytes(ptr, bytes);
    u8()[ptr + bytes.length] = 0;
    return bytes.length;
  };

  // ---------------------------------------------------------------
  // Allocator: first-fit free list over [heapBase, memory end).
  // Block layout: [payloadSize u32][nextOrUsed u32][payload...];
  // the payload pointer handed out is header + 8.
  // ---------------------------------------------------------------
  let heapTop = 0;
  let freeHead = 0;

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

  function rawMalloc(size) {
    size = Math.max(8, (size + 7) & ~7);
    let prev = 0;
    let cur = freeHead;
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
          if (prev) writeU32(prev + 4, rem); else freeHead = rem;
        } else {
          if (prev) writeU32(prev + 4, cnext); else freeHead = cnext;
        }
        writeU32(cur + 4, ALLOC_USED);
        return cur + 8;
      }
      prev = cur;
      cur = cnext;
    }
    if (!heapTop) heapTop = heapBase;
    const header = heapTop;
    const end = header + 8 + size;
    if (!ensureCapacity(end)) return 0;
    writeU32(header, size);
    writeU32(header + 4, ALLOC_USED);
    heapTop = end;
    return header + 8;
  }

  function rawFree(ptr) {
    if (!ptr) return;
    const header = ptr - 8;
    if (readU32(header + 4) !== ALLOC_USED) return; // foreign/double free
    writeU32(header + 4, freeHead);
    freeHead = header;
  }

  function malloc(size) {
    const own = moduleAllocator();
    return own ? own.malloc(size) : rawMalloc(size);
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
      if (!np) return 0;
      // The preserved prefix is what matters; bytes past the old
      // block's end land in the new block's uninitialized tail.
      writeBytes(np, readBytes(ptr, size));
      if (typeof own.free === 'function') own.free(ptr);
      return np;
    }
    const oldSize = readU32(ptr - 8);
    if (size <= oldSize) return ptr;
    const np = rawMalloc(size);
    if (!np) return 0;
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
    const table = holder?.instance?.exports?.__indirect_function_table;
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
    return emitText(formatString(fmt, va), stream === 2);
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
  function puts(ptr) {
    return emitText(readCString(ptr) + '\n', false) >= 0 ? 0 : -1;
  }
  function putchar(c) {
    emitOut(Uint8Array.of(c & 0xff));
    return c;
  }
  function fputs(ptr, stream) {
    emitText(readCString(ptr), stream === 2);
    return 0;
  }
  function putc(c, stream) {
    (stream === 2 ? emitErr : emitOut)(Uint8Array.of(c & 0xff));
    return c;
  }
  function fwrite(ptr, size, nmemb, stream) {
    const total = size * nmemb;
    if (total === 0) return 0;
    (stream === 2 ? emitErr : emitOut)(readBytes(ptr, total));
    return nmemb;
  }
  function fread(ptr, size, nmemb, _stream) {
    if (size === 0 || nmemb === 0) return 0;
    const data = stdinRead(size * nmemb);
    if (data.length) writeBytes(ptr, data);
    return Math.floor(data.length / size);
  }
  function fgetc(_stream) {
    const data = stdinRead(1);
    return data.length ? data[0] : -1;
  }
  function ungetc(c, _stream) {
    if (c >= 0) pushback.push(c & 0xff);
    return c;
  }

  // ---------------------------------------------------------------
  // math / time
  // ---------------------------------------------------------------
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
    // math
    sin: Math.sin,
    cos: Math.cos,
    tan: Math.tan,
    asin: Math.asin,
    acos: Math.acos,
    atan: Math.atan,
    atan2: Math.atan2,
    exp: Math.exp,
    log: Math.log,
    log10: Math.log10,
    pow: Math.pow,
    sqrt: Math.sqrt,
    floor: Math.floor,
    ceil: Math.ceil,
    fabs: Math.abs,
    round: Math.round,
    trunc: Math.trunc,
    fmod: (a, b) => a % b,
    // time
    time: (tloc) => {
      const sec = Math.floor(Date.now() / 1000);
      if (tloc) writeU32(tloc, sec);
      return sec;
    },
    clock: () => Math.floor(performance.now() * 1000),
    difftime: (a, b) => a - b,
  };

  return { env };
}
