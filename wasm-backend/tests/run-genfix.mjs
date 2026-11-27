// Regression runner for test-genfix.c: long double conversion /
// literal handling and call results as sibling arguments.
// Exact-value assertions, same pattern as run-scalar2.mjs.
import { readFileSync } from 'node:fs';

let mem = null;
const u8 = () => new Uint8Array(mem.buffer);
const imports = {
  env: {
    memset: (p, v, n) => { u8().fill(v & 0xff, p, p + n); return p; },
    memcpy: (d, s, n) => { u8().copyWithin(d, s, s + n); return d; },
    memmove: (d, s, n) => { u8().copyWithin(d, s, s + n); return d; },
    fmod: (x, y) => x - Math.trunc(x / y) * y,
  },
};

const bytes = readFileSync(new URL('./test-genfix.wasm', import.meta.url));
const { instance } = await WebAssembly.instantiate(bytes, imports);
mem = instance.exports.memory;
const ex = instance.exports;
let pass = 0, fail = 0;
function check(name, actual, expected) {
  if (actual === expected) { pass++; console.log(`PASS ${name} = ${actual}`); }
  else { fail++; console.log(`FAIL ${name} = ${actual} (want ${expected})`); }
}

/* bug 1: long double conversions keep full f64 precision */
check('ld_from_d(0.1)', ex.ld_from_d(0.1), 0.1);
check('ld_from_d(1e300)', ex.ld_from_d(1e300), 1e300);
check('ld_from_d(5e-324)', ex.ld_from_d(5e-324), 5e-324);
check('ld_cast(0.1)', ex.ld_cast(0.1), 0.1);
check('call_ld_id(0.1)', ex.call_ld_id(0.1), 0.1);
check('ld_from_f(0.1f)', ex.ld_from_f(0.1), 0.10000000149011612);
check('f_from_ld(1.5)', ex.f_from_ld(1.5), 1.5);
check('ld_from_i(-3)', ex.ld_from_i(-3), -3);
check('i_from_ld(42.7)', ex.i_from_ld(42.7), 42);

/* bug 1: long double literals */
check('ld_lit()', ex.ld_lit(), 1.5);
check('ld_lit2()', ex.ld_lit2(), 0.1);
check('d_lit()', ex.d_lit(), 1.5);
check('ld_fold()', ex.ld_fold(), 3.75);
check('get_g()', ex.get_g(), 1.5);
check('lit_arg()', ex.lit_arg(), 1.5);
check('lit_arg2()', ex.lit_arg2(), 0.1);
check('get_s()', ex.get_s(), 3.5);
check('arr_ld()', ex.arr_ld(), 2.5);

/* bug 2: a call result keeps its value next to a sibling call */
check('nested_dd', ex.nested_dd(), 7497.75);
check('nested_cd', ex.nested_cd(), 3497.75);
check('nested_dc', ex.nested_dc(), 7497.75);
check('nested3', ex.nested3(), 15253.5);
check('nested_op', ex.nested_op(), 8497.75);
check('nested_deep', ex.nested_deep(), 1002003);
check('nested_import', ex.nested_import(), -1.5);
check('nested_ii', ex.nested_ii(), 6998);
check('nested_ll', ex.nested_ll(), 6998n);
check('nested_ff', ex.nested_ff(), 7497.75);

console.log(`\n${pass} passed, ${fail} failed`);
if (fail) process.exit(1);
console.log('ALL ASSERTIONS PASSED');
