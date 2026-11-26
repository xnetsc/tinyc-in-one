// Dylink variadic regression: compile the fixture as a dylink
// main module with the freshly built wasm32-tcc and run it
// through the packages/browser process loader, whose stdio
// shims consume the (fixed..., vaPtr) ABI.
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert';

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..');
const tcc = join(root, 'build-src', 'wasm32-tcc');
const dir = mkdtempSync(join(tmpdir(), 'dyva-'));
execFileSync(tcc, ['-nostdlib', '-wasm-dylink=main',
                   '-o', join(dir, 'vamain.wasm'),
                   join(here, 'test-varargs-dylink.c')],
             { stdio: 'inherit' });

const { createProcess } = await import(
  join(root, '..', 'packages', 'browser', 'index.mjs'));
const out = [];
const proc = createProcess({
  readModule: () => undefined,
  io: { out: (t) => out.push(t), err: (t) => out.push(t) },
});
const bytes = readFileSync(join(dir, 'vamain.wasm'));
const result = proc.runMainModule(bytes, { path: '/vamain.wasm' });
const text = out.join('');
console.log(text);
assert.strictEqual(result.exitCode, 0);
assert.ok(text.includes('P:7:tail:0.500000\n'));
console.log('DYLINK VARARGS ASSERTIONS PASSED');
