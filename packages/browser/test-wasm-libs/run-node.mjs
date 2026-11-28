#!/usr/bin/env node
/* Node runner for the wasm-libs suite: compiles the guests
 * with the wasm32 cross compiler, starts the local echo
 * server (HTTP + WebSocket), builds the hybrid units, and
 * runs the suite in the simulated tier. The real tier needs
 * a browser Worker and is covered by the page runner.
 *
 * Usage: node run-node.mjs [--keep]   (build into ./build-node/)
 */
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createProcess } from '../index.mjs';
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

const echo = await startEchoServer({
  serveRoot: [buildDir, path.join(pkg, 'sysroot-lib', 'wasm32-wasi')],
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

const units = new Map();
for (const name of ['libcurl.tinyc.js', 'libpcre2.tinyc.js', 'libmbedtls.tinyc.js']) {
  units.set(name, fs.readFileSync(path.join(pkg, 'sysroot-lib', 'wasm32-wasi', name)));
}
for (const name of ['libhybrid.tinyc.js', 'libhybridbad.tinyc.js']) {
  units.set(name, fs.readFileSync(path.join(buildDir, name)));
}

const guestBytes = async (name) => fs.readFileSync(path.join(buildDir, name));

let ok = false;
try {
  const results = await runSuite({
    createProcess, guestBytes, units,
    baseUrl: `${echo.httpBase}/`,
    tier: 'simulated',
    prefetchRemote: async (name) => {
      const res = await fetch(`${echo.httpBase}/${name}`);
      if (!res.ok) throw new Error(`prefetch ${name}: HTTP ${res.status}`);
      const text = await res.text();
      // The host registers the retrieved unit under its
      // canonical sysroot path — the registry is the loader's
      // first local source.
      const manifest = JSON.parse(
        text.slice(text.indexOf(' = ') + 3, text.lastIndexOf(';')));
      const registry = globalThis.__TINYC_DYLINK_REGISTRY__ ??=
        Object.create(null);
      registry[`/sysroot/lib/${name}`] = manifest;
    },
  });
  ok = summarize(results);
} finally {
  echo.close();
  if (!process.argv.includes('--keep')) {
    fs.rmSync(buildDir, { recursive: true, force: true });
  }
}
process.exit(ok ? 0 : 1);
