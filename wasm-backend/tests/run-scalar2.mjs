// Scalar suite 2 runner: floating point, control flow, conversions,
// 64-bit integers and the vector frontend cases.  Same env-shim +
// exact-value assertion pattern as run-vec.mjs.
import { readFileSync } from 'node:fs';

let mem = null;
const u8 = () => new Uint8Array(mem.buffer);
const imports = {
  env: {
    memset: (p, v, n) => { u8().fill(v & 0xff, p, p + n); return p; },
    memcpy: (d, s, n) => { u8().copyWithin(d, s, s + n); return d; },
    memmove: (d, s, n) => { u8().copyWithin(d, s, s + n); return d; },
  },
};

const bytes = readFileSync(new URL('./test-scalar2.wasm', import.meta.url));
const { instance } = await WebAssembly.instantiate(bytes, imports);
mem = instance.exports.memory;
const ex = instance.exports;

let pass = 0, fail = 0;
function check(name, actual, expected) {
  if (actual === expected) { pass++; console.log(`PASS ${name}() = ${actual}`); }
  else { fail++; console.log(`FAIL ${name}() = ${actual} (want ${expected})`); }
}

/* floating-point arithmetic */
check('d_add', ex.d_add(1.5, 2.25), 3.75);
check('d_sub', ex.d_sub(5.5, 2.0), 3.5);
check('d_mul', ex.d_mul(1.5, 4.0), 6.0);
check('d_div', ex.d_div(7.0, 2.0), 3.5);
check('f_add', ex.f_add(1.5, 2.25), 3.75);
check('f_mul', ex.f_mul(2.5, 4.0), 10.0);
check('f_div', ex.f_div(1.0, 4.0), 0.25);
check('d_neg', ex.d_neg(3.5), -3.5);
check('f_neg', ex.f_neg(3.5), -3.5);
check('d_from_f', ex.d_from_f(0.1), 0.10000000149011612);
check('d_roundtrip', ex.d_roundtrip(0.1), 0.10000000149011612);

/* comparisons as values */
check('cmp_vals_lt', ex.cmp_vals(1, 2), 1 + 8 + 16);
check('cmp_vals_eq', ex.cmp_vals(2, 2), 2 + 8);
check('cmp_vals_gt', ex.cmp_vals(3, 2), 4 + 16);
check('fcmp_vals_lt', ex.fcmp_vals(1.5, 2.5), 1);
check('fcmp_vals_eq', ex.fcmp_vals(2.5, 2.5), 2 + 4);
check('fcmp_vals_gt', ex.fcmp_vals(3.5, 2.5), 4);
check('cmp_and', ex.cmp_and(1, 2, 3, 4), 1);
check('cmp_and_f', ex.cmp_and(1, 2, 4, 3), 0);
check('cmp_or', ex.cmp_or(2, 1, 3, 4), 1);
check('cmp_or_f', ex.cmp_or(2, 1, 4, 3), 0);
check('cmp_not', ex.cmp_not(1, 2), 0);
check('cmp_not_f', ex.cmp_not(2, 1), 1);
check('cmp_chain', ex.cmp_chain(1, 2), 1);

/* comparisons as conditions / control flow */
check('if_else_t', ex.if_else(5), 1);
check('if_else_f', ex.if_else(2), 2);
check('if_float_t', ex.if_float(3.0), 1);
check('if_float_f', ex.if_float(2.0), 0);
check('fcond_and_t', ex.fcond_and(1.5, 2.5, 3.5, 4.5), 1);
check('fcond_and_f', ex.fcond_and(1.5, 2.5, 4.5, 3.5), 0);
check('while_sum', ex.while_sum(10), 55);
check('while_float', ex.while_float(), 4);
check('for_loop', ex.for_loop(), 21);
check('do_while', ex.do_while(), 3);
check('nested_bc', ex.nested_bc(), 64);
check('sw_1', ex.sw(1), 10);
check('sw_2', ex.sw(2), 21);
check('sw_3', ex.sw(3), 1);
check('sw_d', ex.sw(9), 99);
check('tern_t', ex.tern(5), 111);
check('tern_f', ex.tern(-5), 222);
check('go_fwd_t', ex.go_fwd(1), 11);
check('go_fwd_f', ex.go_fwd(0), 12);
check('go_loop', ex.go_loop(5), 10);
check('loop_float_mix', ex.loop_float_mix(), 10.0);

/* conversions */
check('f2i', ex.f2i(3.99), 3);
check('f2i_neg', ex.f2i(-3.99), -3);
check('d2i', ex.d2i(9.99), 9);
check('f2u', ex.f2u(3.5), 3);
check('i2d', ex.i2d(-3), -3.0);
check('u2d', ex.u2d(4000000000), 4000000000.0);
check('i2f_round', ex.i2f_round(16777217), 16777216.0);
check('f2ll', ex.f2ll(3.99), 3n);
check('d2ll', ex.d2ll(-9.99), -9n);
check('d2ull', ex.d2ull(9.99), 9n);
check('d2i_sat', ex.d2i(1e30), 2147483647);
check('d2i_sat_neg', ex.d2i(-1e30), -2147483648);
check('d2ull_neg', ex.d2ull(-3.5), 0n);
check('f2ll_sat', ex.f2ll(1e30), 9223372036854775807n);
check('ll2d', ex.ll2d(5000000000n), 5000000000.0);
check('ll2f', ex.ll2f(16777217n), 16777216.0);
check('ull2d', ex.ull2d(18446744073709551615n), 18446744073709551616.0);

/* mixed int/float */
check('mix1', ex.mix1(3, 2.5), 5.5);
check('mix2', ex.mix2(2), 6.0);
check('mix3', ex.mix3(), 11);
check('mix_cmp_t', ex.mix_cmp(3, 3.5), 1);
check('mix_cmp_f', ex.mix_cmp(4, 3.5), 0);

/* 64-bit integers */
check('ll_const', ex.ll_const(), 5000000000n);
check('ll_const_neg', ex.ll_const_neg(), -5000000000n);
check('ll_hi', ex.ll_hi(), 1);
check('ll_lo', ex.ll_lo(), 705032704);
check('ll_id', ex.ll_id(5000000000n), 5000000000n);
check('ll_add_carry', ex.ll_add(4294967295n, 1n), 4294967296n);
check('ll_sub_borrow', ex.ll_sub(4294967296n, 1n), 4294967295n);
check('ll_mul', ex.ll_mul(4294967295n, 2n), 8589934590n);
check('ll_shr', ex.ll_shr(5000000000n), 1n);
check('ll_shl', ex.ll_shl(1), 1099511627776n);
check('ll_cmp', ex.ll_cmp(5000000000n, 4294967295n), 1);
check('ll_mix', ex.ll_mix(), 705032704.0);

/* i64 vector brace initializers */
check('v64_a', ex.v64_a(), 5000000000n);
check('v64_b', ex.v64_b(), 3n);
check('gv64_a', ex.gv64_a(), 9000000000n);
check('gv64_b', ex.gv64_b(), -2n);

/* vector frontend cases */
check('vec_rvalue_sub', ex.vec_rvalue_sub(), 40);
check('vec_op_sub', ex.vec_op_sub(), 22);
check('vec_ucmp_init', ex.vec_ucmp_init(), -2);

console.log(`scalar2 assertions: ${pass} passed, ${fail} failed`);
if (fail) { console.log('SCALAR2 ASSERTIONS FAILED'); process.exit(1); }
console.log('ALL SCALAR2 ASSERTIONS PASSED');
