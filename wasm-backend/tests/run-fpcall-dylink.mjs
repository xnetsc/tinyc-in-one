// Dylink function-pointer-call regression: compile the side and
// main fixtures with the freshly built wasm32-tcc, load them
// through the packages/browser process loader, and require the
// guest checks (calls through global function-pointer variables,
// to a module-local function and to an imported function) to pass
// end to end.
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, existsSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert';

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..');
const tcc = join(root, 'build-src', 'wasm32-tcc');
const inc = join(root, '..', 'vendor-tinycc', 'include');
const winc = join(root, 'include');
const dir = mkdtempSync(join(tmpdir(), 'dyfp-'));
const build = (args) => execFileSync(tcc, args, { stdio: 'inherit' });

build(['-nostdlib', '-wasm-dylink=side', '-o', join(dir, 'libfp.wasm'),
       join(here, 'test-fpcall-side.c')]);
build(['-nostdlib', '-I', winc, '-I', inc, '-wasm-dylink=main', '-o', join(dir, 'mainfp.wasm'),
       join(here, 'test-fpcall-main.c')]);

const { createProcess } = await import(
  join(root, '..', 'packages', 'browser', 'index.mjs'));
const readModule = (canonical) => {
  const name = canonical.replace('/sysroot/', '');
  const p = join(dir, name);
  return existsSync(p) ? readFileSync(p) : undefined;
};
const out = [];
const proc = createProcess({
  readModule,
  io: { out: (t) => out.push(t), err: (t) => out.push(t) },
});
const bytes = readFileSync(join(dir, 'mainfp.wasm'));
const result = proc.runMainModule(bytes, { path: '/mainfp.wasm' });
const text = out.join('');
console.log(text);
assert.strictEqual(result.exitCode, 0);
assert.ok(text.includes('DYFP-RESULT: PASS'));
console.log('DYLINK FPCALL ASSERTIONS PASSED');
