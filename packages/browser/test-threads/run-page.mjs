#!/usr/bin/env node
/* Page runner for the §11 thread suite:
 *  - http + COOP/COEP: the real tier runs the full suite in
 *    Chromium, and the tier probe must select 'real'.
 *  - file://: the probe must fall back to 'simulated' (there
 *    is no SharedArrayBuffer), and the simulated suite runs
 *    in full, deadlock case included.
 * Guests are compiled with the wasm32 cross compiler, as in
 * the Node runner. Playwright + Chromium are taken from the
 * local ci-repro-browser install.
 *
 * Usage: node run-page.mjs [--keep]
 */
import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

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

const INCLUDES = [
  '-I', path.join(repo, 'wasm-backend/include'),
  '-I', path.join(repo, 'vendor-tinycc/include'),
  '-I', path.join(repo, 'vendor-tinycc/win32/include'),
  '-I', path.join(here, 'guests'),
];
function compile(src, flavor, out, extra = []) {
  execFileSync(tcc, [
    '-nostdlib', ...INCLUDES, `-wasm-dylink=${flavor}`, ...extra,
    '-o', path.join(buildDir, out), path.join(here, 'guests', src),
  ], { stdio: 'inherit' });
}
const MAINS = [
  'create_join', 'mutex_excl', 'mutex_kinds', 'cond_buffer', 'once',
  'spin', 'key_tls', 'thread_var', 'atomics_storm', 'dlopen_thread',
  'deadlock', 'exit7', 'main_exit', 'plain_dylink', 'nojspi',
];
const SIDES = ['libtls', 'libfetch', 'libsecond', 'libplain'];
for (const m of MAINS) {
  compile(`${m}.c`, 'main', m,
    m === 'thread_var' ? ['--dylink-need=libtls'] : []);
}
for (const s of SIDES) compile(`${s}.c`, 'side', s);
const GUESTS = [...MAINS, ...SIDES];

// -- the in-page suite driver (module form, for http) --
const pageModule = (tier, includeDeadlock, probeCheck) => `
import { createProcess } from '/index.mjs';
import { runSuite } from '/test-threads/suite.mjs';
const NAMES = ${JSON.stringify(GUESTS)};
const bytes = {};
for (const n of NAMES) {
  bytes[n] = new Uint8Array(
    await (await fetch('/guests/' + n)).arrayBuffer());
}
const LOCAL = new Set(['libtls', 'libplain']);
const readModule = (canonical) => {
  const name = canonical.replace(/^\\//, '').replace(/^sysroot\\//, '');
  return LOCAL.has(name) ? bytes[name] : undefined;
};
const results = [];
const probe = createProcess({ threads: true });
results.push({
  name: 'tier-probe', ok: probe.threadTier === ${JSON.stringify(probeCheck)},
  detail: 'threadTier=' + probe.threadTier,
});
results.push(...await runSuite({
  createProcess,
  guestBytes: async (n) => bytes[n],
  baseUrl: location.origin + '/',
  readModule,
  tier: ${JSON.stringify(tier)},
  includeDeadlock: ${includeDeadlock},
  onCase: (n) => {
    document.getElementById('p').textContent = n;
  },
}));
document.getElementById('r').textContent = JSON.stringify(results);
`;

const htmlPage = (scriptTag) => `<!doctype html>
<html><head><meta charset="utf-8"><title>tinyc thread suite</title></head>
<body><pre id="r"></pre><pre id="p"></pre>${scriptTag}</body></html>`;

const MIME = {
  '.mjs': 'text/javascript', '.js': 'text/javascript',
  '.html': 'text/html', '.json': 'application/json',
};
function serveFile(res, file) {
  if (!fs.existsSync(file) || !fs.statSync(file).isFile()) {
    res.writeHead(404); res.end('not found'); return;
  }
  res.writeHead(200, {
    'content-type': MIME[path.extname(file)] ?? 'application/octet-stream',
    'cross-origin-opener-policy': 'same-origin',
    'cross-origin-embedder-policy': 'require-corp',
  });
  res.end(fs.readFileSync(file));
}

const server = http.createServer((req, res) => {
  const url = decodeURIComponent((req.url ?? '/').split('?')[0]);
  if (url === '/' || url === '/page.html') {
    res.writeHead(200, {
      'content-type': 'text/html',
      'cross-origin-opener-policy': 'same-origin',
      'cross-origin-embedder-policy': 'require-corp',
    });
    res.end(htmlPage('<script type="module">' +
      pageModule('real', false, 'real') + '</script>'));
    return;
  }
  const guest = /^\/(?:guests\/|sysroot\/)?([a-z0-9_]+)$/.exec(url);
  if (guest && GUESTS.includes(guest[1])) {
    serveFile(res, path.join(buildDir, guest[1]));
    return;
  }
  if (url.startsWith('/test-threads/')) {
    serveFile(res, path.join(here, url.slice('/test-threads/'.length)));
    return;
  }
  serveFile(res, path.join(pkg, url.replace(/^\//, '')));
});
await new Promise((r) => server.listen(0, '127.0.0.1', r));
const origin = `http://127.0.0.1:${server.address().port}`;

// -- the file:// variant: the single-file bundle + inlined guests --
const fileDir = path.join(here, 'build-file');
fs.rmSync(fileDir, { recursive: true, force: true });
fs.mkdirSync(fileDir, { recursive: true });
execFileSync(process.execPath, [path.join(pkg, 'build-web-bundle.mjs')],
  { stdio: 'inherit' });
fs.copyFileSync(path.join(pkg, 'tinyc-web.js'),
  path.join(fileDir, 'tinyc-web.js'));
execFileSync('git', ['checkout', '--', 'packages/browser/tinyc-web.js'],
  { cwd: repo, stdio: 'inherit' });
const suiteSrc = fs.readFileSync(path.join(here, 'suite.mjs'), 'utf8');
const inlined = {};
for (const n of GUESTS) {
  inlined[n] = fs.readFileSync(path.join(buildDir, n)).toString('base64');
}
const filePage = `<!doctype html>
<html><head><meta charset="utf-8"><title>tinyc thread suite (file)</title>
</head><body><pre id="r"></pre><pre id="p"></pre>
<script src="./tinyc-web.js"></script>
<script>
const GUEST_B64 = ${JSON.stringify(inlined)};
const bytes = {};
for (const [n, b64] of Object.entries(GUEST_B64)) {
  const bin = atob(b64);
  const u8 = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) u8[i] = bin.charCodeAt(i);
  bytes[n] = u8;
}
const LOCAL = new Set(['libtls', 'libplain', 'libfetch', 'libsecond']);
const readModule = (canonical) => {
  const name = canonical.replace(/^\\//, '').replace(/^sysroot\\//, '');
  return LOCAL.has(name) ? bytes[name] : undefined;
};
window.__suiteEnv = {
  createProcess: (opts) => TinycWeb.createProcess(opts),
  bytes, readModule,
};
</script>
<script type="module">
${suiteSrc.replace(/^export /gm, '')}
const results = [];
const probe = window.__suiteEnv.createProcess({ threads: true });
results.push({
  name: 'tier-probe', ok: probe.threadTier === 'simulated',
  detail: 'threadTier=' + probe.threadTier,
});
results.push(...await runSuite({
  createProcess: window.__suiteEnv.createProcess,
  guestBytes: async (n) => window.__suiteEnv.bytes[n],
  baseUrl: null,
  readModule: window.__suiteEnv.readModule,
  tier: 'simulated',
  includeDeadlock: true,
  onCase: (n) => {
    document.getElementById('p').textContent = n;
  },
}));
document.getElementById('r').textContent = JSON.stringify(results);
</script>
</body></html>`;
fs.writeFileSync(path.join(fileDir, 'page.html'), filePage);

// -- drive both pages with Chromium --
import { createRequire } from 'node:module';
const requireFromRepro = createRequire(
  path.join(process.env.HOME, 'workspace/ci-repro-browser/package.json'));
const { chromium } = requireFromRepro('playwright');
const browser = await chromium.launch({
  executablePath: process.env.CHROMIUM ??
    path.join(process.env.HOME,
      '.cache/ms-playwright/chromium-1255/chrome-linux/chrome'),
  args: ['--no-sandbox'],
});

async function runPage(url, label) {
  const page = await browser.newPage();
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  page.on('console', (m) => console.log(`${label} [console] ${m.text()}`));
  await page.goto(url);
  try {
    await page.waitForFunction(
      () => document.getElementById('r').textContent.length > 0,
      null, { timeout: 240000 });
  } catch (err) {
    const stuck = await page.textContent('#p').catch(() => '?');
    console.log(`${label} TIMEOUT stuck at case: ${stuck}`);
    throw err;
  }
  const results = JSON.parse(await page.textContent('#r'));
  let ok = true;
  for (const r of results) {
    console.log(`${label} ${r.ok ? 'PASS' : 'FAIL'} ${r.name}` +
      (r.detail ? ` — ${r.detail}` : ''));
    ok = ok && r.ok;
  }
  if (errors.length) {
    console.log(`${label} page errors: ${errors.join(' | ')}`);
    ok = false;
  }
  await page.close();
  return ok;
}

let allOk = true;
try {
  allOk = await runPage(`${origin}/page.html`, '[http-real]') && allOk;
  allOk = await runPage(`file://${path.join(fileDir, 'page.html')}`,
    '[file-sim]') && allOk;
} finally {
  await browser.close();
  server.close();
  if (!process.argv.includes('--keep')) {
    fs.rmSync(buildDir, { recursive: true, force: true });
    fs.rmSync(fileDir, { recursive: true, force: true });
  }
}
process.exit(allOk ? 0 : 1);
