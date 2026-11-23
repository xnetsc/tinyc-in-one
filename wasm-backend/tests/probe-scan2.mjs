// Scan probe round 2: disambiguating inputs, wider ranges.
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

// distinct inputs: A=0x3c B=0x2a -> or 0x3e, xor 0x16, andnot 0x14, and 0x28
await scan('v128.or?', 76, 96, binop(i8(Array(16).fill(0x3c)), i8(Array(16).fill(0x2a))), eq16(i8(Array(16).fill(0x3e))));
await scan('v128.xor?', 76, 96, binop(i8(Array(16).fill(0x3c)), i8(Array(16).fill(0x2a))), eq16(i8(Array(16).fill(0x16))));
await scan('v128.andnot?', 76, 96, binop(i8(Array(16).fill(0x3c)), i8(Array(16).fill(0x2a))), eq16(i8(Array(16).fill(0x14))));
// f64x2 comparisons with distinguishing inputs
await scan('f64x2.eq?', 60, 100, binop(f64([1, 20]), f64([1, 9])), eq16(i64([-1, 0])));
await scan('f64x2.ne?', 60, 100, binop(f64([1, 20]), f64([1, 9])), eq16(i64([0, -1])));
await scan('f64x2.lt?', 60, 100, binop(f64([2, 20]), f64([2, 9])), eq16(i64([0, 0])));
await scan('f64x2.le?', 60, 100, binop(f64([2, 20]), f64([2, 9])), eq16(i64([-1, 0])));
await scan('f64x2.gt?', 60, 100, binop(f64([2, 20]), f64([2, 9])), eq16(i64([0, -1])));
await scan('f64x2.ge?', 60, 100, binop(f64([2, 20]), f64([2, 9])), eq16(i64([-1, -1])));
// float unary with -0.0-correct expectations, wide range
await scan('f32x4.neg?', 220, 260, unop(f32([1.5, -2.5, 3.5, -4.5])), eq16(f32([-1.5, 2.5, -3.5, 4.5])));
await scan('f32x4.ceil?', 220, 420, unop(f32([1.5, -1.5, 2.5, -2.5])), eq16(f32([2, -1, 3, -2])));
await scan('f32x4.floor?', 220, 420, unop(f32([1.5, -1.5, 2.5, -2.5])), eq16(f32([1, -2, 2, -3])));
await scan('f32x4.trunc?', 220, 420, unop(f32([1.5, -1.5, 2.5, -2.5])), eq16(f32([1, -1, 2, -2])));
await scan('f32x4.nearest?', 220, 420, unop(f32([1.5, 2.5, 3.5, 4.5])), eq16(f32([2, 2, 4, 4])));
await scan('f64x2.ceil?', 220, 420, unop(f64([1.5, -1.5])), eq16(f64([2, -1])));
await scan('f64x2.floor?', 220, 420, unop(f64([1.5, -1.5])), eq16(f64([1, -2])));
await scan('f64x2.trunc?', 220, 420, unop(f64([1.5, -1.5])), eq16(f64([1, -1])));
await scan('f64x2.nearest?', 220, 420, unop(f64([1.5, 2.5])), eq16(f64([2, 2])));
await scan('f32x4.demote?', 220, 420, unop(f64([1.5, -2.5])), eq16(f32([1.5, -2.5, 0, 0])));
await scan('f64x2.promote?', 220, 420, unop(f32([1.5, -2.5, 9, 9])), eq16(f64([1.5, -2.5])));
await scan('i64x2.trunc_sat_f64x2_s?', 220, 420, unop(f64([1.9, -1.9])), eq16(i64([1, -1])));
await scan('i64x2.trunc_sat_f64x2_u?', 220, 420, unop(f64([1.9, -1.9])), eq16(i64([1, 0])));
await scan('f64x2.convert_i64x2_s?', 220, 420, unop(i64([3, -4])), eq16(f64([3, -4])));
await scan('f64x2.convert_i64x2_u?', 220, 420, unop(i64([-1, 4])), eq16(f64([18446744073709551616, 4])));
// replace_lane debug with error print
for (const [label, body] of [
  ['i8x16.replace_lane 23', [0x41, 0x00, ...VC(i8(Array(16).fill(0))), 0x41, 0x63, 0xfd, 23, 5, ...VS]],
  ['i32x4.replace_lane 28', [0x41, 0x00, ...VC(i32([0, 0, 0, 0])), 0x41, 77, 0xfd, 28, 2, ...VS]],
]) {
  const bytes = buildModule(body);
  try {
    const { instance } = await WebAssembly.instantiate(bytes, {});
    const mem = new Uint8Array(instance.exports.memory.buffer);
    instance.exports.run();
    console.log(label, 'mem[0..16]=', [...mem.slice(0, 16)].join(','));
  } catch (e) { console.log(label, 'ERROR', e.message); }
}
