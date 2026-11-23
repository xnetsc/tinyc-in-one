// Dump probe: for each opcode in range, print the actual result bytes
// of a v128->v128 unary op on a fixed input, to identify ops by eye.
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
const i64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setBigInt64(0, BigInt(x), true); return [...b]; });
const f32 = a => a.flatMap(x => { const b = new Uint8Array(4); new DataView(b.buffer).setFloat32(0, x, true); return [...b]; });
const f64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setFloat64(0, x, true); return [...b]; });

async function dump(label, A, lo, hi, fmt) {
  console.log(`--- ${label} (input ${fmt(A)}) ---`);
  for (let opc = lo; opc <= hi; opc++) {
    const body = [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), ...VS];
    try {
      const bytes = buildModule(body);
      if (!WebAssembly.validate(bytes)) continue;
      const { instance } = await WebAssembly.instantiate(bytes, {});
      const mem = new Uint8Array(instance.exports.memory.buffer);
      instance.exports.run();
      console.log(`  ${opc}: ${fmt([...mem.slice(0, 16)])}`);
    } catch (e) { }
  }
}
const asF32x4 = b => { const v = new DataView(new Uint8Array(b).buffer); return [0, 4, 8, 12].map(o => v.getFloat32(o, true)).join(', '); };
const asF64x2 = b => { const v = new DataView(new Uint8Array(b).buffer); return [0, 8].map(o => v.getFloat64(o, true)).join(', '); };
const asI64x2 = b => { const v = new DataView(new Uint8Array(b).buffer); return [0, 8].map(o => v.getBigInt64(o, true)).join(', '); };
const asI32x4 = b => { const v = new DataView(new Uint8Array(b).buffer); return [0, 4, 8, 12].map(o => v.getInt32(o, true)).join(', '); };

await dump('unops on f32 [1.5,-1.5,2.5,-2.5]', f32([1.5, -1.5, 2.5, -2.5]), 224, 262, asF32x4);
await dump('unops on f64 [1.5,-2.5] as f32 view', f64([1.5, -2.5]), 236, 262, asF32x4);
await dump('unops on f32 [1.5,-2.5,9,9] as f64 view', f32([1.5, -2.5, 9, 9]), 236, 262, asF64x2);
await dump('unops on f64 [1.9,-1.9] as i64 view', f64([1.9, -1.9]), 236, 262, asI64x2);
await dump('unops on i64 [3,-4] as f64 view', i64([3, -4]), 236, 262, asF64x2);
