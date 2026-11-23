// List every opcode in 0..700 that is a valid v128->v128 unary op,
// then run the unidentified ones on signature inputs.
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
const f32 = a => a.flatMap(x => { const b = new Uint8Array(4); new DataView(b.buffer).setFloat32(0, x, true); return [...b]; });
const f64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setFloat64(0, x, true); return [...b]; });
const i64 = a => a.flatMap(x => { const b = new Uint8Array(8); new DataView(b.buffer).setBigInt64(0, BigInt(x), true); return [...b]; });

const valid = [];
for (let opc = 0; opc <= 700; opc++) {
  const body = [0x41, 0x00, ...VC(f64([1.5, -2.5])), 0xfd, ...uleb(opc), ...VS];
  if (WebAssembly.validate(buildModule(body))) valid.push(opc);
}
console.log('valid v128->v128 unops:', valid.join(', '));

const known = new Set([96, 97, 98, 128, 129, 130, 160, 161, 192, 193, 224, 225, 227, 236, 237, 239,
  248, 249, 250, 251, 252, 253, 254, 255, 257, 258, 259, 260,
  135, 136, 137, 138, 167, 168, 169, 170, 199, 200, 201, 202]);
const unknown = valid.filter(x => !known.has(x));
console.log('unidentified:', unknown.join(', '));
function views(mem) {
  const v = new DataView(mem.buffer);
  const g = (f, n, sz) => { const r = []; for (let i = 0; i < n; i++) r.push(f(i * sz)); return r.join(','); };
  return `i32[${g((o) => v.getInt32(o, true), 4, 4)}] i64[${g((o) => v.getBigInt64(o, true), 2, 8)}] f32[${g((o) => v.getFloat32(o, true), 4, 4)}] f64[${g((o) => v.getFloat64(o, true), 2, 8)}]`;
}
for (const opc of unknown) {
  for (const [label, A] of [['f64[1.5,-2.5]', f64([1.5, -2.5])], ['f32[1.5,-2.5,9,9]', f32([1.5, -2.5, 9, 9])], ['i64[3,-4]', i64([3, -4])]]) {
    const body = [0x41, 0x00, ...VC(A), 0xfd, ...uleb(opc), ...VS];
    try {
      const { instance } = await WebAssembly.instantiate(buildModule(body), {});
      const mem = new Uint8Array(instance.exports.memory.buffer);
      instance.exports.run();
      console.log(`  ${opc} on ${label}: ${views(mem)}`);
    } catch (e) { }
  }
}
