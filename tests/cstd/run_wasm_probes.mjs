#!/usr/bin/env node
/* Wasm runner for the C-standard header probes: compiles the
 * probe guests with the wasm32 cross compiler against the
 * sysroot sources and runs them in the simulated tier.
 *
 * Usage: node run_wasm_probes.mjs
 * Env: TINYC_WASM32_TCC — the wasm32 cross compiler
 *      (default: the wasm-backend build in this repo).
 */
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createProcess } from '../../packages/browser/index.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '../..');
const tcc = process.env.TINYC_WASM32_TCC ??
  path.join(repo, 'wasm-backend/build-src/wasm32-tcc');
if (!fs.existsSync(tcc)) {
  console.error(`wasm32-tcc not found at ${tcc}; run wasm-backend/build.sh first`);
  process.exit(2);
}
const buildDir = path.join(here, 'build-wasm');
fs.rmSync(buildDir, { recursive: true, force: true });
fs.mkdirSync(buildDir, { recursive: true });

const INCLUDES = [
  '-I', path.join(repo, 'wasm-backend/include'),
  '-I', path.join(repo, 'vendor-tinycc/include'),
];

function compile(src, out) {
  execFileSync(tcc, [
    '-nostdlib', ...INCLUDES, '-wasm-dylink=main',
    '-o', path.join(buildDir, out), path.join(here, src),
  ], { stdio: 'inherit' });
}

async function runGuest(name) {
  let text = '';
  const proc = createProcess({
    io: {
      out: (t) => { text += t; },
      err: (t) => { text += t; },
    },
  });
  let exitCode = null;
  let error = null;
  try {
    const res = await proc.runMainModule(
      fs.readFileSync(path.join(buildDir, name)));
    exitCode = res.exitCode;
  } catch (e) {
    error = String(e && e.message || e);
  }
  return { exitCode, error, text };
}

let failures = 0;
const expectOk = async (src, marker) => {
  compile(src, src + '.wasm');
  const r = await runGuest(src + '.wasm');
  if (r.exitCode === 0 && r.text.includes(marker)) {
    console.log(`${src}: run ok (${marker})`);
  } else {
    failures++;
    console.log(`${src}: FAIL exit=${r.exitCode} error=${r.error} ` +
      `output=${JSON.stringify(r.text)}`);
  }
};

await expectOk('probe_wasm_cstd.c', 'wasm cstd probe: OK');
await expectOk('probe_uchar.c', 'uchar probe: OK');
await expectOk('probe_iso646.c', 'iso646 probe: OK');

compile('probe_assert_fail.c', 'probe_assert_fail.c.wasm');
{
  const r = await runGuest('probe_assert_fail.c.wasm');
  const aborted = r.exitCode !== 0 || r.error !== null;
  if (aborted && r.text.includes('assertion failed: 1 == 2') &&
      !r.text.includes('after the assertion')) {
    console.log('probe_assert_fail.c: abort + message ok');
  } else {
    failures++;
    console.log(`probe_assert_fail.c: FAIL exit=${r.exitCode} ` +
      `error=${r.error} output=${JSON.stringify(r.text)}`);
  }
}

if (failures) {
  console.log(`wasm cstd probes: ${failures} FAILURES`);
  process.exit(1);
}
console.log('wasm cstd probes: all ok');
