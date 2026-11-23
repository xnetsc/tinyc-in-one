// Round 6: f64x2 rounding ops + remaining unknowns in 108..160.
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
const f64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setFloat64(0, x, true); return [...b]; });
const eq16 = exp => m => [...m.slice(0, 16)].join() === exp.join();
async function scan(label, lo, hi, A, exp) {
  const hits = [];
  for (let opc = lo; opc <= hi; opc++) {
    const body = [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), ...VS];
    try {
      const bytes = buildModule(body);
      if (!WebAssembly.validate(bytes)) continue;
      const { instance } = await WebAssembly.instantiate(bytes, {});
      const mem = new Uint8Array(instance.exports.memory.buffer);
      instance.exports.run();
      if (eq16(exp)(mem)) hits.push(opc);
    } catch (e) { }
  }
  console.log(`${label}: ${hits.length ? hits.join(', ') : 'NONE'}`);
}
await scan('f64x2.ceil', 100, 170, f64([1.5, -1.5]), f64([2, -1]));
await scan('f64x2.floor', 100, 170, f64([1.5, -1.5]), f64([1, -2]));
await scan('f64x2.trunc', 100, 170, f64([1.5, -1.5]), f64([1, -1]));
await scan('f64x2.nearest', 100, 170, f64([1.5, 2.5]), f64([2, 2]));
await scan('f32x4.ceil(recheck)', 96, 130, (() => { const b = new Uint8Array(16); const v = new DataView(b.buffer); [1.5, -1.5, 2.5, -2.5].forEach((x, i) => v.setFloat32(i * 4, x, true)); return [...b]; })(), (() => { const b = new Uint8Array(16); const v = new DataView(b.buffer); [2, -1, 3, -2].forEach((x, i) => v.setFloat32(i * 4, x, true)); return [...b]; })());
