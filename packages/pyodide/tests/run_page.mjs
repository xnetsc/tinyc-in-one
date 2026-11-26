// Chromium async-tier test driver: serves this repository's
// packages/pyodide tree over HTTP, opens test_page.html (Pyodide
// on the page main thread, tinyc_web's async tier) with
// Playwright, and reports the shared case list's result.
//
// Usage: node run_page.mjs
import fs from 'node:fs';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const pkgRoot = path.resolve(here, '..');
const PORT = 8123;

// The page installs the package from a file list; generate it from
// the tree so it always matches what is on disk.
function walk(dir, base, out) {
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const rel = base ? `${base}/${entry.name}` : entry.name;
    if (entry.isDirectory()) walk(path.join(dir, entry.name), rel, out);
    else out.push(rel);
  }
  return out;
}
const files = walk(path.join(pkgRoot, 'tinyc_web'), '', []);
fs.writeFileSync(path.join(here, 'pkg-files.json'),
  JSON.stringify(files, null, 2) + '\n');

const TYPES = {
  '.html': 'text/html', '.js': 'text/javascript',
  '.mjs': 'text/javascript', '.json': 'application/json',
  '.wasm': 'application/wasm', '.py': 'text/x-python',
  '.md': 'text/plain', '.toml': 'text/plain',
};
const server = http.createServer((req, res) => {
  const urlPath = decodeURIComponent(new URL(req.url, 'http://x').pathname);
  const file = path.join(pkgRoot, urlPath);
  if (!file.startsWith(pkgRoot) || !fs.existsSync(file) ||
      !fs.statSync(file).isFile()) {
    res.writeHead(404).end('not found');
    return;
  }
  res.writeHead(200, {
    'content-type': TYPES[path.extname(file)] ?? 'application/octet-stream',
  });
  fs.createReadStream(file).pipe(res);
});
await new Promise((r) => server.listen(PORT, '127.0.0.1', r));

const pwPath = path.join(os.homedir(),
  'workspace/ci-repro-browser/node_modules/playwright/index.mjs');
const { chromium } = await import(pwPath);
const exe = path.join(os.homedir(),
  '.cache/ms-playwright/chromium-1255/chrome-linux/chrome');
const browser = await chromium.launch({
  executablePath: fs.existsSync(exe) ? exe : undefined,
});
let exitCode = 1;
try {
  const page = await browser.newPage();
  page.on('console', (m) => console.log('[page]', m.text()));
  page.on('pageerror', (e) => console.log('[pageerror]', String(e)));
  await page.goto(`http://127.0.0.1:${PORT}/tests/test_page.html`);
  await page.waitForFunction(() => window.__DONE__ === true,
    null, { timeout: 300000 });
  const summary = await page.evaluate(() => window.__SUMMARY__);
  const error = await page.evaluate(() => window.__ERROR__);
  const log = await page.evaluate(() => window.__LOG__);
  const lines = (log ?? []).filter(
    (l) => l.startsWith('PASS ') || l.startsWith('FAIL ') ||
           l.startsWith('CASES DONE'));
  if (error) {
    lines.push('PAGE ERROR: ' + error.split('\n')
      .filter((l) => l.length < 300).join('\n'));
  }
  fs.writeFileSync(path.join(here, 'page-async-tier.log'),
    lines.join('\n') + '\n');
  console.log(lines.join('\n'));
  if (summary && Array.isArray(summary.failed) &&
      summary.failed.length === 0) {
    console.log('PAGE ASYNC TIER: PASS');
    exitCode = 0;
  } else {
    console.log('PAGE ASYNC TIER: FAIL', JSON.stringify(summary));
  }
} finally {
  await browser.close();
  server.close();
}
process.exit(exitCode);
