import { readFileSync } from 'node:fs';
import assert from 'node:assert';

const file = process.argv[2];
const bytes = readFileSync(file);
console.log('wasm size:', bytes.length);
console.log('magic ok:', bytes.subarray(0, 4).toString('latin1') === '\0asm');

const { instance } = await WebAssembly.instantiate(bytes, {});
const ex = instance.exports;
console.log('exports:', Object.keys(ex).sort().join(', '));

assert.strictEqual(ex.add(2, 3), 5);
assert.strictEqual(ex.add(-4, 4), 0);
assert.strictEqual(ex.triple(5), 15);
assert.strictEqual(ex.poly(3), 29);   // y=9, triple(9)=27, 9+27-add(3,4)=29
assert.strictEqual(ex.mix(17, 5), (3 << 2) | (2 & 7) ^ (17 - 5)); // q=3,r=2 -> 12|2^12=2
console.log('add(2,3) =', ex.add(2, 3));
console.log('triple(5) =', ex.triple(5));
console.log('poly(3) =', ex.poly(3));
console.log('mix(17,5) =', ex.mix(17, 5));
console.log('ALL ASSERTIONS PASSED');
