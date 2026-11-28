// Expression-level dead code regression: side effects in the
// dead arm of a constant-folded &&, || or ?: must not execute.
// The guest counts them and calls the imported exit(3) from a
// dead arm; it must run to its PASS line with exit code 0 in
// both output forms (the pre-fix compiler printed counter=4 on
// a five-shape variant of this probe and exited 3).
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert';

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..');
const tcc = join(root, 'build-src', 'wasm32-tcc');
const winc = join(root, 'include');
const inc = join(root, '..', 'vendor-tinycc', 'include');
const dir = mkdtempSync(join(tmpdir(), 'deadcode-'));
const build = (args) => execFileSync(tcc, args, { stdio: 'inherit' });

build(['-nostdlib', '-I', winc, '-I', inc, '-wasm-dylink=main',
       '-o', join(dir, 'deadcode-main.wasm'),
       join(here, 'test-deadcode.c')]);
build(['-nostdlib', '-I', winc, '-I', inc,
       '-o', join(dir, 'deadcode.wasm'),
       join(here, 'test-deadcode.c')]);

const { createProcess, runWasmModuleSync } = await import(
  join(root, '..', 'packages', 'browser', 'index.mjs'));

const out = [];
const proc = createProcess({
  io: { out: (t) => out.push(t), err: (t) => out.push(t) },
});
const result = proc.runMainModule(
  readFileSync(join(dir, 'deadcode-main.wasm')),
  { path: '/deadcode-main.wasm' });
const text = out.join('');
console.log(text);
assert.strictEqual(result.exitCode, 0);
assert.ok(text.includes('DEADCODE-RESULT: PASS'));

/* The standalone form: main's return value is the verdict (a
   distinct nonzero code per failed shape, 3 if the dead exit
   fires). */
const sa = runWasmModuleSync(readFileSync(join(dir, 'deadcode.wasm')),
                             { entry: 'main', callArgs: [0, 0] });
assert.strictEqual(sa, 0);
console.log('DEADCODE ASSERTIONS PASSED');
