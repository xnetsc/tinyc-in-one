// Imported-variadic (shim printf) regression: the host provides
// printf(fmt, vaPtr) with the documented va-area layout (natural
// alignment, i64/f64 in 8-byte slots) and the guest's formatted
// output is compared byte for byte.
import { readFileSync } from 'node:fs';
import assert from 'node:assert';

const bytes = readFileSync(process.argv[2]);
let mem;
const dv = () => new DataView(mem.buffer);
function readStr(ptr) {
  let s = '';
  const u8 = new Uint8Array(mem.buffer);
  while (u8[ptr]) s += String.fromCharCode(u8[ptr++]);
  return s;
}
function makeVa(vaPtr) {
  let cur = vaPtr;
  const alignTo = (n) => { cur = (cur + n - 1) & ~(n - 1); };
  return {
    i32() { alignTo(4); const v = dv().getInt32(cur, true); cur += 4; return v; },
    u32() { alignTo(4); const v = dv().getUint32(cur, true); cur += 4; return v; },
    i64() { alignTo(8); const v = dv().getBigInt64(cur, true); cur += 8; return v; },
    f64() { alignTo(8); const v = dv().getFloat64(cur, true); cur += 8; return v; },
  };
}
function format(fmt, vaPtr) {
  const va = makeVa(vaPtr);
  let out = '';
  for (let i = 0; i < fmt.length; i++) {
    if (fmt[i] !== '%') { out += fmt[i]; continue; }
    i++;
    if (fmt.startsWith('lld', i)) { out += va.i64().toString(); i += 2; }
    else if (fmt[i] === 'd' || fmt[i] === 'i') out += va.i32().toString();
    else if (fmt[i] === 'u') out += va.u32().toString();
    else if (fmt[i] === 'x') out += va.u32().toString(16);
    else if (fmt[i] === 's') out += readStr(va.i32());
    else if (fmt[i] === 'c') out += String.fromCharCode(va.i32());
    else if (fmt[i] === 'f') out += va.f64().toFixed(6);
    else if (fmt[i] === '%') out += '%';
  }
  return out;
}
let captured = '';
const imports = {
  env: {
    printf(fmt, va) {
      const s = format(readStr(fmt), va);
      captured += s;
      return s.length;
    },
  },
};
const { instance } = await WebAssembly.instantiate(bytes, imports);
mem = instance.exports.memory;
const rc = instance.exports.probe();
console.log('captured:', JSON.stringify(captured));
assert.strictEqual(rc, 0);
assert.strictEqual(captured,
  'n=-7 s=ok x=ff c=q % ll=1234567890123 f=2.500000 u=4000000000\nplain\n');
console.log('VARARGS PRINTF ASSERTIONS PASSED');
