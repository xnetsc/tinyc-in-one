// tests/run-neon.mjs - Node assertions for tests/test-neon.c
// (include/arm_neon.h).  Usage: node tests/run-neon.mjs <file.wasm>
import { readFileSync } from "node:fs";

const bytes = readFileSync(process.argv[2]);
let mem = null;
const u8 = () => new Uint8Array(mem.buffer);
const imports = { env: {
  memset: (ptr, val, n) => { u8().fill(val & 0xff, ptr, ptr + n); return ptr; },
  memcpy: (dst, src, n) => { u8().copyWithin(dst, src, src + n); return dst; },
  memmove: (dst, src, n) => { u8().copyWithin(dst, src, src + n); return dst; },
} };
const { instance } = await WebAssembly.instantiate(bytes, imports);
mem = instance.exports.memory;
const ex = instance.exports;

let pass = 0, fail = 0;
function check(name, got, want) {
  if (Object.is(got, want)) { pass++; console.log(`PASS ${name} = ${want}`); }
  else { fail++; console.log(`FAIL ${name}: got ${got}, expected ${want}`); }
}

check("n_add()", ex.n_add(), 13574);              // {11,22,33,44}
check("n_sat()", ex.n_sat(), 32767);
check("n_minmax()", ex.n_minmax(), 47);           // max[1]=-3, min[2]=50
check("n_cmp()", ex.n_cmp(), -1);                 // m0=-1, m1=0, e0=0
check("n_absneg()", ex.n_absneg(), 105);          // 100 + 5
check("n_zip()", ex.n_zip(), 5736);               // z1=5, u3=7, t2=3, z3=6
check("n_zip8()", ex.n_zip8(), 177);              // z={0,16,1,17,...}: 0+160+17
check("n_tbl()", ex.n_tbl(), 240);                // r0=15 -> 240 + r15=0
check("n_duplane()", ex.n_duplane(), 33);         // all lanes 3
check("n_shift()", ex.n_shift(), 2);              // -4 + 6
check("n_ldst()", ex.n_ldst(), 10987);            // {10,9,8,7} -> 10000+900+80+7
check("n_f32()", ex.n_f32(), 6.0);
check("n_setlane()", ex.n_setlane(), 991);        // 99*10 + 1
{
  const p = ex.n_mul64();
  const dv = new DataView(mem.buffer);
  check("n_mul64()[0]", dv.getBigInt64(p, true), 3000000n);
  check("n_mul64()[1]", dv.getBigInt64(p + 8, true), 35n);
}
check("n_reinterpret()", ex.n_reinterpret(), -2); // two mask lanes of -1

console.log(`neon assertions: ${pass} passed, ${fail} failed`);
if (fail) { console.log("NEON ASSERTIONS FAILED"); process.exit(1); }
console.log("ALL NEON ASSERTIONS PASSED");
