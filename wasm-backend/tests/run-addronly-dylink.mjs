// Dylink address-only classification regression: an imported
// function that is only ever address-taken (never called
// directly, never a data initializer) must still be classified
// by its own type as a function — GOT.func — so the module
// loads and the bound pointer is the canonical one.  Classified
// as GOT.mem (the old fallback for a symbol no call ever
// interned), the loader rejects the module as an unresolved
// variable.
import { execFileSync } from 'node:child_process';
import { mkdtempSync, readFileSync, existsSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import assert from 'node:assert';

const here = dirname(fileURLToPath(import.meta.url));
const root = join(here, '..');
const tcc = join(root, 'build-src', 'wasm32-tcc');
const winc = join(root, 'include');
const inc = join(root, '..', 'vendor-tinycc', 'include');
const dir = mkdtempSync(join(tmpdir(), 'dyaddr-'));
const build = (args) => execFileSync(tcc, args, { stdio: 'inherit' });

build(['-nostdlib', '-wasm-dylink=side', '-o', join(dir, 'libaddronly.wasm'),
       join(here, 'test-addronly-side.c')]);
build(['-nostdlib', '-I', winc, '-I', inc, '-wasm-dylink=main', '-o', join(dir, 'mainaddr.wasm'),
       join(here, 'test-addronly-main.c')]);

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
const bytes = readFileSync(join(dir, 'mainaddr.wasm'));
const result = proc.runMainModule(bytes, { path: '/mainaddr.wasm' });
const text = out.join('');
console.log(text);
assert.strictEqual(result.exitCode, 0);
assert.ok(text.includes('DYADDR-RESULT: PASS'));
console.log('DYLINK ADDRONLY ASSERTIONS PASSED');
