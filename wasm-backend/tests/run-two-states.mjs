// run-two-states.mjs - export-set assertions for the modules that
// test-two-states.c produced.  Each module must export exactly its
// own program's functions: the backend registry belongs to one
// output cycle of one state, so nothing compiled by an earlier
// state (or an earlier cycle of the same state, or a state that
// never output) may appear in a later module.
import { readFile } from 'node:fs/promises';

let failures = 0;
function check(name, cond) {
  console.log((cond ? 'PASS: ' : 'FAIL: ') + name);
  if (!cond) failures++;
}

async function load(path) {
  const bytes = await readFile(new URL(path, import.meta.url));
  const mod = new WebAssembly.Module(bytes);
  const inst = new WebAssembly.Instance(mod, {
    env: new Proxy({}, { get: () => () => 0 }),
  });
  const fns = WebAssembly.Module.exports(mod)
    .filter((e) => e.kind === 'function')
    .map((e) => e.name)
    .sort();
  return { inst, fns };
}

const eq = (a, b) => JSON.stringify(a) === JSON.stringify(b);

const m1 = await load('./ts-m1.wasm');
check('m1 exports exactly {fa}', eq(m1.fns, ['fa']));
check('m1 fa() == 11', m1.inst.exports.fa() === 11);

const m1b = await load('./ts-m1b.wasm');
check('m1b (same program output twice) exports exactly {fa}',
  eq(m1b.fns, ['fa']));
check('m1b fa() == 11', m1b.inst.exports.fa() === 11);

const m3 = await load('./ts-m3.wasm');
check('m3 (same state, next program) exports exactly {fc}',
  eq(m3.fns, ['fc']));
check('m3 fc() == 33', m3.inst.exports.fc() === 33);

const m2 = await load('./ts-m2.wasm');
check('m2 (second state) exports exactly {fb}', eq(m2.fns, ['fb']));
check('m2 fb() == 22', m2.inst.exports.fb() === 22);

const multi = await load('./ts-multi.wasm');
check('multi (one state, two files) exports {entry, helper}',
  eq(multi.fns, ['entry', 'helper']));
check('multi entry() == 8 (helper()+1 across files)',
  multi.inst.exports.entry() === 8);

const m5 = await load('./ts-m5.wasm');
check('m5 (after an abandoned state) exports exactly {fy}',
  eq(m5.fns, ['fy']));
check('m5 fy() == 55', m5.inst.exports.fy() === 55);

console.log(failures
  ? `run-two-states: ${failures} failure(s)`
  : 'run-two-states: all assertions passed');
process.exit(failures ? 1 : 0);
