import { readFileSync } from 'node:fs';
import assert from 'node:assert';

const bytes = readFileSync(process.argv[2]);
const { instance } = await WebAssembly.instantiate(bytes, {});
const bad = instance.exports.probe();
console.log('datareloc probe() bad-mask =', bad);
assert.strictEqual(bad, 0);
console.log('DATARELOC ASSERTIONS PASSED');
