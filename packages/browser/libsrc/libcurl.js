'use strict';
/*
 * libcurl (easy interface + WebSocket) — JS implementation for
 * the wasm32 sysroot, shipped as the js-kind unit
 * libcurl.tinyc.js.
 *
 * The transport is the platform's: HTTP goes through fetch,
 * WebSocket through the platform WebSocket, and TLS is the
 * platform's (https/wss are used as-is). The exports that wait
 * on the transport (curl_easy_perform, curl_ws_send/recv) are
 * declared async in the unit manifest: in the simulated tier
 * they run through JSPI; in a thread worker the same source
 * runs synchronously against the worker's blocking net
 * channel, whose operations the process coordinator executes.
 * The branch is ctx.net.isAsync, taken at call time; the
 * request/response bookkeeping is shared by both shapes.
 *
 * Handle state lives in guest memory: an easy handle is a
 * fixed-size block holding a JSON option/result record, and a
 * curl_slist is the real linked list of the header's struct,
 * allocated node by node. State therefore means the same thing
 * in every context that touches the handle.
 *
 * Deliberate subset (the header surface beyond it resolves to
 * CURLE_UNKNOWN_OPTION at setopt time, or stays unimplemented):
 * no proxy support (the platform has none to offer), no cookie
 * engine (a Cookie header can be set explicitly), no upload
 * streaming (POSTFIELDS bodies only), header names the fetch
 * specification forbids (User-Agent, Cookie, Referer in some
 * contexts, Accept-Encoding) are passed through where the
 * platform accepts them and dropped by the platform where it
 * does not. An unfollowed redirect is likewise only as
 * visible as the platform makes it: fetch answers it with an
 * opaque response (status 0, no headers, no body) in
 * browsers, so CURLINFO_RESPONSE_CODE reports 0 there.
 */

// ---- guest memory helpers --------------------------------------

function dv(ctx) { return new DataView(ctx.memory().buffer); }
function mem(ctx) { return ctx.u8(); }
function getU32(ctx, a) { return dv(ctx).getUint32(a, true); }
function setU32(ctx, a, v) { dv(ctx).setUint32(a, v >>> 0, true); }
function setF64(ctx, a, v) { dv(ctx).setFloat64(a, v, true); }
function setI64(ctx, a, v) { dv(ctx).setBigInt64(a, BigInt(v), true); }

function readCString(ctx, ptr) {
  const m = mem(ctx);
  let end = ptr;
  while (m[end] !== 0) end++;
  // Copy before decoding: a shared memory's views are refused
  // by TextDecoder (real-tier processes).
  return new TextDecoder().decode(m.slice(ptr, end));
}
function readBytesB64(ctx, ptr, len) {
  const bytes = mem(ctx).subarray(ptr, ptr + len);
  let bin = '';
  const CHUNK = 8192;
  for (let i = 0; i < bytes.length; i += CHUNK) {
    bin += String.fromCharCode(...bytes.subarray(i, i + CHUNK));
  }
  return btoa(bin);
}
function writeCString(ctx, ptr, s) {
  const bytes = new TextEncoder().encode(s);
  mem(ctx).set(bytes, ptr);
  mem(ctx)[ptr + bytes.length] = 0;
}

// va_list reading, mirroring the shim layer's makeVa layout:
// i32-sized slots align to 4, 64-bit slots align to 8.
function vaReader(ctx, vaPtr) {
  let cur = vaPtr;
  const alignTo = (n) => { cur = (cur + n - 1) & ~(n - 1); };
  return {
    i32() { alignTo(4); const v = dv(ctx).getInt32(cur, true); cur += 4; return v; },
    ptr() { alignTo(4); const v = dv(ctx).getUint32(cur, true); cur += 4; return v; },
    i64() { alignTo(8); const v = dv(ctx).getBigInt64(cur, true); cur += 8; return v; },
  };
}

// ---- codes ------------------------------------------------------

const CURLE_OK = 0;
const CURLE_UNSUPPORTED_PROTOCOL = 1;
const CURLE_URL_MALFORMAT = 3;
const CURLE_COULDNT_RESOLVE_HOST = 6;
const CURLE_COULDNT_CONNECT = 7;
const CURLE_HTTP_RETURNED_ERROR = 22;
const CURLE_WRITE_ERROR = 23;
const CURLE_OPERATION_TIMEDOUT = 28;
const CURLE_UNKNOWN_OPTION = 48;
const CURLE_TOO_MANY_REDIRECTS = 47;
const CURLE_SEND_ERROR = 55;
const CURLE_RECV_ERROR = 56;
const CURLE_GOT_NOTHING = 52;

const ERROR_TEXTS = new Map([
  [0, 'No error'],
  [1, 'Unsupported protocol'],
  [3, 'URL using bad/illegal format or missing URL'],
  [6, "Couldn't resolve host name"],
  [7, "Couldn't connect to server"],
  [22, 'HTTP response code indicated error'],
  [23, 'Failed writing received data to disk/application'],
  [28, 'Timeout was reached'],
  [47, 'Maximum number of redirects hit'],
  [48, 'An unknown option was passed in to libcurl'],
  [52, 'Server returned nothing (no headers, no data)'],
  [55, 'Failed sending data to the peer'],
  [56, 'Failure when receiving data from the peer'],
]);

// ---- handle state ------------------------------------------------
// Block: magic u32 @0, JSON length u32 @4, JSON @8, capacity
// HANDLE_CAP. The JSON record carries the options and, after a
// transfer, the result fields getinfo reads.

const HANDLE_MAGIC = 0x32525543;   // 'CUR2'
const HANDLE_CAP = 65536;

function defaultState() {
  return {
    url: null, port: 0, method: null,          // null = GET unless postFields set
    customRequest: null,
    postFieldsB64: null, postFieldSize: -1,
    headersList: 0,                            // curl_slist* in guest memory
    writeFunction: 0, writeData: 0,
    headerFunction: 0, headerData: 0,
    userAgent: null, referer: null, cookie: null, acceptEncoding: null,
    userPwd: null, username: null, password: null,
    followLocation: 0, maxRedirs: 30,
    timeoutMs: 0, connectTimeoutMs: 0,
    errorBuffer: 0, failOnError: 0, includeHeader: 0,
    connectOnly: 0, noBody: 0, verbose: 0,
    wsConn: 0, wsProtocols: null, wsMidMessage: 0, wsMeta: 0,
    // result fields
    responseCode: 0, effectiveUrl: null, contentType: null,
    contentLength: -1, downloadSize: 0, totalTime: 0, redirectCount: 0,
    infoUrlPtr: 0, infoTypePtr: 0,
  };
}

function readState(ctx, handle) {
  if (!handle || getU32(ctx, handle) !== HANDLE_MAGIC) return null;
  const len = getU32(ctx, handle + 4);
  const text = new TextDecoder().decode(mem(ctx).slice(handle + 8, handle + 8 + len));
  return JSON.parse(text);
}

function writeState(ctx, handle, state) {
  const bytes = new TextEncoder().encode(JSON.stringify(state));
  if (bytes.length > HANDLE_CAP - 8) return false;
  mem(ctx).set(bytes, handle + 8);
  setU32(ctx, handle + 4, bytes.length);
  return true;
}

function setError(ctx, state, code, detail) {
  if (state && state.errorBuffer) {
    const text = (ERROR_TEXTS.get(code) ?? `error ${code}`) + (detail ? `: ${detail}` : '');
    const bytes = new TextEncoder().encode(text);
    const cap = 256;   // CURL_ERROR_SIZE
    const n = Math.min(bytes.length, cap - 1);
    mem(ctx).set(bytes.subarray(0, n), state.errorBuffer);
    mem(ctx)[state.errorBuffer + n] = 0;
  }
  return code;
}

// Static strings (version, error texts): allocated once per
// evaluation context and kept; the pointers are stable within
// the context, which is what guest code can observe.
const staticStrings = new Map();
function staticString(ctx, key, text) {
  let ptr = staticStrings.get(key);
  if (ptr) return ptr;
  ptr = ctx.alloc(text.length + 1);
  writeCString(ctx, ptr, text);
  staticStrings.set(key, ptr);
  return ptr;
}

// ---- slist --------------------------------------------------------

function slistAppend(ctx, list, strPtr) {
  const text = readCString(ctx, strPtr);
  const bytes = new TextEncoder().encode(text);
  const data = ctx.alloc(bytes.length + 1);
  mem(ctx).set(bytes, data);
  mem(ctx)[data + bytes.length] = 0;
  const node = ctx.alloc(8);
  setU32(ctx, node, data);
  setU32(ctx, node + 4, 0);
  if (!list) return node;
  let tail = list;
  while (getU32(ctx, tail + 4)) tail = getU32(ctx, tail + 4);
  setU32(ctx, tail + 4, node);
  return list;
}

function slistFreeAll(ctx, list) {
  let node = list;
  while (node) {
    const next = getU32(ctx, node + 4);
    ctx.free(getU32(ctx, node));
    ctx.free(node);
    node = next;
  }
}

function slistToArray(ctx, list) {
  const out = [];
  let node = list;
  while (node) {
    out.push(readCString(ctx, getU32(ctx, node)));
    node = getU32(ctx, node + 4);
  }
  return out;
}

// ---- setopt --------------------------------------------------------
// Option numbers from curl/curl.h (8.11.1). Classes: LONG (<
// 10000), pointer (10000-19999, incl. strings/objects/slists),
// FUNCTIONPOINT (20000-29999), OFF_T (30000-39999).

const ACCEPTED_IGNORED_LONGS = new Set([
  41,   // VERBOSE (recorded on the state)
  64,   // SSL_VERIFYPEER — TLS is the platform's
  81,   // SSL_VERIFYHOST — TLS is the platform's
  69,   // FILETIME
  96,   // FRESH_CONNECT family / misc no-ops for this subset
  101,  // BUFSIZE
  105,  // NOSIGNAL
  119,  // HTTP200ALIASES is a slist; not here
  123,  // TIMEVALUE family
  136,  // NETRC
  142,  // TRANSFERTEXT
  150,  // PROXYAUTH (no proxy support)
  156,  // NOPROGRESS
  172,  // IPRESOLVE
  213,  // SUPPRESS_CONNECT_HEADERS
]);
const ACCEPTED_IGNORED_POINTERS = new Set([
  10031,  // COOKIEFILE — no cookie engine; accepted, unused
  10065,  // CAINFO — TLS is the platform's
  10165,  // CAINFO_BLOB family
  10247,  // SSL_CTX family
]);

function setopt(ctx, handle, option, vaPtr) {
  const state = readState(ctx, handle);
  if (!state) return CURLE_URL_MALFORMAT;   // not a handle this unit made
  const va = vaReader(ctx, vaPtr);
  const cls = option >= 30000 ? 'offt' : option >= 20000 ? 'func'
    : option >= 10000 ? 'ptr' : 'long';
  const arg = cls === 'offt' ? va.i64() : va.i32();
  const strArg = () => (arg === 0 ? null : readCString(ctx, arg));
  let ok = true;
  switch (option) {
    case 10002: state.url = strArg(); break;                       // URL
    case 3: state.port = arg; break;                               // PORT
    case 10005: state.userPwd = strArg(); break;                   // USERPWD
    case 10173: state.username = strArg(); break;                  // USERNAME
    case 10174: state.password = strArg(); break;                  // PASSWORD
    case 10010: state.errorBuffer = arg; break;                    // ERRORBUFFER
    case 10001: state.writeData = arg; break;                      // WRITEDATA
    case 20011: state.writeFunction = arg; break;                  // WRITEFUNCTION
    case 10029: state.headerData = arg; break;                     // HEADERDATA
    case 20079: state.headerFunction = arg; break;                 // HEADERFUNCTION
    case 10015: {                                                  // POSTFIELDS
      if (arg === 0) { state.postFieldsB64 = null; state.postFieldSize = -1; }
      else {
        let n = state.postFieldSize;
        if (n < 0) { const m = mem(ctx); n = 0; while (m[arg + n] !== 0) n++; }
        state.postFieldsB64 = readBytesB64(ctx, arg, n);
        state.postFieldSize = n;
      }
      break;
    }
    case 10060: state.postFieldSize = arg; break;                  // POSTFIELDSIZE
    case 30120: state.postFieldSize = Number(arg); break;          // POSTFIELDSIZE_LARGE
    case 10023: state.headersList = arg; break;                    // HTTPHEADER
    case 10016: state.referer = strArg(); break;                   // REFERER
    case 10018: state.userAgent = strArg(); break;                 // USERAGENT
    case 10022: state.cookie = strArg(); break;                    // COOKIE
    case 10102: state.acceptEncoding = strArg(); break;            // ACCEPT_ENCODING
    case 10036: state.customRequest = strArg(); break;             // CUSTOMREQUEST
    case 13: state.timeoutMs = arg * 1000; break;                  // TIMEOUT
    case 155: state.timeoutMs = arg; break;                        // TIMEOUT_MS
    case 78: state.connectTimeoutMs = arg * 1000; break;           // CONNECTTIMEOUT
    case 47: state.method = arg ? 'POST' : state.method;           // POST
      if (!arg && state.method === 'POST') state.method = null;
      break;
    case 80: if (arg) { state.method = null; state.postFieldsB64 = null; } break;  // HTTPGET
    case 52: state.followLocation = arg; break;                    // FOLLOWLOCATION
    case 68: state.maxRedirs = arg; break;                         // MAXREDIRS
    case 45: state.failOnError = arg; break;                       // FAILONERROR
    case 42: state.includeHeader = arg; break;                     // HEADER
    case 44: state.noBody = arg; break;                            // NOBODY
    case 141: state.connectOnly = arg; break;                      // CONNECT_ONLY
    case 41: state.verbose = arg; break;                           // VERBOSE
    case 320: break;                                               // WS_OPTIONS (raw mode unused)
    default:
      if (cls === 'long' && ACCEPTED_IGNORED_LONGS.has(option)) break;
      if (cls === 'ptr' && ACCEPTED_IGNORED_POINTERS.has(option)) break;
      ok = false;
  }
  if (!ok) return CURLE_UNKNOWN_OPTION;
  if (!writeState(ctx, handle, state)) return 27;   // CURLE_OUT_OF_MEMORY
  return CURLE_OK;
}

// ---- transfer ------------------------------------------------------

function buildRequest(ctx, state) {
  if (!state.url) return { error: CURLE_URL_MALFORMAT };
  let url = state.url;
  const isWs = url.startsWith('ws://') || url.startsWith('wss://');
  // Credentials: USERPWD / USERNAME+PASSWORD become URL userinfo
  // when the URL carries none (fetch's own basic-auth channel).
  let creds = null;
  if (state.userPwd) creds = state.userPwd;
  else if (state.username) creds = state.username + ':' + (state.password ?? '');
  if (creds && !isWs) {
    const m = /^([a-zA-Z][a-zA-Z0-9+.-]*:\/\/)([^/?#]*)(.*)$/.exec(url);
    if (m && !m[2].includes('@')) {
      const [user, pass] = creds.split(/:(.*)/s).slice(0, 2);
      url = m[1] + encodeURIComponent(user) +
        (pass !== undefined ? ':' + encodeURIComponent(pass) : '') + '@' + m[2] + m[3];
    }
  }
  if (state.port) {
    try {
      const u = new URL(url);
      u.port = String(state.port);
      url = u.toString();
    } catch { /* keep the URL as given */ }
  }
  const headers = [];
  let hasContentType = false;
  for (const line of state.headersList ? slistToArray(ctx, state.headersList) : []) {
    const idx = line.indexOf(':');
    if (idx > 0) {
      const name = line.slice(0, idx).trim();
      const value = line.slice(idx + 1).trim();
      if (name.toLowerCase() === 'content-type') hasContentType = true;
      headers.push([name, value]);
    } else if (line.endsWith(':')) {
      headers.push([line.slice(0, -1).trim(), '']);   // "Name:" removes a header
    }
  }
  if (state.userAgent) headers.push(['User-Agent', state.userAgent]);
  if (state.referer) headers.push(['Referer', state.referer]);
  if (state.cookie) headers.push(['Cookie', state.cookie]);
  if (state.acceptEncoding) headers.push(['Accept-Encoding', state.acceptEncoding]);
  let method = state.customRequest ?? state.method ??
    (state.postFieldsB64 !== null ? 'POST' : 'GET');
  let body = null;
  if (state.postFieldsB64 !== null && method !== 'GET') {
    body = state.postFieldsB64;
    if (!hasContentType) {
      headers.push(['Content-Type', 'application/x-www-form-urlencoded']);
    }
  }
  return { url, isWs, headers, method, body };
}

const STATUS_REASONS = {
  200: 'OK', 201: 'Created', 204: 'No Content', 301: 'Moved Permanently',
  302: 'Found', 304: 'Not Modified', 400: 'Bad Request', 401: 'Unauthorized',
  403: 'Forbidden', 404: 'Not Found', 500: 'Internal Server Error',
};

// Deliver one chunk to the guest's write/header callback.
// Returns false when the callback consumed less than offered
// (CURLE_WRITE_ERROR upstream).
function deliver(ctx, fnPtr, dataPtr, bytes, scratchPtr) {
  if (!fnPtr) return true;
  mem(ctx).set(bytes, scratchPtr);
  const written = ctx.callFunction(fnPtr, scratchPtr, 1, bytes.length, dataPtr);
  return written === bytes.length;
}

function deliverHeaders(ctx, state, res, scratchPtr) {
  const lines = [`HTTP/1.1 ${res.status} ${STATUS_REASONS[res.status] ?? ''}`];
  for (const [k, v] of res.headers) lines.push(`${k}: ${v}`);
  const targetFn = state.headerFunction || (state.includeHeader ? state.writeFunction : 0);
  const targetData = state.headerFunction ? state.headerData : state.writeData;
  for (const line of lines) {
    const bytes = new TextEncoder().encode(line + '\r\n');
    if (!deliver(ctx, targetFn, targetData, bytes, scratchPtr)) return false;
  }
  if (lines.length) {
    if (!deliver(ctx, targetFn, targetData, new Uint8Array([13, 10]), scratchPtr)) return false;
  }
  return true;
}

function classifyNetError(err) {
  const msg = String(err?.message ?? err);
  if (/timeout|timed out|AbortError|TimeoutError/i.test(msg)) {
    return CURLE_OPERATION_TIMEDOUT;
  }
  if (/resolve|getaddrinfo|ENOTFOUND|dns/i.test(msg)) return CURLE_COULDNT_RESOLVE_HOST;
  return CURLE_COULDNT_CONNECT;
}

// The transfer logic is written once, as a generator that
// yields each ctx.net call. Two drivers run it: driveAsync
// awaits the yields (platform-async net — the shape the
// coordinator binds, and the simulated tier through
// Suspending), and driveSync steps it with the plain values a
// worker replica's blocking net channel returns. An async
// function cannot serve the blocking shape: it would hand
// the caller a promise where a CURLcode belongs. The exported
// functions pick the driver by ctx.net.isAsync at call time.
function driveSync(gen) {
  let step = gen.next();
  while (!step.done) {
    if (step.value instanceof Promise) {
      throw new Error('tinyc: libcurl: the blocking net channel returned a promise');
    }
    step = gen.next(step.value);
  }
  return step.value;
}

async function driveAsync(gen) {
  let step = gen.next();
  while (!step.done) {
    try {
      step = gen.next(await step.value);
    } catch (err) {
      step = gen.throw(err);
    }
  }
  return step.value;
}

function drive(ctx, gen) {
  return ctx.net.isAsync ? driveAsync(gen) : driveSync(gen);
}

function* performTransfer(ctx, handle) {
  const state = readState(ctx, handle);
  if (!state) return CURLE_URL_MALFORMAT;
  const req = buildRequest(ctx, state);
  if (req.error) return setError(ctx, state, req.error, 'no URL set');
  const started = Date.now();

  if (req.isWs) {
    // A WebSocket URL: perform establishes the connection
    // (with CONNECT_ONLY semantics for the send/recv calls
    // that follow; a plain perform connects the same way).
    if (state.wsConn) return CURLE_OK;
    const protocols = [];
    for (const [k, v] of req.headers) {
      if (k.toLowerCase() === 'sec-websocket-protocol') {
        protocols.push(...v.split(',').map((s) => s.trim()).filter(Boolean));
      }
    }
    try {
      const conn = yield ctx.net.wsOpen(req.url, protocols);
      state.wsConn = conn;
      state.responseCode = 101;
      state.effectiveUrl = req.url;
      writeState(ctx, handle, state);
      return CURLE_OK;
    } catch (err) {
      return setError(ctx, state, classifyNetError(err), String(err?.message ?? err));
    }
  }

  if (!/^https?:\/\//.test(req.url)) {
    return setError(ctx, state, CURLE_UNSUPPORTED_PROTOCOL, req.url.split(':')[0]);
  }
  if (state.connectOnly) {
    // No transfer API can "just connect" through fetch; the
    // honest equivalent for this subset is a no-op success
    // only for WebSocket URLs (handled above).
    return setError(ctx, state, CURLE_UNSUPPORTED_PROTOCOL,
      'CONNECT_ONLY is supported for WebSocket URLs only');
  }

  const desc = {
    method: state.noBody ? 'HEAD' : req.method,
    url: req.url,
    headers: req.headers,
    body: req.body,
    follow: !!state.followLocation,
    timeoutMs: state.timeoutMs || state.connectTimeoutMs || 0,
  };
  let res;
  try {
    res = yield ctx.net.http(desc);
  } catch (err) {
    return setError(ctx, state, classifyNetError(err), String(err?.message ?? err));
  }
  state.responseCode = res.status;
  state.effectiveUrl = res.url ?? req.url;
  state.downloadSize = res.bodyLen;
  state.contentLength = -1;
  state.contentType = null;
  for (const [k, v] of res.headers) {
    const lk = k.toLowerCase();
    if (lk === 'content-length') state.contentLength = Number(v) || 0;
    if (lk === 'content-type') state.contentType = v;
  }
  state.totalTime = (Date.now() - started) / 1000;

  const scratch = ctx.alloc(Math.max(16384, 1024));
  try {
    if (!deliverHeaders(ctx, state, res, scratch)) {
      return setError(ctx, state, CURLE_WRITE_ERROR, 'header callback refused data');
    }
    if (state.writeFunction && res.bodyLen > 0) {
      let offset = 0;
      while (offset < res.bodyLen) {
        const chunk = Math.min(16384, res.bodyLen - offset);
        yield ctx.net.readBody(res.transfer, scratch, offset, chunk);
        const bytes = mem(ctx).subarray(scratch, scratch + chunk).slice();
        if (!deliver(ctx, state.writeFunction, state.writeData, bytes, scratch)) {
          return setError(ctx, state, CURLE_WRITE_ERROR, 'write callback refused data');
        }
        offset += chunk;
      }
    }
  } finally {
    ctx.free(scratch);
    try { ctx.net.dropBody(res.transfer); } catch { /* already gone */ }
  }
  writeState(ctx, handle, state);
  if (state.failOnError && res.status >= 400) {
    return setError(ctx, state, CURLE_HTTP_RETURNED_ERROR, `HTTP ${res.status}`);
  }
  return CURLE_OK;
}

function performSyncOrAsync(ctx, handle) {
  return drive(ctx, performTransfer(ctx, handle));
}

// ---- WebSocket send / receive ---------------------------------------

const WS_FRAME_SIZE = 28;   // struct curl_ws_frame, wasm32 layout

function ensureMeta(ctx, state, handle) {
  if (!state.wsMeta) {
    state.wsMeta = ctx.alloc(WS_FRAME_SIZE);
    writeState(ctx, handle, state);
  }
  return state.wsMeta;
}

function* wsSend(ctx, handle, buffer, buflen, sentPtr, flags) {
  const state = readState(ctx, handle);
  if (!state || !state.wsConn) return CURLE_SEND_ERROR;
  const binary = !(flags & 1);   // CURLWS_TEXT is bit 0
  try {
    yield ctx.net.wsSend(state.wsConn, buffer, buflen, binary);
  } catch (err) {
    return setError(ctx, state, CURLE_SEND_ERROR, String(err?.message ?? err));
  }
  if (sentPtr) setU32(ctx, sentPtr, buflen);
  if (flags & 8) {   // CURLWS_CLOSE
    try { ctx.net.wsClose(state.wsConn); } catch { /* ignore */ }
    state.wsConn = 0;
    writeState(ctx, handle, state);
  }
  return CURLE_OK;
}

function* wsRecv(ctx, handle, buffer, buflen, recvPtr, metaPtrPtr) {
  const state = readState(ctx, handle);
  if (!state || !state.wsConn) return CURLE_RECV_ERROR;
  let r;
  try {
    r = yield ctx.net.wsRecv(state.wsConn, buffer, buflen);
  } catch (err) {
    return setError(ctx, state, CURLE_RECV_ERROR, String(err?.message ?? err));
  }
  if (r.received === 0 && r.fin) {
    // Drained and closed.
    return setError(ctx, state, CURLE_GOT_NOTHING, 'connection closed');
  }
  if (recvPtr) setU32(ctx, recvPtr, r.received);
  const meta = ensureMeta(ctx, state, handle);
  const flags = (r.binary ? 2 : 1) | (state.wsMidMessage ? 4 : 0);
  setU32(ctx, meta, 0);                    // age
  setU32(ctx, meta + 4, flags);            // flags
  setI64(ctx, meta + 8, 0);                // offset (this unit serves whole frames from 0)
  setI64(ctx, meta + 16, r.remaining);     // bytesleft
  setU32(ctx, meta + 24, r.received);      // len
  state.wsMidMessage = r.fin ? 0 : 1;
  writeState(ctx, handle, state);
  if (metaPtrPtr) setU32(ctx, metaPtrPtr, meta);
  return CURLE_OK;
}

// ---- getinfo ----------------------------------------------------------

function getinfo(ctx, handle, info, vaPtr) {
  const state = readState(ctx, handle);
  if (!state) return CURLE_URL_MALFORMAT;
  const va = vaReader(ctx, vaPtr);
  const out = va.ptr();
  const cls = info & 0xf00000;
  if (cls === 0x100000) {
    // CURLINFO_STRING: write a char* — the strings live in
    // blocks owned by the handle until cleanup.
    let text = null;
    if (info === 0x100001) text = state.effectiveUrl;         // EFFECTIVE_URL
    else if (info === 0x100012) text = state.contentType;     // CONTENT_TYPE
    else return CURLE_UNKNOWN_OPTION;
    if (text === null) { setU32(ctx, out, 0); return CURLE_OK; }
    const key = info === 0x100001 ? 'infoUrlPtr' : 'infoTypePtr';
    if (state[key]) ctx.free(state[key]);
    const ptr = ctx.alloc(text.length + 1);
    writeCString(ctx, ptr, text);
    state[key] = ptr;
    writeState(ctx, handle, state);
    setU32(ctx, out, ptr);
    return CURLE_OK;
  }
  if (cls === 0x200000) {
    let v = null;
    if (info === 0x200002) v = state.responseCode;            // RESPONSE_CODE
    else if (info === 0x200014) v = state.redirectCount;      // REDIRECT_COUNT
    else return CURLE_UNKNOWN_OPTION;
    setU32(ctx, out, v);
    return CURLE_OK;
  }
  if (cls === 0x300000) {
    let v = null;
    if (info === 0x300003) v = state.totalTime;               // TOTAL_TIME
    else if (info === 0x30000f) {                             // CONTENT_LENGTH_DOWNLOAD
      v = state.contentLength < 0 ? -1 : state.contentLength;
    } else return CURLE_UNKNOWN_OPTION;
    setF64(ctx, out, v);
    return CURLE_OK;
  }
  if (cls === 0x600000) {
    let v = null;
    if (info === 0x60000f) v = state.contentLength;           // CONTENT_LENGTH_DOWNLOAD_T
    else if (info === 0x600008) v = state.downloadSize;       // SIZE_DOWNLOAD_T
    else return CURLE_UNKNOWN_OPTION;
    setI64(ctx, out, v);
    return CURLE_OK;
  }
  return CURLE_UNKNOWN_OPTION;
}

// ---- export table ------------------------------------------------------

const I32 = 'i32', I64 = 'i64';
function sig(params, results, isAsync) {
  return { sig: { params, results }, async: isAsync };
}

const EXPORTS = [
  ['curl_global_init', sig([I32], [I32]), (ctx, flags) => 0],
  ['curl_global_cleanup', sig([], []), () => {}],
  ['curl_version', sig([], [I32]),
    (ctx) => staticString(ctx, 'version', 'libcurl/8.11.1 (tinyc-js) Schannel')],
  ['curl_easy_strerror', sig([I32], [I32]),
    (ctx, code) => staticString(ctx, `err${code}`, ERROR_TEXTS.get(code) ?? 'Unknown error')],
  ['curl_easy_init', sig([], [I32]), (ctx) => {
    const handle = ctx.alloc(HANDLE_CAP);
    mem(ctx).fill(0, handle, handle + HANDLE_CAP);
    setU32(ctx, handle, HANDLE_MAGIC);
    writeState(ctx, handle, defaultState());
    return handle;
  }],
  ['curl_easy_cleanup', sig([I32], []), (ctx, handle) => {
    const state = readState(ctx, handle);
    if (!state) return;
    if (state.wsConn) { try { ctx.net.wsClose(state.wsConn); } catch { /* ignore */ } }
    if (state.infoUrlPtr) ctx.free(state.infoUrlPtr);
    if (state.infoTypePtr) ctx.free(state.infoTypePtr);
    if (state.wsMeta) ctx.free(state.wsMeta);
    ctx.free(handle);
  }],
  ['curl_easy_reset', sig([I32], []), (ctx, handle) => {
    const state = readState(ctx, handle);
    if (!state) return;
    if (state.wsConn) { try { ctx.net.wsClose(state.wsConn); } catch { /* ignore */ } }
    writeState(ctx, handle, defaultState());
  }],
  ['curl_easy_setopt', sig([I32, I32, I32], [I32]),
    (ctx, h, o, va) => setopt(ctx, h, o, va)],
  ['curl_easy_perform', sig([I32], [I32], true),
    (ctx, h) => performSyncOrAsync(ctx, h)],
  ['curl_easy_getinfo', sig([I32, I32, I32], [I32]),
    (ctx, h, i, va) => getinfo(ctx, h, i, va)],
  ['curl_slist_append', sig([I32, I32], [I32]),
    (ctx, list, str) => slistAppend(ctx, list, str)],
  ['curl_slist_free_all', sig([I32], []), (ctx, list) => { slistFreeAll(ctx, list); }],
  ['curl_ws_send', sig([I32, I32, I32, I32, I64, I32], [I32], true),
    (ctx, h, buf, len, sent, fragsize, flags) => drive(ctx, wsSend(ctx, h, buf, len, sent, flags))],
  ['curl_ws_recv', sig([I32, I32, I32, I32, I32], [I32], true),
    (ctx, h, buf, len, recv, meta) => drive(ctx, wsRecv(ctx, h, buf, len, recv, meta))],
  ['curl_ws_meta', sig([I32], [I32]), (ctx, handle) => {
    const state = readState(ctx, handle);
    if (!state) return 0;
    return ensureMeta(ctx, state, handle);
  }],
];

const out = {
  __exports: EXPORTS.map(([name, s]) => ({ name, sig: s.sig, ...(s.async ? { async: true } : {}) })),
};
for (const [name, , fn] of EXPORTS) out[name] = fn;
module.exports = out;
