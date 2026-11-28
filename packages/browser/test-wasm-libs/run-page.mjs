#!/usr/bin/env node
/* Page runner for the wasm-libs suite: compiles the guests
 * and builds the hybrid units as the Node runner does, then
 * serves the package over http with COOP/COEP alongside the
 * echo service (same origin, so guest fetches and WebSocket
 * connects stay same-origin) and runs the suite in Chromium
 * — the real tier when the page is cross-origin isolated.
 *
 * Usage: node run-page.mjs [--keep]   (build into ./build-page/)
 */
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { runSuite, summarize } from './suite.mjs';
import { buildHybridUnits } from './build-hybrid.mjs';
import { startEchoServer } from './echo-server.mjs';

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

// The port is needed for the guests' base URLs before the
// server can start; bind port 0 through a probe server is
// racy, so start the echo server on an ephemeral port first
// and compile against it (the page server is the same one).
const echo = await startEchoServer({
  serveRoot: [buildDir, path.join(pkg, 'sysroot-lib', 'wasm32-wasi'), pkg, path.join(pkg, 'sysroot-manifests'), path.join(pkg, 'sysroot'), here],
  headers: {
    'Cross-Origin-Opener-Policy': 'same-origin',
    'Cross-Origin-Embedder-Policy': 'require-corp',
  },
});

function compile(src, out, extra = []) {
  execFileSync(tcc, [
    '-nostdlib', ...INCLUDES, '-wasm-dylink=main',
    `-DTEST_HTTP_BASE="${echo.httpBase}"`,
    `-DTEST_WS_BASE="${echo.wsBase}"`,
    ...extra,
    '-o', path.join(buildDir, out), path.join(guestsDir, src),
  ], { stdio: 'inherit' });
}

compile('crypto_need.c', 'crypto_need', ['--dylink-need=libmbedtls']);
compile('crypto_dlopen.c', 'crypto_dlopen');
compile('regex_need.c', 'regex_need', ['--dylink-need=libpcre2']);
compile('regex_dlopen.c', 'regex_dlopen');
compile('curl_need.c', 'curl_need', ['--dylink-need=libcurl']);
compile('curl_dlopen.c', 'curl_dlopen');
compile('curl_ws.c', 'curl_ws', ['--dylink-need=libcurl']);
compile('curl_nodecl.c', 'curl_nodecl');
compile('hybrid_need.c', 'hybrid_need', ['--dylink-need=libhybrid']);
compile('hybrid_dlopen.c', 'hybrid_dlopen');
compile('hybrid_bad_need.c', 'hybrid_bad_need', ['--dylink-need=libhybridbad']);
compile('remote_dlopen.c', 'remote_dlopen');
buildHybridUnits({ tcc, includes: INCLUDES, guestsDir, buildDir });

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
  await page.waitForFunction(() => window.__wasmLibsResult !== undefined,
    null, { timeout: 240000 });
  const { tier, results } = await page.evaluate(() => window.__wasmLibsResult);
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
