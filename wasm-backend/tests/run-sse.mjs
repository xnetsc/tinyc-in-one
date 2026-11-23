// tests/run-sse.mjs - Node assertions for tests/test-sse.c
// (include/xmmintrin.h + include/emmintrin.h).
// Usage: node tests/run-sse.mjs <file.wasm>
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

check("x_add_ps()", ex.x_add_ps(), 13.0);
check("x_add_ss_hi()", ex.x_add_ss_hi(), 4.0);
check("x_add_ss_lo()", ex.x_add_ss_lo(), 11.0);
check("x_movemask()", ex.x_movemask(), 10);          // 0b1010
check("x_comi()", ex.x_comi(), 3);
check("x_cvtss()", ex.x_cvtss(), 43);                // 4*10 + 3
check("x_cvtsi32()", ex.x_cvtsi32(), 42.0);
check("x_sqrt_minmax()", ex.x_sqrt_minmax(), 30);    // 15 + 15
check("x_unpack_movehl()", ex.x_unpack_movehl(), 30);
check("e_add_epi32()", ex.e_add_epi32(), 112334);
check("e_sat_epu8()", ex.e_sat_epu8(), 255);
check("e_madd()", ex.e_madd(), 1753);                // 17*100 + 53
check("e_packs()", ex.e_packs(), 127007);            // 127*1000 + 0 + 7
check("e_shuffle_epi32()", ex.e_shuffle_epi32(), 4321);
check("e_byte_shifts()", ex.e_byte_shifts(), 404);
check("e_shuffle_epi8()", ex.e_shuffle_epi8(), 41);
check("e_unpacklo_epi8()", ex.e_unpacklo_epi8(), 1526);
check("e_cvtepi32_roundtrip()", ex.e_cvtepi32_roundtrip(), 28);
check("e_srai()", ex.e_srai(), -4);
check("e_srli()", ex.e_srli(), 1073741820);
check("e_movemask_epi8()", ex.e_movemask_epi8(), 8); // 0b1000
check("e_add_pd()", ex.e_add_pd(), 3.0);
check("e_cvtsd()", ex.e_cvtsd(), 87);                // 8*10 + 7

console.log(`sse assertions: ${pass} passed, ${fail} failed`);
if (fail) { console.log("SSE ASSERTIONS FAILED"); process.exit(1); }
console.log("ALL SSE ASSERTIONS PASSED");
