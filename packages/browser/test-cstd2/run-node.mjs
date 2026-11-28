#!/usr/bin/env node
/* Node runner for the cstd2 suite: compiles the guests with
 * the wasm32 cross compiler and runs the suite in the
 * simulated tier. The real tier needs a browser Worker and is
 * covered by the page runner.
 *
 * Usage: node run-node.mjs [--keep]   (build into ./build-node/)
 */
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import * as indexModule from '../index.mjs';
import { runSuite, summarize, createStandaloneRunner } from './suite.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '../../..');
const pkg = path.resolve(here, '..');
const tcc = process.env.TINYC_WASM32_TCC ??
  path.join(repo, 'wasm-backend/build-src/wasm32-tcc');
if (!fs.existsSync(tcc)) {
  console.error(`wasm32-tcc not found at ${tcc}; run wasm-backend/build.sh first`);
  process.exit(2);
}
const buildDir = path.join(here, 'build-node');
fs.rmSync(buildDir, { recursive: true, force: true });
fs.mkdirSync(buildDir, { recursive: true });

const guestsDir = path.join(here, 'guests');
const INCLUDES = [
  '-I', path.join(repo, 'wasm-backend/include'),
  '-I', path.join(repo, 'vendor-tinycc/include'),
  '-I', path.join(repo, 'vendor-tinycc/win32/include'),
  '-I', guestsDir,
];

function compile(src, out, extra = []) {
  const res = spawnSync(tcc, [
    '-nostdlib', ...INCLUDES, ...extra,
    '-o', path.join(buildDir, out), path.join(guestsDir, src),
  ], { encoding: 'utf8' });
  if (res.status !== 0) {
    console.error(`compile ${src} failed:\n${res.stderr}`);
    process.exit(1);
  }
  // The probes are ordinary user code against the shipped
  // headers: an implicit declaration anywhere means a header
  // still does not declare what the layer supplies.
  if (/implicit declaration/.test(res.stderr)) {
    console.error(`compile ${src}: implicit declaration warning:\n${res.stderr}`);
    process.exit(1);
  }
}

for (const name of ['lldiv_probe', 'addr_probe', 'time_probe',
                    'errno_probe', 'env_probe', 'struct_probe']) {
  compile(`${name}.c`, name, ['-wasm-dylink=main']);
}
for (const name of ['lldiv_probe', 'addr_probe', 'env_probe']) {
  compile(`${name}.c`, `${name}_sa`, []);
}

const units = new Map();
units.set('libtcc1.tinyc.js',
  fs.readFileSync(path.join(pkg, 'sysroot-lib', 'wasm32-wasi', 'libtcc1.tinyc.js')));

const guestBytes = async (name) => fs.readFileSync(path.join(buildDir, name));

const envelopeText = fs.readFileSync(
  path.join(pkg, 'sysroot', 'sysroot-wasm32-wasi.tinyc.js'), 'utf8');
const fetchImpl = async () => ({
  ok: true,
  status: 200,
  text: async () => envelopeText,
});
const runStandalone = await createStandaloneRunner(indexModule, fetchImpl);

let ok = false;
try {
  const results = await runSuite({
    createProcess: indexModule.createProcess,
    guestBytes,
    units,
    runStandalone,
    tier: 'simulated',
  });
  ok = summarize(results);
} finally {
  if (!process.argv.includes('--keep')) {
    fs.rmSync(buildDir, { recursive: true, force: true });
  }
}
process.exit(ok ? 0 : 1);
