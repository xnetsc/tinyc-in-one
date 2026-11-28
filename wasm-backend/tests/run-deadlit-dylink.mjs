// Dead-branch literal regression: a TU with no static functions
// whose string literals are referenced only from compile-time-dead
// branches used to emit those literals as GOT.mem imports in
// dylink output (the loader rejects them eagerly, as no module can
// define an anonymous symbol) and failed standalone output with
// "cannot resolve address of 'L.0'".  The guest must now compile
// in both forms, import no GOT entries at all, and run with its
// live literals byte-exact.
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
const dir = mkdtempSync(join(tmpdir(), 'deadlit-'));
const build = (args) => execFileSync(tcc, args, { stdio: 'inherit' });

build(['-nostdlib', '-I', winc, '-I', inc, '-o', join(dir, 'deadlit.wasm'),
       join(here, 'test-deadlit.c')]);
build(['-nostdlib', '-I', winc, '-I', inc, '-wasm-dylink=main',
       '-o', join(dir, 'deadlit-main.wasm'),
       join(here, 'test-deadlit.c')]);

const dy = readFileSync(join(dir, 'deadlit-main.wasm'));
const dyText = dy.toString('utf8');
const dyMeta = JSON.parse(
  dyText.slice(dyText.indexOf(' = ') + 3, dyText.lastIndexOf(';')));
const mod = new WebAssembly.Module(Buffer.from(dyMeta.body, 'base64'));
const got = WebAssembly.Module.imports(mod)
  .filter((i) => i.module.startsWith('GOT'))
  .map((i) => `${i.module}.${i.name}`);
assert.deepStrictEqual(got, []);

const { createProcess, runWasmModuleSync } = await import(
  join(root, '..', 'packages', 'browser', 'index.mjs'));

const out = [];
const proc = createProcess({
  io: { out: (t) => out.push(t), err: (t) => out.push(t) },
});
const result = proc.runMainModule(dy, { path: '/deadlit-main.wasm' });
const text = out.join('');
console.log(text);
assert.strictEqual(result.exitCode, 0);
assert.ok(text.includes('DEADLIT-RESULT: PASS'));

const sa = runWasmModuleSync(readFileSync(join(dir, 'deadlit.wasm')),
                             { entry: 'main', callArgs: [0, 0] });
assert.strictEqual(sa, 0);
console.log('DYLINK DEADLIT ASSERTIONS PASSED');
