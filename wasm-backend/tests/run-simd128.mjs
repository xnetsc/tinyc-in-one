// tests/run-simd128.mjs - Node assertions for tests/test-simd128.c
// (include/wasm_simd128.h).  Usage: node tests/run-simd128.mjs <file.wasm>
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

check("h_load_store()", ex.h_load_store(), 12344);   // {11,12,13,14}
check("h_const()", ex.h_const(), 1234);
check("h_sat()", ex.h_sat(), -1);                    // 32767 + -32768
check("h_minmax()", ex.h_minmax(), 48);              // lo[2]=50, hi[1]=-3, hi[3]=1
check("h_farith()", ex.h_farith(), 7.0);
check("h_cmp_bitmask()", ex.h_cmp_bitmask(), 5);     // lanes 0,2 set -> 0b0101
check("h_shifts()", ex.h_shifts(), 144);             // -4 + 100 + 48
check("h_shuffle()", ex.h_shuffle(), 255);           // 15*16 + 0 + swizzle lane0=15
check("h_convert()", ex.h_convert(), 28);            // 3*10 + -2
check("h_extend_narrow()", ex.h_extend_narrow(), -864); // -1000+2 + 127 + 7
check("h_dot_extmul()", ex.h_dot_extmul(), 17077);   // dot0=17,dot1=53 -> 17053 + extmul lane1=24
check("h_avgr_popcnt()", ex.h_avgr_popcnt(), 767);   // avgr(100,51)=76 -> 760 + popcnt(0x7f)=7
check("h_reduce()", ex.h_reduce(), 6);               // 4 + 2 + 0
check("h_f64()", ex.h_f64(), 4.5);
check("h_replace()", ex.h_replace(), 100);           // 99 + 1
check("h_load_splat()", ex.h_load_splat(), 7);
check("h_bitops()", ex.h_bitops(), 0x3c3c);

console.log(`simd128 assertions: ${pass} passed, ${fail} failed`);
if (fail) { console.log("SIMD128 ASSERTIONS FAILED"); process.exit(1); }
console.log("ALL SIMD128 ASSERTIONS PASSED");
