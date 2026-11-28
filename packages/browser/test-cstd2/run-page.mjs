#!/usr/bin/env node
/* Page runner for the cstd2 suite: compiles the guests as the
 * Node runner does, serves the package over http with
 * COOP/COEP, and runs the suite in Chromium — the real tier
 * when the page is cross-origin isolated.
 *
 * Usage: node run-page.mjs [--keep]   (build into ./build-page/)
 */
import fs from 'node:fs';
import path from 'node:path';
import { spawnSync, execFileSync } from 'node:child_process';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { summarize } from './suite.mjs';
import { startEchoServer } from '../test-wasm-libs/echo-server.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '../../..');
const pkg = path.resolve(here, '..');
const tcc = process.env.TINYC_WASM32_TCC ??
  path.join(repo, 'wasm-backend/build-src/wasm32-tcc');
if (!fs.existsSync(tcc)) {
  console.error(`wasm32-tcc not found at ${tcc}; run wasm-backend/build.sh first`);
  process.exit(2);
}
const buildDir = path.join(here, 'build-page');
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

const echo = await startEchoServer({
  serveRoot: [buildDir, path.join(pkg, 'sysroot-lib', 'wasm32-wasi'), pkg, path.join(pkg, 'sysroot-manifests'), path.join(pkg, 'sysroot'), here],
  headers: {
    'Cross-Origin-Opener-Policy': 'same-origin',
    'Cross-Origin-Embedder-Policy': 'require-corp',
  },
});

const requireFromPw = createRequire(path.join(
  process.env.TINYC_PLAYWRIGHT_DIR ??
  path.join(process.env.HOME, 'workspace/ci-repro-browser'), 'noop.js'));
const { chromium } = requireFromPw('playwright');
const browser = await chromium.launch({
  ...(process.env.CHROMIUM ? { executablePath: process.env.CHROMIUM } : {}),
  args: ['--no-sandbox'],
});

let ok = false;
try {
  const page = await browser.newPage();
  page.on('pageerror', (e) => console.log(`page [pageerror] ${e}`));
  await page.goto(`${echo.httpBase}/page.html`);
  await page.waitForFunction(() => window.__cstd2Result !== undefined,
    null, { timeout: 240000 });
  const { tier, results } = await page.evaluate(() => window.__cstd2Result);
  console.log(`page tier: ${tier}`);
  ok = summarize(results);
} finally {
  await browser.close();
  echo.close();
  if (!process.argv.includes('--keep')) {
    fs.rmSync(buildDir, { recursive: true, force: true });
  }
}
process.exit(ok ? 0 : 1);
