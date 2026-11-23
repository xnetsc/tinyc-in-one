import { readFileSync } from 'node:fs';

const file = process.argv[2];
const bytes = readFileSync(file);
console.log('wasm size:', bytes.length);
console.log('magic ok:', bytes.subarray(0, 4).toString('latin1') === '\0asm');

/* the compiler emits calls to the C library helpers memset/memcpy/
   memmove (e.g. init_putz zeroing an aggregate before its element
   stores); they arrive as env imports and are served here from the
   module's own exported memory (filled in after instantiation -
   the helpers only run once a compiled function is called). */
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
console.log('exports:', Object.keys(ex).sort().join(', '));

let pass = 0, fail = 0;
function check(name, actual, expected) {
  if (actual === expected) {
    pass++;
    console.log(`PASS ${name} = ${actual}`);
  } else {
    fail++;
    console.log(`FAIL ${name}: got ${actual}, expected ${expected}`);
  }
}

check('sz()', ex.sz(), 16);
check('t_add(30,12)', ex.t_add(30, 12), 42);
check('t_arr()', ex.t_arr(), 30);
check('t_init()', ex.t_init(), 100);
check('t_sub()', ex.t_sub(), 7100);
check('t_cmp(20)', ex.t_cmp(20), -1);  /* lane1: 20>10 -> -1; lane3: == -> 0 */
check('t_cmp(10)', ex.t_cmp(10), -1);  /* lane1: > -> 0; lane3: == -> -1 */
check('t_cmp(5)', ex.t_cmp(5), 0);
check('t_call(21)', ex.t_call(21), 42);
/* t_id takes/returns a v128: the WebAssembly JS API cannot pass v128
   values, so it is covered indirectly through t_call and only its
   presence is checked here. */
check('typeof t_id', typeof ex.t_id, 'function');
check('t_fadd()', ex.t_fadd(), -2);
check('t_i8mul()', ex.t_i8mul(), -30);
check('t_v2()', ex.t_v2(), 68);
check('t_shl(3,4)', ex.t_shl(3, 4), 48);
check('t_shl(1,31)', ex.t_shl(1, 31), -2147483648);
check('t_shuf()', ex.t_shuf(), 41);
check('t_rep()', ex.t_rep(), 100);
check('t_un()', ex.t_un(), 2);
check('t_glob()', ex.t_glob(), 600);
check('t_abs()', ex.t_abs(), 6);
check('t_sat()', ex.t_sat(), 199);

console.log(`vec assertions: ${pass} passed, ${fail} failed`);
if (fail) process.exit(1);
console.log('ALL VEC ASSERTIONS PASSED');
