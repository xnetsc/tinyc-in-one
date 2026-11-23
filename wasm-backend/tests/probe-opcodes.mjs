// Probe: verify wasm SIMD (0xFD) opcode numbers against Node's engine.
// For each named op, try the candidate opcode numbers; a candidate
// counts as verified only if the module validates AND the numeric
// result matches the expected lane values.
import assert from 'node:assert';

function uleb(n) { const b = []; do { let c = n & 0x7f; n >>>= 7; if (n) c |= 0x80; b.push(c); } while (n); return b; }
function section(id, payload) { return [id, ...uleb(payload.length), ...payload]; }
function name(s) { return [s.length, ...[...s].map(c => c.charCodeAt(0))]; }

function buildModule(body) {
  const types = section(1, [1, 0x60, 0, 0]);
  const funcs = section(3, [1, 0]);
  const mem = section(5, [1, 0x00, 1]);
  const exps = section(7, [2, ...name('run'), 0x00, 0, ...name('memory'), 0x02, 0]);
  const entry = [0x00, ...body, 0x0b];
  const code = section(10, [1, ...uleb(entry.length), ...entry]);
  return new Uint8Array([0, 0x61, 0x73, 0x6d, 1, 0, 0, 0, ...types, ...funcs, ...mem, ...exps, ...code]);
}

const VC = (bytes) => [0xfd, 12, ...bytes];                    // v128.const
const VSTORE0 = [0xfd, 11, 0x00, 0x00];                        // v128.store @0 (addr pushed before)
const binop = (opc, A, B) => [0x41, 0x00, ...VC(A), ...VC(B), 0xfd, ...uleb(opc), ...VSTORE0];
const unop = (opc, A) => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), ...VSTORE0];
const shiftop = (opc, A, n) => [0x41, 0x00, ...VC(A), 0x41, n, 0xfd, ...uleb(opc), ...VSTORE0];
const splat32 = (opc, x) => [0x41, 0x00, 0x41, x, 0xfd, ...uleb(opc), ...VSTORE0];
const splatf32 = (opc, x) => { const b = new Uint8Array(4); new DataView(b.buffer).setFloat32(0, x, true); return [0x41, 0x00, 0x43, ...b, 0xfd, ...uleb(opc), ...VSTORE0]; };
const splatf64 = (opc, x) => { const b = new Uint8Array(8); new DataView(b.buffer).setFloat64(0, x, true); return [0x41, 0x00, 0x44, ...b, 0xfd, ...uleb(opc), ...VSTORE0]; };
const extractI = (opc, A, lane) => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), lane, 0x36, 0x00, 0x00]; // i32.store
const extract64 = (opc, A, lane) => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), lane, 0x37, 0x00, 0x00]; // i64.store
const extractF32 = (opc, A, lane) => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), lane, 0x38, 0x00, 0x00];
const extractF64 = (opc, A, lane) => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), lane, 0x39, 0x00, 0x00];
const replaceI = (opc, A, lane, x) => [0x41, 0x00, ...VC(A), 0x41, x, 0xfd, ...uleb(opc), lane, ...VSTORE0];
const scalarOf = (opc, A) => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), 0x36, 0x00, 0x00]; // v128->i32, i32.store

const i8 = a => a.map(x => x & 0xff);
const i16 = a => a.flatMap(x => { const b = new Uint8Array(2); new DataView(b.buffer).setInt16(0, x, true); return [...b]; });
const i32 = a => a.flatMap(x => { const b = new Uint8Array(4); new DataView(b.buffer).setInt32(0, x, true); return [...b]; });
const i64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setBigInt64(0, BigInt(x), true); return [...b]; });
const f32 = a => a.flatMap(x => { const b = new Uint8Array(4); new DataView(b.buffer).setFloat32(0, x, true); return [...b]; });
const f64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setFloat64(0, x, true); return [...b]; });
const rd32 = (mem, off) => new DataView(mem.buffer).getInt32(off, true);
const rd64 = (mem, off) => new DataView(mem.buffer).getBigInt64(off, true);

// [name, candidates, makeBody(opc), expected(mem)->bool]
const CASES = [];
const UU = (name, cands, makeBody, check) => CASES.push([name, cands, makeBody, check]);

// ---- integer binops: add/sub/mul ----
function bin(name, cands, A, Bx, exp) {
  CASES.push([name, cands, opc => binop(opc, A, Bx), m => [...m.slice(0, 16)].join() === exp.join()]);
}
function un(name, cands, A, exp) {
  CASES.push([name, cands, opc => unop(opc, A), m => [...m.slice(0, 16)].join() === exp.join()]);
}
bin('i8x16.add', [110], i8([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16]), i8(Array(16).fill(1)), i8([2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17]));
bin('i8x16.sub', [113], i8(Array(16).fill(10)), i8(Array(16).fill(3)), i8(Array(16).fill(7)));
bin('i8x16.mul', [117], i8(Array(16).fill(6)), i8(Array(16).fill(7)), i8(Array(16).fill(42)));
bin('i16x8.add', [142], i16([1, 2, 3, 4, 5, 6, 7, 8]), i16(Array(8).fill(10)), i16([11, 12, 13, 14, 15, 16, 17, 18]));
bin('i16x8.sub', [145], i16(Array(8).fill(10)), i16(Array(8).fill(3)), i16(Array(8).fill(7)));
bin('i16x8.mul', [149], i16(Array(8).fill(6)), i16(Array(8).fill(7)), i16(Array(8).fill(42)));
bin('i32x4.add', [174], i32([1, 2, 3, 4]), i32([10, 20, 30, 40]), i32([11, 22, 33, 44]));
bin('i32x4.sub', [177], i32([10, 20, 30, 40]), i32([1, 2, 3, 4]), i32([9, 18, 27, 36]));
bin('i32x4.mul', [181], i32([6, 7, 8, 9]), i32([7, 6, 5, 4]), i32([42, 42, 40, 36]));
bin('i64x2.add', [192, 206], i64([100, 200]), i64([23, 45]), i64([123, 245]));
bin('i64x2.sub', [195, 209], i64([100, 200]), i64([23, 45]), i64([77, 155]));
bin('i64x2.mul', [213], i64([6, 7]), i64([7, 6]), i64([42, 42]));
// ---- float binops ----
bin('f32x4.add', [228], f32([1.5, 2.5, 3.5, 4.5]), f32([0.5, 0.5, 0.5, 0.5]), f32([2, 3, 4, 5]));
bin('f32x4.sub', [229], f32([1.5, 2.5, 3.5, 4.5]), f32([0.5, 0.5, 0.5, 0.5]), f32([1, 2, 3, 4]));
bin('f32x4.mul', [230], f32([2, 3, 4, 5]), f32([3, 3, 3, 3]), f32([6, 9, 12, 15]));
bin('f32x4.div', [231], f32([6, 9, 12, 15]), f32([3, 3, 3, 3]), f32([2, 3, 4, 5]));
bin('f32x4.min', [232], f32([1, 5, 3, 8]), f32([2, 4, 3, 9]), f32([1, 4, 3, 8]));
bin('f32x4.max', [233], f32([1, 5, 3, 8]), f32([2, 4, 3, 9]), f32([2, 5, 3, 9]));
bin('f64x2.add', [240, 262, 258], f64([1.5, 2.5]), f64([0.5, 0.25]), f64([2, 2.75]));
bin('f64x2.sub', [241, 263, 259], f64([1.5, 2.5]), f64([0.5, 0.25]), f64([1, 2.25]));
bin('f64x2.mul', [242, 264, 260], f64([2, 3]), f64([4, 5]), f64([8, 15]));
bin('f64x2.div', [243, 265, 261], f64([8, 15]), f64([4, 5]), f64([2, 3]));
bin('f64x2.min', [244, 266], f64([1, 5]), f64([2, 4]), f64([1, 4]));
bin('f64x2.max', [245, 267], f64([1, 5]), f64([2, 4]), f64([2, 5]));
// ---- saturating ----
bin('i8x16.add_sat_s', [111], i8(Array(16).fill(100)), i8(Array(16).fill(100)), i8(Array(16).fill(127)));
bin('i8x16.add_sat_u', [112], i8(Array(16).fill(200)), i8(Array(16).fill(100)), i8(Array(16).fill(255)));
bin('i8x16.sub_sat_s', [114], i8(Array(16).fill(-100)), i8(Array(16).fill(100)), i8(Array(16).fill(-128)));
bin('i8x16.sub_sat_u', [115], i8(Array(16).fill(3)), i8(Array(16).fill(10)), i8(Array(16).fill(0)));
bin('i16x8.add_sat_s', [143], i16(Array(8).fill(30000)), i16(Array(8).fill(10000)), i16(Array(8).fill(32767)));
bin('i16x8.add_sat_u', [144], i16(Array(8).fill(60000)), i16(Array(8).fill(10000)), i16(Array(8).fill(65535)));
bin('i16x8.sub_sat_s', [146], i16(Array(8).fill(-30000)), i16(Array(8).fill(10000)), i16(Array(8).fill(-32768)));
bin('i16x8.sub_sat_u', [147], i16(Array(8).fill(3)), i16(Array(8).fill(10)), i16(Array(8).fill(0)));
// ---- min/max int ----
bin('i8x16.min_s', [118], i8([-5, 3, 100, -100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([-3, 5, 90, -90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([-5, 3, 90, -100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
bin('i8x16.min_u', [119], i8([200, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([100, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([100, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
bin('i8x16.max_s', [120], i8([-5, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([-3, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([-3, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
bin('i8x16.max_u', [121], i8([200, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([100, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([200, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
bin('i8x16.avgr_u', [123], i8(Array(16).fill(10)), i8(Array(16).fill(21)), i8(Array(16).fill(16)));
bin('i16x8.min_s', [150], i16([-5, 3, 0, 0, 0, 0, 0, 0]), i16([-3, 5, 0, 0, 0, 0, 0, 0]), i16([-5, 3, 0, 0, 0, 0, 0, 0]));
bin('i16x8.min_u', [151], i16([60000, 3, 0, 0, 0, 0, 0, 0]), i16([100, 5, 0, 0, 0, 0, 0, 0]), i16([100, 3, 0, 0, 0, 0, 0, 0]));
bin('i16x8.max_s', [152], i16([-5, 3, 0, 0, 0, 0, 0, 0]), i16([-3, 5, 0, 0, 0, 0, 0, 0]), i16([-3, 5, 0, 0, 0, 0, 0, 0]));
bin('i16x8.max_u', [153], i16([60000, 3, 0, 0, 0, 0, 0, 0]), i16([100, 5, 0, 0, 0, 0, 0, 0]), i16([60000, 5, 0, 0, 0, 0, 0, 0]));
bin('i16x8.avgr_u', [155], i16(Array(8).fill(10)), i16(Array(8).fill(21)), i16(Array(8).fill(16)));
bin('i32x4.min_s', [182], i32([-5, 3, 0, 0]), i32([-3, 5, 0, 0]), i32([-5, 3, 0, 0]));
bin('i32x4.min_u', [183], i32([4000000000, 3, 0, 0]), i32([100, 5, 0, 0]), i32([100, 3, 0, 0]));
bin('i32x4.max_s', [184], i32([-5, 3, 0, 0]), i32([-3, 5, 0, 0]), i32([-3, 5, 0, 0]));
bin('i32x4.max_u', [185], i32([4000000000, 3, 0, 0]), i32([100, 5, 0, 0]), i32([4000000000, 5, 0, 0]));
// ---- comparisons ----
bin('i8x16.eq', [35], i8([1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([1, 9, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([255, 0, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
bin('i8x16.lt_s', [37], i8([-1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
bin('i16x8.eq', [45], i16([1, 2, 0, 0, 0, 0, 0, 0]), i16([1, 9, 0, 0, 0, 0, 0, 0]), i16([-1, 0, 0, 0, 0, 0, 0, 0]));
bin('i32x4.eq', [55], i32([1, 2, 3, 4]), i32([1, 9, 3, 4]), i32([-1, 0, -1, -1]));
bin('i32x4.gt_s', [59], i32([5, 2, 3, 4]), i32([1, 9, 3, 4]), i32([-1, 0, 0, 0]));
bin('i64x2.eq', [65, 214], i64([1, 2]), i64([1, 9]), i64([-1, 0]));
bin('i64x2.lt_s', [67, 216], i64([-1, 2]), i64([1, 9]), i64([-1, -1]));
bin('f32x4.eq', [71, 65], f32([1, 2, 3, 4]), f32([1, 9, 3, 4]), i32([-1, 0, -1, -1]));
bin('f32x4.lt', [73, 67], f32([1, 20, 3, 4]), f32([2, 9, 3, 4]), i32([-1, 0, 0, 0]));
bin('f64x2.eq', [77, 83], f64([1, 2]), f64([1, 9]), i64([-1, 0]));
bin('f64x2.lt', [79, 85], f64([1, 20]), f64([2, 9]), i64([-1, 0]));
// ---- bit ops ----
bin('v128.and', [84, 85], i8(Array(16).fill(0x3c)), i8(Array(16).fill(0x1e)), i8(Array(16).fill(0x1c)));
bin('v128.or', [86, 87], i8(Array(16).fill(0x30)), i8(Array(16).fill(0x0f)), i8(Array(16).fill(0x3f)));
bin('v128.xor', [87, 88], i8(Array(16).fill(0x33)), i8(Array(16).fill(0x0f)), i8(Array(16).fill(0x3c)));
bin('v128.andnot', [85, 86], i8(Array(16).fill(0x3c)), i8(Array(16).fill(0x1e)), i8(Array(16).fill(0x22)));
bin('i8x16.swizzle', [14], i8([10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]), i8([3, 2, 1, 0, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([13, 12, 11, 10, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10]));
bin('i8x16.narrow_i16x8_s', [101], i16([1000, -1000, 5, -5, 0, 0, 0, 0]), i16([2000, -2000, 6, -6, 0, 0, 0, 0]), i8([127, -128, 5, -5, 0, 0, 0, 0, 127, -128, 6, -6, 0, 0, 0, 0]));
bin('i8x16.narrow_i16x8_u', [102], i16([1000, -5, 5, 0, 0, 0, 0, 0]), i16([0, 0, 0, 0, 0, 0, 0, 0]), i8([255, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
bin('i16x8.narrow_i32x4_s', [133], i32([100000, -100000, 5, -5]), i32([70000, -70000, 6, -6]), i16([32767, -32768, 5, -5, 32767, -32768, 6, -6]));
bin('i16x8.narrow_i32x4_u', [134], i32([100000, -5, 5, 0]), i32([0, 0, 0, 0]), i16([65535, 0, 5, 0, 0, 0, 0, 0]));
bin('i32x4.dot_i16x8_s', [186], i16([1, 2, 3, 4, 0, 0, 0, 0]), i16([5, 6, 7, 8, 0, 0, 0, 0]), i32([17, 53, 0, 0]));
// ---- unary ----
un('i8x16.abs', [96], i8([-5, 5, -128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([5, 5, -128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
un('i8x16.neg', [97], i8([5, -5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([-5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
un('i8x16.popcnt', [98], i8([0xff, 0x0f, 0x00, 0x81, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([8, 4, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
un('i16x8.abs', [128], i16([-5, 5, 0, 0, 0, 0, 0, 0]), i16([5, 5, 0, 0, 0, 0, 0, 0]));
un('i16x8.neg', [129], i16([5, -5, 0, 0, 0, 0, 0, 0]), i16([-5, 5, 0, 0, 0, 0, 0, 0]));
un('i32x4.abs', [160], i32([-5, 5, 0, 0]), i32([5, 5, 0, 0]));
un('i32x4.neg', [161], i32([5, -5, 0, 0]), i32([-5, 5, 0, 0]));
un('i64x2.abs', [192, 224], i64([-5, 5]), i64([5, 5]));
un('i64x2.neg', [193, 225], i64([5, -5]), i64([-5, 5]));
un('f32x4.abs', [236, 248], f32([-1.5, 2.5, 0, 0]), f32([1.5, 2.5, 0, 0]));
un('f32x4.neg', [237, 249], f32([1.5, -2.5, 0, 0]), f32([-1.5, 2.5, 0, 0]));
un('f32x4.sqrt', [239, 251], f32([4, 9, 16, 25]), f32([2, 3, 4, 5]));
un('f32x4.ceil', [224, 260], f32([1.5, -1.5, 0, 0]), f32([2, -1, 0, 0]));
un('f32x4.floor', [225, 261], f32([1.5, -1.5, 0, 0]), f32([1, -2, 0, 0]));
un('f32x4.trunc', [226, 262], f32([1.5, -1.5, 0, 0]), f32([1, -1, 0, 0]));
un('f32x4.nearest', [227, 263], f32([1.5, 2.5, 0, 0]), f32([2, 2, 0, 0]));
un('f64x2.abs', [268, 280], f64([-1.5, 2.5]), f64([1.5, 2.5]));
un('f64x2.neg', [269, 281], f64([1.5, -2.5]), f64([-1.5, 2.5]));
un('f64x2.sqrt', [271, 283], f64([4, 9]), f64([2, 3]));
un('f64x2.ceil', [256, 292], f64([1.5, -1.5]), f64([2, -1]));
un('f64x2.floor', [257, 293], f64([1.5, -1.5]), f64([1, -2]));
un('f64x2.trunc', [258, 294], f64([1.5, -1.5]), f64([1, -1]));
un('f64x2.nearest', [259, 295], f64([1.5, 2.5]), f64([2, 2]));
// extends (unary, shape-changing)
un('i16x8.extend_low_i8x16_s', [135], i8([-1, 2, -3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i16([-1, 2, -3, 4, 0, 0, 0, 0]));
un('i16x8.extend_high_i8x16_s', [136], i8([0, 0, 0, 0, 0, 0, 0, 0, -1, 2, -3, 4, 0, 0, 0, 0]), i16([-1, 2, -3, 4, 0, 0, 0, 0]));
un('i16x8.extend_low_i8x16_u', [137], i8([255, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i16([255, 2, 0, 0, 0, 0, 0, 0]));
un('i16x8.extend_high_i8x16_u', [138], i8([0, 0, 0, 0, 0, 0, 0, 0, 255, 2, 0, 0, 0, 0, 0, 0]), i16([255, 2, 0, 0, 0, 0, 0, 0]));
un('i32x4.extend_low_i16x8_s', [167], i16([-1, 2, -3, 4, 0, 0, 0, 0]), i32([-1, 2, -3, 4]));
un('i32x4.extend_high_i16x8_s', [168], i16([0, 0, 0, 0, -1, 2, -3, 4]), i32([-1, 2, -3, 4]));
un('i32x4.extend_low_i16x8_u', [169], i16([65535, 2, 0, 0, 0, 0, 0, 0]), i32([65535, 2, 0, 0]));
un('i32x4.extend_high_i16x8_u', [170], i16([0, 0, 0, 0, 65535, 2, 0, 0]), i32([65535, 2, 0, 0]));
un('i64x2.extend_low_i32x4_s', [199, 231], i32([-1, 2, 0, 0]), i64([-1, 2]));
un('i64x2.extend_high_i32x4_s', [200, 232], i32([0, 0, -1, 2]), i64([-1, 2]));
un('i64x2.extend_low_i32x4_u', [201, 233], i32([-1, 2, 0, 0]), i64([4294967295, 2]));
un('i64x2.extend_high_i32x4_u', [202, 234], i32([0, 0, -1, 2]), i64([4294967295, 2]));
// conversions
un('i32x4.trunc_sat_f32x4_s', [252, 284], f32([1.9, -1.9, 3e9, 0]), i32([1, -1, 2147483647, 0]));
un('i32x4.trunc_sat_f32x4_u', [253, 285], f32([1.9, -1.9, 3e9, 0]), i32([1, 0, -1294967296, 0]));
un('f32x4.convert_i32x4_s', [254, 286], i32([3, -4, 0, 0]), f32([3, -4, 0, 0]));
un('f32x4.convert_i32x4_u', [255, 287], i32([-1, 4, 0, 0]), f32([4294967296, 4, 0, 0]));
un('f32x4.demote_f64x2_zero', [238, 270, 302], f64([1.5, -2.5]), f32([1.5, -2.5, 0, 0]));
un('f64x2.promote_low_f32x4', [239, 271, 303], f32([1.5, -2.5, 9, 9]), f64([1.5, -2.5]));
un('i64x2.trunc_sat_f64x2_s', [284, 316], f64([1.9, -1.9]), i64([1, -1]));
un('i64x2.trunc_sat_f64x2_u', [285, 317], f64([1.9, -1.9]), i64([1, 0]));
un('f64x2.convert_i64x2_s', [286, 318], i64([3, -4]), f64([3, -4]));
un('f64x2.convert_i64x2_u', [287, 319], i64([-1, 4]), f64([18446744073709551616, 4]));
// shifts
UU('i8x16.shl', [107], opc => shiftop(opc, i8([1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), 3), m => m[0] === 8 && m[1] === 16);
UU('i8x16.shr_s', [108], opc => shiftop(opc, i8([-16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), 2), m => m[0] === 0xfc);
UU('i8x16.shr_u', [109], opc => shiftop(opc, i8([0xf0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), 2), m => m[0] === 0x3c);
UU('i16x8.shl', [139], opc => shiftop(opc, i16([1, 0, 0, 0, 0, 0, 0, 0]), 3), m => rd32(m, 0) === 8);
UU('i32x4.shl', [171], opc => shiftop(opc, i32([1, 0, 0, 0]), 3), m => rd32(m, 0) === 8);
UU('i32x4.shr_s', [172], opc => shiftop(opc, i32([-16, 0, 0, 0]), 2), m => rd32(m, 0) === -4);
UU('i32x4.shr_u', [173], opc => shiftop(opc, i32([-16, 0, 0, 0]), 2), m => rd32(m, 0) === 1073741820);
UU('i64x2.shl', [203, 235], opc => shiftop(opc, i64([1, 0]), 3), m => rd64(m, 0) === 8n);
// splat / extract / replace / scalar-out
UU('i8x16.splat', [15], opc => splat32(opc, 0x7f), m => [...m.slice(0, 16)].every(b => b === 0x7f));
UU('i16x8.splat', [16], opc => splat32(opc, 0x1234), m => rd32(m, 0) === 0x12341234);
UU('i32x4.splat', [17], opc => splat32(opc, 42), m => rd32(m, 0) === 42 && rd32(m, 12) === 42);
UU('i64x2.splat', [18], opc => [0x41, 0x00, 0x42, 42, 0xfd, ...uleb(opc), ...VSTORE0], m => rd64(m, 0) === 42n && rd64(m, 8) === 42n);
UU('f32x4.splat', [19], opc => splatf32(opc, 1.5), m => new DataView(m.buffer).getFloat32(0, true) === 1.5);
UU('f64x2.splat', [20], opc => splatf64(opc, 1.5), m => new DataView(m.buffer).getFloat64(0, true) === 1.5);
UU('i8x16.extract_lane_s', [21], opc => extractI(opc, i8([0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), 0), m => rd32(m, 0) === -1);
UU('i8x16.extract_lane_u', [22], opc => extractI(opc, i8([0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), 0), m => rd32(m, 0) === 255);
UU('i16x8.extract_lane_s', [24], opc => extractI(opc, i16([-2, 0, 0, 0, 0, 0, 0, 0]), 0), m => rd32(m, 0) === -2);
UU('i16x8.extract_lane_u', [25], opc => extractI(opc, i16([-2, 0, 0, 0, 0, 0, 0, 0]), 0), m => rd32(m, 0) === 65534);
UU('i32x4.extract_lane', [27], opc => extractI(opc, i32([0, 77, 0, 0]), 1), m => rd32(m, 0) === 77);
UU('i64x2.extract_lane', [29], opc => extract64(opc, i64([0, 99]), 1), m => rd64(m, 0) === 99n);
UU('f32x4.extract_lane', [31], opc => extractF32(opc, f32([0, 2.5, 0, 0]), 1), m => new DataView(m.buffer).getFloat32(0, true) === 2.5);
UU('f64x2.extract_lane', [33], opc => extractF64(opc, f64([0, 2.5]), 1), m => new DataView(m.buffer).getFloat64(0, true) === 2.5);
UU('i8x16.replace_lane', [23], opc => replaceI(opc, i8(Array(16).fill(0)), 5, 0x63), m => m[5] === 0x63 && m[0] === 0);
UU('i32x4.replace_lane', [28], opc => replaceI(opc, i32([0, 0, 0, 0]), 2, 77), m => rd32(m, 8) === 77);
UU('i8x16.bitmask', [100], opc => scalarOf(opc, i8([0x80, 0x01, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80])), m => rd32(m, 0) === 0x8005);
UU('i16x8.bitmask', [132], opc => scalarOf(opc, i16([0x8000, 1, 0, 0, 0, 0, 0, 0x8000])), m => rd32(m, 0) === 0x81);
UU('i32x4.bitmask', [164], opc => scalarOf(opc, i32([0x80000000, 1, 0, 0x80000000])), m => rd32(m, 0) === 0x9);
UU('i64x2.bitmask', [196, 228], opc => scalarOf(opc, i64([0x8000000000000000n, 1n].map(Number))), m => rd32(m, 0) === 0x1);
UU('i8x16.all_true', [99], opc => scalarOf(opc, i8(Array(16).fill(1))), m => rd32(m, 0) === 1);
UU('i8x16.any_true', [98, 99, 100], opc => scalarOf(opc, i8([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1])), m => rd32(m, 0) === 1);
UU('v128.not', [77, 83], opc => unop(opc, i8(Array(16).fill(0x0f))), m => [...m.slice(0, 16)].every(b => b === 0xf0));
UU('i8x16.shuffle', [13], opc => [0x41, 0x00, ...VC(i8([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])), ...VC(i8(Array(16).fill(99))), 0xfd, ...uleb(opc), 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, ...VSTORE0], m => m[0] === 15 && m[15] === 0);
// extmul (binary, shape-changing result) - probe via binop shape: result lanes wider
bin('i16x8.extmul_low_i8x16_s', [156], i8([2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i16([8, 15, 0, 0, 0, 0, 0, 0]));

let fail = 0;
for (const [name, cands, makeBody, check] of CASES) {
  let found = null;
  for (const opc of cands) {
    try {
      const bytes = buildModule(makeBody(opc));
      if (!WebAssembly.validate(bytes)) continue;
      const { instance } = await WebAssembly.instantiate(bytes, {});
      const mem = new Uint8Array(instance.exports.memory.buffer);
      instance.exports.run();
      if (check(mem)) { found = opc; break; }
    } catch (e) { /* try next candidate */ }
  }
  if (found === null) { console.log(`MISS  ${name}  (candidates: ${cands.join(',')})`); fail++; }
  else console.log(`OK    ${name} = ${found}`);
}
console.log(fail ? `${fail} MISSES` : 'ALL PROBED OK');
