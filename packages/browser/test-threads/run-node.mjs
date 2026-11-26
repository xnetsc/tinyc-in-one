#!/usr/bin/env node
/* Node runner for the §11 thread suite: compiles the guests
 * with the wasm32 cross compiler, serves the side modules
 * over a local http server (so a guest dlopen of an absent
 * module takes the fetch path), and runs the suite in the
 * simulated tier. The real tier needs a Worker constructor
 * and is covered by the page runner; this runner also checks
 * that forcing it in Node fails by name, and that a no-JSPI
 * environment makes pthread_create fail by name (§4.2).
 *
 * Usage: node run-node.mjs [--keep]   (guests build into
 * ./build-node/, removed unless --keep)
 */
import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createProcess } from '../index.mjs';
import { runSuite, summarize } from './suite.mjs';

const here = path.dirname(fileURLToPath(import.meta.url));
const repo = path.resolve(here, '../../..');
const tcc = process.env.TINYC_WASM32_TCC ??
  path.join(repo, 'wasm-backend/build-src/wasm32-tcc');
if (!fs.existsSync(tcc)) {
  console.error(`wasm32-tcc not found at ${tcc}; run wasm-backend/build.sh first`);
  process.exit(2);
}
const buildDir = path.join(here, 'build-node');
fs.rmSync(buildDir, { recursive: true, force: true });
fs.mkdirSync(buildDir, { recursive: true });

const INCLUDES = [
  '-I', path.join(repo, 'wasm-backend/include'),
  '-I', path.join(repo, 'vendor-tinycc/include'),
  // TCC's own stdint.h lives here; without it <stdint.h>
  // resolves to the host's, which does not parse for wasm32.
  '-I', path.join(repo, 'vendor-tinycc/win32/include'),
  '-I', path.join(here, 'guests'),
];
function compile(src, flavor, out) {
  execFileSync(tcc, [
    '-nostdlib', ...INCLUDES, `-wasm-dylink=${flavor}`,
    '-o', path.join(buildDir, out), path.join(here, 'guests', src),
  ], { stdio: 'inherit' });
}
const MAINS = [
  'create_join', 'mutex_excl', 'mutex_kinds', 'cond_buffer', 'once',
  'spin', 'key_tls', 'thread_var', 'atomics_storm', 'dlopen_thread',
  'deadlock', 'exit7', 'main_exit', 'plain_dylink', 'nojspi',
  'host_proxy', 'host_fail', 'dir_boundary', 'churn',
  'entry_return',
];
const SIDES = ['libtls', 'libfetch', 'libsecond', 'libplain'];
for (const m of MAINS) {
  if (m === 'thread_var') {
    execFileSync(tcc, [
      '-nostdlib', ...INCLUDES, '-wasm-dylink=main',
      '--dylink-need=libtls',
      '-o', path.join(buildDir, m), path.join(here, 'guests', `${m}.c`),
    ], { stdio: 'inherit' });
  } else {
    compile(`${m}.c`, 'main', m);
  }
}
for (const s of SIDES) compile(`${s}.c`, 'side', s);

const server = http.createServer((req, res) => {
  // The transport resolves a canonical path against the
  // base URL's root: /sysroot/<name> arrives as /<name>.
  const m = /^\/(?:sysroot\/)?([a-z0-9_]+)$/.exec(req.url ?? '');
  const file = m && path.join(buildDir, m[1]);
  if (file && fs.existsSync(file)) {
    res.writeHead(200, { 'content-type': 'text/javascript' });
    res.end(fs.readFileSync(file));
  } else {
    res.writeHead(404);
    res.end('not found');
  }
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const baseUrl = `http://127.0.0.1:${server.address().port}/`;

const guestBytes = async (name) => fs.readFileSync(path.join(buildDir, name));

/* The local module source: only the modules a guest may
 * treat as present (a needed dependency, and the regression
 * case's dlopen target). libfetch/libsecond stay absent so
 * dlopen_thread takes the fetch path. */
const LOCAL_MODULES = new Set(['libtls', 'libplain']);
const readModule = (canonical) => {
  const name = canonical.replace(/^\//, '').replace(/^sysroot\//, '');
  if (!LOCAL_MODULES.has(name)) return undefined;
  const file = path.join(buildDir, name);
  return fs.existsSync(file) ? fs.readFileSync(file) : undefined;
};

let allOk = true;
try {
  const results = await runSuite({
    createProcess, guestBytes, baseUrl, readModule,
    tier: 'simulated', includeDeadlock: true,
  });
  allOk = summarize(results) && allOk;

  // Forcing the real tier where the probe cannot pass fails
  // at creation, by name (§2.2).
  try {
    createProcess({ threads: true, threadTier: 'real' });
    console.log('FAIL real-tier-forced — creation did not fail');
    allOk = false;
  } catch (err) {
    const named = /real-tier|Worker|SharedArrayBuffer/.test(err.message);
    console.log(`${named ? 'PASS' : 'FAIL'} real-tier-forced — ${err.message}`);
    allOk = allOk && named;
  }

  // No JSPI: pthread_create in a direct module fails at the
  // call site with the §4.2 error.
  const savedPromising = WebAssembly.promising;
  const savedSuspending = WebAssembly.Suspending;
  try {
    WebAssembly.promising = undefined;
    WebAssembly.Suspending = undefined;
    let text = '';
    const proc = createProcess({
      threads: true, threadTier: 'simulated', baseUrl,
      io: { out: (t) => { text += t; }, err: (t) => { text += t; } },
    });
    let failure = null;
    try {
      await proc.runMainModule(await guestBytes('nojspi'));
    } catch (err) {
      failure = err;
    }
    const msg = failure ? String(failure.message || failure) : '';
    const named = /pthread_create/.test(msg) && /promising|JSPI/.test(msg);
    console.log(`${named ? 'PASS' : 'FAIL'} nojspi — ` +
      (failure ? msg : `run completed: ${text}`));
    allOk = allOk && named;
  } finally {
    WebAssembly.promising = savedPromising;
    WebAssembly.Suspending = savedSuspending;
  }
} finally {
  server.close();
  if (!process.argv.includes('--keep')) {
    fs.rmSync(buildDir, { recursive: true, force: true });
  }
}
process.exit(allOk ? 0 : 1);
