// Dylink data-GOT regression: a static data initializer that
// points at an imported function must produce a GOT.func
// reference whose value the synthesized init copies into the
// slot at load time (bound eagerly by the loader, since the
// copy can never be repaired later).  Checks, end to end
// through the packages/browser process loader: the pointer is
// callable, and it is identical to the pointer produced by the
// defining module's own materialisation, by dlsym, by reading
// the raw slot, and by a runtime address-take inside the side
// module.  A side module whose initializer names a function
// nobody provides must fail to load, naming the symbol — under
// RTLD_NOW from guest code and under RTLD_LAZY from the host.
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
const dir = mkdtempSync(join(tmpdir(), 'dygot-'));
const build = (args) => execFileSync(tcc, args, { stdio: 'inherit' });

build(['-nostdlib', '-wasm-dylink=side', '-o', join(dir, 'libdatagot.wasm'),
       join(here, 'test-datagot-side.c')]);
build(['-nostdlib', '-wasm-dylink=side', '-o', join(dir, 'libdatagot2.wasm'),
       join(here, 'test-datagot-side2.c')]);
build(['-nostdlib', '-wasm-dylink=side', '-o', join(dir, 'libdatagotvar.wasm'),
       join(here, 'test-datagot-varside.c')]);
build(['-nostdlib', '-wasm-dylink=side', '-o', join(dir, 'libbadgot.wasm'),
       join(here, 'test-datagot-badside.c')]);
build(['-nostdlib', '-I', winc, '-I', inc, '-wasm-dylink=main', '-o', join(dir, 'maingot.wasm'),
       join(here, 'test-datagot-main.c')]);

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
const bytes = readFileSync(join(dir, 'maingot.wasm'));
const result = proc.runMainModule(bytes, { path: '/maingot.wasm' });
const text = out.join('');
console.log(text);
assert.strictEqual(result.exitCode, 0);
assert.ok(text.includes('DYGOT-RESULT: PASS'));

// The eager failure does not depend on RTLD_NOW: a host dlopen
// under RTLD_LAZY refuses the bad module too, naming the symbol.
const proc2 = createProcess({ readModule, io: { out: () => {}, err: () => {} } });
let lazyError = null;
try {
  proc2.dlopen('libbadgot.wasm', 1 /* RTLD_LAZY */);
} catch (err) {
  lazyError = err;
}
assert.ok(lazyError, 'host RTLD_LAZY dlopen of libbadgot must fail');
assert.ok(String(lazyError.message).includes('missing_fn'),
  `error must name missing_fn, got: ${lazyError.message}`);
console.log('DYLINK DATAGOT ASSERTIONS PASSED');
