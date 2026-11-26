import { readFileSync } from 'node:fs';
import assert from 'node:assert';

const bytes = readFileSync(process.argv[2]);
const mod = new WebAssembly.Module(bytes);
// The miscompilation's fingerprint: a direct call invented an
// import named after the pointer variable / array.  A correct
// module imports nothing at all.
assert.deepStrictEqual(WebAssembly.Module.imports(mod), []);
const { instance } = await WebAssembly.instantiate(bytes, {});
const bad = instance.exports.probe();
console.log('fpcall probe() bad-mask =', bad);
assert.strictEqual(bad, 0);
console.log('FPCALL ASSERTIONS PASSED');
