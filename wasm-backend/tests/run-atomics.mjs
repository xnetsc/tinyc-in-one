// Atomics suite runner (threads design §8): the __atomic_* helper
// family emitted inline, the C11 surface over it, fences, lock-free
// queries, atomic_flag and direct _Atomic access.  Same env-shim +
// exact-value assertion pattern as run-scalar2.mjs.  The module
// must carry no __atomic import: interception means the operations
// are instructions, not calls.
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

const bytes = readFileSync(new URL('./test-atomics.wasm', import.meta.url));

let fail = 0;
const mod = new WebAssembly.Module(bytes);
for (const imp of WebAssembly.Module.imports(mod)) {
  if (imp.name.includes('__atomic') || imp.name.startsWith('atomic_')) {
    fail++;
    console.log(`FAIL module imports ${imp.module}.${imp.name} (must be emitted inline)`);
  }
}

const instance = await WebAssembly.instantiate(mod, imports);
mem = instance.exports.memory;
const ex = instance.exports;

let pass = 0;
function check(name, actual, expected) {
  if (actual === expected) { pass++; console.log(`PASS ${name}() = ${actual}`); }
  else { fail++; console.log(`FAIL ${name}() = ${actual} (want ${expected})`); }
}

check('fa_seq', ex.fa_seq(), 11215110);
check('fa_bits', ex.fa_bits(), 632);
check('fa_nand', ex.fa_nand(), 1);
check('xchg', ex.xchg(), 7042);
check('load_store_explicit', ex.load_store_explicit(), 123);
check('cmpxchg_ok', ex.cmpxchg_ok(), 1);
check('cmpxchg_fail', ex.cmpxchg_fail(), 1);
check('cmpxchg_weak', ex.cmpxchg_weak(), 1);
check('subword', ex.subword(), 1);
check('ll_seq', ex.ll_seq(), 8000000001n);
check('ll_cmpxchg', ex.ll_cmpxchg(), 1);
check('float_ops', ex.float_ops(), 0);
check('flag_ops', ex.flag_ops(), 0);
check('fence_and_lockfree', ex.fence_and_lockfree(), 0);
check('direct_access', ex.direct_access(), 1);
check('user_defined_helper', ex.user_defined_helper(), 1);

console.log(`atomics assertions: ${pass} passed, ${fail} failed`);
if (fail) { console.log('ATOMICS ASSERTIONS FAILED'); process.exit(1); }
console.log('ALL ATOMICS ASSERTIONS PASSED');
