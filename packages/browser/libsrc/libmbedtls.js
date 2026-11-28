'use strict';
/*
 * mbedTLS crypto subset — JS implementation for the wasm32
 * sysroot, shipped as the js-kind unit libmbedtls.tinyc.js.
 *
 * The surface is the one the shipped configuration
 * (mbedtls/mbedtls_config.h) enables and this implementation
 * supplies: the MD5 / SHA-256 / SHA-512 digests and the generic
 * MD layer with HMAC, AES (ECB, CBC, CFB128, CFB8, OFB, CTR)
 * and Base64. Everything is synchronous plain JS — the C
 * interface is synchronous, so no async facility is involved.
 *
 * Hash and cipher state lives in the guest's own context
 * structs, at the layouts the shipped headers declare: an
 * operation started in one context (the coordinator's copy, a
 * thread worker's replica) reads and writes the same shared
 * memory as any other. The MD layer's heap contexts
 * (md_ctx/hmac_ctx) and the static md_info blocks are allocated
 * from the process allocator through ctx.alloc, for the same
 * reason.
 */

// ---- guest memory helpers ------------------------------------

function dv(ctx) { return new DataView(ctx.memory().buffer); }
function mem(ctx) { return ctx.u8(); }
function getU32(ctx, addr) { return dv(ctx).getUint32(addr, true); }
function setU32(ctx, addr, v) { dv(ctx).setUint32(addr, v >>> 0, true); }
function getU64(ctx, addr) { return dv(ctx).getBigUint64(addr, true); }
function setU64(ctx, addr, v) { dv(ctx).setBigUint64(addr, BigInt.asUintN(64, v), true); }
function readStr(ctx, ptr) {
  const m = mem(ctx);
  let s = '';
  for (let p = ptr; m[p] !== 0; p++) s += String.fromCharCode(m[p]);
  return s;
}
function writeStr(ctx, ptr, s) {
  const m = mem(ctx);
  for (let i = 0; i < s.length; i++) m[ptr + i] = s.charCodeAt(i) & 0xff;
  m[ptr + s.length] = 0;
}

// Error codes, as the shipped headers define them.
const ERR_MD_BAD_INPUT = -0x5100;      // MBEDTLS_ERR_MD_BAD_INPUT_DATA
const ERR_MD_ALLOC_FAILED = -0x5180;   // MBEDTLS_ERR_MD_ALLOC_FAILED
const ERR_AES_INVALID_KEY = -0x0020;   // MBEDTLS_ERR_AES_INVALID_KEY_LENGTH
const ERR_AES_BAD_INPUT = -0x0021;     // MBEDTLS_ERR_AES_BAD_INPUT_DATA
const ERR_B64_TOO_SMALL = -0x002a;     // MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL
const ERR_B64_BAD_CHAR = -0x002c;      // MBEDTLS_ERR_BASE64_INVALID_CHARACTER

// ---- SHA-256 --------------------------------------------------
// Context layout (mbedtls/sha256.h, MBEDTLS_SHA224_C off):
//   buffer[64] @0, total[2] u32 @64, state[8] u32 @72.

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

function sha256Process(ctx, c, addr) {
  const m = mem(ctx);
  const w = new Uint32Array(64);
  for (let i = 0; i < 16; i++) {
    const o = addr + i * 4;
    w[i] = ((m[o] << 24) | (m[o + 1] << 16) | (m[o + 2] << 8) | m[o + 3]) >>> 0;
  }
  for (let i = 16; i < 64; i++) {
    const x = w[i - 15], y = w[i - 2];
    const s0 = (((x >>> 7) | (x << 25)) ^ ((x >>> 18) | (x << 14)) ^ (x >>> 3)) >>> 0;
    const s1 = (((y >>> 17) | (y << 15)) ^ ((y >>> 19) | (y << 13)) ^ (y >>> 10)) >>> 0;
    w[i] = (w[i - 16] + s0 + w[i - 7] + s1) >>> 0;
  }
  let a = getU32(ctx, c + 72), b = getU32(ctx, c + 76),
    cc = getU32(ctx, c + 80), d = getU32(ctx, c + 84);
  let e = getU32(ctx, c + 88), f = getU32(ctx, c + 92),
    g = getU32(ctx, c + 96), h = getU32(ctx, c + 100);
  for (let i = 0; i < 64; i++) {
    const S1 = (((e >>> 6) | (e << 26)) ^ ((e >>> 11) | (e << 21)) ^ ((e >>> 25) | (e << 7))) >>> 0;
    const ch = ((e & f) ^ (~e & g)) >>> 0;
    const t1 = (h + S1 + ch + SHA256_K[i] + w[i]) >>> 0;
    const S0 = (((a >>> 2) | (a << 30)) ^ ((a >>> 13) | (a << 19)) ^ ((a >>> 22) | (a << 10))) >>> 0;
    const maj = ((a & b) ^ (a & cc) ^ (b & cc)) >>> 0;
    const t2 = (S0 + maj) >>> 0;
    h = g; g = f; f = e; e = (d + t1) >>> 0;
    d = cc; cc = b; b = a; a = (t1 + t2) >>> 0;
  }
  setU32(ctx, c + 72, getU32(ctx, c + 72) + a);
  setU32(ctx, c + 76, getU32(ctx, c + 76) + b);
  setU32(ctx, c + 80, getU32(ctx, c + 80) + cc);
  setU32(ctx, c + 84, getU32(ctx, c + 84) + d);
  setU32(ctx, c + 88, getU32(ctx, c + 88) + e);
  setU32(ctx, c + 92, getU32(ctx, c + 92) + f);
  setU32(ctx, c + 96, getU32(ctx, c + 96) + g);
  setU32(ctx, c + 100, getU32(ctx, c + 100) + h);
}

function sha256Starts(ctx, c, is224) {
  if (is224) return -0x7400; // SHA-224 is not in the shipped configuration
  setU32(ctx, c + 64, 0); setU32(ctx, c + 68, 0);
  const H = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
             0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19];
  for (let i = 0; i < 8; i++) setU32(ctx, c + 72 + i * 4, H[i]);
  return 0;
}

function sha256Update(ctx, c, input, ilen) {
  if (ilen === 0) return 0;
  const m = mem(ctx);
  let left = getU32(ctx, c + 64) & 63;
  const lo = (getU32(ctx, c + 64) + ilen) >>> 0;
  if (lo < getU32(ctx, c + 64)) setU32(ctx, c + 68, getU32(ctx, c + 68) + 1);
  setU32(ctx, c + 64, lo);
  let p = input, n = ilen;
  if (left) {
    const take = Math.min(64 - left, n);
    mem(ctx).set(m.subarray(p, p + take), c + left);
    left += take; p += take; n -= take;
    if (left === 64) sha256Process(ctx, c, c);
  }
  while (n >= 64) { sha256Process(ctx, c, p); p += 64; n -= 64; }
  if (n) mem(ctx).set(mem(ctx).subarray(p, p + n), c);
  return 0;
}

function sha256Finish(ctx, c, output) {
  const totalLo = getU32(ctx, c + 64), totalHi = getU32(ctx, c + 68);
  const left = totalLo & 63;
  const padLen = left < 56 ? 56 - left : 120 - left;
  const m = mem(ctx);
  const pad = new Uint8Array(padLen + 8);
  pad[0] = 0x80;
  const bitLo = (totalLo << 3) >>> 0;
  const bitHi = ((totalHi << 3) | (totalLo >>> 29)) >>> 0;
  for (let i = 0; i < 4; i++) {
    pad[padLen + i] = (bitHi >>> (24 - i * 8)) & 0xff;
    pad[padLen + 4 + i] = (bitLo >>> (24 - i * 8)) & 0xff;
  }
  // Feed the padding through a scratch buffer in guest memory:
  // reuse the context's own buffer tail via update on a copied
  // pad block — simplest correct route: stage the pad in a
  // freshly allocated block.
  const scratch = ctx.alloc(pad.length);
  try {
    mem(ctx).set(pad, scratch);
    sha256Update(ctx, c, scratch, pad.length);
  } finally {
    ctx.free(scratch);
  }
  const out = mem(ctx);
  for (let i = 0; i < 8; i++) {
    const v = getU32(ctx, c + 72 + i * 4);
    out[output + i * 4] = (v >>> 24) & 0xff;
    out[output + i * 4 + 1] = (v >>> 16) & 0xff;
    out[output + i * 4 + 2] = (v >>> 8) & 0xff;
    out[output + i * 4 + 3] = v & 0xff;
  }
  return 0;
}

// ---- SHA-512 --------------------------------------------------
// Context layout (mbedtls/sha512.h, MBEDTLS_SHA384_C off):
//   total[2] u64 @0, state[8] u64 @16, buffer[128] @80.
// 64-bit words are BigInt throughout.

const SHA512_K = [
  0x428a2f98d728ae22n, 0x7137449123ef65cdn, 0xb5c0fbcfec4d3b2fn, 0xe9b5dba58189dbbcn,
  0x3956c25bf348b538n, 0x59f111f1b605d019n, 0x923f82a4af194f9bn, 0xab1c5ed5da6d8118n,
  0xd807aa98a3030242n, 0x12835b0145706fben, 0x243185be4ee4b28cn, 0x550c7dc3d5ffb4e2n,
  0x72be5d74f27b896fn, 0x80deb1fe3b1696b1n, 0x9bdc06a725c71235n, 0xc19bf174cf692694n,
  0xe49b69c19ef14ad2n, 0xefbe4786384f25e3n, 0x0fc19dc68b8cd5b5n, 0x240ca1cc77ac9c65n,
  0x2de92c6f592b0275n, 0x4a7484aa6ea6e483n, 0x5cb0a9dcbd41fbd4n, 0x76f988da831153b5n,
  0x983e5152ee66dfabn, 0xa831c66d2db43210n, 0xb00327c898fb213fn, 0xbf597fc7beef0ee4n,
  0xc6e00bf33da88fc2n, 0xd5a79147930aa725n, 0x06ca6351e003826fn, 0x142929670a0e6e70n,
  0x27b70a8546d22ffcn, 0x2e1b21385c26c926n, 0x4d2c6dfc5ac42aedn, 0x53380d139d95b3dfn,
  0x650a73548baf63den, 0x766a0abb3c77b2a8n, 0x81c2c92e47edaee6n, 0x92722c851482353bn,
  0xa2bfe8a14cf10364n, 0xa81a664bbc423001n, 0xc24b8b70d0f89791n, 0xc76c51a30654be30n,
  0xd192e819d6ef5218n, 0xd69906245565a910n, 0xf40e35855771202an, 0x106aa07032bbd1b8n,
  0x19a4c116b8d2d0c8n, 0x1e376c085141ab53n, 0x2748774cdf8eeb99n, 0x34b0bcb5e19b48a8n,
  0x391c0cb3c5c95a63n, 0x4ed8aa4ae3418acbn, 0x5b9cca4f7763e373n, 0x682e6ff3d6b2b8a3n,
  0x748f82ee5defb2fcn, 0x78a5636f43172f60n, 0x84c87814a1f0ab72n, 0x8cc702081a6439ecn,
  0x90befffa23631e28n, 0xa4506cebde82bde9n, 0xbef9a3f7b2c67915n, 0xc67178f2e372532bn,
  0xca273eceea26619cn, 0xd186b8c721c0c207n, 0xeada7dd6cde0eb1en, 0xf57d4f7fee6ed178n,
  0x06f067aa72176fban, 0x0a637dc5a2c898a6n, 0x113f9804bef90daen, 0x1b710b35131c471bn,
  0x28db77f523047d84n, 0x32caab7b40c72493n, 0x3c9ebe0a15c9bebcn, 0x431d67c49c100d4cn,
  0x4cc5d4becb3e42b6n, 0x597f299cfc657e2an, 0x5fcb6fab3ad6faecn, 0x6c44198c4a475817n,
];
const M64 = (1n << 64n) - 1n;
function rotr64(x, n) { return ((x >> BigInt(n)) | (x << BigInt(64 - n))) & M64; }

function sha512Process(ctx, c, addr) {
  const m = mem(ctx);
  const w = new Array(80);
  for (let i = 0; i < 16; i++) {
    let v = 0n;
    for (let j = 0; j < 8; j++) v = (v << 8n) | BigInt(m[addr + i * 8 + j]);
    w[i] = v;
  }
  for (let i = 16; i < 80; i++) {
    const x = w[i - 15], y = w[i - 2];
    const s0 = rotr64(x, 1) ^ rotr64(x, 8) ^ (x >> 7n);
    const s1 = rotr64(y, 19) ^ rotr64(y, 61) ^ (y >> 6n);
    w[i] = (w[i - 16] + s0 + w[i - 7] + s1) & M64;
  }
  let a = getU64(ctx, c + 16), b = getU64(ctx, c + 24),
    cc = getU64(ctx, c + 32), d = getU64(ctx, c + 40);
  let e = getU64(ctx, c + 48), f = getU64(ctx, c + 56),
    g = getU64(ctx, c + 64), h = getU64(ctx, c + 72);
  for (let i = 0; i < 80; i++) {
    const S1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
    const ch = (e & f) ^ (~e & g);
    const t1 = (h + S1 + ch + SHA512_K[i] + w[i]) & M64;
    const S0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
    const maj = (a & b) ^ (a & cc) ^ (b & cc);
    const t2 = (S0 + maj) & M64;
    h = g; g = f; f = e; e = (d + t1) & M64;
    d = cc; cc = b; b = a; a = (t1 + t2) & M64;
  }
  setU64(ctx, c + 16, getU64(ctx, c + 16) + a);
  setU64(ctx, c + 24, getU64(ctx, c + 24) + b);
  setU64(ctx, c + 32, getU64(ctx, c + 32) + cc);
  setU64(ctx, c + 40, getU64(ctx, c + 40) + d);
  setU64(ctx, c + 48, getU64(ctx, c + 48) + e);
  setU64(ctx, c + 56, getU64(ctx, c + 56) + f);
  setU64(ctx, c + 64, getU64(ctx, c + 64) + g);
  setU64(ctx, c + 72, getU64(ctx, c + 72) + h);
}

function sha512Starts(ctx, c, is384) {
  if (is384) return -0x7e00; // SHA-384 is not in the shipped configuration
  setU64(ctx, c, 0n); setU64(ctx, c + 8, 0n);
  const H = [
    0x6a09e667f3bcc908n, 0xbb67ae8584caa73bn, 0x3c6ef372fe94f82bn, 0xa54ff53a5f1d36f1n,
    0x510e527fade682d1n, 0x9b05688c2b3e6c1fn, 0x1f83d9abfb41bd6bn, 0x5be0cd19137e2179n,
  ];
  for (let i = 0; i < 8; i++) setU64(ctx, c + 16 + i * 8, H[i]);
  return 0;
}

function sha512Update(ctx, c, input, ilen) {
  if (ilen === 0) return 0;
  const m = mem(ctx);
  let left = Number(getU64(ctx, c) & 127n);
  setU64(ctx, c, getU64(ctx, c) + BigInt(ilen));
  if (getU64(ctx, c) < BigInt(ilen)) setU64(ctx, c + 8, getU64(ctx, c + 8) + 1n);
  let p = input, n = ilen;
  if (left) {
    const take = Math.min(128 - left, n);
    mem(ctx).set(m.subarray(p, p + take), c + 80 + left);
    left += take; p += take; n -= take;
    if (left === 128) sha512Process(ctx, c, c + 80);
  }
  while (n >= 128) { sha512Process(ctx, c, p); p += 128; n -= 128; }
  if (n) mem(ctx).set(mem(ctx).subarray(p, p + n), c + 80);
  return 0;
}

function sha512Finish(ctx, c, output) {
  const total = getU64(ctx, c);
  const left = Number(total & 127n);
  const padLen = left < 112 ? 112 - left : 240 - left;
  const pad = new Uint8Array(padLen + 16);
  pad[0] = 0x80;
  const bitLen = total * 8n;
  for (let i = 0; i < 8; i++) {
    pad[padLen + 8 + i] = Number((bitLen >> BigInt(56 - i * 8)) & 0xffn);
  }
  const scratch = ctx.alloc(pad.length);
  try {
    mem(ctx).set(pad, scratch);
    sha512Update(ctx, c, scratch, pad.length);
  } finally {
    ctx.free(scratch);
  }
  const out = mem(ctx);
  for (let i = 0; i < 8; i++) {
    const v = getU64(ctx, c + 16 + i * 8);
    for (let j = 0; j < 8; j++) {
      out[output + i * 8 + j] = Number((v >> BigInt(56 - j * 8)) & 0xffn);
    }
  }
  return 0;
}

// ---- MD5 ------------------------------------------------------
// Context layout (mbedtls/md5.h):
//   total[2] u32 @0, state[4] u32 @8, buffer[64] @24.

const MD5_S = [
  7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
  5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
];
const MD5_K = [
  0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
  0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
  0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
  0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
  0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
  0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
  0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
  0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
];

function md5Process(ctx, c, addr) {
  const m = mem(ctx);
  const x = new Uint32Array(16);
  for (let i = 0; i < 16; i++) {
    const o = addr + i * 4;
    x[i] = (m[o] | (m[o + 1] << 8) | (m[o + 2] << 16) | (m[o + 3] << 24)) >>> 0;
  }
  let a = getU32(ctx, c + 8), b = getU32(ctx, c + 12),
    cc = getU32(ctx, c + 16), d = getU32(ctx, c + 20);
  for (let i = 0; i < 64; i++) {
    let f, g;
    if (i < 16) { f = (b & cc) | (~b & d); g = i; }
    else if (i < 32) { f = (d & b) | (~d & cc); g = (5 * i + 1) % 16; }
    else if (i < 48) { f = b ^ cc ^ d; g = (3 * i + 5) % 16; }
    else { f = cc ^ (b | ~d); g = (7 * i) % 16; }
    const tmp = d;
    d = cc;
    cc = b;
    const sum = (a + f + MD5_K[i] + x[g]) >>> 0;
    const rot = ((sum << MD5_S[i]) | (sum >>> (32 - MD5_S[i]))) >>> 0;
    b = (b + rot) >>> 0;
    a = tmp;
  }
  setU32(ctx, c + 8, getU32(ctx, c + 8) + a);
  setU32(ctx, c + 12, getU32(ctx, c + 12) + b);
  setU32(ctx, c + 16, getU32(ctx, c + 16) + cc);
  setU32(ctx, c + 20, getU32(ctx, c + 20) + d);
}

function md5Starts(ctx, c) {
  setU32(ctx, c, 0); setU32(ctx, c + 4, 0);
  setU32(ctx, c + 8, 0x67452301); setU32(ctx, c + 12, 0xefcdab89);
  setU32(ctx, c + 16, 0x98badcfe); setU32(ctx, c + 20, 0x10325476);
  return 0;
}

function md5Update(ctx, c, input, ilen) {
  if (ilen === 0) return 0;
  const m = mem(ctx);
  let left = getU32(ctx, c) & 63;
  const lo = (getU32(ctx, c) + ilen) >>> 0;
  if (lo < getU32(ctx, c)) setU32(ctx, c + 4, getU32(ctx, c + 4) + 1);
  setU32(ctx, c, lo);
  let p = input, n = ilen;
  if (left) {
    const take = Math.min(64 - left, n);
    mem(ctx).set(m.subarray(p, p + take), c + 24 + left);
    left += take; p += take; n -= take;
    if (left === 64) md5Process(ctx, c, c + 24);
  }
  while (n >= 64) { md5Process(ctx, c, p); p += 64; n -= 64; }
  if (n) mem(ctx).set(mem(ctx).subarray(p, p + n), c + 24);
  return 0;
}

function md5Finish(ctx, c, output) {
  const totalLo = getU32(ctx, c), totalHi = getU32(ctx, c + 4);
  const left = totalLo & 63;
  const padLen = left < 56 ? 56 - left : 120 - left;
  const pad = new Uint8Array(padLen + 8);
  pad[0] = 0x80;
  const bitLo = (totalLo << 3) >>> 0;
  const bitHi = ((totalHi << 3) | (totalLo >>> 29)) >>> 0;
  for (let i = 0; i < 4; i++) {
    pad[padLen + i] = (bitLo >>> (i * 8)) & 0xff;
    pad[padLen + 4 + i] = (bitHi >>> (i * 8)) & 0xff;
  }
  const scratch = ctx.alloc(pad.length);
  try {
    mem(ctx).set(pad, scratch);
    md5Update(ctx, c, scratch, pad.length);
  } finally {
    ctx.free(scratch);
  }
  const out = mem(ctx);
  for (let i = 0; i < 4; i++) {
    const v = getU32(ctx, c + 8 + i * 4);
    out[output + i * 4] = v & 0xff;
    out[output + i * 4 + 1] = (v >>> 8) & 0xff;
    out[output + i * 4 + 2] = (v >>> 16) & 0xff;
    out[output + i * 4 + 3] = (v >>> 24) & 0xff;
  }
  return 0;
}

// One-shot + lifecycle wrappers for the three digests. The
// context structs are zeroed by init; starts (re)initializes
// the state; clone is a struct copy.
function zero(ctx, addr, n) { mem(ctx).fill(0, addr, addr + n); }

const SHA256_SIZE = 104, SHA512_SIZE = 208, MD5_SIZE = 88;

function digestOneShot(starts, update, finish, size) {
  return (ctx, input, ilen, output, flag) => {
    const c = ctx.alloc(size);
    try {
      const rc = starts(ctx, c, flag);
      if (rc !== 0) return rc;
      update(ctx, c, input, ilen);
      return finish(ctx, c, output);
    } finally {
      ctx.free(c);
    }
  };
}

// ---- AES ------------------------------------------------------
// Context layout (mbedtls/aes.h):
//   nr i32 @0, rk_offset u32 @4, buf[68] u32 @8.
// The expanded round keys live in buf from offset 0; rk_offset
// stays 0. S-boxes are derived at evaluation time from the
// GF(2^8) definition rather than transcribed.

const AES_TABLES = (() => {
  const exp = new Uint8Array(512), log = new Uint8Array(256);
  let x = 1;
  for (let i = 0; i < 255; i++) {
    exp[i] = x; log[x] = i;
    const x2 = ((x << 1) & 0xff) ^ (x & 0x80 ? 0x1b : 0); // x * 2
    x = (x2 ^ x) & 0xff;                                 // x * 3, the generator
  }
  for (let i = 255; i < 512; i++) exp[i] = exp[i - 255];
  const mul = (a, b) => (a === 0 || b === 0) ? 0 : exp[log[a] + log[b]];
  const sbox = new Uint8Array(256), invSbox = new Uint8Array(256);
  for (let i = 0; i < 256; i++) {
    const inv = i === 0 ? 0 : exp[255 - log[i]];
    let s = inv;
    s ^= ((inv << 1) | (inv >>> 7)) & 0xff;
    s ^= ((inv << 2) | (inv >>> 6)) & 0xff;
    s ^= ((inv << 3) | (inv >>> 5)) & 0xff;
    s ^= ((inv << 4) | (inv >>> 4)) & 0xff;
    s = (s ^ 0x63) & 0xff;
    sbox[i] = s;
    invSbox[s] = i;
  }
  return { mul, sbox, invSbox };
})();

const AES_RCON = [0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36];

function aesSetkey(ctx, c, key, keybits, forDec) {
  if (keybits !== 128 && keybits !== 192 && keybits !== 256) {
    return ERR_AES_INVALID_KEY;
  }
  const m = mem(ctx);
  const Nk = keybits / 32;
  const Nr = Nk + 6;
  const words = 4 * (Nr + 1);
  const w = new Uint8Array(words * 4);
  for (let i = 0; i < keybits / 8; i++) w[i] = m[key + i];
  const { sbox } = AES_TABLES;
  for (let i = Nk; i < words; i++) {
    let t0 = w[(i - 1) * 4], t1 = w[(i - 1) * 4 + 1],
      t2 = w[(i - 1) * 4 + 2], t3 = w[(i - 1) * 4 + 3];
    if (i % Nk === 0) {
      const r = sbox[t1] ^ AES_RCON[i / Nk - 1];
      const n1 = sbox[t2], n2 = sbox[t3], n3 = sbox[t0];
      t0 = r; t1 = n1; t2 = n2; t3 = n3;
    } else if (Nk > 6 && i % Nk === 4) {
      t0 = sbox[t0]; t1 = sbox[t1]; t2 = sbox[t2]; t3 = sbox[t3];
    }
    w[i * 4] = w[(i - Nk) * 4] ^ t0;
    w[i * 4 + 1] = w[(i - Nk) * 4 + 1] ^ t1;
    w[i * 4 + 2] = w[(i - Nk) * 4 + 2] ^ t2;
    w[i * 4 + 3] = w[(i - Nk) * 4 + 3] ^ t3;
  }
  if (forDec) {
    // Store the schedule in reverse round order, with the
    // middle rounds passed through InvMixColumns, so the
    // decryption path can use the same round structure.
    const { mul } = AES_TABLES;
    const rev = new Uint8Array(words * 4);
    for (let r = 0; r <= Nr; r++) {
      rev.set(w.subarray((Nr - r) * 16, (Nr - r) * 16 + 16), r * 16);
    }
    for (let r = 1; r < Nr; r++) {
      for (let col = 0; col < 4; col++) {
        const o = r * 16 + col * 4;
        const a0 = rev[o], a1 = rev[o + 1], a2 = rev[o + 2], a3 = rev[o + 3];
        rev[o] = mul(a0, 14) ^ mul(a1, 11) ^ mul(a2, 13) ^ mul(a3, 9);
        rev[o + 1] = mul(a0, 9) ^ mul(a1, 14) ^ mul(a2, 11) ^ mul(a3, 13);
        rev[o + 2] = mul(a0, 13) ^ mul(a1, 9) ^ mul(a2, 14) ^ mul(a3, 11);
        rev[o + 3] = mul(a0, 11) ^ mul(a1, 13) ^ mul(a2, 9) ^ mul(a3, 14);
      }
    }
    w.set(rev);
  }
  for (let i = 0; i < words; i++) {
    setU32(ctx, c + 8 + i * 4,
      (w[i * 4] | (w[i * 4 + 1] << 8) | (w[i * 4 + 2] << 16) | (w[i * 4 + 3] << 24)) >>> 0);
  }
  setU32(ctx, c, Nr);
  setU32(ctx, c + 4, 0);
  return 0;
}

function aesRoundKeys(ctx, c) {
  const Nr = getU32(ctx, c);
  const rk = new Uint8Array(16 * (Nr + 1));
  const m = mem(ctx);
  for (let i = 0; i < rk.length; i++) rk[i] = m[c + 8 + i];
  return { Nr, rk };
}

function aesEncryptBlock(ctx, c, input, output) {
  const { Nr, rk } = aesRoundKeys(ctx, c);
  const { sbox, mul } = AES_TABLES;
  const m = mem(ctx);
  const s = new Uint8Array(16);
  for (let i = 0; i < 16; i++) s[i] = m[input + i] ^ rk[i];
  for (let round = 1; round <= Nr; round++) {
    // SubBytes + ShiftRows (state is column-major: s[r + 4c])
    const t = new Uint8Array(16);
    for (let r = 0; r < 4; r++) {
      for (let col = 0; col < 4; col++) {
        t[r + 4 * col] = sbox[s[r + 4 * ((col + r) % 4)]];
      }
    }
    if (round < Nr) {
      for (let col = 0; col < 4; col++) {
        const o = col * 4;
        const a0 = t[o], a1 = t[o + 1], a2 = t[o + 2], a3 = t[o + 3];
        s[o] = mul(a0, 2) ^ mul(a1, 3) ^ a2 ^ a3;
        s[o + 1] = a0 ^ mul(a1, 2) ^ mul(a2, 3) ^ a3;
        s[o + 2] = a0 ^ a1 ^ mul(a2, 2) ^ mul(a3, 3);
        s[o + 3] = mul(a0, 3) ^ a1 ^ a2 ^ mul(a3, 2);
      }
    } else {
      s.set(t);
    }
    for (let i = 0; i < 16; i++) s[i] ^= rk[round * 16 + i];
  }
  mem(ctx).set(s, output);
  return 0;
}

function aesDecryptBlock(ctx, c, input, output) {
  // The context must hold a decryption schedule (setkey_dec):
  // rounds run the inverse cipher against it directly.
  const { Nr, rk } = aesRoundKeys(ctx, c);
  const { invSbox, mul } = AES_TABLES;
  const m = mem(ctx);
  const s = new Uint8Array(16);
  for (let i = 0; i < 16; i++) s[i] = m[input + i] ^ rk[i];
  for (let round = 1; round <= Nr; round++) {
    const t = new Uint8Array(16);
    for (let r = 0; r < 4; r++) {
      for (let col = 0; col < 4; col++) {
        t[r + 4 * col] = invSbox[s[r + 4 * ((col - r + 4) % 4)]];
      }
    }
    if (round < Nr) {
      for (let col = 0; col < 4; col++) {
        const o = col * 4;
        const a0 = t[o], a1 = t[o + 1], a2 = t[o + 2], a3 = t[o + 3];
        s[o] = mul(a0, 14) ^ mul(a1, 11) ^ mul(a2, 13) ^ mul(a3, 9);
        s[o + 1] = mul(a0, 9) ^ mul(a1, 14) ^ mul(a2, 11) ^ mul(a3, 13);
        s[o + 2] = mul(a0, 13) ^ mul(a1, 9) ^ mul(a2, 14) ^ mul(a3, 11);
        s[o + 3] = mul(a0, 11) ^ mul(a1, 13) ^ mul(a2, 9) ^ mul(a3, 14);
      }
    } else {
      s.set(t);
    }
    for (let i = 0; i < 16; i++) s[i] ^= rk[round * 16 + i];
  }
  mem(ctx).set(s, output);
  return 0;
}

function aesCryptEcb(ctx, c, mode, input, output) {
  if (mode === 1) return aesEncryptBlock(ctx, c, input, output);
  if (mode === 0) return aesDecryptBlock(ctx, c, input, output);
  return ERR_AES_BAD_INPUT;
}

function aesCryptCbc(ctx, c, mode, length, iv, input, output) {
  if (length % 16 !== 0) return ERR_AES_BAD_INPUT;
  const m = mem(ctx);
  const chain = new Uint8Array(16);   // the block to XOR with
  const saved = new Uint8Array(16);   // ciphertext block, decrypt path
  for (let i = 0; i < 16; i++) chain[i] = m[iv + i];
  const block = ctx.alloc(16);
  try {
    for (let off = 0; off < length; off += 16) {
      if (mode === 1) {
        for (let i = 0; i < 16; i++) m[block + i] = m[input + off + i] ^ chain[i];
        aesEncryptBlock(ctx, c, block, output + off);
        for (let i = 0; i < 16; i++) chain[i] = m[output + off + i];
      } else {
        for (let i = 0; i < 16; i++) saved[i] = m[input + off + i];
        aesDecryptBlock(ctx, c, input + off, block);
        for (let i = 0; i < 16; i++) m[output + off + i] = m[block + i] ^ chain[i];
        chain.set(saved);
      }
    }
  } finally {
    ctx.free(block);
  }
  // The header contract: iv leaves holding the last ciphertext
  // block (the next call's chain value).
  mem(ctx).set(chain, iv);
  return 0;
}

// Stream modes over the block function. CFB128/OFB keep their
// shift register in the caller's iv; CTR keeps the counter in
// nonce_counter and the keystream remainder in stream_block
// with *nc_off, per the header contracts.
function aesCryptCfb128(ctx, c, mode, length, ivOffPtr, iv, input, output) {
  const m = mem(ctx);
  let ivOff = getU32(ctx, ivOffPtr);
  const block = ctx.alloc(16);
  try {
    for (let i = 0; i < length; i++) {
      if (ivOff === 0) aesEncryptBlock(ctx, c, iv, block);
      const ks = mem(ctx)[block + ivOff];
      if (mode === 1) {
        const v = m[input + i] ^ ks;
        m[output + i] = v;
        m[iv + ivOff] = v;
      } else {
        const v = m[input + i];
        m[output + i] = v ^ ks;
        m[iv + ivOff] = v;
      }
      ivOff = (ivOff + 1) & 15;
    }
  } finally {
    ctx.free(block);
  }
  setU32(ctx, ivOffPtr, ivOff);
  return 0;
}

function aesCryptCfb8(ctx, c, mode, length, iv, input, output) {
  const m = mem(ctx);
  const block = ctx.alloc(16);
  try {
    for (let i = 0; i < length; i++) {
      aesEncryptBlock(ctx, c, iv, block);
      const ks = mem(ctx)[block];
      let v;
      if (mode === 1) { v = m[input + i] ^ ks; m[output + i] = v; }
      else { v = m[input + i]; m[output + i] = v ^ ks; }
      for (let j = 0; j < 15; j++) m[iv + j] = m[iv + j + 1];
      m[iv + 15] = v;
    }
  } finally {
    ctx.free(block);
  }
  return 0;
}

function aesCryptOfb(ctx, c, mode, length, ivOffPtr, iv, input, output) {
  void mode; // OFB is symmetric; the header's mode is ignored upstream too
  const m = mem(ctx);
  let ivOff = getU32(ctx, ivOffPtr);
  for (let i = 0; i < length; i++) {
    if (ivOff === 0) aesEncryptBlock(ctx, c, iv, iv);
    m[output + i] = m[input + i] ^ m[iv + ivOff];
    ivOff = (ivOff + 1) & 15;
  }
  setU32(ctx, ivOffPtr, ivOff);
  return 0;
}

function aesCryptCtr(ctx, c, length, ncOffPtr, nonceCounter, streamBlock, input, output) {
  const m = mem(ctx);
  let ncOff = getU32(ctx, ncOffPtr);
  for (let i = 0; i < length; i++) {
    if (ncOff === 0) {
      aesEncryptBlock(ctx, c, nonceCounter, streamBlock);
      for (let j = 15; j >= 0; j--) {
        m[nonceCounter + j] = (m[nonceCounter + j] + 1) & 0xff;
        if (m[nonceCounter + j] !== 0) break;
      }
    }
    m[output + i] = m[input + i] ^ m[streamBlock + ncOff];
    ncOff = (ncOff + 1) & 15;
  }
  setU32(ctx, ncOffPtr, ncOff);
  return 0;
}

// ---- Base64 ---------------------------------------------------

const B64_ALPHABET = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
const B64_REVERSE = (() => {
  const t = new Int16Array(256).fill(-1);
  for (let i = 0; i < 64; i++) t[B64_ALPHABET.charCodeAt(i)] = i;
  return t;
})();

function base64Encode(ctx, dst, dlen, olenPtr, src, slen) {
  const m = mem(ctx);
  const needed = 4 * Math.ceil(slen / 3);
  setU32(ctx, olenPtr, needed);
  if (dlen < needed) return ERR_B64_TOO_SMALL;
  let o = dst;
  for (let i = 0; i < slen; i += 3) {
    const b0 = m[src + i];
    const b1 = i + 1 < slen ? m[src + i + 1] : 0;
    const b2 = i + 2 < slen ? m[src + i + 2] : 0;
    m[o++] = B64_ALPHABET.charCodeAt(b0 >> 2);
    m[o++] = B64_ALPHABET.charCodeAt(((b0 & 3) << 4) | (b1 >> 4));
    m[o++] = i + 1 < slen ? B64_ALPHABET.charCodeAt(((b1 & 15) << 2) | (b2 >> 6)) : 0x3d;
    m[o++] = i + 2 < slen ? B64_ALPHABET.charCodeAt(b2 & 63) : 0x3d;
  }
  return 0;
}

function base64Decode(ctx, dst, dlen, olenPtr, src, slen) {
  const m = mem(ctx);
  // Validate and count, per the upstream contract: whitespace
  // is skipped, '=' padding ends the input, anything else
  // outside the alphabet is an error.
  let n = 0;         // significant characters
  let pad = 0;
  for (let i = 0; i < slen; i++) {
    const ch = m[src + i];
    if (ch === 0x3d) { pad++; n++; continue; }
    if (ch === 0x20 || ch === 0x09 || ch === 0x0a || ch === 0x0d) continue;
    if (pad > 0 || B64_REVERSE[ch] < 0) {
      setU32(ctx, olenPtr, 0);
      return ERR_B64_BAD_CHAR;
    }
    n++;
  }
  const outLen = Math.floor(n / 4) * 3 - pad;
  setU32(ctx, olenPtr, Math.max(0, outLen));
  if (dlen < outLen) return ERR_B64_TOO_SMALL;
  let acc = 0, accBits = 0, o = dst;
  for (let i = 0; i < slen; i++) {
    const ch = m[src + i];
    if (ch === 0x3d) break;
    if (ch === 0x20 || ch === 0x09 || ch === 0x0a || ch === 0x0d) continue;
    acc = (acc << 6) | B64_REVERSE[ch];
    accBits += 6;
    if (accBits >= 8) {
      accBits -= 8;
      m[o++] = (acc >> accBits) & 0xff;
    }
  }
  return 0;
}

// ---- the generic MD layer -------------------------------------
// mbedtls_md_context_t layout (mbedtls/md.h):
//   md_info ptr @0, md_ctx ptr @4, hmac_ctx ptr @8.
// An md_info block is this implementation's own: the header
// type is opaque to guest code. Layout: type u32 @0, digest
// size u32 @4, block size u32 @8, name ptr @12. The blocks are
// allocated once per type and never freed; every consumer
// reads the fields, so a block allocated by one context is
// valid in all of them.

const MD_TYPE_MD5 = 0x03, MD_TYPE_SHA256 = 0x09, MD_TYPE_SHA512 = 0x0b;
const MD_DEFS = {
  [MD_TYPE_MD5]: {
    name: 'MD5', size: 16, block: 64, ctxSize: MD5_SIZE,
    starts: (ctx, c) => md5Starts(ctx, c),
    update: md5Update, finish: md5Finish,
  },
  [MD_TYPE_SHA256]: {
    name: 'SHA256', size: 32, block: 64, ctxSize: SHA256_SIZE,
    starts: (ctx, c) => sha256Starts(ctx, c, 0),
    update: sha256Update, finish: sha256Finish,
  },
  [MD_TYPE_SHA512]: {
    name: 'SHA512', size: 64, block: 128, ctxSize: SHA512_SIZE,
    starts: (ctx, c) => sha512Starts(ctx, c, 0),
    update: sha512Update, finish: sha512Finish,
  },
};
const infoBlocks = new Map();   // type -> guest address (this evaluation)

function mdInfo(ctx, type) {
  const def = MD_DEFS[type];
  if (!def) return 0;
  let addr = infoBlocks.get(type);
  if (addr) return addr;
  addr = ctx.alloc(16);
  const namePtr = ctx.alloc(def.name.length + 1);
  writeStr(ctx, namePtr, def.name);
  setU32(ctx, addr, type);
  setU32(ctx, addr + 4, def.size);
  setU32(ctx, addr + 8, def.block);
  setU32(ctx, addr + 12, namePtr);
  infoBlocks.set(type, addr);
  return addr;
}

function mdDefOfInfo(ctx, info) {
  if (!info) return null;
  return MD_DEFS[getU32(ctx, info)] ?? null;
}

const HMAC_BLOCK_SIZE = 264;   // ipad[128] @0, opad[128] @128,
                               // inner ctx ptr @256, outer @260

function mdSetup(ctx, c, info, hmac) {
  const def = mdDefOfInfo(ctx, info);
  if (!def) return ERR_MD_BAD_INPUT;
  mdFree(ctx, c);
  const mdCtx = ctx.alloc(def.ctxSize);
  if (!mdCtx) return ERR_MD_ALLOC_FAILED;
  zero(ctx, mdCtx, def.ctxSize);
  setU32(ctx, c, info);
  setU32(ctx, c + 4, mdCtx);
  if (hmac) {
    const h = ctx.alloc(HMAC_BLOCK_SIZE);
    const inner = ctx.alloc(def.ctxSize);
    const outer = ctx.alloc(def.ctxSize);
    if (!h || !inner || !outer) return ERR_MD_ALLOC_FAILED;
    zero(ctx, h, HMAC_BLOCK_SIZE);
    setU32(ctx, h + 256, inner);
    setU32(ctx, h + 260, outer);
    setU32(ctx, c + 8, h);
  }
  return 0;
}

function mdFree(ctx, c) {
  const mdCtx = getU32(ctx, c + 4);
  if (mdCtx) ctx.free(mdCtx);
  const h = getU32(ctx, c + 8);
  if (h) {
    ctx.free(getU32(ctx, h + 256));
    ctx.free(getU32(ctx, h + 260));
    ctx.free(h);
  }
  zero(ctx, c, 12);
}

function mdHmacStarts(ctx, c, key, keylen) {
  const info = getU32(ctx, c);
  const def = mdDefOfInfo(ctx, info);
  const h = getU32(ctx, c + 8);
  if (!def || !h) return ERR_MD_BAD_INPUT;
  const m = mem(ctx);
  const keyBlock = new Uint8Array(def.block);
  if (keylen > def.block) {
    const tmp = ctx.alloc(def.ctxSize);
    try {
      def.starts(ctx, tmp);
      def.update(ctx, tmp, key, keylen);
      const dig = ctx.alloc(def.size);
      try {
        def.finish(ctx, tmp, dig);
        for (let i = 0; i < def.size; i++) keyBlock[i] = mem(ctx)[dig + i];
      } finally { ctx.free(dig); }
    } finally { ctx.free(tmp); }
  } else {
    for (let i = 0; i < keylen; i++) keyBlock[i] = m[key + i];
  }
  const ipad = new Uint8Array(def.block), opad = new Uint8Array(def.block);
  for (let i = 0; i < def.block; i++) {
    ipad[i] = keyBlock[i] ^ 0x36;
    opad[i] = keyBlock[i] ^ 0x5c;
  }
  mem(ctx).set(ipad, h);
  mem(ctx).set(opad, h + 128);
  const inner = getU32(ctx, h + 256);
  def.starts(ctx, inner);
  const scratch = ctx.alloc(def.block);
  try {
    mem(ctx).set(ipad, scratch);
    def.update(ctx, inner, scratch, def.block);
  } finally { ctx.free(scratch); }
  return 0;
}

function mdHmacFinish(ctx, c, output) {
  const info = getU32(ctx, c);
  const def = mdDefOfInfo(ctx, info);
  const h = getU32(ctx, c + 8);
  if (!def || !h) return ERR_MD_BAD_INPUT;
  const inner = getU32(ctx, h + 256), outer = getU32(ctx, h + 260);
  const tmp = ctx.alloc(def.size);
  const padScratch = ctx.alloc(def.block);
  try {
    def.finish(ctx, inner, tmp);
    def.starts(ctx, outer);
    mem(ctx).set(mem(ctx).subarray(h + 128, h + 128 + def.block), padScratch);
    def.update(ctx, outer, padScratch, def.block);
    def.update(ctx, outer, tmp, def.size);
    return def.finish(ctx, outer, output);
  } finally {
    ctx.free(tmp);
    ctx.free(padScratch);
  }
}

function mdHmacReset(ctx, c) {
  const info = getU32(ctx, c);
  const def = mdDefOfInfo(ctx, info);
  const h = getU32(ctx, c + 8);
  if (!def || !h) return ERR_MD_BAD_INPUT;
  const inner = getU32(ctx, h + 256);
  def.starts(ctx, inner);
  const scratch = ctx.alloc(def.block);
  try {
    mem(ctx).set(mem(ctx).subarray(h, h + def.block), scratch);
    def.update(ctx, inner, scratch, def.block);
  } finally { ctx.free(scratch); }
  return 0;
}

// ---- export table ----------------------------------------------

const I32 = 'i32';
function sig(params, results) { return { params, results }; }

const EXPORTS = [
  // SHA-256
  ['mbedtls_sha256_init', sig([I32], []), (ctx, c) => { zero(ctx, c, SHA256_SIZE); }],
  ['mbedtls_sha256_free', sig([I32], []), (ctx, c) => { zero(ctx, c, SHA256_SIZE); }],
  ['mbedtls_sha256_starts', sig([I32, I32], [I32]), (ctx, c, is224) => sha256Starts(ctx, c, is224)],
  ['mbedtls_sha256_update', sig([I32, I32, I32], [I32]), (ctx, c, i, l) => sha256Update(ctx, c, i, l)],
  ['mbedtls_sha256_finish', sig([I32, I32], [I32]), (ctx, c, o) => sha256Finish(ctx, c, o)],
  ['mbedtls_sha256_clone', sig([I32, I32], []), (ctx, dst, src) => {
    mem(ctx).set(mem(ctx).subarray(src, src + SHA256_SIZE), dst);
  }],
  ['mbedtls_sha256', sig([I32, I32, I32, I32], [I32]),
    digestOneShot(sha256Starts, sha256Update, sha256Finish, SHA256_SIZE)],
  // SHA-512
  ['mbedtls_sha512_init', sig([I32], []), (ctx, c) => { zero(ctx, c, SHA512_SIZE); }],
  ['mbedtls_sha512_free', sig([I32], []), (ctx, c) => { zero(ctx, c, SHA512_SIZE); }],
  ['mbedtls_sha512_starts', sig([I32, I32], [I32]), (ctx, c, is384) => sha512Starts(ctx, c, is384)],
  ['mbedtls_sha512_update', sig([I32, I32, I32], [I32]), (ctx, c, i, l) => sha512Update(ctx, c, i, l)],
  ['mbedtls_sha512_finish', sig([I32, I32], [I32]), (ctx, c, o) => sha512Finish(ctx, c, o)],
  ['mbedtls_sha512_clone', sig([I32, I32], []), (ctx, dst, src) => {
    mem(ctx).set(mem(ctx).subarray(src, src + SHA512_SIZE), dst);
  }],
  ['mbedtls_sha512', sig([I32, I32, I32, I32], [I32]),
    digestOneShot(sha512Starts, sha512Update, sha512Finish, SHA512_SIZE)],
  // MD5
  ['mbedtls_md5_init', sig([I32], []), (ctx, c) => { zero(ctx, c, MD5_SIZE); }],
  ['mbedtls_md5_free', sig([I32], []), (ctx, c) => { zero(ctx, c, MD5_SIZE); }],
  ['mbedtls_md5_starts', sig([I32], [I32]), (ctx, c) => md5Starts(ctx, c)],
  ['mbedtls_md5_update', sig([I32, I32, I32], [I32]), (ctx, c, i, l) => md5Update(ctx, c, i, l)],
  ['mbedtls_md5_finish', sig([I32, I32], [I32]), (ctx, c, o) => md5Finish(ctx, c, o)],
  ['mbedtls_md5_clone', sig([I32, I32], []), (ctx, dst, src) => {
    mem(ctx).set(mem(ctx).subarray(src, src + MD5_SIZE), dst);
  }],
  ['mbedtls_md5', sig([I32, I32, I32], [I32]),
    (ctx, i, l, o) => digestOneShot(md5Starts, md5Update, md5Finish, MD5_SIZE)(ctx, i, l, o, 0)],
  // AES
  ['mbedtls_aes_init', sig([I32], []), (ctx, c) => { zero(ctx, c, 280); }],
  ['mbedtls_aes_free', sig([I32], []), (ctx, c) => { zero(ctx, c, 280); }],
  ['mbedtls_aes_setkey_enc', sig([I32, I32, I32], [I32]), (ctx, c, k, bits) => aesSetkey(ctx, c, k, bits, false)],
  ['mbedtls_aes_setkey_dec', sig([I32, I32, I32], [I32]), (ctx, c, k, bits) => aesSetkey(ctx, c, k, bits, true)],
  ['mbedtls_aes_crypt_ecb', sig([I32, I32, I32, I32], [I32]), (ctx, c, mode, i, o) => aesCryptEcb(ctx, c, mode, i, o)],
  ['mbedtls_aes_crypt_cbc', sig([I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, c, mode, len, iv, i, o) => aesCryptCbc(ctx, c, mode, len, iv, i, o)],
  ['mbedtls_aes_crypt_cfb128', sig([I32, I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, c, mode, len, off, iv, i, o) => aesCryptCfb128(ctx, c, mode, len, off, iv, i, o)],
  ['mbedtls_aes_crypt_cfb8', sig([I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, c, mode, len, iv, i, o) => aesCryptCfb8(ctx, c, mode, len, iv, i, o)],
  ['mbedtls_aes_crypt_ofb', sig([I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, c, mode, len, off, iv, i, o) => aesCryptOfb(ctx, c, mode, len, off, iv, i, o)],
  ['mbedtls_aes_crypt_ctr', sig([I32, I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, c, len, off, nc, sb, i, o) => aesCryptCtr(ctx, c, len, off, nc, sb, i, o)],
  // Base64
  ['mbedtls_base64_encode', sig([I32, I32, I32, I32, I32], [I32]),
    (ctx, d, dl, ol, s, sl) => base64Encode(ctx, d, dl, ol, s, sl)],
  ['mbedtls_base64_decode', sig([I32, I32, I32, I32, I32], [I32]),
    (ctx, d, dl, ol, s, sl) => base64Decode(ctx, d, dl, ol, s, sl)],
  // MD layer
  ['mbedtls_md_init', sig([I32], []), (ctx, c) => { zero(ctx, c, 12); }],
  ['mbedtls_md_free', sig([I32], []), (ctx, c) => { mdFree(ctx, c); }],
  ['mbedtls_md_info_from_type', sig([I32], [I32]), (ctx, type) => mdInfo(ctx, type)],
  ['mbedtls_md_info_from_string', sig([I32], [I32]), (ctx, strPtr) => {
    const want = readStr(ctx, strPtr).toUpperCase().replace('-', '');
    for (const [type, def] of Object.entries(MD_DEFS)) {
      if (def.name === want) return mdInfo(ctx, Number(type));
    }
    return 0;
  }],
  ['mbedtls_md_info_from_ctx', sig([I32], [I32]), (ctx, c) => getU32(ctx, c)],
  ['mbedtls_md_get_type', sig([I32], [I32]), (ctx, info) => (info ? getU32(ctx, info) : 0)],
  ['mbedtls_md_get_size', sig([I32], [I32]), (ctx, info) => (info ? getU32(ctx, info + 4) : 0)],
  ['mbedtls_md_get_name', sig([I32], [I32]), (ctx, info) => (info ? getU32(ctx, info + 12) : 0)],
  ['mbedtls_md_setup', sig([I32, I32, I32], [I32]), (ctx, c, info, hmac) => mdSetup(ctx, c, info, hmac)],
  ['mbedtls_md_starts', sig([I32], [I32]), (ctx, c) => {
    const def = mdDefOfInfo(ctx, getU32(ctx, c));
    return def ? def.starts(ctx, getU32(ctx, c + 4)) : ERR_MD_BAD_INPUT;
  }],
  ['mbedtls_md_update', sig([I32, I32, I32], [I32]), (ctx, c, i, l) => {
    const def = mdDefOfInfo(ctx, getU32(ctx, c));
    return def ? def.update(ctx, getU32(ctx, c + 4), i, l) : ERR_MD_BAD_INPUT;
  }],
  ['mbedtls_md_finish', sig([I32, I32], [I32]), (ctx, c, o) => {
    const def = mdDefOfInfo(ctx, getU32(ctx, c));
    return def ? def.finish(ctx, getU32(ctx, c + 4), o) : ERR_MD_BAD_INPUT;
  }],
  ['mbedtls_md', sig([I32, I32, I32, I32], [I32]), (ctx, info, i, l, o) => {
    const def = mdDefOfInfo(ctx, info);
    if (!def) return ERR_MD_BAD_INPUT;
    const tmp = ctx.alloc(12);
    try {
      zero(ctx, tmp, 12);
      const rc = mdSetup(ctx, tmp, info, 0);
      if (rc !== 0) return rc;
      def.starts(ctx, getU32(ctx, tmp + 4));
      def.update(ctx, getU32(ctx, tmp + 4), i, l);
      return def.finish(ctx, getU32(ctx, tmp + 4), o);
    } finally {
      mdFree(ctx, tmp);
      ctx.free(tmp);
    }
  }],
  ['mbedtls_md_clone', sig([I32, I32], [I32]), (ctx, dst, src) => {
    const def = mdDefOfInfo(ctx, getU32(ctx, src));
    if (!def || getU32(ctx, dst) !== getU32(ctx, src)) return ERR_MD_BAD_INPUT;
    if (!getU32(ctx, dst + 4)) {
      const rc = mdSetup(ctx, dst, getU32(ctx, src), getU32(ctx, src + 8) ? 1 : 0);
      if (rc !== 0) return rc;
    }
    mem(ctx).set(
      mem(ctx).subarray(getU32(ctx, src + 4), getU32(ctx, src + 4) + def.ctxSize),
      getU32(ctx, dst + 4));
    const sh = getU32(ctx, src + 8), dh = getU32(ctx, dst + 8);
    if (sh && dh) {
      mem(ctx).set(mem(ctx).subarray(sh, sh + 256), dh);
      const sInner = getU32(ctx, sh + 256), sOuter = getU32(ctx, sh + 260);
      const dInner = getU32(ctx, dh + 256), dOuter = getU32(ctx, dh + 260);
      mem(ctx).set(mem(ctx).subarray(sInner, sInner + def.ctxSize), dInner);
      mem(ctx).set(mem(ctx).subarray(sOuter, sOuter + def.ctxSize), dOuter);
    }
    return 0;
  }],
  ['mbedtls_md_hmac_starts', sig([I32, I32, I32], [I32]), (ctx, c, k, kl) => mdHmacStarts(ctx, c, k, kl)],
  ['mbedtls_md_hmac_update', sig([I32, I32, I32], [I32]), (ctx, c, i, l) => {
    const def = mdDefOfInfo(ctx, getU32(ctx, c));
    const h = getU32(ctx, c + 8);
    return def && h ? def.update(ctx, getU32(ctx, h + 256), i, l) : ERR_MD_BAD_INPUT;
  }],
  ['mbedtls_md_hmac_finish', sig([I32, I32], [I32]), (ctx, c, o) => mdHmacFinish(ctx, c, o)],
  ['mbedtls_md_hmac_reset', sig([I32], [I32]), (ctx, c) => mdHmacReset(ctx, c)],
  ['mbedtls_md_hmac', sig([I32, I32, I32, I32, I32, I32], [I32]),
    (ctx, info, k, kl, i, l, o) => {
      const def = mdDefOfInfo(ctx, info);
      if (!def) return ERR_MD_BAD_INPUT;
      const tmp = ctx.alloc(12);
      try {
        zero(ctx, tmp, 12);
        let rc = mdSetup(ctx, tmp, info, 1);
        if (rc !== 0) return rc;
        rc = mdHmacStarts(ctx, tmp, k, kl);
        if (rc !== 0) return rc;
        rc = mdHmacUpdateEntry(ctx, tmp, i, l);
        if (rc !== 0) return rc;
        return mdHmacFinish(ctx, tmp, o);
      } finally {
        mdFree(ctx, tmp);
        ctx.free(tmp);
      }
    }],
];

function mdHmacUpdateEntry(ctx, c, i, l) {
  const def = mdDefOfInfo(ctx, getU32(ctx, c));
  const h = getU32(ctx, c + 8);
  return def && h ? def.update(ctx, getU32(ctx, h + 256), i, l) : ERR_MD_BAD_INPUT;
}

const out = { __exports: EXPORTS.map(([name, s, , a]) => ({ name, sig: s, ...(a ? { async: true } : {}) })) };
for (const [name, , fn] of EXPORTS) out[name] = fn;
module.exports = out;
