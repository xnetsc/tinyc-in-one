// Dump probe round 4: multi-view results per opcode.
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
function views(mem) {
  const v = new DataView(mem.buffer);
  const g = (f, o, n) => { const r = []; for (let i = 0; i < n; i++) r.push(f(o + i * (16 / n))); return r.join(','); };
  return `i32[${g((o) => v.getInt32(o, true), 0, 4)}] i64[${g((o) => v.getBigInt64(o, true), 0, 2)}] f32[${g((o) => v.getFloat32(o, true), 0, 4)}] f64[${g((o) => v.getFloat64(o, true), 0, 2)}]`;
}
async function dump(label, A, lo, hi) {
  console.log(`--- ${label} ---`);
  for (let opc = lo; opc <= hi; opc++) {
    const body = [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), ...VS];
    try {
      const bytes = buildModule(body);
      if (!WebAssembly.validate(bytes)) continue;
      const { instance } = await WebAssembly.instantiate(bytes, {});
      const mem = new Uint8Array(instance.exports.memory.buffer);
      instance.exports.run();
      console.log(`  ${opc}: ${views(mem)}`);
    } catch (e) { }
  }
}
// input chosen so demote/promote/convert/trunc each give a distinct signature
await dump('input f64 [1.5,-2.5]', f64([1.5, -2.5]), 248, 275);
await dump('input i64 [3,-4]', i64([3, -4]), 248, 275);
await dump('input f32 [1.5,-2.5,9,9]', f32([1.5, -2.5, 9, 9]), 262, 340);
await dump('input f64 [1.5,-2.5] wide', f64([1.5, -2.5]), 262, 340);
