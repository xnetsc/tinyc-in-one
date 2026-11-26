import { readFileSync } from 'node:fs';
import assert from 'node:assert';

const bytes = readFileSync(process.argv[2]);
const { instance } = await WebAssembly.instantiate(bytes, {});
const bad = instance.exports.probe();
console.log('varargs probe() bad-mask =', bad);
assert.strictEqual(bad, 0);
console.log('VARARGS ASSERTIONS PASSED');
