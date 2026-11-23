// Scan probe: for a given body shape + expected result, scan an opcode
// range and report every opcode that validates and matches.
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
const VC = (bytes) => [0xfd, 12, ...bytes];
const VS = [0xfd, 11, 0x00, 0x00];
const i8 = a => a.map(x => x & 0xff);
const i16 = a => a.flatMap(x => { const b = new Uint8Array(2); new DataView(b.buffer).setInt16(0, x, true); return [...b]; });
const i32 = a => a.flatMap(x => { const b = new Uint8Array(4); new DataView(b.buffer).setInt32(0, x, true); return [...b]; });
const i64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setBigInt64(0, BigInt(x), true); return [...b]; });
const f32 = a => a.flatMap(x => { const b = new Uint8Array(4); new DataView(b.buffer).setFloat32(0, x, true); return [...b]; });
const f64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setFloat64(0, x, true); return [...b]; });

async function scan(label, lo, hi, makeBody, check) {
  const hits = [];
  for (let opc = lo; opc <= hi; opc++) {
    try {
      const bytes = buildModule(makeBody(opc));
      if (!WebAssembly.validate(bytes)) continue;
      const { instance } = await WebAssembly.instantiate(bytes, {});
      const mem = new Uint8Array(instance.exports.memory.buffer);
      instance.exports.run();
      if (check(mem)) hits.push(opc);
    } catch (e) { }
  }
  console.log(`${label}: ${hits.length ? hits.join(', ') : 'NONE'}`);
}
const eq16 = exp => m => [...m.slice(0, 16)].join() === exp.join();
const binop = (A, B) => opc => [0x41, 0x00, ...VC(A), ...VC(B), 0xfd, ...uleb(opc), ...VS];
const unop = A => opc => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), ...VS];
const scalarOf = A => opc => [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), 0x36, 0x00, 0x00];
const rd32 = (m, o) => new DataView(m.buffer).getInt32(o, true);

await scan('v128.and', 76, 96, binop(i8(Array(16).fill(0x3c)), i8(Array(16).fill(0x1e))), eq16(i8(Array(16).fill(0x1c))));
await scan('v128.or', 76, 96, binop(i8(Array(16).fill(0x30)), i8(Array(16).fill(0x0f))), eq16(i8(Array(16).fill(0x3f))));
await scan('v128.xor', 76, 96, binop(i8(Array(16).fill(0x33)), i8(Array(16).fill(0x0f))), eq16(i8(Array(16).fill(0x3c))));
await scan('v128.andnot', 76, 96, binop(i8(Array(16).fill(0x3c)), i8(Array(16).fill(0x1e))), eq16(i8(Array(16).fill(0x22))));
await scan('v128.bitselect', 76, 96, opc => [0x41, 0x00, ...VC(i8(Array(16).fill(0xaa))), ...VC(i8(Array(16).fill(0xbb))), ...VC(i8(Array(16).fill(0xf0))), 0xfd, ...uleb(opc), ...VS], eq16(i8(Array(16).fill(0xa0 | 0x0b))));
await scan('v128.any_true', 76, 110, scalarOf(i8([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1])), m => rd32(m, 0) === 1);
await scan('f64x2.eq', 60, 100, binop(f64([1, 2]), f64([1, 9])), eq16(i64([-1, 0])));
await scan('f64x2.lt', 60, 100, binop(f64([1, 20]), f64([2, 9])), eq16(i64([-1, 0])));
await scan('i8x16.mul?', 96, 131, binop(i8(Array(16).fill(6)), i8(Array(16).fill(7))), eq16(i8(Array(16).fill(42))));
await scan('f32x4.abs', 224, 320, unop(f32([-1.5, 2.5, 0, 0])), eq16(f32([1.5, 2.5, 0, 0])));
await scan('f32x4.neg', 224, 320, unop(f32([1.5, -2.5, 0, 0])), eq16(f32([-1.5, 2.5, 0, 0])));
await scan('f32x4.sqrt', 224, 320, unop(f32([4, 9, 16, 25])), eq16(f32([2, 3, 4, 5])));
await scan('f32x4.ceil', 224, 320, unop(f32([1.5, -1.5, 0, 0])), eq16(f32([2, -1, 0, 0])));
await scan('f32x4.floor', 224, 320, unop(f32([1.5, -1.5, 0, 0])), eq16(f32([1, -2, 0, 0])));
await scan('f32x4.trunc', 224, 320, unop(f32([1.5, -1.5, 0, 0])), eq16(f32([1, -1, 0, 0])));
await scan('f32x4.nearest', 224, 320, unop(f32([1.5, 2.5, 0, 0])), eq16(f32([2, 2, 0, 0])));
await scan('i32x4.trunc_sat_f32x4_s', 224, 330, unop(f32([1.9, -1.9, 3e9, 0])), eq16(i32([1, -1, 2147483647, 0])));
await scan('i32x4.trunc_sat_f32x4_u', 224, 330, unop(f32([1.9, -1.9, 3e9, 0])), eq16(i32([1, 0, -1294967296, 0])));
await scan('f32x4.convert_i32x4_s', 224, 330, unop(i32([3, -4, 0, 0])), eq16(f32([3, -4, 0, 0])));
await scan('f32x4.convert_i32x4_u', 224, 330, unop(i32([-1, 4, 0, 0])), eq16(f32([4294967296, 4, 0, 0])));
await scan('f32x4.demote_f64x2_zero', 224, 330, unop(f64([1.5, -2.5])), eq16(f32([1.5, -2.5, 0, 0])));
await scan('f64x2.promote_low_f32x4', 224, 330, unop(f32([1.5, -2.5, 9, 9])), eq16(f64([1.5, -2.5])));
await scan('f64x2.abs', 236, 330, unop(f64([-1.5, 2.5])), eq16(f64([1.5, 2.5])));
await scan('f64x2.neg', 236, 330, unop(f64([1.5, -2.5])), eq16(f64([-1.5, 2.5])));
await scan('f64x2.sqrt', 236, 330, unop(f64([4, 9])), eq16(f64([2, 3])));
await scan('f64x2.ceil', 236, 330, unop(f64([1.5, -1.5])), eq16(f64([2, -1])));
await scan('f64x2.floor', 236, 330, unop(f64([1.5, -1.5])), eq16(f64([1, -2])));
await scan('i64x2.trunc_sat_f64x2_s', 236, 340, unop(f64([1.9, -1.9])), eq16(i64([1, -1])));
await scan('i64x2.trunc_sat_f64x2_u', 236, 340, unop(f64([1.9, -1.9])), eq16(i64([1, 0])));
await scan('f64x2.convert_i64x2_s', 236, 340, unop(i64([3, -4])), eq16(f64([3, -4])));
await scan('f64x2.convert_i64x2_u', 236, 340, unop(i64([-1, 4])), eq16(f64([18446744073709551616, 4])));
await scan('i16x8.extmul_high_i8x16_s', 150, 165, binop(i8([0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 0, 0, 0, 0, 0, 0]), i8([0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 0, 0, 0, 0])), eq16(i16([8, 15, 0, 0, 0, 0, 0, 0])));
await scan('i16x8.extmul_low_i8x16_u', 150, 165, binop(i8([200, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), i8([4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])), eq16(i16([800, 15, 0, 0, 0, 0, 0, 0])));
await scan('i16x8.extmul_high_i8x16_u', 150, 165, binop(i8([0, 0, 0, 0, 0, 0, 0, 0, 200, 3, 0, 0, 0, 0, 0, 0]), i8([0, 0, 0, 0, 0, 0, 0, 0, 4, 5, 0, 0, 0, 0, 0, 0])), eq16(i16([800, 15, 0, 0, 0, 0, 0, 0])));
// replace_lane debug: print validation result for specific candidates
for (const opc of [23, 28]) {
  const body = [0x41, 0x00, ...VC(i8(Array(16).fill(0))), 0x41, 0x63, 0xfd, ...uleb(opc), 5, ...VS];
  const bytes = buildModule(body);
  console.log(`replace_lane candidate ${opc}: validate=${WebAssembly.validate(bytes)}`);
}
const rl32 = [0x41, 0x00, ...VC(i32([0, 0, 0, 0])), 0x41, 77, 0xfd, ...uleb(28), 2, ...VS];
console.log(`i32x4.replace_lane 28: validate=${WebAssembly.validate(buildModule(rl32))}`);
// load64_zero / store64_lane
{
  const body = [0x41, 16, 0x41, 0x00, 0xfd, ...uleb(93), 0x00, 0x00, ...VS];
  const bytes = buildModule(body);
  let res = 'validate=' + WebAssembly.validate(bytes);
  if (WebAssembly.validate(bytes)) {
    const { instance } = await WebAssembly.instantiate(bytes, {});
    const mem = new Uint8Array(instance.exports.memory.buffer);
    mem.set([1, 2, 3, 4, 5, 6, 7, 8], 0);
    instance.exports.run();
    res += ' mem[16..24]=' + [...mem.slice(16, 24)].join(',') + ' mem[24..32]=' + [...mem.slice(24, 32)].join(',');
  }
  console.log('v128.load64_zero=93:', res);
}
{
  const body = [0x41, 32, ...VC(i8([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16])), 0xfd, ...uleb(91), 0x00, 0x00, 0x00];
  const bytes = buildModule(body);
  let res = 'validate=' + WebAssembly.validate(bytes);
  if (WebAssembly.validate(bytes)) {
    const { instance } = await WebAssembly.instantiate(bytes, {});
    const mem = new Uint8Array(instance.exports.memory.buffer);
    instance.exports.run();
    res += ' mem[32..40]=' + [...mem.slice(32, 40)].join(',');
  }
  console.log('v128.store64_lane=91:', res);
}
